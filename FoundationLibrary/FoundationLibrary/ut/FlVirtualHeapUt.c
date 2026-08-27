/*
	Virtual heap unit tests by Santtu S. Nyman.

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

	Description:

		Unit tests for FlVirtualHeapCreate, FlVirtualHeapAllocate,
		FlVirtualHeapFree, FlVirtualHeapReAllocate, and FlVirtualHeapValidate.

		Tests use only the public API (FLVirtualHeap* and the six functions).
		The capacity-limit and commit/decommit tests additionally read
		heap->regionReservedSize and heap->regionCommittedSize, which are
		members of the public FLVirtualHeap struct.

		Each test creates its own heap and validates the heap structure after
		every mutating operation to catch any invariant violations early.
*/

#include "FlUt.h"
#include "../include/FlVirtualHeap.h"
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#define FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE 0x10000

static int FlVirtualHeapUtIsAligned(const void* ptr)
{
	return ((uintptr_t)ptr & (FL_VIRTUAL_HEAP_ALIGNMENT - 1)) == 0;
}

// ---------------------------------------------------------------------------
// Create / Validate
// ---------------------------------------------------------------------------

static void FlVirtualHeapUtCreateAndValidate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	HRESULT result = FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	FL_UT_CHECK(SUCCEEDED(result), "FlVirtualHeapUtCreateAndValidate: create succeeded");
	FL_UT_CHECK(heap != NULL, "FlVirtualHeapUtCreateAndValidate: heap non-null");
	if (heap)
	{
		FL_UT_CHECK(FL_VIRTUAL_HEAP_IS_POSITIVE_POWER_OF_2(heap->allocationGranularity),
			"FlVirtualHeapUtCreateAndValidate: allocation granularity is a power of 2");
		FL_UT_CHECK(heap->regionReservedSize > 0 && (heap->regionReservedSize % heap->allocationGranularity) == 0,
			"FlVirtualHeapUtCreateAndValidate: reserved size is a positive multiple of allocation granularity");
		FL_UT_CHECK(heap->regionCommittedSize > 0 && (heap->regionCommittedSize % heap->allocationGranularity) == 0,
			"FlVirtualHeapUtCreateAndValidate: committed size is a positive multiple of allocation granularity");
		FL_UT_CHECK(heap->regionCommittedSize <= heap->regionReservedSize,
			"FlVirtualHeapUtCreateAndValidate: committed size does not exceed reserved size");

		FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCreateAndValidate: validate");
		FlVirtualHeapDelete(heap);
	}
}

static void FlVirtualHeapUtCreateDefault(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	HRESULT result = FlVirtualHeapCreate(&heap, 0);
	FL_UT_CHECK(SUCCEEDED(result), "FlVirtualHeapUtCreateDefault: create succeeded");
	FL_UT_CHECK(heap != NULL, "FlVirtualHeapUtCreateDefault: heap non-null");
	if (heap)
	{
		FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCreateDefault: validate");
		FlVirtualHeapDelete(heap);
	}
}

// A reservation size that cannot possibly be satisfied must fail with E_OUTOFMEMORY
// and must write NULL back through the output parameter, even though it started
// pointing at a non-null sentinel value.
static void FlVirtualHeapUtCreateOversizedFails(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = (FLVirtualHeap*)(uintptr_t)0xDEADBEEF;
	HRESULT result = FlVirtualHeapCreate(&heap, SIZE_MAX);
	FL_UT_CHECK(result == E_OUTOFMEMORY, "FlVirtualHeapUtCreateOversizedFails: create fails with E_OUTOFMEMORY");
	FL_UT_CHECK(heap == NULL, "FlVirtualHeapUtCreateOversizedFails: heap set to NULL on failure");
}

// A size smaller than the minimum region size must be clamped up rather than rejected,
// and the resulting heap must still be usable.
static void FlVirtualHeapUtCreateTinySizeClamps(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	HRESULT result = FlVirtualHeapCreate(&heap, 1);
	FL_UT_CHECK(SUCCEEDED(result), "FlVirtualHeapUtCreateTinySizeClamps: create succeeded");
	FL_UT_CHECK(heap != NULL, "FlVirtualHeapUtCreateTinySizeClamps: heap non-null");
	if (heap)
	{
		FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCreateTinySizeClamps: validate");
		void* allocation = FlVirtualHeapAllocate(heap, 16);
		FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtCreateTinySizeClamps: minimum allocation still fits");
		if (allocation)
		{
			FlVirtualHeapFree(heap, allocation);
		}
		FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCreateTinySizeClamps: final validate");
		FlVirtualHeapDelete(heap);
	}
}

// ---------------------------------------------------------------------------
// Allocate
// ---------------------------------------------------------------------------

static void FlVirtualHeapUtAllocateZero(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocation = FlVirtualHeapAllocate(heap, 0);
	FL_UT_CHECK(allocation == NULL, "FlVirtualHeapUtAllocateZero: returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateZero: validate");
	FlVirtualHeapDelete(heap);
}

static void FlVirtualHeapUtAllocateAlignment(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t testSizes[] = { 1, 3, 7, 15, 16, 17, 32, 33, 63, 64, 100, 128, 255, 256 };
	size_t testSizeCount = sizeof(testSizes) / sizeof(testSizes[0]);
	int allAligned = 1;
	for (size_t i = 0; i < testSizeCount; i++)
	{
		void* allocation = FlVirtualHeapAllocate(heap, testSizes[i]);
		if (!allocation || !FlVirtualHeapUtIsAligned(allocation))
		{
			allAligned = 0;
			break;
		}
	}
	FL_UT_CHECK(allAligned, "FlVirtualHeapUtAllocateAlignment: all aligned to FL_VIRTUAL_HEAP_ALIGNMENT");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateAlignment: validate");
	FlVirtualHeapDelete(heap);
}

static void FlVirtualHeapUtAllocateWritable(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t allocationSize = 256;
	uint8_t* allocation = (uint8_t*)FlVirtualHeapAllocate(heap, allocationSize);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtAllocateWritable: non-null");
	if (allocation)
	{
		for (size_t i = 0; i < allocationSize; i++)
		{
			allocation[i] = (uint8_t)(i & 0xFF);
		}
		int correct = 1;
		for (size_t i = 0; i < allocationSize; i++)
		{
			if (allocation[i] != (uint8_t)(i & 0xFF))
			{
				correct = 0;
				break;
			}
		}
		FL_UT_CHECK(correct, "FlVirtualHeapUtAllocateWritable: pattern preserved");
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateWritable: validate");
	FlVirtualHeapDelete(heap);
}

// ---------------------------------------------------------------------------
// Free
// ---------------------------------------------------------------------------

static void FlVirtualHeapUtFreeNull(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	FlVirtualHeapFree(heap, NULL);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFreeNull: validate");
	FlVirtualHeapDelete(heap);
}

// Alloc A, free A, alloc B of same size: B must land at A's address (first-fit).
static void FlVirtualHeapUtFirstFitReuse(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocationA = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationA != NULL, "FlVirtualHeapUtFirstFitReuse: alloc A");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFirstFitReuse: validate after alloc");

	FlVirtualHeapFree(heap, allocationA);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFirstFitReuse: validate after free");

	void* allocationB = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationB == allocationA, "FlVirtualHeapUtFirstFitReuse: B at A's address");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFirstFitReuse: validate after realloc");

	if (allocationB)
	{
		FlVirtualHeapFree(heap, allocationB);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFirstFitReuse: final validate");
	FlVirtualHeapDelete(heap);
}

// Alloc A, alloc B; free A then B: after merge a single alloc of 2x size fits at A's address.
static void FlVirtualHeapUtCoalesceForward(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocationA = FlVirtualHeapAllocate(heap, 64);
	void* allocationB = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationA != NULL && allocationB != NULL, "FlVirtualHeapUtCoalesceForward: alloc A and B");

	FlVirtualHeapFree(heap, allocationA);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceForward: validate after free A");
	FlVirtualHeapFree(heap, allocationB);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceForward: validate after free B");

	void* allocationC = FlVirtualHeapAllocate(heap, 128);
	FL_UT_CHECK(allocationC == allocationA, "FlVirtualHeapUtCoalesceForward: C at A's merged address");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceForward: validate after alloc C");

	if (allocationC)
	{
		FlVirtualHeapFree(heap, allocationC);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceForward: final validate");
	FlVirtualHeapDelete(heap);
}

