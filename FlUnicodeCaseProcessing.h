/*
	Unicode case processing library version 1.0.0 2024-04-01 by Santtu S. Nyman.

	Description
		Simple unicode case processing library.

	Version history
		version 1.0.0 2024-04-01
			Initial version.

	License
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
		For more information, please refer to <https://unlicense.org>
*/

#ifndef FL_UNICODE_CASE_PROCESSING_H
#define FL_UNICODE_CASE_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>

int FlCodepointToUpperCase(int Codepoint);

int FlCodepointToLowerCase(int Codepoint);

int FlCompareStringOrdinal(const WCHAR* String1, size_t String1Length, const WCHAR* String2, size_t String2Length, BOOL IgnoreCase);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_UNICODE_CASE_PROCESSING_H
