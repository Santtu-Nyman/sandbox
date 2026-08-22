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

			[sizeof(FLVirtualHeap) .. regionSize - EPILOGUE_SIZE - 1]
				User blocks.  Each block begins with an 8-byte header word:
					bits [63..4]  block size (multiple of 16)
					bit  [1]      PREV_ALLOCATED (previous block is allocated)
					bit  [0]      ALLOCATED
				Free blocks also have an 8-byte footer at block+size-8 that
				mirrors the header, and carry an FLVirtualHeapNode in the
				first 32 bytes of their payload (block+8).

			[regionSize - 16 .. regionSize - 1]
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
	return (void*)((uintptr_t)heap + heap->regionSize - FL_VIRTUAL_HEAP_EPILOGUE_SIZE);
}

// Writes the header (and footer for free blocks) for a block.
static void FlVirtualHeapWriteBlockHeader(void* blockAddress, size_t blockSize, size_t flags)
{
	size_t headerWord = blockSize | flags;
	*FlVirtualHeapHeaderAt(blockAddress) = headerWord;
}

static void FlVirtualHeapWriteFreeBlock(void* blockAddress, size_t blockSize, size_t prevAllocatedFlag)
{
	size_t headerWord = blockSize | prevAllocatedFlag;
	*FlVirtualHeapHeaderAt(blockAddress) = headerWord;
	*FlVirtualHeapFooterAt(blockAddress, blockSize) = headerWord;
}

// Updates the PREV_ALLOCATED flag in the next block's header.
static void FlVirtualHeapSetNextPrevAllocated(void* blockAddress, size_t blockSize, size_t prevAllocatedFlag)
{
	size_t* nextHeader = FlVirtualHeapNextBlockHeader(blockAddress, blockSize);
	*nextHeader = (*nextHeader & ~FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG) | prevAllocatedFlag;
}

// ---------- AVL tree ----------

static uint32_t AvlHeight(const FLVirtualHeapNode* node)
{
	return node ? node->height : 0;
}

static size_t AvlSubtreeMax(const FLVirtualHeapNode* node)
{
	return node ? node->subtreeMax : 0;
}

static void AvlUpdate(FLVirtualHeapNode* node)
{
	uint32_t leftHeight = AvlHeight(node->left);
	uint32_t rightHeight = AvlHeight(node->right);
	node->height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);

	void* blockAddress = FlVirtualHeapNodeBlock(node);
	size_t selfSize = FlVirtualHeapBlockSize(*FlVirtualHeapHeaderAt(blockAddress));
	size_t leftMax = AvlSubtreeMax(node->left);
	size_t rightMax = AvlSubtreeMax(node->right);
	size_t maxSize = selfSize;
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

static FLVirtualHeapNode* AvlRotateRight(FLVirtualHeapNode* y)
{
	FLVirtualHeapNode* x = y->left;
	FLVirtualHeapNode* t2 = x->right;
	x->right = y;
	y->left = t2;
	AvlUpdate(y);
	AvlUpdate(x);
	return x;
}

static FLVirtualHeapNode* AvlRotateLeft(FLVirtualHeapNode* x)
{
	FLVirtualHeapNode* y = x->right;
	FLVirtualHeapNode* t2 = y->left;
	y->left = x;
	x->right = t2;
	AvlUpdate(x);
	AvlUpdate(y);
	return y;
}

static int AvlBalance(const FLVirtualHeapNode* node)
{
	return (int)AvlHeight(node->left) - (int)AvlHeight(node->right);
}

static FLVirtualHeapNode* AvlRebalance(FLVirtualHeapNode* node)
{
	AvlUpdate(node);
	int balance = AvlBalance(node);
	if (balance > 1)
	{
		if (AvlBalance(node->left) < 0)
		{
			node->left = AvlRotateLeft(node->left);
		}
		return AvlRotateRight(node);
	}
	if (balance < -1)
	{
		if (AvlBalance(node->right) > 0)
		{
			node->right = AvlRotateRight(node->right);
		}
		return AvlRotateLeft(node);
	}
	return node;
}