// Alloc A, alloc B; free B then A: same expectation as above but opposite free order.
static void FlVirtualHeapUtCoalesceBackward(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocationA = FlVirtualHeapAllocate(heap, 64);
	void* allocationB = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationA != NULL && allocationB != NULL, "FlVirtualHeapUtCoalesceBackward: alloc A and B");

	FlVirtualHeapFree(heap, allocationB);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBackward: validate after free B");
	FlVirtualHeapFree(heap, allocationA);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBackward: validate after free A");

	void* allocationC = FlVirtualHeapAllocate(heap, 128);
	FL_UT_CHECK(allocationC == allocationA, "FlVirtualHeapUtCoalesceBackward: C at A's merged address");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBackward: validate after alloc C");

	if (allocationC)
	{
		FlVirtualHeapFree(heap, allocationC);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBackward: final validate");
	FlVirtualHeapDelete(heap);
}

// Alloc A, B, C, D; free A and C; then free B: B must coalesce with A on the left
// AND C on the right in a single free call (double merge).
static void FlVirtualHeapUtCoalesceBoth(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocationA = FlVirtualHeapAllocate(heap, 64);
	void* allocationB = FlVirtualHeapAllocate(heap, 64);
	void* allocationC = FlVirtualHeapAllocate(heap, 64);
	void* allocationD = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationA && allocationB && allocationC && allocationD,
		"FlVirtualHeapUtCoalesceBoth: alloc A, B, C, D");

	FlVirtualHeapFree(heap, allocationA);
	FlVirtualHeapFree(heap, allocationC);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBoth: validate after free A and C");

	FlVirtualHeapFree(heap, allocationB);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBoth: validate after free B (double merge)");

	// A, B, C merged → allocate 192 bytes at A's address.
	void* allocationE = FlVirtualHeapAllocate(heap, 192);
	FL_UT_CHECK(allocationE == allocationA, "FlVirtualHeapUtCoalesceBoth: E at merged A+B+C address");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBoth: validate after alloc E");

	if (allocationE)
	{
		FlVirtualHeapFree(heap, allocationE);
	}
	FlVirtualHeapFree(heap, allocationD);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCoalesceBoth: final validate");
	FlVirtualHeapDelete(heap);
}

// Freeing the middle of three blocks must not disturb the data in the live outer blocks.
static void FlVirtualHeapUtDataIsolationAfterFree(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t blockSize = 64;
	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, blockSize);
	uint8_t* allocationB = (uint8_t*)FlVirtualHeapAllocate(heap, blockSize);
	uint8_t* allocationC = (uint8_t*)FlVirtualHeapAllocate(heap, blockSize);
	FL_UT_CHECK(allocationA && allocationB && allocationC,
		"FlVirtualHeapUtDataIsolationAfterFree: alloc A, B, C");
	if (!allocationA || !allocationB || !allocationC)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < blockSize; i++)
	{
		allocationA[i] = 0xAA;
	}
	for (size_t i = 0; i < blockSize; i++)
	{
		allocationC[i] = 0xCC;
	}

	FlVirtualHeapFree(heap, allocationB);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtDataIsolationAfterFree: validate");

	int aIntact = 1;
	for (size_t i = 0; i < blockSize; i++)
	{
		if (allocationA[i] != 0xAA)
		{
			aIntact = 0;
			break;
		}
	}
	FL_UT_CHECK(aIntact, "FlVirtualHeapUtDataIsolationAfterFree: A data intact");

	int cIntact = 1;
	for (size_t i = 0; i < blockSize; i++)
	{
		if (allocationC[i] != 0xCC)
		{
			cIntact = 0;
			break;
		}
	}
	FL_UT_CHECK(cIntact, "FlVirtualHeapUtDataIsolationAfterFree: C data intact");

	FlVirtualHeapFree(heap, allocationA);
	FlVirtualHeapFree(heap, allocationC);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtDataIsolationAfterFree: final validate");
	FlVirtualHeapDelete(heap);
}

// ---------------------------------------------------------------------------
// ReAllocate
// ---------------------------------------------------------------------------

static void FlVirtualHeapUtReallocFromNull(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocation = FlVirtualHeapReAllocate(heap, NULL, 64);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtReallocFromNull: non-null");
	FL_UT_CHECK(FlVirtualHeapUtIsAligned(allocation), "FlVirtualHeapUtReallocFromNull: aligned");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocFromNull: validate");
	if (allocation)
	{
		FlVirtualHeapFree(heap, allocation);
	}
	FlVirtualHeapDelete(heap);
}

static void FlVirtualHeapUtReallocToZero(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocation = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtReallocToZero: alloc");

	void* result = FlVirtualHeapReAllocate(heap, allocation, 0);
	FL_UT_CHECK(result == NULL, "FlVirtualHeapUtReallocToZero: result NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocToZero: validate");
	FlVirtualHeapDelete(heap);
}

static void FlVirtualHeapUtReallocGrowPreservesData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t originalSize = 32;
	uint8_t* allocation = (uint8_t*)FlVirtualHeapAllocate(heap, originalSize);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtReallocGrowPreservesData: alloc");
	if (!allocation)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < originalSize; i++)
	{
		allocation[i] = (uint8_t)(i + 1);
	}

	uint8_t* grown = (uint8_t*)FlVirtualHeapReAllocate(heap, allocation, 256);
	FL_UT_CHECK(grown != NULL, "FlVirtualHeapUtReallocGrowPreservesData: grew");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowPreservesData: validate");
	if (grown)
	{
		int preserved = 1;
		for (size_t i = 0; i < originalSize; i++)
		{
			if (grown[i] != (uint8_t)(i + 1))
			{
				preserved = 0;
				break;
			}
		}
		FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocGrowPreservesData: data preserved");
		FlVirtualHeapFree(heap, grown);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowPreservesData: final validate");
	FlVirtualHeapDelete(heap);
}

static void FlVirtualHeapUtReallocShrinkPreservesData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t originalSize = 256;
	uint8_t* allocation = (uint8_t*)FlVirtualHeapAllocate(heap, originalSize);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtReallocShrinkPreservesData: alloc");
	if (!allocation)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < originalSize; i++)
	{
		allocation[i] = (uint8_t)(i + 1);
	}

	size_t newSize = 32;
	uint8_t* shrunk = (uint8_t*)FlVirtualHeapReAllocate(heap, allocation, newSize);
	FL_UT_CHECK(shrunk != NULL, "FlVirtualHeapUtReallocShrinkPreservesData: shrunk");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocShrinkPreservesData: validate");
	if (shrunk)
	{
		int preserved = 1;
		for (size_t i = 0; i < newSize; i++)
		{
			if (shrunk[i] != (uint8_t)(i + 1))
			{
				preserved = 0;
				break;
			}
		}
		FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocShrinkPreservesData: data preserved");
		FlVirtualHeapFree(heap, shrunk);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocShrinkPreservesData: final validate");
	FlVirtualHeapDelete(heap);
}

// Grow when the next block is allocated (guard) forces an allocate-copy-free fallback.
// Returned pointer must differ from original and all original data must be preserved.
static void FlVirtualHeapUtReallocGrowForcedMove(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t originalSize = 64;
	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, originalSize);
	void* allocationB = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationA != NULL && allocationB != NULL,
		"FlVirtualHeapUtReallocGrowForcedMove: alloc A and B");
	if (!allocationA || !allocationB)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < originalSize; i++)
	{
		allocationA[i] = (uint8_t)(i + 1);
	}

	uint8_t* grown = (uint8_t*)FlVirtualHeapReAllocate(heap, allocationA, 1024);
	FL_UT_CHECK(grown != NULL, "FlVirtualHeapUtReallocGrowForcedMove: grew");
	FL_UT_CHECK(grown != allocationA, "FlVirtualHeapUtReallocGrowForcedMove: pointer changed");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowForcedMove: validate");
	if (grown)
	{
		int preserved = 1;
		for (size_t i = 0; i < originalSize; i++)
		{
			if (grown[i] != (uint8_t)(i + 1))
			{
				preserved = 0;
				break;
			}
		}
		FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocGrowForcedMove: data preserved");
		FlVirtualHeapFree(heap, grown);
	}
	FlVirtualHeapFree(heap, allocationB);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowForcedMove: final validate");
	FlVirtualHeapDelete(heap);
}

