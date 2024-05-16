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
#include "FlSha256.h"
#include <stdint.h>
#include <string.h>

#define FL_SHA256_HMAC_BLOCK_SIZE 64

void FlSha256Hmac(size_t KeySize, const void* Key, size_t DataSize, const void* Data, void* Digest)
{
	FlSha256Context Context;

	uint8_t Pad[FL_SHA256_HMAC_BLOCK_SIZE];
	if (KeySize <= FL_SHA256_HMAC_BLOCK_SIZE)
	{
		memcpy(&Pad[0], Key, KeySize);
		memset(&Pad[KeySize], 0, FL_SHA256_HMAC_BLOCK_SIZE - KeySize);
	}
	else
	{
		FlSha256CreateHash(&Context);
		FlSha256HashData(&Context, KeySize, Key);
		FlSha256FinishHash(&Context, &Pad[0]);
		memset(&Pad[FL_SHA256_DIGEST_SIZE], 0, FL_SHA256_HMAC_BLOCK_SIZE - FL_SHA256_DIGEST_SIZE);
	}
	
	uint8_t InnerDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256CreateHash(&Context);
	for (size_t i = 0; i < FL_SHA256_HMAC_BLOCK_SIZE; i++)
	{
		Pad[i] ^= 0x36;
	}
	FlSha256HashData(&Context, FL_SHA256_HMAC_BLOCK_SIZE, &Pad[0]);
	FlSha256HashData(&Context, DataSize, Data);
	FlSha256FinishHash(&Context, &InnerDigest[0]);

	FlSha256CreateHash(&Context);
	for (size_t i = 0; i < FL_SHA256_HMAC_BLOCK_SIZE; i++)
	{
		Pad[i] ^= 0x6A;
	}
	FlSha256HashData(&Context, FL_SHA256_HMAC_BLOCK_SIZE, &Pad[0]);
	FlSha256HashData(&Context, FL_SHA256_DIGEST_SIZE, &InnerDigest[0]);
	FlSha256FinishHash(&Context, Digest);
}

#ifdef __cplusplus
}
#endif // __cplusplus