static FLVirtualHeapNode* AvlInsert(FLVirtualHeapNode* root, FLVirtualHeapNode* newNode)
{
	if (!root)
	{
		newNode->left = NULL;
		newNode->right = NULL;
		newNode->height = 1;
		void* blockAddress = FlVirtualHeapNodeBlock(newNode);
		newNode->subtreeMax = FlVirtualHeapBlockSize(*FlVirtualHeapHeaderAt(blockAddress));
		return newNode;
	}
	void* rootBlock = FlVirtualHeapNodeBlock(root);
	void* newBlock = FlVirtualHeapNodeBlock(newNode);
	if ((uintptr_t)newBlock < (uintptr_t)rootBlock)
	{
		root->left = AvlInsert(root->left, newNode);
	}
	else
	{
		root->right = AvlInsert(root->right, newNode);
	}
	return AvlRebalance(root);
}

static FLVirtualHeapNode* AvlMinNode(FLVirtualHeapNode* node)
{
	while (node->left)
	{
		node = node->left;
	}
	return node;
}

static FLVirtualHeapNode* AvlDelete(FLVirtualHeapNode* root, FLVirtualHeapNode* target)
{
	if (!root)
	{
		return NULL;
	}
	void* rootBlock = FlVirtualHeapNodeBlock(root);
	void* targetBlock = FlVirtualHeapNodeBlock(target);
	if ((uintptr_t)targetBlock < (uintptr_t)rootBlock)
	{
		root->left = AvlDelete(root->left, target);
	}
	else if ((uintptr_t)targetBlock > (uintptr_t)rootBlock)
	{
		root->right = AvlDelete(root->right, target);
	}
	else
	{
		if (!root->left)
		{
			return root->right;
		}
		if (!root->right)
		{
			return root->left;
		}
		FLVirtualHeapNode* successor = AvlMinNode(root->right);
		root->right = AvlDelete(root->right, successor);
		successor->left = root->left;
		successor->right = root->right;
		root = successor;
	}
	return AvlRebalance(root);
}