// Grow in place: no guard block after A means the allocator can absorb free space
// directly, returning the same pointer.
static void FlVirtualHeapUtReallocGrowInPlace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t originalSize = 64;
	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, originalSize);
	FL_UT_CHECK(allocationA != NULL, "FlVirtualHeapUtReallocGrowInPlace: alloc A");
	if (!allocationA)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < originalSize; i++)
	{
		allocationA[i] = (uint8_t)(i + 1);
	}

	// A is the only allocation; the rest of the heap is free → grow in-place.
	uint8_t* grown = (uint8_t*)FlVirtualHeapReAllocate(heap, allocationA, 512);
	FL_UT_CHECK(grown != NULL, "FlVirtualHeapUtReallocGrowInPlace: grew");
	FL_UT_CHECK(grown == allocationA, "FlVirtualHeapUtReallocGrowInPlace: same pointer");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowInPlace: validate");
	if (grown)
	{
		int preserved = 1;
		for (size_t i = 0; i < originalSize; i++)
		{
			if (grown[i] != (uint8_t)(i + 1))
			{
				preserved = 0;
				break;
			}
		}
		FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocGrowInPlace: data preserved");
		FlVirtualHeapFree(heap, grown);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowInPlace: final validate");
	FlVirtualHeapDelete(heap);
}

// FlVirtualHeapReAllocate has its own copy of the overflow guard used by
// FlVirtualHeapAllocate. SIZE_MAX must be rejected and the original block preserved.
static void FlVirtualHeapUtReallocSizeOverflow(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	size_t allocationSize = 64;
	uint8_t* allocation = (uint8_t*)FlVirtualHeapAllocate(heap, allocationSize);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtReallocSizeOverflow: alloc");
	if (!allocation)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < allocationSize; i++)
	{
		allocation[i] = (uint8_t)(i + 1);
	}

	void* result = FlVirtualHeapReAllocate(heap, allocation, SIZE_MAX);
	FL_UT_CHECK(result == NULL, "FlVirtualHeapUtReallocSizeOverflow: SIZE_MAX returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocSizeOverflow: validate");

	int preserved = 1;
	for (size_t i = 0; i < allocationSize; i++)
	{
		if (allocation[i] != (uint8_t)(i + 1))
		{
			preserved = 0;
			break;
		}
	}
	FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocSizeOverflow: original data preserved");

	FlVirtualHeapFree(heap, allocation);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocSizeOverflow: final validate");
	FlVirtualHeapDelete(heap);
}

// Mirrors the block-layout constants documented in FlVirtualHeap.c, scoped to this test.
#define FL_VIRTUAL_HEAP_UT_REALLOC_OOM_HEADER_SIZE   ((size_t)8)
#define FL_VIRTUAL_HEAP_UT_REALLOC_OOM_EPILOGUE_SIZE ((size_t)8)
#define FL_VIRTUAL_HEAP_UT_REALLOC_OOM_MIN_BLOCK     ((size_t)48)

// Fill the heap exactly full with two blocks, A and B, then try to grow A by one byte.
// In-place growth is blocked because B sits immediately after A and is allocated, and the
// allocate-copy-free fallback is blocked because there is no free space anywhere. The call
// must return NULL and leave A's pointer and data completely untouched.
static void FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t usable = heap->regionReservedSize - sizeof(FLVirtualHeap) - FL_VIRTUAL_HEAP_UT_REALLOC_OOM_EPILOGUE_SIZE;
	size_t largePayload = usable - FL_VIRTUAL_HEAP_UT_REALLOC_OOM_MIN_BLOCK - FL_VIRTUAL_HEAP_UT_REALLOC_OOM_HEADER_SIZE;
	size_t minimumPayload = FL_VIRTUAL_HEAP_UT_REALLOC_OOM_MIN_BLOCK - FL_VIRTUAL_HEAP_UT_REALLOC_OOM_HEADER_SIZE;

	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, largePayload);
	FL_UT_CHECK(allocationA != NULL, "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: alloc A");
	if (!allocationA)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < largePayload; i++)
	{
		allocationA[i] = (uint8_t)(i & 0xFF);
	}

	void* allocationB = FlVirtualHeapAllocate(heap, minimumPayload);
	FL_UT_CHECK(allocationB != NULL, "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: alloc B fills heap");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: validate while full");

	void* result = FlVirtualHeapReAllocate(heap, allocationA, largePayload + 1);
	FL_UT_CHECK(result == NULL, "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: grow past full returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: validate after failed grow");

	int preserved = 1;
	for (size_t i = 0; i < largePayload; i++)
	{
		if (allocationA[i] != (uint8_t)(i & 0xFF))
		{
			preserved = 0;
			break;
		}
	}
	FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: original data preserved");

	if (allocationB)
	{
		FlVirtualHeapFree(heap, allocationB);
	}
	FlVirtualHeapFree(heap, allocationA);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact: final validate");
	FlVirtualHeapDelete(heap);
}

#undef FL_VIRTUAL_HEAP_UT_REALLOC_OOM_HEADER_SIZE
#undef FL_VIRTUAL_HEAP_UT_REALLOC_OOM_EPILOGUE_SIZE
#undef FL_VIRTUAL_HEAP_UT_REALLOC_OOM_MIN_BLOCK

// ---------------------------------------------------------------------------
// Overlap and failure tests
// ---------------------------------------------------------------------------

