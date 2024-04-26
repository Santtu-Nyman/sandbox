/*
	Win32 file path manipulation library version 1.0.0 2023-02-25 by Santtu S. Nyman.

	Description
		Win32 file path manipulation library for creating qualified paths from
		different types of relative paths and for extractig volume directory path
		from file path. All the functionality is implemented both in Win32 native UTF-16
		or UTF-8 encoding. However the UTF-8 versions of the functionality use UTF-16 length 
		for MAX_PATH logic and in determining when to use extended path prefix.

		This library does depend on the "FlUtf8Utf16Converter.c".

	Version history
		version 1.0.0 2024-04-01
			Bug fixes.
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

#ifndef FL_WIN32_FILE_PATH_H
#define FL_WIN32_FILE_PATH_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <Windows.h>

BOOL FlWin32IsPathFullyQualified(SIZE_T PathLength, const WCHAR* Path);

SIZE_T FlWin32GetFullyQualifiedPath(SIZE_T PathLength, const WCHAR* Path, SIZE_T BasePathLength, const WCHAR* BasePath, SIZE_T PathBufferSize, WCHAR* PathBuffer);

SIZE_T FlWin32GetVolumeDirectoryPath(SIZE_T PathLength, const WCHAR* Path, SIZE_T BasePathLength, const WCHAR* BasePath, SIZE_T PathBufferSize, WCHAR* PathBuffer);

BOOL FlWin32IsPathFullyQualifiedUtf8(SIZE_T PathLength, const char* Path);

SIZE_T FlWin32GetFullyQualifiedPathUtf8(SIZE_T PathLength, const char* Path, SIZE_T BasePathLength, const char* BasePath, SIZE_T PathBufferSize, char* PathBuffer);

SIZE_T FlWin32GetVolumeDirectoryPathUtf8(SIZE_T PathLength, const char* Path, SIZE_T BasePathLength, const char* BasePath, SIZE_T PathBufferSize, char* PathBuffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_WIN32_FILE_PATH_H
