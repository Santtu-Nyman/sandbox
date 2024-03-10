/*
	Santtu S. Nyman's 2024 public domain UUID utilities.

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

#ifndef UUID_H
#define UUID_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <Windows.h>

void UuidCreateRandomId(void* Uuid);
/*
	Procedure:
		UuidCreateRandomId

	Description:
		This procedure creates a new random UUID.
		The byte order of the UUID used by this procedure is the Windows UUID byte order.
		This procedure can not fail.

	Parameters:
		Uuid:
			Pointer to a buffer that will receive a new random UUID.
*/

size_t UuidEncodeStringUtf8(const void* Uuid, char* Buffer);
/*
	Procedure:
		UuidEncodeString

	Description:
		This procedure prints UUID into non-null terminated UTF-8 string.
		The byte order of the UUID used by this procedure is the Windows UUID byte order.
		This procedure can not fail.

	Parameters:
		Uuid:
			Pointer to a UUID to be printed.

		Buffer:
			Pointer to a buffer that will receive the non-null terminated printed UUID UTF-8 string.
			This buffer must be at least 38 characters long.

	Return:
		The procedure returns the length of the string that is always 38.
*/

size_t UuidDecodeStringUtf8(void* Uuid, size_t Length, const char* String);
/*
	Procedure:
		UuidDecodeString

	Description:
		This procedure decodes non-null terminated UTF-8 string encoding an UUID value.
		The UUID string maybe optionally be surrounded by '{' and '}' characters like on Windows.
		The byte order of the UUID used by this procedure is the Windows UUID byte order.
		This procedure only fails if the string does not represent an UUID.

	Parameters:
		Uuid:
			Pointer to a buffer that will receive the decoded UUID.

		Length:
			Length of the non-null terminated UTF-8 string.
			Note that the string may be null terminated if the length excludes the null terminating character.

		String:
			Pointer to a string encoding an UUID value.

	Return:
		Returns the length of the UUID part of the string on success and zero if the string does not represent an UUID.

*/

size_t UuidEncodeStringUtf16(const void* Uuid, WCHAR* Buffer);
/*
	Procedure:
		UuidEncodeString

	Description:
		This procedure prints UUID into non-null terminated UTF-16 string.
		The byte order of the UUID used by this procedure is the Windows UUID byte order.
		This procedure can not fail.

	Parameters:
		Uuid:
			Pointer to a UUID to be printed.

		Buffer:
			Pointer to a buffer that will receive the non-null terminated printed UUID UTF-16 string.
			This buffer must be at least 38 characters long.

	Return:
		The procedure returns the length of the string that is always 38.
*/

size_t UuidDecodeStringUtf16(void* Uuid, size_t Length, const WCHAR* String);
/*
	Procedure:
		UuidDecodeString

	Description:
		This procedure decodes non-null terminated UTF-16 string encoding an UUID value.
		The UUID string maybe optionally be surrounded by '{' and '}' characters like on Windows.
		The byte order of the UUID used by this procedure is the Windows UUID byte order.
		This procedure only fails if the string does not represent an UUID.

	Parameters:
		Uuid:
			Pointer to a buffer that will receive the decoded UUID.

		Length:
			Length of the non-null terminated UTF-16 string.
			Note that the string may be null terminated if the length excludes the null terminating character.

		String:
			Pointer to a string encoding an UUID value.

	Return:
		Returns the length of the UUID part of the string on success and zero if the string does not represent an UUID.

*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // UUID_H
