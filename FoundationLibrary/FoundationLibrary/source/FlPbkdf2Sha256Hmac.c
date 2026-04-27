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

void FlPbkdf2Sha256Hmac(_In_ uint64_t iterationCount, _In_ size_t passwordSize, _In_reads_bytes_(passwordSize) const void* password, _In_ size_t saltSize, _In_reads_bytes_(saltSize) const void* salt, _In_ size_t derivedKeySize, _Out_writes_bytes_all_(derivedKeySize) void* derivedKey)
{
	if (!iterationCount)
	{
		memset(derivedKey, 0, derivedKeySize);
		return;
	}

	uint8_t block[FL_SHA256_DIGEST_SIZE];
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacContext baseContext;
	FlSha256HmacContext context;
	FlSha256HmacCreateHmac(&baseContext, passwordSize, password);

	for (size_t keyBytesLeft = derivedKeySize, blockIndex = 1; keyBytesLeft;)
	{
		uint8_t blockIndexData[4] = { (uint8_t)((blockIndex >> 24) & 0xFF), (uint8_t)((blockIndex >> 16) & 0xFF), (uint8_t)((blockIndex >> 8) & 0xFF), (uint8_t)(blockIndex & 0xFF) };

		memcpy(&context, &baseContext, sizeof(FlSha256HmacContext));
		FlSha256HmacHashData(&context, saltSize, salt);
		FlSha256HmacHashData(&context, sizeof(uint32_t), &blockIndexData[0]);
		FlSha256Hmac256FinishHmac(&context, &digest[0]);
		memcpy(block, digest, FL_SHA256_DIGEST_SIZE);

		for (size_t i = 1; i < iterationCount; i++)
		{
			memcpy(&context, &baseContext, sizeof(FlSha256HmacContext));
			FlSha256HmacHashData(&context, FL_SHA256_DIGEST_SIZE, &digest[0]);
			FlSha256Hmac256FinishHmac(&context, &digest[0]);
			for (size_t j = 0; j < FL_SHA256_DIGEST_SIZE; j++)
			{
				block[j] ^= digest[j];
			}
		}

		size_t blockKeyBytes = FL_SHA256_DIGEST_SIZE;
		if (blockKeyBytes > keyBytesLeft)
		{
			blockKeyBytes = keyBytesLeft;
		}
		memcpy((void*)((uintptr_t)derivedKey + (derivedKeySize - keyBytesLeft)), block, blockKeyBytes);
		blockIndex++;
		keyBytesLeft -= blockKeyBytes;
	}
}

#ifdef __cplusplus
}
#endif // __cplusplus
