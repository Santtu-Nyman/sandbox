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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "FlSha256Hmac.h"
#include <string.h>

void FlSha256HmacCreateHmac(_Out_ FlSha256HmacContext* context, _In_ size_t keySize, _In_reads_bytes_(keySize) const void* key)
{
	if (keySize <= FL_SHA256_HMAC_BLOCK_SIZE)
	{
		memcpy(&context->pad[0], key, keySize);
		memset(&context->pad[keySize], 0, FL_SHA256_HMAC_BLOCK_SIZE - keySize);
	}
	else
	{
		FlSha256CreateHash(&context->hashContext);
		FlSha256HashData(&context->hashContext, keySize, key);
		FlSha256FinishHash(&context->hashContext, &context->pad[0]);
		memset(&context->pad[FL_SHA256_DIGEST_SIZE], 0, FL_SHA256_HMAC_BLOCK_SIZE - FL_SHA256_DIGEST_SIZE);
	}
	for (size_t i = 0; i < FL_SHA256_HMAC_BLOCK_SIZE; i++)
	{
		context->pad[i] ^= 0x36;
	}
	FlSha256CreateHash(&context->hashContext);
	FlSha256HashData(&context->hashContext, FL_SHA256_HMAC_BLOCK_SIZE, &context->pad[0]);
	for (size_t i = 0; i < FL_SHA256_HMAC_BLOCK_SIZE; i++)
	{
		context->pad[i] ^= 0x6A;
	}
}

void FlSha256HmacHashData(_Inout_ FlSha256HmacContext* context, _In_ size_t inputSize, _In_reads_bytes_(inputSize) const void* inputData)
{
	FlSha256HashData(&context->hashContext, inputSize, inputData);
}

void FlSha256Hmac256FinishHmac(_Inout_ FlSha256HmacContext* context, _Out_writes_bytes_all_(FL_SHA256_DIGEST_SIZE) void* digest)
{
	uint8_t innerDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256FinishHash(&context->hashContext, &innerDigest[0]);
	FlSha256CreateHash(&context->hashContext);
	FlSha256HashData(&context->hashContext, FL_SHA256_HMAC_BLOCK_SIZE, &context->pad[0]);
	FlSha256HashData(&context->hashContext, FL_SHA256_DIGEST_SIZE, &innerDigest[0]);
	FlSha256FinishHash(&context->hashContext, digest);
}

#ifdef __cplusplus
}
#endif // __cplusplus
