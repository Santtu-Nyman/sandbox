/*
	Santtu S. Nyman's public domain SHA-256 HMAC implementation.

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

#ifndef FL_SHA256_HMAC_H
#define FL_SHA256_HMAC_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>

void FlSha256Hmac(size_t KeySize, const void* Key, size_t DataSize, const void* Data, void* Digest);
/*
	Procedure:
		FlSha256Hmac

	Description:
		This procedure calculates SHA-256 HMAC for given key and data pair.

	Parameters:
		KeySize:
			Size of the key data in bytes.

		Key:
			Pointer to the location that contains the key data.

		DataSize:
			Size of the message data in bytes.

		Data:
			Pointer to the location that contains the message data.

		Digest:
			The calculated 32 byte SHA-256 HMAC is stored in the location pointed by this parameter.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_SHA256_HMAC_H