// Returns the lowest-address free block with size >= need, or NULL.
static FLVirtualHeapNode* AvlFindFit(FLVirtualHeapNode* root, size_t need)
{
	if (!root)
	{
		return NULL;
	}
	if (root->left && root->left->subtreeMax >= need)
	{
		FLVirtualHeapNode* result = AvlFindFit(root->left, need);
		if (result)
		{
			return result;
		}
	}
	void* blockAddress = FlVirtualHeapNodeBlock(root);
	size_t blockSize = FlVirtualHeapBlockSize(*FlVirtualHeapHeaderAt(blockAddress));
	if (blockSize >= need)
	{
		return root;
	}
	if (root->right && root->right->subtreeMax >= need)
	{
		return AvlFindFit(root->right, need);
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

_Success_(return != NULL) static void* FlVirtualHeapInternalCommitMemory(_In_ size_t allocationGranularity, _In_ size_t commitSize)
{
	assert(FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2(allocationGranularity));
	if (!allocationGranularity || !commitSize || (commitSize & (allocationGranularity - 1)))
	{
		return NULL;
	}
	MEM_ADDRESS_REQUIREMENTS addressRequirements = { .LowestStartingAddress = NULL, .HighestEndingAddress = NULL, .Alignment = allocationGranularity };
	MEM_EXTENDED_PARAMETER extendedParameter = { .Type = MemExtendedParameterAddressRequirements, .Pointer = &addressRequirements };
	void* memory = VirtualAlloc2(NULL, NULL, commitSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, &extendedParameter, 1);
	return memory;
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
	assert(FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE == 24);
	assert((FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE & 0xF) == 8);
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

	void* memory = FlVirtualHeapInternalCommitMemory(allocationGranularity, size);
	if (!memory)
	{
		return E_OUTOFMEMORY;
	}

	FLVirtualHeap* heapDescriptor = (FLVirtualHeap*)memory;
	heapDescriptor->regionSize = size;
	heapDescriptor->allocationGranularity = allocationGranularity;

	void* firstBlock = FlVirtualHeapFirstBlock(heapDescriptor);
	void* epilogueBlock = FlVirtualHeapEpilogueBlock(heapDescriptor);
	size_t spanningBlockSize = (uintptr_t)epilogueBlock - (uintptr_t)firstBlock;

	FlVirtualHeapWriteFreeBlock(firstBlock, spanningBlockSize, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
	FLVirtualHeapNode* spanningNode = FlVirtualHeapBlockNode(firstBlock);
	spanningNode->left = NULL;
	spanningNode->right = NULL;
	spanningNode->height = 1;
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

	FLVirtualHeapNode* fitNode = AvlFindFit(heap->treeRoot, need);
	if (!fitNode)
	{
		return NULL;
	}

	void* blockAddress = FlVirtualHeapNodeBlock(fitNode);
	size_t headerWord = *FlVirtualHeapHeaderAt(blockAddress);
	size_t blockSize = FlVirtualHeapBlockSize(headerWord);
	size_t prevAllocatedFlag = headerWord & FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG;

	heap->treeRoot = AvlDelete(heap->treeRoot, fitNode);

	size_t remainder = blockSize - need;
	if (remainder >= FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
	{
		void* tailBlock = (void*)((uintptr_t)blockAddress + need);
		FlVirtualHeapWriteFreeBlock(tailBlock, remainder, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
		FLVirtualHeapNode* tailNode = FlVirtualHeapBlockNode(tailBlock);
		tailNode->left = NULL;
		tailNode->right = NULL;
		tailNode->height = 1;
		tailNode->subtreeMax = remainder;
		heap->treeRoot = AvlInsert(heap->treeRoot, tailNode);
		FlVirtualHeapSetNextPrevAllocated(tailBlock, remainder, 0);

		FlVirtualHeapWriteBlockHeader(blockAddress, need, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
	}
	else
	{
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

	// Coalesce forward: check next block.
	void* nextBlock = (void*)((uintptr_t)blockAddress + blockSize);
	size_t nextHeaderWord = *FlVirtualHeapHeaderAt(nextBlock);
	if (!FlVirtualHeapIsAllocated(nextHeaderWord))
	{
		size_t nextSize = FlVirtualHeapBlockSize(nextHeaderWord);
		heap->treeRoot = AvlDelete(heap->treeRoot, FlVirtualHeapBlockNode(nextBlock));
		blockSize += nextSize;
	}

	// Coalesce backward: check previous block via PREV_ALLOCATED flag.
	if (!FlVirtualHeapIsPrevAllocated(headerWord))
	{
		size_t prevFooterWord = *((size_t*)((uintptr_t)blockAddress - 8));
		size_t prevSize = FlVirtualHeapBlockSize(prevFooterWord);
		void* prevBlock = (void*)((uintptr_t)blockAddress - prevSize);
		size_t prevHeaderWord = *FlVirtualHeapHeaderAt(prevBlock);
		prevAllocatedFlag = prevHeaderWord & FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG;
		heap->treeRoot = AvlDelete(heap->treeRoot, FlVirtualHeapBlockNode(prevBlock));
		blockSize += prevSize;
		blockAddress = prevBlock;
	}

	FlVirtualHeapWriteFreeBlock(blockAddress, blockSize, prevAllocatedFlag);
	FLVirtualHeapNode* node = FlVirtualHeapBlockNode(blockAddress);
	node->left = NULL;
	node->right = NULL;
	node->height = 1;
	node->subtreeMax = blockSize;
	heap->treeRoot = AvlInsert(heap->treeRoot, node);
	FlVirtualHeapSetNextPrevAllocated(blockAddress, blockSize, 0);
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

			if (!FlVirtualHeapIsAllocated(nextHeaderWord))
			{
				size_t nextSize = FlVirtualHeapBlockSize(nextHeaderWord);
				heap->treeRoot = AvlDelete(heap->treeRoot, FlVirtualHeapBlockNode(nextBlock));
				remainder += nextSize;
			}

			FlVirtualHeapWriteBlockHeader(blockAddress, need, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
			FlVirtualHeapWriteFreeBlock(tailBlock, remainder, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
			FLVirtualHeapNode* tailNode = FlVirtualHeapBlockNode(tailBlock);
			tailNode->left = NULL;
			tailNode->right = NULL;
			tailNode->height = 1;
			tailNode->subtreeMax = remainder;
			heap->treeRoot = AvlInsert(heap->treeRoot, tailNode);
			FlVirtualHeapSetNextPrevAllocated(tailBlock, remainder, 0);
		}
		return allocation;
	}

	// Grow in place: absorb the immediately following free block.
	void* nextBlock = (void*)((uintptr_t)blockAddress + blockSize);
	size_t nextHeaderWord = *FlVirtualHeapHeaderAt(nextBlock);
	if (!FlVirtualHeapIsAllocated(nextHeaderWord))
	{
		size_t nextSize = FlVirtualHeapBlockSize(nextHeaderWord);
		size_t combinedSize = blockSize + nextSize;
		if (combinedSize >= need)
		{
			heap->treeRoot = AvlDelete(heap->treeRoot, FlVirtualHeapBlockNode(nextBlock));
			size_t remainder = combinedSize - need;
			if (remainder >= FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE)
			{
				void* tailBlock = (void*)((uintptr_t)blockAddress + need);
				FlVirtualHeapWriteFreeBlock(tailBlock, remainder, FL_VIRTUAL_HEAP_PREV_ALLOCATED_FLAG);
				FLVirtualHeapNode* tailNode = FlVirtualHeapBlockNode(tailBlock);
				tailNode->left = NULL;
				tailNode->right = NULL;
				tailNode->height = 1;
				tailNode->subtreeMax = remainder;
				heap->treeRoot = AvlInsert(heap->treeRoot, tailNode);
				FlVirtualHeapSetNextPrevAllocated(tailBlock, remainder, 0);
				FlVirtualHeapWriteBlockHeader(blockAddress, need, prevAllocatedFlag | FL_VIRTUAL_HEAP_ALLOCATED_FLAG);
			}
			else
			{
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

int FlVirtualHeapValidate(_In_ FLVirtualHeap* heap)
{
	assert(heap);

	if (heap->regionSize < FL_VIRTUAL_HEAP_MIN_REGION_SIZE)
	{
		return 0;
	}

	uintptr_t heapBase = (uintptr_t)heap;
	uintptr_t regionEnd = heapBase + heap->regionSize;
	uintptr_t epilogueAddress = regionEnd - FL_VIRTUAL_HEAP_EPILOGUE_SIZE;

	// Linear walk: validate every block.
	uintptr_t current = heapBase + FL_VIRTUAL_HEAP_DESCRIPTOR_SIZE;
	int previousWasFree = 0;
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

		if (!isAllocated)
		{
			// Free block: header must equal footer.
			size_t footer = *FlVirtualHeapFooterAt((void*)current, blockSize);
			if (footer != hdr)
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
		current += blockSize;
	}

	if (current != regionEnd)
	{
		return 0;
	}

	// AVL tree walk: collect free blocks in address order and cross-check.
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
		size_t leftMax = AvlSubtreeMax(treeNode->left);
		size_t rightMax = AvlSubtreeMax(treeNode->right);
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

		// AVL balance factor must be in {-1, 0, 1}.
		int balance = AvlBalance(treeNode);
		if (balance < -1 || balance > 1)
		{
			return 0;
		}

		treeNodeCount++;
		previousNodeAddress = nodeBlockAddress;
		treeNode = treeNode->right;
	}
#undef FL_VIRTUAL_HEAP_VALIDATE_STACK_SIZE

	// Number of free blocks in the linear walk must match the AVL tree.
	if (freeBlockCount != treeNodeCount)
	{
		return 0;
	}

	return 1;
}
