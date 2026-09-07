/*
	Virtual page backed heap implementation by Santtu S. Nyman.

	Version history
		version 0.9.0 2026-09-07
			Initial version.

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
*/

#ifndef FL_VIRTUAL_HEAP_H
#define FL_VIRTUAL_HEAP_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include "flHRESULT.h"
#include "FlSAL.h"

// Minimum alignment in bytes for all heap allocations.
#define FL_VIRTUAL_HEAP_ALIGNMENT 0x10

// Evaluates to non-zero when N is a positive power of 2.
#define FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2(N) (((N) - 1) < ((N) ^ ((N) - 1)))

// Rounds N up to the nearest multiple of P, where P must be a power of 2.
#define FL_VIRTUAL_HEAP_ROUND_UP_TO_POWER_OF_2(N, P) (((N) + ((P) - 1)) & ~((P) - 1))

// Node of the red-black tree of free blocks. The node lives in the first 32
// bytes of a free block's payload, so its address is always blockAddress + 8.
//
// The node carries no color field: a free block's header word holds the color
// in its bit 2 (see FL_VIRTUAL_HEAP_RED_FLAG in FlVirtualHeap.c). Keeping the
// color there is what makes room for parent, and the header word is loaded by
// the augmentation code anyway, so reading a node's color and its own block
// size costs a single load.
//
// The structure must stay exactly 32 bytes: FL_VIRTUAL_HEAP_MIN_BLOCK_SIZE is
// 8 (header) + sizeof(FLVirtualHeapNode) + 8 (footer), and every block must be
// large enough to hold a node so that any allocated block can be freed without
// reallocating.
typedef struct FLVirtualHeapNode
{
	struct FLVirtualHeapNode* left;
	struct FLVirtualHeapNode* right;
	struct FLVirtualHeapNode* parent;
	size_t subtreeMax;
} FLVirtualHeapNode;

typedef struct FLVirtualHeap
{
	FLVirtualHeapNode* treeRoot;
	size_t regionReservedSize;
	size_t regionCommittedSize;
	size_t allocationGranularity;
	uint64_t padding_;
} FLVirtualHeap;

HRESULT FlVirtualHeapCreate(_Out_ FLVirtualHeap** heap, _In_ size_t size);
/*
	Procedure:
			FlVirtualHeapCreate

		Description:
			This procedure creates a new heap that can be used by other Fl heap functions.
			The function reserves the entire heap backing address range up front but only
			commits a small initial amount of physical/page-file backed memory. Additional
			memory is committed on demand, in allocation granularity increments, as
			allocations require it, and trailing committed pages are decommitted again as
			the heap shrinks. The function initializes the heap base structure.

		Parameters:
			heap:
				Pointer to a variable that will receive the heap base address on success.
				Set to NULL on failure.
				The entire heap is located inside a continuous block of memory and the
				returned heap base address points into the beginning od this continuous block.

			size:
				Minimum total size in bytes of virtual address space reserved for the heap.
				This is the maximum size the heap can grow to; it does not mean this many
				bytes are committed up front.
				If this value is zero the then heuristic value based on total system RAM is used.

		Return:
			Returns S_OK on success.
			Returns E_OUTOFMEMORY if the virtual memory reservation fails or if size
			is too large to round up without overflow.
*/

void FlVirtualHeapDelete(_In_ FLVirtualHeap* heap);
/*
	Procedure:
			FlVirtualHeapDelete

		Description:
			Destroys a heap that was previously created by FlVirtualHeapCreate.
			The function releases the entire backing virtual address space reservation,
			which decommits and frees all memory associated with the heap in a single
			operation. After this call the heap pointer and any pointers to memory
			allocated from the heap are invalid and must not be accessed.

		Parameters:
			heap:
				The heap base address. Must have been returned by
				a successful call to FlVirtualHeapCreate.

		Return:
			void.
*/

_Ret_maybenull_ _Post_writable_byte_size_(size) DECLSPEC_ALLOCATOR void* FlVirtualHeapAllocate(_Inout_ FLVirtualHeap* heap, _In_ size_t size);
/*
	Procedure:
			FlVirtualHeapAllocate

		Description:
			Allocates a block of memory from the given heap.
			The returned pointer is aligned to FL_VIRTUAL_HEAP_ALIGNMENT bytes.
			The memory can be freed using FlVirtualHeapFree or reallocated using
			FlVirtualHeapReAllocate.
			The memory will be allocated at the lowest possbile address.

		Parameters:
			heap:
				The heap base address.

			size:
				The number of bytes to allocate. If zero, the function returns NULL.

		Return:
			If the function succeeds, the return value is a pointer to the allocated
			memory block. If the function fails the return value is NULL.
*/

void FlVirtualHeapFree(_Inout_ FLVirtualHeap* heap, _Pre_maybenull_ _Post_invalid_ void* allocation);
/*
	Procedure:
			FlVirtualHeapFree

		Description:
			Frees a block of memory previously allocated from the given heap.
			If allocation is NULL the function returns without performing any action.
			The memory block must have been allocated with FlVirtualHeapAllocate
			or FlVirtualHeapReAllocate.

		Parameters:
			heap:
				The heap base address.

			allocation:
				A pointer to the memory block to be freed. May be NULL.

		Return:
			void.
*/

_Ret_maybenull_ _Post_writable_byte_size_(size) DECLSPEC_ALLOCATOR void* FlVirtualHeapReAllocate(_Inout_ FLVirtualHeap* heap, _Pre_maybenull_ _Post_invalid_ void* allocation, _In_ size_t size);
/*
	Procedure:
			FlVirtualHeapReAllocate

		Description:
			Changes the size of a previously allocated memory block.
			The function first tries to resize the block in place. If the block
			cannot be resized in place it allocates a new block at lowest possbile address,
			copies the data and frees the old block.
			If allocation is NULL the function behaves like FlVirtualHeapAllocate.
			If size is zero the function frees the block and returns NULL.
			The memory can be freed using FlVirtualHeapFree.

		Parameters:
			heap:
				The heap base address.

			allocation:
				A pointer to the memory block to be reallocated. May be NULL.

			size:
				The number of bytes to allocate.

		Return:
			If the function succeeds, the return value is a pointer to the reallocated
			memory block. If the function fails the return value is NULL and the original
			allocation remains valid and unchanged.
*/

int FlVirtualHeapValidate(_In_ FLVirtualHeap* heap);
/*
	Procedure:
			FlVirtualHeapValidate

		Description:
			Walks the entire node list and validates the heap structure.

		Parameters:
			heap:
				The heap base address.

		Return:
			Returns non-zero if the heap structure is valid, zero otherwise.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_VIRTUAL_HEAP_H
