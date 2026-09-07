/*
	Virtual page backed heap implementation by Santtu S. Nyman.

	License:

		This is free and unencumbered software released into the public domain.

		Anyone is free to copy, modify, publish, use, compile, sell, or
		distribute this software, either in source code form or as a compiled
		binary, for any purpose, commercial or non-commercial, and by any
		means.

		In jurisdictions that recognize copyright laws, the author or authors
		of this software dedicate any and all copyright interest in the
		software to the public domain. We make this dedication for the benefit
		of the public at large and to the detriment of our heirs and
		successors. We intend this dedication to be an overt act of
		relinquishment in perpetuity of all present and future rights to this
		software under copyright law.

		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
		IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
		OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
		ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
		OTHER DEALINGS IN THE SOFTWARE.

	Implementation notes:

		Memory layout of the backing allocation:

			[0 .. sizeof(FLVirtualHeap) - 1]
				Heap descriptor.  Not treated as a block.

			[sizeof(FLVirtualHeap) .. regionReservedSize - EPILOGUE_SIZE - 1]
				User blocks.  Each block begins with an 8-byte header word:
					bits [63..4]  block size (multiple of 16)
					bit  [2]      RED (red-black tree color; free blocks only)
					bit  [1]      PREV_ALLOCATED (previous block is allocated)
					bit  [0]      ALLOCATED
				Free blocks also have an 8-byte footer at block+size-8 that
				mirrors the header, and carry an FLVirtualHeapNode in the
				first 32 bytes of their payload (block+8).

				The footer never carries the RED bit.  Recoloring a node is a
				read-modify-write of the header word alone; the footer is only
				ever read for its size (during backward coalescing), so
				mirroring the color into it would cost a second store on every
				recolor for no benefit.  Allocated blocks never set RED.

			[regionReservedSize - 16 .. regionReservedSize - 1]
				Epilogue sentinel: size=16, ALLOCATED=1.  Prevents forward
				coalescing past the end of the region.

		Block addresses are always == 8 (mod 16) because sizeof(FLVirtualHeap)
		== 24 == 8 (mod 16) and the heap base is page-aligned (>=16 bytes).
		Therefore every payload pointer (block+8) is 16-byte aligned.

		The epilogue occupies the last 8 bytes of the region (just a header
		word).  This places it at regionEnd - 8 == 8 (mod 16), consistent with
		all other block addresses.  Its header word has ALLOCATED=1 and size
		bits=0 — only the ALLOCATED bit is checked by the allocator.

		Minimum block size is 48:  8 (header) + 32 (FLVirtualHeapNode) + 8
		(footer).  Every block, whether free or allocated, is at least this
		large so that any allocated block can be freed without reallocation.
*/

#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <Windows.h>
#include "FlVirtualHeap.h"
#pragma comment(lib, "mincore")

// ---------- Block format constants ----------

#define FL_VIRTUAL_HEAP_SIZE_MASK           ((size_t)(~(size_t)0xF))
#define FL_VIRTUAL_HEAP_ALLOCATED_FLAG      ((size_t)0x1)
#define FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG ((size_t)0x2)
#define FL_VIRTUAL_HEAP_RED_FLAG            ((size_t)0x4)
#define FL_VIRTUAL_HEAP_NODE_BLACK          0
#define FL_VIRTUAL_HEAP_NODE_RED            1
#define FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE      ((size_t)48)
#define FL_VIRTUAL_HEAP_EPILOGUE_SIZE       ((size_t)8)
#define FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE     sizeof(FLVirtualHeap)
#define FL_VIRTUAL_HEAP_MIN_REGION_SIZE     (FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE + FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE + FL_VIRTUAL_HEAP_EPILOGUE_SIZE)

// ---------- Block field access helpers ----------

static size_t FlVirtualHeapBlockSize(size_t headerWord)
{
	return headerWord & FL_VIRTUAL_HEAP_SIZE_MASK;
}

static int FlVirtualHeapIsAllocated(size_t headerWord)
{
	return (headerWord & FL_VIRTUAL_HEAP_ALLOCATED_FLAG) != 0;
}

static int FlVirtualHeapIsPrevAllocated(size_t headerWord)
{
	return (headerWord & FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG) != 0;
}

static size_t* FlVirtualHeapHeaderAt(void* blockAddress)
{
	return (size_t*)blockAddress;
}

static size_t* FlVirtualHeapFooterAt(void* blockAddress, size_t blockSize)
{
	return (size_t*)((uintptr_t)blockAddress + blockSize - 8);
}

static size_t* FlVirtualHeapNextBlockHeader(void* blockAddress, size_t blockSize)
{
	return (size_t*)((uintptr_t)blockAddress + blockSize);
}

static FLVirtualHeapNode* FlVirtualHeapBlockNode(void* blockAddress)
{
	return (FLVirtualHeapNode*)((uintptr_t)blockAddress + 8);
}

static void* FlVirtualHeapNodeBlock(const FLVirtualHeapNode* node)
{
	return (void*)((uintptr_t)node - 8);
}

static void* FlVirtualHeapBlockPayload(void* blockAddress)
{
	return (void*)((uintptr_t)blockAddress + 8);
}

static void* FlVirtualHeapPayloadBlock(void* payloadAddress)
{
	return (void*)((uintptr_t)payloadAddress - 8);
}

static void* FlVirtualHeapFirstBlock(const FLVirtualHeap* heap)
{
	return (void*)((uintptr_t)heap + FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE);
}

static void* FlVirtualHeapEpilogueBlock(const FLVirtualHeap* heap)
{
	return (void*)((uintptr_t)heap + heap->regionCommittedSize - FL_VIRTUAL_HEAP_EPILOGUE_SIZE);
}

// Writes the header (and footer for free blocks) for a block.
static void FlVirtualHeapWriteBlockHeader(void* blockAddress, size_t blockSize, size_t flags)
{
	size_t headerWord = blockSize | flags;
	*FlVirtualHeapHeaderAt(blockAddress) = headerWord;
}

// Writes a free block's header and footer. flags may carry PREV_ALLOCATED and
// RED; the footer is written without RED (see the layout note at the top of
// this file), so a later recolor only has to touch the header.
static void FlVirtualHeapWriteFreeBlock(void* blockAddress, size_t blockSize, size_t flags)
{
	*FlVirtualHeapHeaderAt(blockAddress) = blockSize | flags;
	*FlVirtualHeapFooterAt(blockAddress, blockSize) = blockSize | (flags & ~FL_VIRTUAL_HEAP_RED_FLAG);
}

// Updates the PREV_ALLOCATED flag in the next block's header.
static void FlVirtualHeapSetNextPrevAllocated(void* blockAddress, size_t blockSize, size_t prevAllocatedFlag)
{
	size_t* nextHeader = FlVirtualHeapNextBlockHeader(blockAddress, blockSize);
	*nextHeader = (*nextHeader & ~FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG) | prevAllocatedFlag;
}

// ---------- Red-black tree ----------
//
// The free blocks are kept in a standard (non-left-leaning) red-black tree,
// ordered by block address, and augmented with subtreeMax (the largest block
// size anywhere in the subtree) to answer best-address fitting queries in
// FlRbFindFit without a linear scan.
//
// Nodes carry parent pointers, so both RB-INSERT-FIXUP and RB-DELETE-FIXUP are
// the plain iterative CLRS algorithms and FlRbDelete needs no search phase at
// all -- every caller already holds the node it wants removed, derived in O(1)
// from a block address. Parent pointers also make the two operations the
// coalescing fast paths depend on cheap:
//
//   FlRbReplaceNode              swap a node for another that belongs in the
//                              exact same in-order slot, in O(1).
//   FlRbRefreshAugmentationUpward  republish a changed block size to the
//                              ancestors that can see it, stopping as soon as
//                              one of them does not change.
//
// Neither performs a root-down search or a single rotation, which is what
// makes coalescing in FlVirtualHeapFree nearly free.
//
// A node's identity *is* its memory address (FlVirtualHeapNodeBlock derives
// the block address from the node's own pointer, not from stored data), so
// FlRbDelete cannot use the textbook trick of copying the in-order successor's
// key into the deleted node. The successor is physically relinked into the
// removed node's position instead.
//
// The color lives in bit 2 of the node's block header word rather than in the
// node, which is what leaves room for the parent pointer inside 32 bytes. The
// header word is the same word FlRbNodeSelfSize reads, so a node's color and its
// own block size come from a single load.

