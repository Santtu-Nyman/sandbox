/*
	Santtu S. Nyman's 2024 version of Alain Mosnier's public domain SHA-256 implementation ( https://github.com/amosnier/sha-2 Sunday, 10 March 2024 ).
	This modified implementation is released to public domain as well.

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

#ifndef SHA256_H
#define SHA256_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_SIZE 32

typedef struct
{
	uint64_t Size;
	uint32_t Buffer[8];
	uint8_t Input[64];
} Sha256Context;

void Sha256Initialize(Sha256Context* Context);
/*
	Procedure:
		Sha256Initialize

	Description:
		This procedure initilizes a new SHA-256 calculation context.
		The pre-initialization contents of the context are ignored and overridden on initialization.

	Parameters:
		Context:
			Address of the SHA-256 calculation context.
*/

void Sha256Update(Sha256Context* Context, size_t InputSize, const void* InputData);
/*
	Procedure:
		Sha256Update

	Description:
		This procedure processes a chunk of input data for SHA-256 calculation.
		The user may combine arbitrary number of input chunks for a hash calculation by calling this procedure repeatedly passing input chunks in order.

	Parameters:
		Context:
			Address of the SHA-256 calculation context.

		InputSize:
			Size of the next data chunk to process in bytes.

		InputData:
			pointer to the location that contains the next data chunk to process in the hash calculation.
*/

void Sha256Finalize(Sha256Context* Context, void* Digest);
/*
	Procedure:
		Sha256Finalize

	Description:
		This procedure finalizes calculating the SHA-256 and invalidates the calculation context.
		The context may not be used again before it has been re-initialized.
		Calling this procedure immediately after context initialization calculates disgust of no input.
		
	Parameters:
		Context:
			Address of the SHA-256 calculation context.

		Digest:
			The calculated 32 byte SHA-256 digest is stored in the location pointed by this parameter.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SHA256_H
