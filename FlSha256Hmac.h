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
#include <stdint.h>
#include "FlSha256.h"

#define FL_SHA256_HMAC_BLOCK_SIZE 64

typedef struct
{
	FlSha256Context HashContext;
	uint8_t Pad[FL_SHA256_HMAC_BLOCK_SIZE];
} FlSha256HmacContext;

void FlSha256HmacCreateHmac(FlSha256HmacContext* Context, size_t KeySize, const void* Key);
/*
	Procedure:
		FlSha256HmacCreateHmac

	Description:
		This procedure initilizes a new SHA-256 HMAC calculation context.
		The pre-initialization contents of the context are ignored and overridden on initialization.

	Parameters:
		Context:
			Address of the HMAC calculation context.

		KeySize:
			Size of the key data in bytes.

		Key:
			Pointer to the location that contains the key data.
*/

void FlSha256HmacHashData(FlSha256HmacContext* Context, size_t InputSize, const void* InputData);
/*
	Procedure:
		FlSha256HmacHashData

	Description:
		This procedure processes a chunk of input data for SHA-256 HMAC calculation.
		The user may combine arbitrary number of input chunks for a HMAC calculation by calling this procedure repeatedly passing input chunks in order.

	Parameters:
		Context:
			Address of the HMAC calculation context.

		InputSize:
			Size of the next data chunk to process in bytes.

		InputData:
			pointer to the location that contains the next data chunk to process in the HMAC calculation.
*/

void FlSha256Hmac256FinishHmac(FlSha256HmacContext* Context, void* Digest);
/*
	Procedure:
		FlSha256Hmac256Finish

	Description:
		This procedure finalizes calculating the SHA-256 HMAC and invalidates the calculation context.
		The context may not be used again before it has been re-initialized.
		Calling this procedure immediately after context initialization calculates disgust of no input.

	Parameters:
		Context:
			Address of the HMAC calculation context.

		Digest:
			The calculated 32 byte HMAC is stored in the location pointed by this parameter.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_SHA256_HMAC_H
