/*
	Santtu S. Nyman's version of r10nw7fd3's public domain SHA1 implementation ( https://github.com/clibs/sha1/ Friday, 26 April 2024 ).
	This modified implementation is released to public domain as well.

	Version history
		version 1.0.0 2024-04-26
			First public version.

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

#ifndef FL_SHA1_H
#define FL_SHA1_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include "FlSAL.h"

#define FL_SHA1_DIGEST_SIZE 20

typedef struct
{
	uint64_t size;
	uint32_t state[5];
	uint32_t padding;
	uint8_t input[64];
} FlSha1Context;

void FlSha1CreateHash(_Out_ FlSha1Context* context);
/*
	Procedure:
		FlSha1CreateHash

	Description:
		This procedure initilizes a new Sha1 calculation context.
		The pre-initialization contents of the context are ignored and overridden on initialization.

	Parameters:
		context:
			Address of the Sha1 calculation context.
*/

void FlSha1HashData(_Inout_ FlSha1Context* context, _In_ size_t inputSize, _In_reads_bytes_(inputSize) const void* inputData);
/*
	Procedure:
		FlSha1HashData

	Description:
		This procedure processes a chunk of input data for Sha1 calculation.
		The user may combine arbitrary number of input chunks for a hash calculation by calling this procedure repeatedly passing input chunks in order.

	Parameters:
		context:
			Address of the initialized Sha1 calculation context.

		inputSize:
			Size of the next data chunk to process in bytes.

		inputData:
			pointer to the location that contains the next data chunk to process in the hash calculation.
*/

void FlSha1FinishHash(_Inout_ FlSha1Context* context, _Out_writes_bytes_all_(FL_SHA1_DIGEST_SIZE) void* digest);
/*
	Procedure:
		FlSha1FinishHash

	Description:
		This procedure finalizes calculating the Sha1 and invalidates the calculation context.
		The context may not be used again before it has been re-initialized.
		Calling this procedure immediately after context initialization calculates disgust of no input.

	Parameters:
		context:
			Address of the initialized Sha1 calculation context.

		digest:
			The calculated 20 byte Sha1 digest is stored in the location pointed by this parameter.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_SHA1_H