static size_t* FlRbNodeHeaderAt(const FLVirtualHeapNode* node)
{
	return FlVirtualHeapHeaderAt(FlVirtualHeapNodeBlock(node));
}

static int FlRbIsRed(const FLVirtualHeapNode* node)
{
	return node != NULL && (*FlRbNodeHeaderAt(node) & FL_VIRTUAL_HEAP_RED_FLAG) != 0;
}

// color must be FL_VIRTUAL_HEAP_NODE_BLACK or FL_VIRTUAL_HEAP_NODE_RED.
static void FlRbSetColor(FLVirtualHeapNode* node, int color)
{
	size_t* headerWord = FlRbNodeHeaderAt(node);
	*headerWord = color ? (*headerWord | FL_VIRTUAL_HEAP_RED_FLAG) : (*headerWord & ~FL_VIRTUAL_HEAP_RED_FLAG);
}

static size_t FlRbSubtreeMax(const FLVirtualHeapNode* node)
{
	return node ? node->subtreeMax : 0;
}

static size_t FlRbNodeSelfSize(const FLVirtualHeapNode* node)
{
	return FlVirtualHeapBlockSize(*FlRbNodeHeaderAt(node));
}

static void FlRbUpdateSubtreeMax(FLVirtualHeapNode* node)
{
	size_t maxSize = FlRbNodeSelfSize(node);
	size_t leftMax = FlRbSubtreeMax(node->left);
	size_t rightMax = FlRbSubtreeMax(node->right);
	if (leftMax > maxSize)
	{
		maxSize = leftMax;
	}
	if (rightMax > maxSize)
	{
		maxSize = rightMax;
	}
	node->subtreeMax = maxSize;
}

// Recomputes subtreeMax for node and every ancestor up to the root.
// Unconditional: safe even when a node somewhere along the path had its
// children replaced wholesale, as happens to the spliced-in successor in
// FlRbDelete.
static void FlRbRefreshAugmentationToRoot(FLVirtualHeapNode* node)
{
	while (node)
	{
		FlRbUpdateSubtreeMax(node);
		node = node->parent;
	}
}

// Republishes a changed block size upwards, stopping at the first ancestor
// whose subtreeMax does not change -- nothing above such a node can change
// either, because subtreeMax depends on the ancestors' children only through
// that value.
//
// Requires every node below `node` to already have a correct subtreeMax, which
// holds for the callers that only resized one block in place or swapped one
// node for another in the same slot. Typically stops after one or two levels,
// since one block's size is rarely the maximum of a whole subtree.
static void FlRbRefreshAugmentationUpward(FLVirtualHeapNode* node)
{
	while (node)
	{
		size_t previousSubtreeMax = node->subtreeMax;
		FlRbUpdateSubtreeMax(node);
		if (node->subtreeMax == previousSubtreeMax)
		{
			return;
		}
		node = node->parent;
	}
}

// Pure structural rotations: pointer surgery, parent relinking and subtreeMax
// refresh only. Colors are never touched here; every call site assigns colors
// explicitly, since the correct post-rotation colors differ between the
// insert-fixup and delete-fixup call sites.
//
// A rotation never changes which nodes a subtree contains, only how they are
// arranged, so the subtree root's subtreeMax is identical before and after.
// Refreshing the two nodes that actually moved is therefore sufficient; no
// ancestor can be affected by a rotation.
static void FlRbRotateLeft(FLVirtualHeap* heap, FLVirtualHeapNode* h)
{
	FLVirtualHeapNode* x = h->right;
	assert(x);
	h->right = x->left;
	if (x->left)
	{
		x->left->parent = h;
	}
	x->parent = h->parent;
	if (!h->parent)
	{
		heap->treeRoot = x;
	}
	else if (h == h->parent->left)
	{
		h->parent->left = x;
	}
	else
	{
		h->parent->right = x;
	}
	x->left = h;
	h->parent = x;
	FlRbUpdateSubtreeMax(h);
	FlRbUpdateSubtreeMax(x);
}

static void FlRbRotateRight(FLVirtualHeap* heap, FLVirtualHeapNode* h)
{
	FLVirtualHeapNode* x = h->left;
	assert(x);
	h->left = x->right;
	if (x->right)
	{
		x->right->parent = h;
	}
	x->parent = h->parent;
	if (!h->parent)
	{
		heap->treeRoot = x;
	}
	else if (h == h->parent->right)
	{
		h->parent->right = x;
	}
	else
	{
		h->parent->left = x;
	}
	x->right = h;
	h->parent = x;
	FlRbUpdateSubtreeMax(h);
	FlRbUpdateSubtreeMax(x);
}