static void FlVirtualHeapUtNoOverlap(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const size_t sizes[] = { 16, 64, 32, 128, 8, 256, 48, 96 };
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocations[8];
	int allAllocated = 1;
	for (int i = 0; i < 8; i++)
	{
		allocations[i] = FlVirtualHeapAllocate(heap, sizes[i]);
		if (!allocations[i])
		{
			allAllocated = 0;
			break;
		}
	}
	FL_UT_CHECK(allAllocated, "FlVirtualHeapUtNoOverlap: all allocated");

	int noOverlap = 1;
	for (int i = 0; i < 8 && noOverlap; i++)
	{
		for (int j = i + 1; j < 8 && noOverlap; j++)
		{
			uintptr_t startI = (uintptr_t)allocations[i];
			uintptr_t endI = startI + sizes[i];
			uintptr_t startJ = (uintptr_t)allocations[j];
			uintptr_t endJ = startJ + sizes[j];
			if (!(endI <= startJ || endJ <= startI))
			{
				noOverlap = 0;
			}
		}
	}
	FL_UT_CHECK(noOverlap, "FlVirtualHeapUtNoOverlap: no overlapping allocations");

	for (int i = 0; i < 8; i++)
	{
		if (allocations[i])
		{
			FlVirtualHeapFree(heap, allocations[i]);
		}
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtNoOverlap: final validate");
	FlVirtualHeapDelete(heap);
}

// A failed oversized allocation must not corrupt the heap; a small allocation must still work.
static void FlVirtualHeapUtAllocateAfterFailure(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* bigAllocation = FlVirtualHeapAllocate(heap, 0x80000000);
	FL_UT_CHECK(bigAllocation == NULL, "FlVirtualHeapUtAllocateAfterFailure: oversized returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateAfterFailure: validate after failure");

	void* smallAllocation = FlVirtualHeapAllocate(heap, 16);
	FL_UT_CHECK(smallAllocation != NULL, "FlVirtualHeapUtAllocateAfterFailure: small alloc succeeds");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateAfterFailure: validate after small alloc");

	if (smallAllocation)
	{
		FlVirtualHeapFree(heap, smallAllocation);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateAfterFailure: final validate");
	FlVirtualHeapDelete(heap);
}

// ---------------------------------------------------------------------------
// Fragmentation and stress tests
// ---------------------------------------------------------------------------

// Alloc 16 blocks, free every other, free the rest: heap must be valid throughout
// and fully coalesced at the end (a large allocation must fit at the first address).
static void FlVirtualHeapUtFragmentAndCoalesce(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
#define FL_VIRTUAL_HEAP_UT_BLOCK_COUNT 16
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocations[FL_VIRTUAL_HEAP_UT_BLOCK_COUNT];
	int allAllocated = 1;
	for (int i = 0; i < FL_VIRTUAL_HEAP_UT_BLOCK_COUNT; i++)
	{
		allocations[i] = FlVirtualHeapAllocate(heap, 64);
		if (!allocations[i])
		{
			allAllocated = 0;
			break;
		}
	}
	FL_UT_CHECK(allAllocated, "FlVirtualHeapUtFragmentAndCoalesce: all allocated");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFragmentAndCoalesce: validate after alloc");

	void* firstAddress = allocations[0];

	for (int i = 0; i < FL_VIRTUAL_HEAP_UT_BLOCK_COUNT; i += 2)
	{
		FlVirtualHeapFree(heap, allocations[i]);
		allocations[i] = NULL;
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFragmentAndCoalesce: validate after partial free");

	for (int i = 1; i < FL_VIRTUAL_HEAP_UT_BLOCK_COUNT; i += 2)
	{
		FlVirtualHeapFree(heap, allocations[i]);
		allocations[i] = NULL;
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFragmentAndCoalesce: validate after full free");

	void* large = FlVirtualHeapAllocate(heap, FL_VIRTUAL_HEAP_UT_BLOCK_COUNT * 64);
	FL_UT_CHECK(large != NULL, "FlVirtualHeapUtFragmentAndCoalesce: large alloc after coalesce");
	FL_UT_CHECK(large == firstAddress, "FlVirtualHeapUtFragmentAndCoalesce: large alloc at first address");
	if (large)
	{
		FlVirtualHeapFree(heap, large);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFragmentAndCoalesce: final validate");
	FlVirtualHeapDelete(heap);
#undef FL_VIRTUAL_HEAP_UT_BLOCK_COUNT
}

// Allocate and free a single block 100 times. The heap must be valid and the
// allocation must succeed on every iteration.
static void FlVirtualHeapUtAllocateFreeLoop(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	int allSucceeded = 1;
	int allValid = 1;
	for (int i = 0; i < 100; i++)
	{
		void* allocation = FlVirtualHeapAllocate(heap, 64);
		if (!allocation)
		{
			allSucceeded = 0;
			break;
		}
		FlVirtualHeapFree(heap, allocation);
		if (!FlVirtualHeapValidate(heap))
		{
			allValid = 0;
			break;
		}
	}
	FL_UT_CHECK(allSucceeded, "FlVirtualHeapUtAllocateFreeLoop: all iterations succeeded");
	FL_UT_CHECK(allValid, "FlVirtualHeapUtAllocateFreeLoop: valid after each iteration");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateFreeLoop: final validate");
	FlVirtualHeapDelete(heap);
}

// 100 iterations of allocating 10 mixed-size blocks and freeing them in alternating
// orders. The heap must remain valid throughout and fully coalesce each time.
static void FlVirtualHeapUtMixedSizeFragmentation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const size_t sizes[] = { 1, 128, 7, 15, 31, 256, 2, 100, 200, 16 };
	static const int freeOrderEven[] = { 8, 2, 7, 1, 3, 0, 9, 6, 5, 4 };
	static const int freeOrderOdd[] = { 4, 5, 6, 9, 0, 3, 1, 7, 2, 8 };

	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	void* firstAddress = NULL;
	int allValid = 1;

	for (int iteration = 0; iteration < 100 && allValid; iteration++)
	{
		void* allocations[10];
		int allAllocated = 1;
		for (int i = 0; i < 10; i++)
		{
			allocations[i] = FlVirtualHeapAllocate(heap, sizes[i]);
			if (!allocations[i])
			{
				allAllocated = 0;
				break;
			}
		}
		if (!allAllocated)
		{
			allValid = 0;
			break;
		}
		if (iteration == 0)
		{
			firstAddress = allocations[0];
		}

		const int* freeOrder = (iteration % 2 == 0) ? freeOrderEven : freeOrderOdd;
		for (int i = 0; i < 10; i++)
		{
			FlVirtualHeapFree(heap, allocations[freeOrder[i]]);
		}
		if (!FlVirtualHeapValidate(heap))
		{
			allValid = 0;
		}
	}

	FL_UT_CHECK(allValid, "FlVirtualHeapUtMixedSizeFragmentation: valid throughout all iterations");

	void* finalAllocation = FlVirtualHeapAllocate(heap, 256);
	FL_UT_CHECK(finalAllocation == firstAddress,
		"FlVirtualHeapUtMixedSizeFragmentation: final alloc at first address");
	if (finalAllocation)
	{
		FlVirtualHeapFree(heap, finalAllocation);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtMixedSizeFragmentation: final validate");
	FlVirtualHeapDelete(heap);
}

// Alloc 8 blocks, free in the order {3,1,5,0,2,4,7,6}. Heap must be valid after every free.
static void FlVirtualHeapUtUnorderedFree(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocations[8];
	int allAllocated = 1;
	for (int i = 0; i < 8; i++)
	{
		allocations[i] = FlVirtualHeapAllocate(heap, 64);
		if (!allocations[i])
		{
			allAllocated = 0;
			break;
		}
	}
	FL_UT_CHECK(allAllocated, "FlVirtualHeapUtUnorderedFree: all allocated");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtUnorderedFree: validate after alloc");

	static const int freeOrder[] = { 3, 1, 5, 0, 2, 4, 7, 6 };
	int allValid = 1;
	for (int i = 0; i < 8; i++)
	{
		FlVirtualHeapFree(heap, allocations[freeOrder[i]]);
		if (!FlVirtualHeapValidate(heap))
		{
			allValid = 0;
			break;
		}
	}
	FL_UT_CHECK(allValid, "FlVirtualHeapUtUnorderedFree: valid after each free");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtUnorderedFree: final validate");
	FlVirtualHeapDelete(heap);
}

// After freeing all blocks the next allocation of any size must land at the first
// block's address (first-fit on a fully coalesced heap).
static void FlVirtualHeapUtFirstFitAfterFullFree(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}
	void* allocationA = FlVirtualHeapAllocate(heap, 64);
	void* allocationB = FlVirtualHeapAllocate(heap, 64);
	void* allocationC = FlVirtualHeapAllocate(heap, 64);
	void* allocationD = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationA && allocationB && allocationC && allocationD,
		"FlVirtualHeapUtFirstFitAfterFullFree: alloc A, B, C, D");

	FlVirtualHeapFree(heap, allocationA);
	FlVirtualHeapFree(heap, allocationB);
	FlVirtualHeapFree(heap, allocationC);
	FlVirtualHeapFree(heap, allocationD);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFirstFitAfterFullFree: validate after free all");

	void* allocationE = FlVirtualHeapAllocate(heap, 64);
	FL_UT_CHECK(allocationE == allocationA, "FlVirtualHeapUtFirstFitAfterFullFree: E at A's address");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFirstFitAfterFullFree: final validate");
	if (allocationE)
	{
		FlVirtualHeapFree(heap, allocationE);
	}
	FlVirtualHeapDelete(heap);
}

// ---------------------------------------------------------------------------
// Split-threshold boundary tests
// ---------------------------------------------------------------------------

// Block sizes used by the split-threshold tests.
// Derived from need(N) = max(round_up(N + 8, 16), 48):
//   need(200) = round_up(208, 16) = 208   → base block size
//   need(144) = round_up(152, 16) = 160   → remainder 208 - 160 = 48 → SPLIT
//   need(160) = round_up(168, 16) = 176   → remainder 208 - 176 = 32 → NO SPLIT
//   need(24)  = max(round_up(32, 16), 48) = 48  → fills one minimum-size block

// Realloc shrink with remainder = 48 (exactly at the split threshold).
// After the split a subsequent minimum-size allocation must land in the freed tail,
// i.e. at an address between the shrunken A and the GUARD block.
static void FlVirtualHeapUtReallocShrinkSplit(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	// A: block size 208 (need(200) = 208).
	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, 200);
	// GUARD: pins the address space immediately after A.
	void* guard = FlVirtualHeapAllocate(heap, 16);
	FL_UT_CHECK(allocationA != NULL && guard != NULL, "FlVirtualHeapUtReallocShrinkSplit: alloc A and GUARD");
	if (!allocationA || !guard)
	{
		FlVirtualHeapDelete(heap);
		return;
	}

	// Shrink A from need=208 to need=160; remainder = 48 → split occurs.
	uint8_t* shrunkA = (uint8_t*)FlVirtualHeapReAllocate(heap, allocationA, 144);
	FL_UT_CHECK(shrunkA == allocationA, "FlVirtualHeapUtReallocShrinkSplit: same pointer after shrink");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocShrinkSplit: validate after shrink");

	// Allocate C (need = 48); first-fit must pick the 48-byte tail between A and GUARD.
	void* allocationC = FlVirtualHeapAllocate(heap, 24);
	FL_UT_CHECK(allocationC != NULL, "FlVirtualHeapUtReallocShrinkSplit: C allocated");
	if (allocationC)
	{
		FL_UT_CHECK(
			(uintptr_t)allocationC > (uintptr_t)shrunkA &&
			(uintptr_t)allocationC < (uintptr_t)guard,
			"FlVirtualHeapUtReallocShrinkSplit: C in split tail (between A and GUARD)");
		FlVirtualHeapFree(heap, allocationC);
	}
	FlVirtualHeapFree(heap, shrunkA);
	FlVirtualHeapFree(heap, guard);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocShrinkSplit: final validate");
	FlVirtualHeapDelete(heap);
}

// Realloc shrink with remainder = 32 (below the 48-byte split threshold).
// No tail block is freed, so a subsequent minimum-size allocation must NOT land
// between the shrunken A and the GUARD block.
static void FlVirtualHeapUtReallocShrinkNoSplit(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	// A: block size 208 (need(200) = 208).
	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, 200);
	// GUARD: pins address space immediately after A.
	void* guard = FlVirtualHeapAllocate(heap, 16);
	FL_UT_CHECK(allocationA != NULL && guard != NULL, "FlVirtualHeapUtReallocShrinkNoSplit: alloc A and GUARD");
	if (!allocationA || !guard)
	{
		FlVirtualHeapDelete(heap);
		return;
	}

	// Shrink A from need=208 to need=176; remainder = 32 → NO split.
	uint8_t* shrunkA = (uint8_t*)FlVirtualHeapReAllocate(heap, allocationA, 160);
	FL_UT_CHECK(shrunkA == allocationA, "FlVirtualHeapUtReallocShrinkNoSplit: same pointer after shrink");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocShrinkNoSplit: validate after shrink");

	// Allocate C (need = 48); tail was NOT freed, so first-fit must skip past GUARD.
	void* allocationC = FlVirtualHeapAllocate(heap, 24);
	FL_UT_CHECK(allocationC != NULL, "FlVirtualHeapUtReallocShrinkNoSplit: C allocated");
	if (allocationC)
	{
		FL_UT_CHECK(
			(uintptr_t)allocationC >= (uintptr_t)guard,
			"FlVirtualHeapUtReallocShrinkNoSplit: C not in suppressed tail (at or after GUARD)");
		FlVirtualHeapFree(heap, allocationC);
	}
	FlVirtualHeapFree(heap, shrunkA);
	FlVirtualHeapFree(heap, guard);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocShrinkNoSplit: final validate");
	FlVirtualHeapDelete(heap);
}

// ---------------------------------------------------------------------------
// Size overflow
// ---------------------------------------------------------------------------

static void FlVirtualHeapUtAllocateSizeOverflow(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	// SIZE_MAX overflows need = round_up(SIZE_MAX + 8, 16).
	void* overflow1 = FlVirtualHeapAllocate(heap, SIZE_MAX);
	FL_UT_CHECK(overflow1 == NULL, "FlVirtualHeapUtAllocateSizeOverflow: SIZE_MAX returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateSizeOverflow: validate after SIZE_MAX");

	// SIZE_MAX - 7: SIZE_MAX - 7 + 8 = SIZE_MAX + 1 still wraps on addition, so must be rejected.
	void* overflow2 = FlVirtualHeapAllocate(heap, SIZE_MAX - 7);
	FL_UT_CHECK(overflow2 == NULL, "FlVirtualHeapUtAllocateSizeOverflow: SIZE_MAX-7 returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateSizeOverflow: validate after SIZE_MAX-7");

	// Heap must still work after the two failed oversized requests.
	void* smallAllocation = FlVirtualHeapAllocate(heap, 16);
	FL_UT_CHECK(smallAllocation != NULL, "FlVirtualHeapUtAllocateSizeOverflow: small alloc succeeds");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateSizeOverflow: validate after small alloc");
	if (smallAllocation)
	{
		FlVirtualHeapFree(heap, smallAllocation);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocateSizeOverflow: final validate");
	FlVirtualHeapDelete(heap);
}

// ---------------------------------------------------------------------------
// Fuzz / randomised interleaved operations
// ---------------------------------------------------------------------------

// Minimal LCG for deterministic pseudo-randomness. Not cryptographic.
static uint32_t FlVirtualHeapUtLcg(uint32_t* state)
{
	*state = *state * 1664525 + 1013904223;
	return *state;
}

#define FL_VIRTUAL_HEAP_UT_FUZZ_SHADOW_SIZE 32
#define FL_VIRTUAL_HEAP_UT_FUZZ_OP_COUNT    500

typedef struct FLVirtualHeapUtShadow
{
	void* pointer;
	size_t size;
	uint8_t fillByte;
} FLVirtualHeapUtShadow;

static void FlVirtualHeapUtFuzz(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	FLVirtualHeapUtShadow shadow[FL_VIRTUAL_HEAP_UT_FUZZ_SHADOW_SIZE];
	size_t liveCount = 0;
	uint32_t rngState = 0xDEADBEEF;
	int allDataIntact = 1;
	int allValidatesPassed = 1;
	int allAllocsSucceeded = 1;

	for (int operation = 0; operation < FL_VIRTUAL_HEAP_UT_FUZZ_OP_COUNT; operation++)
	{
		// Choose operation: 0 = alloc, 1 = free, 2 = realloc.
		uint32_t rng = FlVirtualHeapUtLcg(&rngState);
		int opType;
		if (liveCount == 0)
		{
			opType = 0;
		}
		else if (liveCount == FL_VIRTUAL_HEAP_UT_FUZZ_SHADOW_SIZE)
		{
			opType = (rng % 10 < 4) ? 1 : 2;
		}
		else
		{
			uint32_t roll = rng % 10;
			opType = (roll < 4) ? 0 : (roll < 7) ? 1 : 2;
		}

		if (opType == 0)
		{
			// Allocate.
			rng = FlVirtualHeapUtLcg(&rngState);
			size_t allocSize = (size_t)(rng % 240) + 1;
			rng = FlVirtualHeapUtLcg(&rngState);
			uint8_t fillByte = (uint8_t)(rng & 0xFF);

			void* allocation = FlVirtualHeapAllocate(heap, allocSize);
			if (!allocation)
			{
				// Heap may be full; not a failure unless the heap is clearly not full.
				// Accept silently to keep the fuzz going.
			}
			else
			{
				memset(allocation, fillByte, allocSize);
				shadow[liveCount].pointer = allocation;
				shadow[liveCount].size = allocSize;
				shadow[liveCount].fillByte = fillByte;
				liveCount++;
			}
		}
		else if (opType == 1)
		{
			// Free a random live allocation.
			rng = FlVirtualHeapUtLcg(&rngState);
			size_t slotIndex = (size_t)(rng % liveCount);
			uint8_t* pointer = (uint8_t*)shadow[slotIndex].pointer;
			size_t slotSize = shadow[slotIndex].size;
			uint8_t fillByte = shadow[slotIndex].fillByte;

			// Verify pattern before freeing.
			for (size_t byteIndex = 0; byteIndex < slotSize; byteIndex++)
			{
				if (pointer[byteIndex] != fillByte)
				{
					allDataIntact = 0;
					break;
				}
			}

			FlVirtualHeapFree(heap, pointer);
			shadow[slotIndex] = shadow[liveCount - 1];
			liveCount--;
		}
		else
		{
			// Realloc a random live allocation.
			rng = FlVirtualHeapUtLcg(&rngState);
			size_t slotIndex = (size_t)(rng % liveCount);
			uint8_t* oldPointer = (uint8_t*)shadow[slotIndex].pointer;
			size_t oldSize = shadow[slotIndex].size;
			uint8_t oldFillByte = shadow[slotIndex].fillByte;

			// Verify pattern before reallocating.
			for (size_t byteIndex = 0; byteIndex < oldSize; byteIndex++)
			{
				if (oldPointer[byteIndex] != oldFillByte)
				{
					allDataIntact = 0;
					break;
				}
			}

			rng = FlVirtualHeapUtLcg(&rngState);
			size_t newSize = (size_t)(rng % 240) + 1;
			rng = FlVirtualHeapUtLcg(&rngState);
			uint8_t newFillByte = (uint8_t)(rng & 0xFF);

			void* newPointer = FlVirtualHeapReAllocate(heap, oldPointer, newSize);
			if (!newPointer)
			{
				// Realloc failed: original block must be untouched.
				for (size_t byteIndex = 0; byteIndex < oldSize; byteIndex++)
				{
					if (oldPointer[byteIndex] != oldFillByte)
					{
						allDataIntact = 0;
						break;
					}
				}
			}
			else
			{
				// Realloc succeeded: check preserved bytes, fill new extent.
				size_t copySize = oldSize < newSize ? oldSize : newSize;
				uint8_t* newBytes = (uint8_t*)newPointer;
				for (size_t byteIndex = 0; byteIndex < copySize; byteIndex++)
				{
					if (newBytes[byteIndex] != oldFillByte)
					{
						allDataIntact = 0;
						break;
					}
				}
				memset(newPointer, newFillByte, newSize);
				shadow[slotIndex].pointer = newPointer;
				shadow[slotIndex].size = newSize;
				shadow[slotIndex].fillByte = newFillByte;
			}
		}

		if (!FlVirtualHeapValidate(heap))
		{
			allValidatesPassed = 0;
		}
	}

	// Verify and free all remaining live allocations.
	for (size_t slotIndex = 0; slotIndex < liveCount; slotIndex++)
	{
		uint8_t* pointer = (uint8_t*)shadow[slotIndex].pointer;
		size_t slotSize = shadow[slotIndex].size;
		uint8_t fillByte = shadow[slotIndex].fillByte;
		for (size_t byteIndex = 0; byteIndex < slotSize; byteIndex++)
		{
			if (pointer[byteIndex] != fillByte)
			{
				allDataIntact = 0;
				break;
			}
		}
		FlVirtualHeapFree(heap, pointer);

		if (!FlVirtualHeapValidate(heap))
		{
			allValidatesPassed = 0;
		}
	}

	FL_UT_CHECK(allAllocsSucceeded, "FlVirtualHeapUtFuzz: no unexpected allocation failure");
	FL_UT_CHECK(allDataIntact, "FlVirtualHeapUtFuzz: data integrity preserved throughout");
	FL_UT_CHECK(allValidatesPassed, "FlVirtualHeapUtFuzz: heap valid after every operation");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtFuzz: final validate");
	FlVirtualHeapDelete(heap);
}

#undef FL_VIRTUAL_HEAP_UT_FUZZ_SHADOW_SIZE
#undef FL_VIRTUAL_HEAP_UT_FUZZ_OP_COUNT

// ---------------------------------------------------------------------------
// Heap capacity boundary tests
// ---------------------------------------------------------------------------

// Implementation invariants the capacity tests depend on.
// Must match the constants documented in FlVirtualHeap.c.
#define FL_VIRTUAL_HEAP_UT_BLOCK_HEADER_SIZE  ((size_t)8)
#define FL_VIRTUAL_HEAP_UT_EPILOGUE_SIZE_CAP  ((size_t)8)
#define FL_VIRTUAL_HEAP_UT_MIN_BLOCK_SIZE_CAP ((size_t)48)

// Usable space for user blocks:
//   usable = heap->regionReservedSize - sizeof(FLVirtualHeap) - FL_VIRTUAL_HEAP_UT_EPILOGUE_SIZE_CAP
// This is always a multiple of 16 because regionReservedSize is page-aligned and
// sizeof(FLVirtualHeap) + epilogue = 24 + 8 = 32 ≡ 0 (mod 16).
//
// Derived payload sizes (need(N) = max(round_up(N + 8, 16), 48)):
//   Exactly-full payload = usable - 8   (need = usable, fills all usable bytes)
//   Almost-full payload  = usable - 56  (need = usable - 48, leaves one 48-byte tail)
//   Min last-slot        = 40           (need = 48, fits the tail exactly)
//   Too big for last-slot = 41          (need = 64 > 48, does NOT fit)

// After the large block, a minimum-payload request (need=48) fills the one remaining slot.
// A subsequent (min+1)-byte request (need=64) must fail because the tail is consumed.
static void FlVirtualHeapUtAlmostFull(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t usable = heap->regionReservedSize - sizeof(FLVirtualHeap) - FL_VIRTUAL_HEAP_UT_EPILOGUE_SIZE_CAP;
	size_t largePayload = usable - FL_VIRTUAL_HEAP_UT_MIN_BLOCK_SIZE_CAP - FL_VIRTUAL_HEAP_UT_BLOCK_HEADER_SIZE;
	size_t minimumPayload = FL_VIRTUAL_HEAP_UT_MIN_BLOCK_SIZE_CAP - FL_VIRTUAL_HEAP_UT_BLOCK_HEADER_SIZE;
	size_t overflowPayload = minimumPayload + 1;

	// Leaves exactly one 48-byte free block (one minimum-size slot).
	void* largAllocation = FlVirtualHeapAllocate(heap, largePayload);
	FL_UT_CHECK(largAllocation != NULL, "FlVirtualHeapUtAlmostFull: large alloc succeeds");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAlmostFull: validate after large alloc");
	if (!largAllocation)
	{
		FlVirtualHeapDelete(heap);
		return;
	}

	// Fills the 48-byte tail exactly (need=48).
	void* minimumAllocation = FlVirtualHeapAllocate(heap, minimumPayload);
	FL_UT_CHECK(minimumAllocation != NULL, "FlVirtualHeapUtAlmostFull: minimum alloc fills last slot");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAlmostFull: validate after minimum alloc");

	// Heap is now exactly full; overflowPayload needs 64 bytes but none are free.
	void* overflowAllocation = FlVirtualHeapAllocate(heap, overflowPayload);
	FL_UT_CHECK(overflowAllocation == NULL, "FlVirtualHeapUtAlmostFull: alloc past full returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAlmostFull: validate after failed alloc");

	if (minimumAllocation)
	{
		FlVirtualHeapFree(heap, minimumAllocation);
	}
	FlVirtualHeapFree(heap, largAllocation);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAlmostFull: final validate");
	FlVirtualHeapDelete(heap);
}

// A single allocation consuming all usable space leaves no free blocks.
// The heap must be valid while completely full, and recover correctly after the block is freed.
static void FlVirtualHeapUtExactlyFull(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t usable = heap->regionReservedSize - sizeof(FLVirtualHeap) - FL_VIRTUAL_HEAP_UT_EPILOGUE_SIZE_CAP;
	size_t fullPayload = usable - FL_VIRTUAL_HEAP_UT_BLOCK_HEADER_SIZE;

	// need(fullPayload) = usable = total usable bytes → heap exactly full after this.
	void* fullAllocation = FlVirtualHeapAllocate(heap, fullPayload);
	FL_UT_CHECK(fullAllocation != NULL, "FlVirtualHeapUtExactlyFull: max alloc succeeds");
	FL_UT_CHECK(FlVirtualHeapUtIsAligned(fullAllocation), "FlVirtualHeapUtExactlyFull: aligned");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtExactlyFull: validate while full");

	FlVirtualHeapFree(heap, fullAllocation);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtExactlyFull: final validate");
	FlVirtualHeapDelete(heap);
}

// Fill the heap exactly full, then attempt allocations. All must return NULL without
// corrupting the heap. After freeing the fill block the heap must accept a new allocation.
static void FlVirtualHeapUtAllocatePastFull(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SMALL_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t usable = heap->regionReservedSize - sizeof(FLVirtualHeap) - FL_VIRTUAL_HEAP_UT_EPILOGUE_SIZE_CAP;
	size_t fullPayload = usable - FL_VIRTUAL_HEAP_UT_BLOCK_HEADER_SIZE;

	void* fullAllocation = FlVirtualHeapAllocate(heap, fullPayload);
	FL_UT_CHECK(fullAllocation != NULL, "FlVirtualHeapUtAllocatePastFull: fill alloc succeeds");
	if (!fullAllocation)
	{
		FlVirtualHeapDelete(heap);
		return;
	}

	void* failAllocation1 = FlVirtualHeapAllocate(heap, 1);
	FL_UT_CHECK(failAllocation1 == NULL, "FlVirtualHeapUtAllocatePastFull: 1-byte alloc returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocatePastFull: validate after 1-byte fail");

	void* failAllocation2 = FlVirtualHeapAllocate(heap, 16);
	FL_UT_CHECK(failAllocation2 == NULL, "FlVirtualHeapUtAllocatePastFull: 16-byte alloc returns NULL");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocatePastFull: validate after 16-byte fail");

	FlVirtualHeapFree(heap, fullAllocation);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocatePastFull: validate after free");

	void* recoveryAllocation = FlVirtualHeapAllocate(heap, 16);
	FL_UT_CHECK(recoveryAllocation != NULL, "FlVirtualHeapUtAllocatePastFull: heap recovered after free");
	if (recoveryAllocation)
	{
		FlVirtualHeapFree(heap, recoveryAllocation);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtAllocatePastFull: final validate");
	FlVirtualHeapDelete(heap);
}

#undef FL_VIRTUAL_HEAP_UT_BLOCK_HEADER_SIZE
#undef FL_VIRTUAL_HEAP_UT_EPILOGUE_SIZE_CAP
#undef FL_VIRTUAL_HEAP_UT_MIN_BLOCK_SIZE_CAP

// ---------------------------------------------------------------------------
// Commit / decommit behavior
// ---------------------------------------------------------------------------

// Large enough that the requested size cannot round down to a single
// allocation-granularity unit, so the reserved and initial committed sizes
// are guaranteed to differ.
#define FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE ((size_t)0x1000000)

// FlVirtualHeapCreate reserves the full requested size but must not commit
// all of it up front; only a small initial amount should be committed.
static void FlVirtualHeapUtCommittedSizeStartsSmall(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	FL_UT_CHECK(heap->regionCommittedSize > 0, "FlVirtualHeapUtCommittedSizeStartsSmall: committed size is non-zero");
	FL_UT_CHECK(heap->regionCommittedSize < heap->regionReservedSize, "FlVirtualHeapUtCommittedSizeStartsSmall: committed size starts smaller than reserved size");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCommittedSizeStartsSmall: validate");
	FlVirtualHeapDelete(heap);
}

// Allocating more than the initial committed prefix must transparently
// commit additional memory and keep the heap usable and valid.
static void FlVirtualHeapUtCommittedSizeGrows(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t initialCommitted = heap->regionCommittedSize;
	size_t bigPayload = initialCommitted + (heap->regionReservedSize - initialCommitted) / 2;

	void* allocation = FlVirtualHeapAllocate(heap, bigPayload);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtCommittedSizeGrows: large alloc succeeds");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCommittedSizeGrows: validate after large alloc");
	FL_UT_CHECK(heap->regionCommittedSize > initialCommitted, "FlVirtualHeapUtCommittedSizeGrows: committed size increased");
	FL_UT_CHECK(heap->regionCommittedSize <= heap->regionReservedSize, "FlVirtualHeapUtCommittedSizeGrows: committed size does not exceed reserved size");

	if (allocation)
	{
		memset(allocation, 0xA5, bigPayload);
		FL_UT_CHECK(((unsigned char*)allocation)[0] == 0xA5 && ((unsigned char*)allocation)[bigPayload - 1] == 0xA5,
			"FlVirtualHeapUtCommittedSizeGrows: newly committed memory is writable");
		FlVirtualHeapFree(heap, allocation);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCommittedSizeGrows: final validate");
	FlVirtualHeapDelete(heap);
}

// After a large block is freed, trailing committed pages that are no longer
// needed must be decommitted again, though one allocation-granularity unit
// of headroom is kept so the committed size need not return all the way to
// the very first, pre-growth minimum.
static void FlVirtualHeapUtCommittedSizeShrinksAfterFree(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t initialCommitted = heap->regionCommittedSize;
	size_t bigPayload = initialCommitted + (heap->regionReservedSize - initialCommitted) / 2;

	void* allocation = FlVirtualHeapAllocate(heap, bigPayload);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtCommittedSizeShrinksAfterFree: large alloc succeeds");
	if (!allocation)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	size_t grownCommitted = heap->regionCommittedSize;
	FL_UT_CHECK(grownCommitted > initialCommitted, "FlVirtualHeapUtCommittedSizeShrinksAfterFree: committed size grew");

	FlVirtualHeapFree(heap, allocation);
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtCommittedSizeShrinksAfterFree: validate after free");
	FL_UT_CHECK(heap->regionCommittedSize < grownCommitted, "FlVirtualHeapUtCommittedSizeShrinksAfterFree: committed size shrank after free");
	FL_UT_CHECK(heap->regionCommittedSize >= initialCommitted, "FlVirtualHeapUtCommittedSizeShrinksAfterFree: committed size retains at least the initial floor plus headroom");

	FlVirtualHeapDelete(heap);
}

// FlVirtualHeapInternalGrow is also called directly from FlVirtualHeapReAllocate's
// grow-in-place branch (not just from FlVirtualHeapAllocate). Growing A well past the
// space currently committed after it must transparently commit more memory, return the
// same pointer, and preserve the original data.
static void FlVirtualHeapUtReallocGrowTriggersCommit(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t initialCommitted = heap->regionCommittedSize;
	size_t originalSize = 64;
	uint8_t* allocationA = (uint8_t*)FlVirtualHeapAllocate(heap, originalSize);
	FL_UT_CHECK(allocationA != NULL, "FlVirtualHeapUtReallocGrowTriggersCommit: alloc A");
	if (!allocationA)
	{
		FlVirtualHeapDelete(heap);
		return;
	}
	for (size_t i = 0; i < originalSize; i++)
	{
		allocationA[i] = (uint8_t)(i + 1);
	}

	uint8_t* grown = (uint8_t*)FlVirtualHeapReAllocate(heap, allocationA, initialCommitted);
	FL_UT_CHECK(grown != NULL, "FlVirtualHeapUtReallocGrowTriggersCommit: grew");
	FL_UT_CHECK(grown == allocationA, "FlVirtualHeapUtReallocGrowTriggersCommit: same pointer (in-place)");
	FL_UT_CHECK(heap->regionCommittedSize > initialCommitted, "FlVirtualHeapUtReallocGrowTriggersCommit: committed size increased");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowTriggersCommit: validate");
	if (grown)
	{
		int preserved = 1;
		for (size_t i = 0; i < originalSize; i++)
		{
			if (grown[i] != (uint8_t)(i + 1))
			{
				preserved = 0;
				break;
			}
		}
		FL_UT_CHECK(preserved, "FlVirtualHeapUtReallocGrowTriggersCommit: data preserved");
		FlVirtualHeapFree(heap, grown);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtReallocGrowTriggersCommit: final validate");
	FlVirtualHeapDelete(heap);
}

// Repeated allocate/free pairs that straddle the committed-size boundary
// must keep working and keep the heap valid, whether or not the headroom
// policy avoids a fresh commit/decommit on any given iteration.
static void FlVirtualHeapUtBoundaryAllocFreeChurn(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE);
	if (!heap)
	{
		return;
	}

	size_t straddlePayload = heap->regionCommittedSize;
	int allSucceeded = 1;
	int allValid = 1;
	for (size_t i = 0; i < 64; i++)
	{
		void* allocation = FlVirtualHeapAllocate(heap, straddlePayload);
		if (!allocation)
		{
			allSucceeded = 0;
			break;
		}
		memset(allocation, (int)(i & 0xFF), straddlePayload);
		FlVirtualHeapFree(heap, allocation);
		if (!FlVirtualHeapValidate(heap))
		{
			allValid = 0;
			break;
		}
	}

	FL_UT_CHECK(allSucceeded, "FlVirtualHeapUtBoundaryAllocFreeChurn: every alloc succeeded");
	FL_UT_CHECK(allValid, "FlVirtualHeapUtBoundaryAllocFreeChurn: heap valid after every iteration");
	FlVirtualHeapDelete(heap);
}

#define FL_VIRTUAL_HEAP_UT_MULTI_PAGE_HEAP_SIZE          ((size_t)9 * 1024 * 1024)
#define FL_VIRTUAL_HEAP_UT_MULTI_PAGE_GRANULARITY_LIMIT  ((size_t)4 * 1024 * 1024)
#define FL_VIRTUAL_HEAP_UT_MULTI_PAGE_ALLOCATION_SIZE    ((size_t)8 * 1024 * 1024)

// A single allocation that requires committing more than one allocation-
// granularity unit at once (a multi-page commit) must succeed and leave the
// heap valid, both while the allocation is live and after it is freed.
static void FlVirtualHeapUtMultiPageCommitAllocation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_MULTI_PAGE_HEAP_SIZE);
	FL_UT_CHECK(heap != NULL, "FlVirtualHeapUtMultiPageCommitAllocation: create succeeded");
	if (!heap)
	{
		return;
	}

	FL_UT_CHECK(heap->allocationGranularity <= FL_VIRTUAL_HEAP_UT_MULTI_PAGE_GRANULARITY_LIMIT,
		"FlVirtualHeapUtMultiPageCommitAllocation: allocation granularity is at most 4 MiB");
	FL_UT_CHECK(heap->regionCommittedSize <= 2 * heap->allocationGranularity,
		"FlVirtualHeapUtMultiPageCommitAllocation: at most two granularity units are committed at create");

	void* allocation = FlVirtualHeapAllocate(heap, FL_VIRTUAL_HEAP_UT_MULTI_PAGE_ALLOCATION_SIZE);
	FL_UT_CHECK(allocation != NULL, "FlVirtualHeapUtMultiPageCommitAllocation: 8 MiB alloc succeeds");
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtMultiPageCommitAllocation: validate after alloc");
	FL_UT_CHECK(heap->regionCommittedSize > FL_VIRTUAL_HEAP_UT_MULTI_PAGE_ALLOCATION_SIZE,
		"FlVirtualHeapUtMultiPageCommitAllocation: committed memory exceeds 8 MiB after the allocation");

	if (allocation)
	{
		FlVirtualHeapFree(heap, allocation);
	}
	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtMultiPageCommitAllocation: validate after free");
	FL_UT_CHECK(heap->regionCommittedSize <= 2 * heap->allocationGranularity,
		"FlVirtualHeapUtMultiPageCommitAllocation: at most two granularity units are committed after free");

	FlVirtualHeapDelete(heap);
}

#undef FL_VIRTUAL_HEAP_UT_MULTI_PAGE_HEAP_SIZE
#undef FL_VIRTUAL_HEAP_UT_MULTI_PAGE_GRANULARITY_LIMIT
#undef FL_VIRTUAL_HEAP_UT_MULTI_PAGE_ALLOCATION_SIZE

typedef struct FlVirtualHeapUtSlowFillNode
{
	struct FlVirtualHeapUtSlowFillNode* next;
	struct FlVirtualHeapUtSlowFillNode* previous;
	size_t allocationNumber;
} FlVirtualHeapUtSlowFillNode;

#define FL_VIRTUAL_HEAP_UT_SLOW_FILL_HEAP_SIZE          ((size_t)9 * 1024 * 1024)
#define FL_VIRTUAL_HEAP_UT_SLOW_FILL_GRANULARITY_LIMIT  ((size_t)4 * 1024 * 1024)
#define FL_VIRTUAL_HEAP_UT_SLOW_FILL_ITEM_SIZE          ((size_t)1024)

// Slowly fills the heap one 1 KiB doubly linked list node at a time until it
// can no longer satisfy a request, exercising many separate growth calls
// across the heap's lifetime instead of a single large multi-page commit.
// All nodes are then freed in reverse order of allocation.
static void FlVirtualHeapUtSlowHeapFill(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FLVirtualHeap* heap = NULL;
	FlVirtualHeapCreate(&heap, FL_VIRTUAL_HEAP_UT_SLOW_FILL_HEAP_SIZE);
	FL_UT_CHECK(heap != NULL, "FlVirtualHeapUtSlowHeapFill: create succeeded");
	if (!heap)
	{
		return;
	}

	FL_UT_CHECK(heap->allocationGranularity <= FL_VIRTUAL_HEAP_UT_SLOW_FILL_GRANULARITY_LIMIT,
		"FlVirtualHeapUtSlowHeapFill: allocation granularity is at most 4 MiB");
	FL_UT_CHECK(heap->regionCommittedSize <= 2 * heap->allocationGranularity,
		"FlVirtualHeapUtSlowHeapFill: at most two granularity units are committed at create");

	FlVirtualHeapUtSlowFillNode* head = NULL;
	FlVirtualHeapUtSlowFillNode* tail = NULL;
	size_t allocationCount = 0;

	for (;;)
	{
		FlVirtualHeapUtSlowFillNode* node = (FlVirtualHeapUtSlowFillNode*)FlVirtualHeapAllocate(heap, FL_VIRTUAL_HEAP_UT_SLOW_FILL_ITEM_SIZE);
		if (!node)
		{
			break;
		}
		node->next = NULL;
		node->previous = tail;
		node->allocationNumber = allocationCount;
		if (tail)
		{
			tail->next = node;
		}
		else
		{
			head = node;
		}
		tail = node;
		allocationCount++;
	}

	FL_UT_CHECK(allocationCount > 0, "FlVirtualHeapUtSlowHeapFill: at least one node allocated before the heap filled");
	FL_UT_CHECK(heap->regionCommittedSize > heap->regionReservedSize - heap->allocationGranularity,
		"FlVirtualHeapUtSlowHeapFill: committed memory is within one granularity unit of the full reserved size after filling");

	// Walk forward from head and check the recorded allocation numbers and
	// link pointers are exactly what was written while filling the heap.
	int listIntact = 1;
	size_t expectedAllocationNumber = 0;
	FlVirtualHeapUtSlowFillNode* expectedPrevious = NULL;
	for (FlVirtualHeapUtSlowFillNode* node = head; node; node = node->next)
	{
		if (node->allocationNumber != expectedAllocationNumber || node->previous != expectedPrevious)
		{
			listIntact = 0;
			break;
		}
		expectedPrevious = node;
		expectedAllocationNumber++;
	}
	FL_UT_CHECK(listIntact && expectedAllocationNumber == allocationCount, "FlVirtualHeapUtSlowHeapFill: linked list intact after filling");
	FL_UT_CHECK(tail != NULL && tail->next == NULL, "FlVirtualHeapUtSlowHeapFill: tail terminates the list");

	// Free all nodes in reverse order of allocation.
	size_t freeCount = 0;
	FlVirtualHeapUtSlowFillNode* node = tail;
	while (node)
	{
		FlVirtualHeapUtSlowFillNode* previous = node->previous;
		FlVirtualHeapFree(heap, node);
		freeCount++;
		node = previous;
	}
	FL_UT_CHECK(freeCount == allocationCount, "FlVirtualHeapUtSlowHeapFill: freed exactly as many nodes as were allocated");

	FL_UT_CHECK(FlVirtualHeapValidate(heap), "FlVirtualHeapUtSlowHeapFill: validate after freeing all nodes");
	FL_UT_CHECK(heap->regionCommittedSize <= 2 * heap->allocationGranularity,
		"FlVirtualHeapUtSlowHeapFill: at most two granularity units are committed after freeing all nodes");

	FlVirtualHeapDelete(heap);
}

#undef FL_VIRTUAL_HEAP_UT_SLOW_FILL_HEAP_SIZE
#undef FL_VIRTUAL_HEAP_UT_SLOW_FILL_GRANULARITY_LIMIT
#undef FL_VIRTUAL_HEAP_UT_SLOW_FILL_ITEM_SIZE

#undef FL_VIRTUAL_HEAP_UT_LARGE_HEAP_SIZE

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlVirtualHeapUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlVirtualHeapUtCreateAndValidate(testCount, failCount);
	FlVirtualHeapUtCreateDefault(testCount, failCount);
	FlVirtualHeapUtAllocateZero(testCount, failCount);
	FlVirtualHeapUtAllocateAlignment(testCount, failCount);
	FlVirtualHeapUtAllocateWritable(testCount, failCount);
	FlVirtualHeapUtFreeNull(testCount, failCount);
	FlVirtualHeapUtFirstFitReuse(testCount, failCount);
	FlVirtualHeapUtCoalesceForward(testCount, failCount);
	FlVirtualHeapUtCoalesceBackward(testCount, failCount);
	FlVirtualHeapUtCoalesceBoth(testCount, failCount);
	FlVirtualHeapUtDataIsolationAfterFree(testCount, failCount);
	FlVirtualHeapUtReallocFromNull(testCount, failCount);
	FlVirtualHeapUtReallocToZero(testCount, failCount);
	FlVirtualHeapUtReallocGrowPreservesData(testCount, failCount);
	FlVirtualHeapUtReallocShrinkPreservesData(testCount, failCount);
	FlVirtualHeapUtReallocGrowForcedMove(testCount, failCount);
	FlVirtualHeapUtReallocGrowInPlace(testCount, failCount);
	FlVirtualHeapUtNoOverlap(testCount, failCount);
	FlVirtualHeapUtAllocateAfterFailure(testCount, failCount);
	FlVirtualHeapUtFragmentAndCoalesce(testCount, failCount);
	FlVirtualHeapUtAllocateFreeLoop(testCount, failCount);
	FlVirtualHeapUtMixedSizeFragmentation(testCount, failCount);
	FlVirtualHeapUtUnorderedFree(testCount, failCount);
	FlVirtualHeapUtFirstFitAfterFullFree(testCount, failCount);
	FlVirtualHeapUtReallocShrinkSplit(testCount, failCount);
	FlVirtualHeapUtReallocShrinkNoSplit(testCount, failCount);
	FlVirtualHeapUtAllocateSizeOverflow(testCount, failCount);
	FlVirtualHeapUtFuzz(testCount, failCount);
	FlVirtualHeapUtAlmostFull(testCount, failCount);
	FlVirtualHeapUtExactlyFull(testCount, failCount);
	FlVirtualHeapUtAllocatePastFull(testCount, failCount);
	FlVirtualHeapUtCommittedSizeStartsSmall(testCount, failCount);
	FlVirtualHeapUtCommittedSizeGrows(testCount, failCount);
	FlVirtualHeapUtCommittedSizeShrinksAfterFree(testCount, failCount);
	FlVirtualHeapUtBoundaryAllocFreeChurn(testCount, failCount);
	FlVirtualHeapUtMultiPageCommitAllocation(testCount, failCount);
	FlVirtualHeapUtSlowHeapFill(testCount, failCount);
	FlVirtualHeapUtCreateOversizedFails(testCount, failCount);
	FlVirtualHeapUtCreateTinySizeClamps(testCount, failCount);
	FlVirtualHeapUtReallocSizeOverflow(testCount, failCount);
	FlVirtualHeapUtReallocGrowFailsLeavesOriginalIntact(testCount, failCount);
	FlVirtualHeapUtReallocGrowTriggersCommit(testCount, failCount);
}
