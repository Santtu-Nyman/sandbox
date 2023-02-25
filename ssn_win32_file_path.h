/*
	Win32 file path manipulation library version 1.0.0 2023-02-25 by Santtu S. Nyman.

	Description
		Win32 file path manipulation library for creating qualified paths from
		different types of relative paths and for extractig volume directory path
		from file path. All the functionality is implemented both in Win32 native UTF-16
		or UTF-8 encoding. However the UTF-8 versions of the functionality use UTF-16 length 
		for MAX_PATH logic and in determining when to use extended path prefix.

		This library does depend on the "ssn_utf8_utf16_converter.c".

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

#ifndef SSN_WIN32_FILE_PATH_H
#define SSN_WIN32_FILE_PATH_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <Windows.h>

BOOL ssn_win32_is_path_fully_qualified(SIZE_T path_length, const WCHAR* path);

SIZE_T ssn_win32_get_fully_qualified_path(SIZE_T path_length, const WCHAR* path, SIZE_T base_path_length, const WCHAR* base_path, SIZE_T path_buffer_size, WCHAR* path_buffer);

SIZE_T ssn_win32_get_volume_directory_path(SIZE_T path_length, const WCHAR* path, SIZE_T base_path_length, const WCHAR* base_path, SIZE_T path_buffer_size, WCHAR* path_buffer);

BOOL ssn_win32_is_path_fully_qualified_utf8(SIZE_T path_length, const char* path);

SIZE_T ssn_win32_get_fully_qualified_path_utf8(SIZE_T path_length, const char* path, SIZE_T base_path_length, const char* base_path, SIZE_T path_buffer_size, char* path_buffer);

SIZE_T ssn_win32_get_volume_directory_path_utf8(SIZE_T path_length, const char* path, SIZE_T base_path_length, const char* base_path, SIZE_T path_buffer_size, char* path_buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SSN_WIN32_FILE_PATH_H