// RB-INSERT-FIXUP. newNode has just been attached as a red leaf, so the only
// possible violation is a red node with a red parent. Each iteration either
// recolors the violation one level further up (case 1) or removes it outright
// with at most two rotations (cases 2 and 3), which ends the loop.
static void FlRbInsertFixup(FLVirtualHeap* heap, FLVirtualHeapNode* newNode)
{
	FLVirtualHeapNode* node = newNode;
	while (FlRbIsRed(node->parent))
	{
		FLVirtualHeapNode* parent = node->parent;
		FLVirtualHeapNode* grandParent = parent->parent;
		// parent is red, so parent is not the root, so grandParent exists.
		assert(grandParent);
		if (parent == grandParent->left)
		{
			FLVirtualHeapNode* uncle = grandParent->right;
			if (FlRbIsRed(uncle))
			{
				// Case 1: uncle red. Split the temporary 4-node by recoloring
				// and continue from the grandparent.
				FlRbSetColor(parent, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(uncle, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(grandParent, FL_VIRTUAL_HEAP_NODE_RED);
				node = grandParent;
			}
			else
			{
				if (node == parent->right)
				{
					// Case 2: inner grandchild. One rotation reduces this to
					// case 3.
					node = parent;
					FlRbRotateLeft(heap, node);
					parent = node->parent;
				}
				// Case 3: outer grandchild, uncle black. The new subtree root
				// is black, so the loop condition fails on the next test.
				FlRbSetColor(parent, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(grandParent, FL_VIRTUAL_HEAP_NODE_RED);
				FlRbRotateRight(heap, grandParent);
			}
		}
		else
		{
			FLVirtualHeapNode* uncle = grandParent->left;
			if (FlRbIsRed(uncle))
			{
				FlRbSetColor(parent, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(uncle, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(grandParent, FL_VIRTUAL_HEAP_NODE_RED);
				node = grandParent;
			}
			else
			{
				if (node == parent->left)
				{
					node = parent;
					FlRbRotateRight(heap, node);
					parent = node->parent;
				}
				FlRbSetColor(parent, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(grandParent, FL_VIRTUAL_HEAP_NODE_RED);
				FlRbRotateLeft(heap, grandParent);
			}
		}
	}
	// Case 1 can color the root red on its last iteration; this puts it back.
	FlRbSetColor(heap->treeRoot, FL_VIRTUAL_HEAP_NODE_BLACK);
}

// Inserts newNode into the address-ordered tree. The caller must have written
// newNode's block header already (FlRbNodeSelfSize reads it); every field of the
// node itself is initialized here.
static void FlRbInsert(_Inout_ FLVirtualHeap* heap, _Inout_ FLVirtualHeapNode* newNode)
{
	const void* newBlock = FlVirtualHeapNodeBlock(newNode);
	size_t newSize = FlRbNodeSelfSize(newNode);

	FLVirtualHeapNode* parent = NULL;
	FLVirtualHeapNode* current = heap->treeRoot;
	int insertLeft = 0;
	while (current)
	{
		// An insertion can only ever raise an ancestor's subtreeMax, and the
		// new block's size is known before the descent starts, so the
		// augmentation is published on the way down. No second pass back up is
		// needed, and the rotations in FlRbInsertFixup preserve subtree maxima.
		if (current->subtreeMax < newSize)
		{
			current->subtreeMax = newSize;
		}
		parent = current;
		insertLeft = (uintptr_t)newBlock < (uintptr_t)FlVirtualHeapNodeBlock(current);
		current = insertLeft ? current->left : current->right;
	}

	newNode->left = NULL;
	newNode->right = NULL;
	newNode->parent = parent;
	newNode->subtreeMax = newSize;
	FlRbSetColor(newNode, FL_VIRTUAL_HEAP_NODE_RED);

	if (!parent)
	{
		heap->treeRoot = newNode;
	}
	else if (insertLeft)
	{
		parent->left = newNode;
	}
	else
	{
		parent->right = newNode;
	}

	FlRbInsertFixup(heap, newNode);
}

// Replaces the subtree rooted at target with the subtree rooted at
// replacement, which may be NULL. target's own child pointers are left alone.
static void FlRbTransplant(FLVirtualHeap* heap, FLVirtualHeapNode* target, FLVirtualHeapNode* replacement)
{
	if (!target->parent)
	{
		heap->treeRoot = replacement;
	}
	else if (target == target->parent->left)
	{
		target->parent->left = replacement;
	}
	else
	{
		target->parent->right = replacement;
	}
	if (replacement)
	{
		replacement->parent = target->parent;
	}
}

// RB-DELETE-FIXUP. node carries one extra unit of black and may be NULL, so
// its parent is passed explicitly rather than relying on a nil sentinel. When
// node is NULL its sibling is always a real node: a doubly-black NULL child
// means the sibling's subtree has black-height at least one, and it also means
// exactly one of nodeParent's children is NULL, so comparing node against
// nodeParent->left still identifies the correct side.
static void FlRbDeleteFixup(FLVirtualHeap* heap, FLVirtualHeapNode* node, FLVirtualHeapNode* nodeParent)
{
	while (node != heap->treeRoot && !FlRbIsRed(node))
	{
		assert(nodeParent);
		if (node == nodeParent->left)
		{
			FLVirtualHeapNode* sibling = nodeParent->right;
			assert(sibling);
			if (FlRbIsRed(sibling))
			{
				// Case 1: red sibling, so nodeParent is provably black. Rotate
				// the sibling up so the cases below face a black one.
				FlRbSetColor(sibling, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(nodeParent, FL_VIRTUAL_HEAP_NODE_RED);
				FlRbRotateLeft(heap, nodeParent);
				sibling = nodeParent->right;
				assert(sibling);
			}
			if (!FlRbIsRed(sibling->left) && !FlRbIsRed(sibling->right))
			{
				// Case 2: both nephews black. Either nodeParent absorbs the
				// deficiency by turning black or it propagates one level up.
				FlRbSetColor(sibling, FL_VIRTUAL_HEAP_NODE_RED);
				node = nodeParent;
				nodeParent = node->parent;
				continue;
			}
			if (!FlRbIsRed(sibling->right))
			{
				// Case 3: near nephew red, far nephew black. Rotating the near
				// nephew up reduces this to case 4.
				FlRbSetColor(sibling->left, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(sibling, FL_VIRTUAL_HEAP_NODE_RED);
				FlRbRotateRight(heap, sibling);
				sibling = nodeParent->right;
				assert(sibling);
			}
			// Case 4: far nephew red. Resolves the deficiency outright.
			FlRbSetColor(sibling, FlRbIsRed(nodeParent) ? FL_VIRTUAL_HEAP_NODE_RED : FL_VIRTUAL_HEAP_NODE_BLACK);
			FlRbSetColor(nodeParent, FL_VIRTUAL_HEAP_NODE_BLACK);
			FlRbSetColor(sibling->right, FL_VIRTUAL_HEAP_NODE_BLACK);
			FlRbRotateLeft(heap, nodeParent);
			node = heap->treeRoot;
			nodeParent = NULL;
		}
		else
		{
			FLVirtualHeapNode* sibling = nodeParent->left;
			assert(sibling);
			if (FlRbIsRed(sibling))
			{
				FlRbSetColor(sibling, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(nodeParent, FL_VIRTUAL_HEAP_NODE_RED);
				FlRbRotateRight(heap, nodeParent);
				sibling = nodeParent->left;
				assert(sibling);
			}
			if (!FlRbIsRed(sibling->left) && !FlRbIsRed(sibling->right))
			{
				FlRbSetColor(sibling, FL_VIRTUAL_HEAP_NODE_RED);
				node = nodeParent;
				nodeParent = node->parent;
				continue;
			}
			if (!FlRbIsRed(sibling->left))
			{
				FlRbSetColor(sibling->right, FL_VIRTUAL_HEAP_NODE_BLACK);
				FlRbSetColor(sibling, FL_VIRTUAL_HEAP_NODE_RED);
				FlRbRotateLeft(heap, sibling);
				sibling = nodeParent->left;
				assert(sibling);
			}
			FlRbSetColor(sibling, FlRbIsRed(nodeParent) ? FL_VIRTUAL_HEAP_NODE_RED : FL_VIRTUAL_HEAP_NODE_BLACK);
			FlRbSetColor(nodeParent, FL_VIRTUAL_HEAP_NODE_BLACK);
			FlRbSetColor(sibling->left, FL_VIRTUAL_HEAP_NODE_BLACK);
			FlRbRotateRight(heap, nodeParent);
			node = heap->treeRoot;
			nodeParent = NULL;
		}
	}
	if (node)
	{
		FlRbSetColor(node, FL_VIRTUAL_HEAP_NODE_BLACK);
	}
}

// Removes node from the tree. node must currently be in the tree.
//
// There is no search phase: every caller already holds the node, derived in
// O(1) from a block address, so the only work is the splice plus the fixup.
static void FlRbDelete(_Inout_ FLVirtualHeap* heap, _Inout_ FLVirtualHeapNode* node)
{
	assert(heap->treeRoot);

	// replacement moves into the removed node's structural position and may be
	// NULL; replacementParent is its parent afterwards, tracked separately
	// because a NULL replacement cannot carry one.
	FLVirtualHeapNode* replacement;
	FLVirtualHeapNode* replacementParent;
	int removedWasRed;

	if (!node->left || !node->right)
	{
		removedWasRed = FlRbIsRed(node);
		replacement = node->left ? node->left : node->right;
		replacementParent = node->parent;
		FlRbTransplant(heap, node, replacement);
	}
	else
	{
		// Two children: the in-order successor takes over the removed node's
		// position, children and color. It is relinked physically rather than
		// key-copied, because a node's identity is its own memory address.
		FLVirtualHeapNode* successor = node->right;
		while (successor->left)
		{
			successor = successor->left;
		}
		removedWasRed = FlRbIsRed(successor);
		replacement = successor->right;
		if (successor->parent == node)
		{
			replacementParent = successor;
		}
		else
		{
			replacementParent = successor->parent;
			FlRbTransplant(heap, successor, successor->right);
			successor->right = node->right;
			successor->right->parent = successor;
		}
		FlRbTransplant(heap, node, successor);
		successor->left = node->left;
		successor->left->parent = successor;
		FlRbSetColor(successor, FlRbIsRed(node) ? FL_VIRTUAL_HEAP_NODE_RED : FL_VIRTUAL_HEAP_NODE_BLACK);
	}

	// Every subtree from replacementParent up lost a node. The spliced-in
	// successor above also inherited an entirely new pair of children, and it
	// always lies on that path, so this pass must be the unconditional one:
	// an early exit could stop below the successor and leave its stale
	// subtreeMax in place.
	FlRbRefreshAugmentationToRoot(replacementParent);

	if (!removedWasRed)
	{
		FlRbDeleteFixup(heap, replacement, replacementParent);
	}
}

// Puts newNode into oldNode's exact in-order slot, taking over its parent,
// children and color, without a search and without a single rotation.
//
// Valid only when no other free block's address lies between the two blocks.
// That is what the coalescing and splitting fast paths rely on: the block
// replacing oldNode's block is carved out of it or directly adjacent to it,
// with nothing free in between, so
//     predecessor(oldNode) < newNode < oldNode < successor(oldNode)
// (or the same with newNode above oldNode) and the address ordering holds
// without moving anything.
//
// The caller passes newNodeSelfSize rather than letting this read the new
// block's header, because the relink must happen BEFORE that block is written:
// when a block is grown in place the tail left over from the absorbed block
// starts as little as 16 bytes past it, so writing the tail's header would
// overwrite the old node's right child pointer. Reading every field of oldNode
// up front makes the two safe to overlap. Callers must therefore follow this
// with the FlVirtualHeapWriteFreeBlock for newNode's block, carrying over
// oldNode's color, which lives in a header word this never touches.
static void FlRbReplaceNode(_Inout_ FLVirtualHeap* heap, _In_ const FLVirtualHeapNode* oldNode, _Inout_ FLVirtualHeapNode* newNode, _In_ size_t newNodeSelfSize)
{
	FLVirtualHeapNode* left = oldNode->left;
	FLVirtualHeapNode* right = oldNode->right;
	FLVirtualHeapNode* parent = oldNode->parent;

	newNode->left = left;
	newNode->right = right;
	newNode->parent = parent;
	if (left)
	{
		left->parent = newNode;
	}
	if (right)
	{
		right->parent = newNode;
	}
	if (!parent)
	{
		heap->treeRoot = newNode;
	}
	else if (parent->left == oldNode)
	{
		parent->left = newNode;
	}
	else
	{
		parent->right = newNode;
	}

	// Inlined FlRbUpdateSubtreeMax: the new block's header does not exist yet, so
	// the self size comes from the caller rather than from FlRbNodeSelfSize.
	size_t maxSize = newNodeSelfSize;
	size_t leftMax = FlRbSubtreeMax(left);
	size_t rightMax = FlRbSubtreeMax(right);
	if (leftMax > maxSize)
	{
		maxSize = leftMax;
	}
	if (rightMax > maxSize)
	{
		maxSize = rightMax;
	}
	newNode->subtreeMax = maxSize;

	FlRbRefreshAugmentationUpward(parent);
}

// Returns the lowest-address free block with size >= need, or NULL.
static FLVirtualHeapNode* FlRbFindFit(FLVirtualHeapNode* root, size_t need)
{
	if (!root)
	{
		return NULL;
	}
	if (root->left && root->left->subtreeMax >= need)
	{
		FLVirtualHeapNode* result = FlRbFindFit(root->left, need);
		if (result)
		{
			return result;
		}
	}
	if (FlRbNodeSelfSize(root) >= need)
	{
		return root;
	}
	if (root->right && root->right->subtreeMax >= need)
	{
		return FlRbFindFit(root->right, need);
	}
	return NULL;
}

// ---------- Platform helpers ----------

_Success_(return != 0) static int FlVirtualHeapInternalGetSystemMemoryInformation(
	_Out_ size_t* allocationGranularityAddress,
	_Out_ size_t* totalPhysicalMemoryAddress,
	_Out_ size_t* totalVirtualMemoryAddress)
{
	SYSTEM_INFO systemInfo;
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&systemInfo);
	size_t largePageMinimum = GetLargePageMinimum();
	size_t allocationGranularity = (size_t)systemInfo.dwAllocationGranularity;
	if (!FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2(allocationGranularity))
	{
		allocationGranularity = 0x10000;
	}
	if (FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2((size_t)systemInfo.dwPageSize) && (size_t)systemInfo.dwPageSize > allocationGranularity)
	{
		allocationGranularity = (size_t)systemInfo.dwPageSize;
	}
	if (FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2(largePageMinimum) && largePageMinimum > allocationGranularity)
	{
		allocationGranularity = largePageMinimum;
	}

	size_t totalVirtualMemory = (size_t)((uintptr_t)systemInfo.lpMaximumApplicationAddress - (uintptr_t)systemInfo.lpMinimumApplicationAddress + 1);

	size_t totalPhysicalMemory;
	MEMORYSTATUSEX memoryStatus;
	memset(&memoryStatus, 0, sizeof(MEMORYSTATUSEX));
	memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memoryStatus))
	{
#ifdef _WIN64
		totalPhysicalMemory = (size_t)memoryStatus.ullTotalPhys;
#else
		totalPhysicalMemory = !(memoryStatus.ullTotalPhys >> 32) ? (size_t)memoryStatus.ullTotalPhys : ULONG_MAX;
#endif
	}
	else
	{
		ULONGLONG physicallyInstalledMemory = 0;
		if (GetPhysicallyInstalledSystemMemory(&physicallyInstalledMemory))
		{
#ifdef _WIN64
			totalPhysicalMemory = !(physicallyInstalledMemory >> 54) ? (size_t)(physicallyInstalledMemory << 10) : ULLONG_MAX;
#else
			totalPhysicalMemory = !(physicallyInstalledMemory >> 22) ? (size_t)(physicallyInstalledMemory << 10) : ULONG_MAX;
#endif
		}
		else
		{
			*allocationGranularityAddress = 0;
			*totalPhysicalMemoryAddress = 0;
			*totalVirtualMemoryAddress = 0;
			return 0;
		}
	}

	*allocationGranularityAddress = allocationGranularity;
	*totalPhysicalMemoryAddress = totalPhysicalMemory;
	*totalVirtualMemoryAddress = totalVirtualMemory;
	return 1;
}

_Success_(return != NULL) static void* FlVirtualHeapInternalReserveMemory(_In_ size_t allocationGranularity, _In_ size_t reserveSize)
{
	assert(FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2(allocationGranularity));
	if (!allocationGranularity || !reserveSize || (reserveSize & (allocationGranularity - 1)))
	{
		return NULL;
	}
	MEM_ADDRESS_REQUIREMENTS addressRequirements = { .LowestStartingAddress = NULL, .HighestEndingAddress = NULL, .Alignment = allocationGranularity };
	MEM_EXTENDED_PARAMETER extendedParameter = { .Type = MemExtendedParameterAddressRequirements, .Pointer = &addressRequirements };
	void* memory = VirtualAlloc2(NULL, NULL, reserveSize, MEM_RESERVE, PAGE_READWRITE, &extendedParameter, 1);
	return memory;
}

_Success_(return != 0) static int FlVirtualHeapInternalCommitRange(_In_ void* address, _In_ size_t size)
{
	return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

_Success_(return != 0) static int FlVirtualHeapInternalDecommitRange(_In_ void* address, _In_ size_t size)
{
	return VirtualFree(address, size, MEM_DECOMMIT) != 0;
}

static void FlVirtualHeapInternalFreeMemory(_In_ void* address)
{
	VirtualFree(address, 0, MEM_RELEASE);
}

static size_t FlVirtualHeapInternalCalculateHeuristicMaximumHeapSize(
	_In_ size_t allocationGranularity,
	_In_ size_t totalPhysicalMemory,
	_In_ size_t totalVirtualMemory)
{
	const size_t platformMinimumHeuristicHeapSize = (size_t)0x20000000; // 512 MiB
	const size_t platformSystemReservedPercentage = 15; // percentage of memory reserved for the rest of the system

	size_t minimumHeuristicHeapSize = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(platformMinimumHeuristicHeapSize, allocationGranularity);
	if (totalPhysicalMemory > ((~(size_t)0) & ~(allocationGranularity - 1)))
	{
		totalPhysicalMemory = ((~(size_t)0) & ~(allocationGranularity - 1));
	}
	totalPhysicalMemory = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(totalPhysicalMemory, allocationGranularity);
	if (totalVirtualMemory > ((~(size_t)0) & ~(allocationGranularity - 1)))
	{
		totalVirtualMemory = ((~(size_t)0) & ~(allocationGranularity - 1));
	}
	totalVirtualMemory = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(totalVirtualMemory, allocationGranularity);
	
	size_t heapSize;
	if ((totalPhysicalMemory / (size_t)100) * platformSystemReservedPercentage >= minimumHeuristicHeapSize)
	{
		heapSize = (totalPhysicalMemory / (size_t)100) * ((size_t)100 - platformSystemReservedPercentage); // This is the normal result. The other code path are only taken on low memory.
	}
	else if(totalPhysicalMemory / (size_t)2 >= minimumHeuristicHeapSize)
	{
		heapSize = totalPhysicalMemory - minimumHeuristicHeapSize;
	}
	else
	{
		heapSize = totalPhysicalMemory / (size_t)2;
	}
	if (heapSize < allocationGranularity)
	{
		heapSize = allocationGranularity;
	}
	heapSize &= ~(allocationGranularity - (size_t)1);

	if (heapSize > ((totalVirtualMemory / (size_t)2) & ~(allocationGranularity - (size_t)1)))
	{
		heapSize = ((totalVirtualMemory / (size_t)2) & ~(allocationGranularity - (size_t)1));
	}
	return heapSize;
}

// ---------- Public API ----------

HRESULT FlVirtualHeapCreate(_Out_ FLVirtualHeap** heap, _In_ size_t size)
{
	assert(FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE == 40);
	assert((FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE & 0xF) == 0x8);
	assert(FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE == 48);
	assert(FL_VIRTUAL_HEAP_EPILOGUE_SIZE == 8);
	assert(sizeof(FLVirtualHeapNode) == 32);

	*heap = NULL;

	size_t allocationGranularity;
	size_t totalPhysicalMemory;
	size_t totalVirtualMemory;
	if (!FlVirtualHeapInternalGetSystemMemoryInformation(&allocationGranularity, &totalPhysicalMemory, &totalVirtualMemory))
	{
		return E_FAIL;
	}

	if (!size)
	{
		size = FlVirtualHeapInternalCalculateHeuristicMaximumHeapSize(allocationGranularity, totalPhysicalMemory, totalVirtualMemory);
		assert(size && (size % allocationGranularity) == 0 && size <= ((~(size_t)0) & ~(allocationGranularity - 1)));
	}
	if (size < FL_VIRTUAL_HEAP_MIN_REGION_SIZE)
	{
		size = FL_VIRTUAL_HEAP_MIN_REGION_SIZE;
		assert(size <= ((~(size_t)0) & ~(allocationGranularity - 1)));
	}
	if (size > ((~(size_t)0) & ~(allocationGranularity - 1)))
	{
		return E_OUTOFMEMORY;
	}
	size = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(size, allocationGranularity);

	void* memory = FlVirtualHeapInternalReserveMemory(allocationGranularity, size);
	if (!memory)
	{
		return E_OUTOFMEMORY;
	}

	size_t committedSize = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(FL_VIRTUAL_HEAP_MIN_REGION_SIZE, allocationGranularity);
	if (!FlVirtualHeapInternalCommitRange(memory, committedSize))
	{
		FlVirtualHeapInternalFreeMemory(memory);
		return E_OUTOFMEMORY;
	}

	FLVirtualHeap* heapDescriptor = (FLVirtualHeap*)memory;
	heapDescriptor->regionReservedSize = size;
	heapDescriptor->regionCommittedSize = committedSize;
	heapDescriptor->allocationGranularity = allocationGranularity;

	void* firstBlock = FlVirtualHeapFirstBlock(heapDescriptor);
	void* epilogueBlock = FlVirtualHeapEpilogueBlock(heapDescriptor);
	size_t spanningBlockSize = (uintptr_t)epilogueBlock - (uintptr_t)firstBlock;

	// The spanning block is the whole tree: black root, no children. Its header
	// was just written without FL_VIRTUAL_HEAP_RED_FLAG, so it is already black.
	FlVirtualHeapWriteFreeBlock(firstBlock, spanningBlockSize, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
	FLVirtualHeapNode* spanningNode = FlVirtualHeapBlockNode(firstBlock);
	spanningNode->left = NULL;
	spanningNode->right = NULL;
	spanningNode->parent = NULL;
	spanningNode->subtreeMax = spanningBlockSize;
	heapDescriptor->treeRoot = spanningNode;

	*(size_t*)epilogueBlock = FL_VIRTUAL_HEAP_ALLOCATED_FLAG;

	*heap = heapDescriptor;
	return S_OK;
}

void FlVirtualHeapDelete(_In_ FLVirtualHeap* heap)
{
	assert(heap);
	FlVirtualHeapInternalFreeMemory(heap);
}

// Ensures at least additionalNeeded bytes of free space exist immediately
// before the epilogue, committing more memory in allocationGranularity
// increments if necessary. Returns non-zero on success, zero if doing so
// would exceed regionReservedSize.
static int FlVirtualHeapInternalGrow(_Inout_ FLVirtualHeap* heap, _In_ size_t additionalNeeded)
{
	void* oldEpilogue = FlVirtualHeapEpilogueBlock(heap);
	size_t epilogueHeaderWord = *FlVirtualHeapHeaderAt(oldEpilogue);

	void* trailingBlock;
	size_t trailingSize;
	size_t trailingPreservedFlags;
	if (FlVirtualHeapIsPrevAllocated(epilogueHeaderWord))
	{
		trailingBlock = oldEpilogue;
		trailingSize = 0;
		trailingPreservedFlags = FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG;
	}
	else
	{
		size_t footerWord = *(size_t*)((uintptr_t)oldEpilogue - 8);
		trailingSize = FlVirtualHeapBlockSize(footerWord);
		trailingBlock = (void*)((uintptr_t)oldEpilogue - trailingSize);
		// The color comes along too: an existing trailing free block keeps its
		// node, and therefore its color, across the growth.
		trailingPreservedFlags = *FlVirtualHeapHeaderAt(trailingBlock) &
			(FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG | FL_VIRTUAL_HEAP_RED_FLAG);
	}

	if (additionalNeeded <= trailingSize)
	{
		return 1;
	}

	size_t shortfall = additionalNeeded - trailingSize;
	size_t growAmount = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(shortfall, heap->allocationGranularity);
	if (growAmount > heap->regionReservedSize - heap->regionCommittedSize)
	{
		return 0;
	}

	void* commitAddress = (void*)((uintptr_t)heap + heap->regionCommittedSize);
	if (!FlVirtualHeapInternalCommitRange(commitAddress, growAmount))
	{
		return 0;
	}

	heap->regionCommittedSize += growAmount;

	size_t newTrailingSize = trailingSize + growAmount;
	FlVirtualHeapWriteFreeBlock(trailingBlock, newTrailingSize, trailingPreservedFlags);
	if (trailingSize)
	{
		// The trailing free block only got longer. Its address is unchanged, so
		// its node keeps its slot in the address-ordered tree and only the
		// subtreeMax augmentation needs republishing.
		FlRbRefreshAugmentationUpward(FlVirtualHeapBlockNode(trailingBlock));
	}
	else
	{
		// There was no trailing free block; the newly committed space becomes
		// one, starting where the old epilogue sentinel was.
		FlRbInsert(heap, FlVirtualHeapBlockNode(trailingBlock));
	}

	*(size_t*)FlVirtualHeapEpilogueBlock(heap) = FL_VIRTUAL_HEAP_ALLOCATED_FLAG;

	return 1;
}

// Decommits trailing pages that are no longer needed after a free/shrink
// operation leaves trailingFreeBlock as the last block before the epilogue,
// keeping one allocationGranularity unit of headroom to avoid commit/
// decommit churn under repeated allocation at the boundary.
static void FlVirtualHeapInternalShrink(_Inout_ FLVirtualHeap* heap, _In_ void* trailingFreeBlock)
{
	size_t allocationGranularity = heap->allocationGranularity;
	size_t headerWord = *FlVirtualHeapHeaderAt(trailingFreeBlock);
	size_t preservedFlags = headerWord & (FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG | FL_VIRTUAL_HEAP_RED_FLAG);
	size_t trailingSize = FlVirtualHeapBlockSize(headerWord);
	size_t trailingOffset = (uintptr_t)trailingFreeBlock - (uintptr_t)heap;

	size_t committedFloor = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(FL_VIRTUAL_HEAP_MIN_REGION_SIZE, allocationGranularity);
	// Smallest committed size that still fully backs the block and the
	// epilogue after it, plus one allocation-granularity unit of headroom.
	size_t target = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(trailingOffset + FL_VIRTUAL_HEAP_EPILOGUE_SIZE, allocationGranularity) + allocationGranularity;
	if (target < committedFloor)
	{
		target = committedFloor;
	}
	if (target >= heap->regionCommittedSize)
	{
		return;
	}

	size_t decommitAmount = heap->regionCommittedSize - target;
	void* decommitAddress = (void*)((uintptr_t)heap + target);
	if (!FlVirtualHeapInternalDecommitRange(decommitAddress, decommitAmount))
	{
		return;
	}

	heap->regionCommittedSize = target;
	size_t newTrailingSize = trailingSize - decommitAmount;

	// Only the trailing block's length changes; its address, and therefore its
	// slot in the address-ordered tree and its color, stay as they are.
	FlVirtualHeapWriteFreeBlock(trailingFreeBlock, newTrailingSize, preservedFlags);
	FlRbRefreshAugmentationUpward(FlVirtualHeapBlockNode(trailingFreeBlock));

	*(size_t*)FlVirtualHeapEpilogueBlock(heap) = FL_VIRTUAL_HEAP_ALLOCATED_FLAG;
}

_Ret_maybenull_ _Post_writable_byte_size_(size) DECLSPEC_ALLOCATOR void* FlVirtualHeapAllocate(_Inout_ FLVirtualHeap* heap, _In_ size_t size)
{
	assert(heap);

	if (!size)
	{
		return NULL;
	}

	if (size > (SIZE_MAX - 8 - 15))
	{
		return NULL;
	}
	size_t need = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(size + 8, 16);
	if (need < FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
	{
		need = FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE;
	}

	FLVirtualHeapNode* fitNode = FlRbFindFit(heap->treeRoot, need);
	if (!fitNode)
	{
		if (!FlVirtualHeapInternalGrow(heap, need))
		{
			return NULL;
		}
		fitNode = FlRbFindFit(heap->treeRoot, need);
		if (!fitNode)
		{
			return NULL;
		}
	}

	void* blockAddress = FlVirtualHeapNodeBlock(fitNode);
	size_t headerWord = *FlVirtualHeapHeaderAt(blockAddress);
	size_t blockSize = FlVirtualHeapBlockSize(headerWord);
	size_t prevAllocatedFlag = headerWord & FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG;

	size_t remainder = blockSize - need;
	if (remainder >= FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
	{
		// The remainder is carved out of the block being handed out, so nothing
		// free lies between the two and the tail's node belongs in exactly the
		// slot the fit block's node occupies. Swapping it in costs no search
		// and no rebalancing, unlike the delete plus insert it replaces.
		void* tailBlock = (void*)((uintptr_t)blockAddress + need);
		FlRbReplaceNode(heap, fitNode, FlVirtualHeapBlockNode(tailBlock), remainder);
		FlVirtualHeapWriteFreeBlock(tailBlock, remainder,
			FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG | (headerWord & FL_VIRTUAL_HEAP_RED_FLAG));
		FlVirtualHeapSetNextPrevAllocated(tailBlock, remainder, 0);

		FlVirtualHeapWriteBlockHeader(blockAddress, need, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
	}
	else
	{
		FlRbDelete(heap, fitNode);
		FlVirtualHeapWriteBlockHeader(blockAddress, blockSize, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
		FlVirtualHeapSetNextPrevAllocated(blockAddress, blockSize, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
	}

	return FlVirtualHeapBlockPayload(blockAddress);
}

void FlVirtualHeapFree(_Inout_ FLVirtualHeap* heap, _Pre_maybenull_ _Post_invalid_ void* allocation)
{
	assert(heap);

	if (!allocation)
	{
		return;
	}

	assert(((uintptr_t)allocation & 0xF) == 0);
	assert((uintptr_t)allocation >= (uintptr_t)FlVirtualHeapFirstBlock(heap) + 8);
	assert((uintptr_t)allocation < (uintptr_t)FlVirtualHeapEpilogueBlock(heap));

	void* blockAddress = FlVirtualHeapPayloadBlock(allocation);
	size_t headerWord = *FlVirtualHeapHeaderAt(blockAddress);

	assert(FlVirtualHeapIsAllocated(headerWord));

	size_t blockSize = FlVirtualHeapBlockSize(headerWord);
	size_t prevAllocatedFlag = headerWord & FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG;

	// Look at both neighbours before touching the tree, so the four coalescing
	// outcomes can each take the cheapest route into it. The block being freed
	// is allocated and the two neighbours are adjacent to it, so no free block
	// lies strictly between them: whatever the merged block turns out to be, it
	// occupies the in-order slot of at most one existing tree node and there is
	// never anything to reorder.
	void* nextBlock = (void*)((uintptr_t)blockAddress + blockSize);
	size_t nextHeaderWord = *FlVirtualHeapHeaderAt(nextBlock);
	int nextIsFree = !FlVirtualHeapIsAllocated(nextHeaderWord);

	void* prevBlock = NULL;
	size_t prevSize = 0;
	if (!FlVirtualHeapIsPrevAllocated(headerWord))
	{
		size_t prevFooterWord = *((size_t*)((uintptr_t)blockAddress - 8));
		prevSize = FlVirtualHeapBlockSize(prevFooterWord);
		prevBlock = (void*)((uintptr_t)blockAddress - prevSize);
	}

	if (prevBlock)
	{
		// Merging backward: the merged block starts at prevBlock, which is
		// already in the tree at exactly that address. Its key does not change,
		// only its size, so the tree structure is left completely untouched and
		// only the subtreeMax augmentation has to be republished upwards.
		if (nextIsFree)
		{
			// The block after this one disappears into the merge, so it is the
			// one node that genuinely has to be removed. Do that first, while
			// the tree still agrees with the block headers.
			FlRbDelete(heap, FlVirtualHeapBlockNode(nextBlock));
			blockSize += FlVirtualHeapBlockSize(nextHeaderWord);
		}

		blockSize += prevSize;
		blockAddress = prevBlock;

		// prevBlock's header must be read after the delete above: its fixup can
		// recolor any node still in the tree, prevBlock's included, and that
		// color has to survive this rewrite because the node is staying put.
		size_t prevHeaderWord = *FlVirtualHeapHeaderAt(prevBlock);
		FlVirtualHeapWriteFreeBlock(blockAddress, blockSize,
			prevHeaderWord & (FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG | FL_VIRTUAL_HEAP_RED_FLAG));
		FlRbRefreshAugmentationUpward(FlVirtualHeapBlockNode(blockAddress));
	}
	else if (nextIsFree)
	{
		// Merging forward only: the merged block starts at blockAddress and
		// swallows nextBlock. Nothing free lies between the two, so the merged
		// block's node belongs in precisely the slot nextBlock's node occupies
		// and can be swapped in without any comparison or rotation.
		const FLVirtualHeapNode* nextNode = FlVirtualHeapBlockNode(nextBlock);
		blockSize += FlVirtualHeapBlockSize(nextHeaderWord);

		FlRbReplaceNode(heap, nextNode, FlVirtualHeapBlockNode(blockAddress), blockSize);
		FlVirtualHeapWriteFreeBlock(blockAddress, blockSize,
			prevAllocatedFlag | (nextHeaderWord & FL_VIRTUAL_HEAP_RED_FLAG));
	}
	else
	{
		// No neighbour to merge with: a genuinely new free block.
		FlVirtualHeapWriteFreeBlock(blockAddress, blockSize, prevAllocatedFlag);
		FlRbInsert(heap, FlVirtualHeapBlockNode(blockAddress));
	}

	FlVirtualHeapSetNextPrevAllocated(blockAddress, blockSize, 0);

	if ((uintptr_t)blockAddress + blockSize == (uintptr_t)FlVirtualHeapEpilogueBlock(heap))
	{
		FlVirtualHeapInternalShrink(heap, blockAddress);
	}
}

_Ret_maybenull_ _Post_writable_byte_size_(size) DECLSPEC_ALLOCATOR void* FlVirtualHeapReAllocate(
	_Inout_ FLVirtualHeap* heap,
	_Pre_maybenull_ _Post_invalid_ void* allocation,
	_In_ size_t size)
{
	assert(heap);

	if (!allocation)
	{
		return FlVirtualHeapAllocate(heap, size);
	}

	if (!size)
	{
		FlVirtualHeapFree(heap, allocation);
		return NULL;
	}

	assert(((uintptr_t)allocation & 0xF) == 0);
	assert((uintptr_t)allocation >= (uintptr_t)FlVirtualHeapFirstBlock(heap) + 8);
	assert((uintptr_t)allocation < (uintptr_t)FlVirtualHeapEpilogueBlock(heap));

	void* blockAddress = FlVirtualHeapPayloadBlock(allocation);
	size_t headerWord = *FlVirtualHeapHeaderAt(blockAddress);

	assert(FlVirtualHeapIsAllocated(headerWord));

	size_t blockSize = FlVirtualHeapBlockSize(headerWord);
	size_t prevAllocatedFlag = headerWord & FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG;
	size_t currentPayloadSize = blockSize - 8;

	if (size > (SIZE_MAX - 8 - 15))
	{
		return NULL;
	}
	size_t need = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(size + 8, 16);
	if (need < FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
	{
		need = FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE;
	}

	// Shrink in place.
	if (need <= blockSize)
	{
		size_t remainder = blockSize - need;
		if (remainder >= FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
		{
			// Split off a free tail and coalesce it forward if possible.
			void* tailBlock = (void*)((uintptr_t)blockAddress + need);
			void* nextBlock = (void*)((uintptr_t)blockAddress + blockSize);
			size_t nextHeaderWord = *FlVirtualHeapHeaderAt(nextBlock);

			FlVirtualHeapWriteBlockHeader(blockAddress, need, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);

			if (!FlVirtualHeapIsAllocated(nextHeaderWord))
			{
				// The tail is adjacent to an already free block and absorbs it,
				// with nothing free in between, so it inherits that block's slot.
				const FLVirtualHeapNode* nextNode = FlVirtualHeapBlockNode(nextBlock);
				remainder += FlVirtualHeapBlockSize(nextHeaderWord);
				FlRbReplaceNode(heap, nextNode, FlVirtualHeapBlockNode(tailBlock), remainder);
				FlVirtualHeapWriteFreeBlock(tailBlock, remainder,
					FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG | (nextHeaderWord & FL_VIRTUAL_HEAP_RED_FLAG));
			}
			else
			{
				// A brand new free block in what was the middle of an allocation.
				FlVirtualHeapWriteFreeBlock(tailBlock, remainder, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
				FlRbInsert(heap, FlVirtualHeapBlockNode(tailBlock));
			}
			FlVirtualHeapSetNextPrevAllocated(tailBlock, remainder, 0);

			if ((uintptr_t)tailBlock + remainder == (uintptr_t)FlVirtualHeapEpilogueBlock(heap))
			{
				FlVirtualHeapInternalShrink(heap, tailBlock);
			}
		}
		return allocation;
	}

	// Grow in place: absorb the immediately following free block, growing the
	// committed region first if that block (or the tail, if none exists yet)
	// does not already reach far enough and it sits at the end of the heap.
	void* nextBlock = (void*)((uintptr_t)blockAddress + blockSize);
	size_t nextHeaderWord = *FlVirtualHeapHeaderAt(nextBlock);
	size_t nextSize = FlVirtualHeapIsAllocated(nextHeaderWord) ? 0 : FlVirtualHeapBlockSize(nextHeaderWord);
	if (blockSize + nextSize < need)
	{
		void* epilogueBlock = FlVirtualHeapEpilogueBlock(heap);
		int atTail = nextSize
			? ((uintptr_t)nextBlock + nextSize == (uintptr_t)epilogueBlock)
			: ((uintptr_t)nextBlock == (uintptr_t)epilogueBlock);
		if (atTail && FlVirtualHeapInternalGrow(heap, need - blockSize))
		{
			nextHeaderWord = *FlVirtualHeapHeaderAt(nextBlock);
			nextSize = FlVirtualHeapBlockSize(nextHeaderWord);
		}
	}
	if (!FlVirtualHeapIsAllocated(nextHeaderWord))
	{
		size_t combinedSize = blockSize + nextSize;
		if (combinedSize >= need)
		{
			size_t remainder = combinedSize - need;
			if (remainder >= FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
			{
				// The tail is what is left of nextBlock once the allocation has
				// eaten into it, so it lies inside nextBlock's old extent and
				// takes over its slot. Note the tail can start as little as 16
				// bytes past nextBlock, overlapping its node -- which is why
				// FlRbReplaceNode reads that node before this writes the tail.
				const FLVirtualHeapNode* nextNode = FlVirtualHeapBlockNode(nextBlock);
				void* tailBlock = (void*)((uintptr_t)blockAddress + need);
				FlRbReplaceNode(heap, nextNode, FlVirtualHeapBlockNode(tailBlock), remainder);
				FlVirtualHeapWriteFreeBlock(tailBlock, remainder,
					FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG | (nextHeaderWord & FL_VIRTUAL_HEAP_RED_FLAG));
				FlVirtualHeapSetNextPrevAllocated(tailBlock, remainder, 0);
				FlVirtualHeapWriteBlockHeader(blockAddress, need, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
			}
			else
			{
				FlRbDelete(heap, FlVirtualHeapBlockNode(nextBlock));
				FlVirtualHeapWriteBlockHeader(blockAddress, combinedSize, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
				FlVirtualHeapSetNextPrevAllocated(blockAddress, combinedSize, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
			}
			return allocation;
		}
	}

	// Fallback: allocate new, copy, free old.
	void* newAllocation = FlVirtualHeapAllocate(heap, size);
	if (!newAllocation)
	{
		return NULL;
	}
	size_t copySize = currentPayloadSize < size ? currentPayloadSize : size;
	memcpy(newAllocation, allocation, copySize);
	FlVirtualHeapFree(heap, allocation);
	return newAllocation;
}

// Returns the black-height of the subtree rooted at node (the number of
// black nodes on any root-to-null-leaf path, not counting node itself iff it
// is red) if every such path has an equal count, or -1 if the subtree
// violates that invariant.
static long FlRbValidateBlackHeight(const FLVirtualHeapNode* node)
{
	if (!node)
	{
		return 0;
	}
	long leftHeight = FlRbValidateBlackHeight(node->left);
	if (leftHeight < 0)
	{
		return -1;
	}
	long rightHeight = FlRbValidateBlackHeight(node->right);
	if (rightHeight < 0 || rightHeight != leftHeight)
	{
		return -1;
	}
	return leftHeight + (FlRbIsRed(node) ? 0 : 1);
}

int FlVirtualHeapValidate(_In_ FLVirtualHeap* heap)
{
	assert(heap);

	if (heap->regionReservedSize < FL_VIRTUAL_HEAP_MIN_REGION_SIZE)
	{
		return 0;
	}

	size_t committedFloor = FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(FL_VIRTUAL_HEAP_MIN_REGION_SIZE, heap->allocationGranularity);
	if (!heap->regionCommittedSize ||
		(heap->regionCommittedSize % heap->allocationGranularity) != 0 ||
		heap->regionCommittedSize < committedFloor ||
		heap->regionCommittedSize > heap->regionReservedSize)
	{
		return 0;
	}

	uintptr_t heapBase = (uintptr_t)heap;
	uintptr_t regionEnd = heapBase + heap->regionCommittedSize;
	uintptr_t epilogueAddress = regionEnd - FL_VIRTUAL_HEAP_EPILOGUE_SIZE;

	// Linear walk: validate every block.
	uintptr_t current = heapBase + FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE;
	int previousWasFree = 0;
	size_t previousBlockSize = 0;
	size_t freeBlockCount = 0;

	while (current < regionEnd)
	{
		size_t hdr = *(size_t*)current;
		size_t blockSize = FlVirtualHeapBlockSize(hdr);
		int isAllocated = FlVirtualHeapIsAllocated(hdr);
		int isPrevAllocated = FlVirtualHeapIsPrevAllocated(hdr);

		if (current == epilogueAddress)
		{
			// Epilogue sentinel: only ALLOCATED flag is required; size bits are 0.
			if (!isAllocated)
			{
				return 0;
			}
			// A free block directly before the epilogue must be smaller than
			// two allocation-granularity units; anything at or above that
			// should already have been decommitted by FlVirtualHeapInternalShrink.
			if (previousWasFree && previousBlockSize >= 2 * heap->allocationGranularity)
			{
				return 0;
			}
			current += FL_VIRTUAL_HEAP_EPILOGUE_SIZE;
			continue;
		}

		// Regular block checks.
		if (blockSize < FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE || (blockSize & 0xF) != 0)
		{
			return 0;
		}
		if (current + blockSize > regionEnd)
		{
			return 0;
		}

		// PREV_ALLOCATED must reflect the previous block.
		if (current == heapBase + FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE)
		{
			// First block: descriptor is always "allocated".
			if (!isPrevAllocated)
			{
				return 0;
			}
		}
		else
		{
			int expectedPrevAllocated = !previousWasFree;
			if (isPrevAllocated != expectedPrevAllocated)
			{
				return 0;
			}
		}

		// No two adjacent free blocks allowed (coalescing must have merged them).
		if (!isAllocated && previousWasFree)
		{
			return 0;
		}

		// Only free blocks carry a red-black tree color.
		if (isAllocated && (hdr & FL_VIRTUAL_HEAP_RED_FLAG))
		{
			return 0;
		}

		if (!isAllocated)
		{
			// Free block: footer must mirror the header, except for the color
			// bit, which the footer never carries.
			size_t footer = *FlVirtualHeapFooterAt((void*)current, blockSize);
			if (footer != (hdr & ~FL_VIRTUAL_HEAP_RED_FLAG))
			{
				return 0;
			}
			// Payload (block+8) must be 16-byte aligned.
			if (((current + 8) & 0xF) != 0)
			{
				return 0;
			}
			freeBlockCount++;
		}

		previousWasFree = !isAllocated;
		previousBlockSize = blockSize;
		current += blockSize;
	}

	if (current != regionEnd)
	{
		return 0;
	}

	// Red-black tree walk: collect free blocks in address order and cross-check.
	// Uses an iterative in-order traversal with a small stack.
#define FL_VIRTUAL_HEAP_VALIDATE_STACK_SIZE 64
	FLVirtualHeapNode* stack[FL_VIRTUAL_HEAP_VALIDATE_STACK_SIZE];
	int stackTop = 0;
	size_t treeNodeCount = 0;
	uintptr_t previousNodeAddress = 0;
	FLVirtualHeapNode* treeNode = heap->treeRoot;

	while (treeNode || stackTop > 0)
	{
		while (treeNode)
		{
			if (stackTop >= FL_VIRTUAL_HEAP_VALIDATE_STACK_SIZE)
			{
				return 0;
			}
			stack[stackTop++] = treeNode;
			treeNode = treeNode->left;
		}
		treeNode = stack[--stackTop];

		void* nodeBlock = FlVirtualHeapNodeBlock(treeNode);
		uintptr_t nodeBlockAddress = (uintptr_t)nodeBlock;

		// Addresses must be strictly increasing (address-ordered BST).
		if (nodeBlockAddress <= previousNodeAddress && treeNodeCount > 0)
		{
			return 0;
		}

		// Block must be in the valid user range and not the epilogue.
		if (nodeBlockAddress < heapBase + FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE ||
			nodeBlockAddress >= epilogueAddress)
		{
			return 0;
		}

		size_t nodeHdr = *(size_t*)nodeBlockAddress;
		size_t nodeBlockSize = FlVirtualHeapBlockSize(nodeHdr);

		// Must be marked free.
		if (FlVirtualHeapIsAllocated(nodeHdr))
		{
			return 0;
		}

		// subtreeMax must equal max of self, left, right.
		size_t expectedMax = nodeBlockSize;
		size_t leftMax = FlRbSubtreeMax(treeNode->left);
		size_t rightMax = FlRbSubtreeMax(treeNode->right);
		if (leftMax > expectedMax)
		{
			expectedMax = leftMax;
		}
		if (rightMax > expectedMax)
		{
			expectedMax = rightMax;
		}
		if (treeNode->subtreeMax != expectedMax)
		{
			return 0;
		}

		// Parent links must agree with the child pointers in both directions.
		if (treeNode->left && treeNode->left->parent != treeNode)
		{
			return 0;
		}
		if (treeNode->right && treeNode->right->parent != treeNode)
		{
			return 0;
		}

		// A red node cannot have a red child.
		if (FlRbIsRed(treeNode) && (FlRbIsRed(treeNode->left) || FlRbIsRed(treeNode->right)))
		{
			return 0;
		}

		treeNodeCount++;
		previousNodeAddress = nodeBlockAddress;
		treeNode = treeNode->right;
	}
#undef FL_VIRTUAL_HEAP_VALIDATE_STACK_SIZE

	// Number of free blocks in the linear walk must match the red-black tree.
	if (freeBlockCount != treeNodeCount)
	{
		return 0;
	}

	// The root must be black, must have no parent, and every root-to-null-leaf
	// path must cross the same number of black nodes.
	if (heap->treeRoot && (FlRbIsRed(heap->treeRoot) || heap->treeRoot->parent != NULL))
	{
		return 0;
	}
	if (FlRbValidateBlackHeight(heap->treeRoot) < 0)
	{
		return 0;
	}

	return 1;
}
