/*
	UTF-16 - UTF-8 conversion library version 1.0.0 2023-02-25 by Santtu S. Nyman.

	Description
		Simple library for converting between UTF-16 UTF-8.
		The functionality of this library is intended to be a replacement for
		MultiByteToWideChar and WideCharToMultiByte from Kernel32.dll.
		The handling of corrupted unicode is handle the same way as the previously
		mentioned Win32 functions.

	Version history
		version 1.0.0 2023-02-25
			First publicly available version.

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

#ifndef LT_UTF8_UTF16_CONVERTER_H
#define LT_UTF8_UTF16_CONVERTER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>

size_t LtConvertUtf8ToUtf16Le(size_t Utf8Length, const char* Utf8Data, size_t Utf16BufferLength, WCHAR* Utf16Buffer);

size_t LtConvertUtf16LeToUtf8(size_t Utf16Length, const WCHAR* Utf16Data, size_t Utf8BufferLength, char* Utf8Buffer);

size_t LtConvertUtf8ToUtf16Be(size_t Utf8Length, const char* Utf8Data, size_t Utf16BufferLength, WCHAR* Utf16Buffer);

size_t LtConvertUtf16BeToUtf8(size_t Utf16Length, const WCHAR* Utf16Data, size_t Utf8BufferLength, char* Utf8Buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LT_UTF8_UTF16_CONVERTER_H
