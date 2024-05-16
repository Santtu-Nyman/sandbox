/*
	Santtu S. Nyman's random file name generation library

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

#ifndef FL_RANDOM_FILE_NAME_H
#define FL_RANDOM_FILE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <Windows.h>

// File name length for a 8.3 file name without dot or extension
#define FL_RANDOM_FILE_NAME_LENGTH 8

size_t FlCreateRandomFileName(WCHAR* NameBuffer);
/*
	Procedure:
		FlCreateRandomFileName

	Description:
		This procedure generates a new random file name.
		The file name is the name for a 8.3 compliant file name without dot or extension, so the length is 8 characters long with limited sot of characters.
		The entropy of the file name is log2(52^8) = ~46 bits, so file name collisions are cuite unlikely, but the caller should still check if the file by the generated name already exists.

	Parameters:
		NameBuffer:
			Address of the buffer where the new file name is written into in as non null-terminated UTF-16 string.
			This buffer must be at least 8 characters long.

	Return:
		The procedure returns the length of the string that is always 8.
*/

size_t FlCreateRandomFileNameUtf8(char* NameBuffer);
/*
	Procedure:
		FlCreateRandomFileNameUtf8

	Description:
		This procedure generates a new random file name.
		The file name is the name for a 8.3 compliant file name without dot or extension, so the length is 8 characters long with limited sot of characters.
		The entropy of the file name is log2(52^8) = ~46 bits, so file name collisions are cuite unlikely, but the caller should still check if the file by the generated name already exists.

	Parameters:
		NameBuffer:
			Address of the buffer where the new file name is written into in as non null-terminated UTF-8 string.
			This buffer must be at least 8 characters long.

	Return:
		The procedure returns the length of the string that is always 8.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_RANDOM_FILE_NAME_H
