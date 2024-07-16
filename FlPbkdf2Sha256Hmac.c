/*
	Santtu S. Nyman's public domain PBKDF2-HMAC-SHA256 implementation.

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

#include "FlPbkdf2Sha256Hmac.h"
#include "FlSha256Hmac.h"
#include <string.h>

void FlPbkdf2Sha256Hmac(uint64_t IterationCount, size_t PasswordSize, const void* Password, size_t SaltSize, const void* Salt, size_t DerivedKeySize, void* DerivedKey)
{
	if (!IterationCount)
	{
		memset(DerivedKey, 0, DerivedKeySize);
		return;
	}

	uint8_t Block[FL_SHA256_DIGEST_SIZE];
	uint8_t Digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacContext BaseContext;
	FlSha256HmacContext Context;
	FlSha256HmacCreateHmac(&BaseContext, PasswordSize, Password);

	for (size_t KeyBytesLeft = DerivedKeySize, BlockIndex = 1; KeyBytesLeft;)
	{
		uint8_t BlockIndexData[4] = { (uint8_t)((BlockIndex >> 24) & 0xFF), (uint8_t)((BlockIndex >> 16) & 0xFF), (uint8_t)((BlockIndex >> 8) & 0xFF), (uint8_t)(BlockIndex & 0xFF) };

		memcpy(&Context, &BaseContext, sizeof(FlSha256HmacContext));
		FlSha256HmacHashData(&Context, SaltSize, Salt);
		FlSha256HmacHashData(&Context, sizeof(uint32_t), &BlockIndexData[0]);
		FlSha256Hmac256FinishHmac(&Context, &Digest[0]);
		memcpy(Block, Digest, FL_SHA256_DIGEST_SIZE);

		for (size_t i = 1; i < IterationCount; i++)
		{
			memcpy(&Context, &BaseContext, sizeof(FlSha256HmacContext));
			FlSha256HmacHashData(&Context, FL_SHA256_DIGEST_SIZE, &Digest[0]);
			FlSha256Hmac256FinishHmac(&Context, &Digest[0]);
			for (size_t j = 0; j < FL_SHA256_DIGEST_SIZE; j++)
			{
				Block[j] ^= Digest[j];
			}
		}

		size_t BlockKeyBytes = FL_SHA256_DIGEST_SIZE;
		if (BlockKeyBytes > KeyBytesLeft)
		{
			BlockKeyBytes = KeyBytesLeft;
		}
		memcpy((void*)((uintptr_t)DerivedKey + (DerivedKeySize - KeyBytesLeft)), Block, BlockKeyBytes);
		BlockIndex++;
		KeyBytesLeft -= BlockKeyBytes;
	}
}

#ifdef __cplusplus
}
#endif // __cplusplus
