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

void FlSha256HmacCreateHmac(FlSha256HmacContext* Context, size_t KeySize, const void* Key)
{
	if (KeySize <= FL_SHA256_HMAC_BLOCK_SIZE)
	{
		memcpy(&Context->Pad[0], Key, KeySize);
		memset(&Context->Pad[KeySize], 0, FL_SHA256_HMAC_BLOCK_SIZE - KeySize);
	}
	else
	{
		FlSha256CreateHash(&Context->HashContext);
		FlSha256HashData(&Context->HashContext, KeySize, Key);
		FlSha256FinishHash(&Context->HashContext, &Context->Pad[0]);
		memset(&Context->Pad[FL_SHA256_DIGEST_SIZE], 0, FL_SHA256_HMAC_BLOCK_SIZE - FL_SHA256_DIGEST_SIZE);
	}
	for (size_t i = 0; i < FL_SHA256_HMAC_BLOCK_SIZE; i++)
	{
		Context->Pad[i] ^= 0x36;
	}
	FlSha256CreateHash(&Context->HashContext);
	FlSha256HashData(&Context->HashContext, FL_SHA256_HMAC_BLOCK_SIZE, &Context->Pad[0]);
	for (size_t i = 0; i < FL_SHA256_HMAC_BLOCK_SIZE; i++)
	{
		Context->Pad[i] ^= 0x6A;
	}
}

void FlSha256HmacHashData(FlSha256HmacContext* Context, size_t InputSize, const void* InputData)
{
	FlSha256HashData(&Context->HashContext, InputSize, InputData);
}

void FlSha256Hmac256FinishHmac(FlSha256HmacContext* Context, void* Digest)
{
	uint8_t InnerDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256FinishHash(&Context->HashContext, &InnerDigest[0]);
	FlSha256CreateHash(&Context->HashContext);
	FlSha256HashData(&Context->HashContext, FL_SHA256_HMAC_BLOCK_SIZE, &Context->Pad[0]);
	FlSha256HashData(&Context->HashContext, FL_SHA256_DIGEST_SIZE, &InnerDigest[0]);
	FlSha256FinishHash(&Context->HashContext, Digest);
}

#ifdef __cplusplus
}
#endif // __cplusplus
