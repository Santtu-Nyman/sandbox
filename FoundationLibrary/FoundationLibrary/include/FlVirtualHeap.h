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
*/
#ifndef FL_VIRTUAL_HEAP_H
#define FL_VIRTUAL_HEAP_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include "flHRESULT.h"
#include "FlSAL.h"

#define FL_VIRTUAL_HEAP_ALIGNMENT 0x10

HRESULT FlVirtualHeapCreate(_Out_ void** _In_ heap, _In_ int flags, _In_ size_t size);

_Ret_maybenull_ _Post_writable_byte_size_(size) DECLSPEC_ALLOCATOR void* FlVirtualHeapAllocate(_In_ void* heap, _In_ size_t size);

void FlVirtualHeapFree(_In_ void* heap, _Pre_maybenull_ _Post_invalid_ void* allocation);

_Ret_maybenull_ _Post_writable_byte_size_(size) DECLSPEC_ALLOCATOR void* FlVirtualHeapReAllocate(_In_ void* heap, _Pre_maybenull_ _Post_invalid_ void* allocation, _In_ size_t size);

#ifdef _DEBUG
void FlVirtualHeapDebugValidate(_In_ void* heap);
#endif // _DEBUG

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_VIRTUAL_HEAP_H
