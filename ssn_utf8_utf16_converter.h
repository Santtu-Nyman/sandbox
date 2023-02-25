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

#ifndef SSN_UTF8_UTF16_CONVERTER_H
#define SSN_UTF8_UTF16_CONVERTER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>

size_t ssn_convert_utf8_to_utf16le(size_t utf8_lenght, const uint8_t* utf8, size_t utf16_buffer_length, uint16_t* utf16_buffer);

size_t ssn_convert_utf16le_to_utf8(size_t utf16_lenght, const uint16_t* utf16, size_t utf8_buffer_length, uint8_t* utf8_buffer);

size_t ssn_convert_utf8_to_utf16be(size_t utf8_lenght, const uint8_t* utf8, size_t utf16_buffer_length, uint16_t* utf16_buffer);

size_t ssn_convert_utf16be_to_utf8(size_t utf16_lenght, const uint16_t* utf16, size_t utf8_buffer_length, uint8_t* utf8_buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SSN_UTF8_UTF16_CONVERTER_H
