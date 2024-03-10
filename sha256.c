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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "sha256.h"
#include <string.h>

#define SHA256_INTERNAL_ROTATE_RIGHT_32(X, N) ((X >> N) | (X << (32 - N)))

static const uint32_t Sha256InternalConstantTableK[64] = {
	0x428a2f98lu, 0x71374491lu, 0xb5c0fbcflu, 0xe9b5dba5lu, 0x3956c25blu, 0x59f111f1lu, 0x923f82a4lu, 0xab1c5ed5lu,
	0xd807aa98lu, 0x12835b01lu, 0x243185belu, 0x550c7dc3lu, 0x72be5d74lu, 0x80deb1felu, 0x9bdc06a7lu, 0xc19bf174lu,
	0xe49b69c1lu, 0xefbe4786lu, 0x0fc19dc6lu, 0x240ca1cclu, 0x2de92c6flu, 0x4a7484aalu, 0x5cb0a9dclu, 0x76f988dalu,
	0x983e5152lu, 0xa831c66dlu, 0xb00327c8lu, 0xbf597fc7lu, 0xc6e00bf3lu, 0xd5a79147lu, 0x06ca6351lu, 0x14292967lu,
	0x27b70a85lu, 0x2e1b2138lu, 0x4d2c6dfclu, 0x53380d13lu, 0x650a7354lu, 0x766a0abblu, 0x81c2c92elu, 0x92722c85lu,
	0xa2bfe8a1lu, 0xa81a664blu, 0xc24b8b70lu, 0xc76c51a3lu, 0xd192e819lu, 0xd6990624lu, 0xf40e3585lu, 0x106aa070lu,
	0x19a4c116lu, 0x1e376c08lu, 0x2748774clu, 0x34b0bcb5lu, 0x391c0cb3lu, 0x4ed8aa4alu, 0x5b9cca4flu, 0x682e6ff3lu,
	0x748f82eelu, 0x78a5636flu, 0x84c87814lu, 0x8cc70208lu, 0x90befffalu, 0xa4506ceblu, 0xbef9a3f7lu, 0xc67178f2lu };

static void Sha256InternalConsumeChunk(uint32_t* h, const uint32_t* p)
{
	uint32_t ah[8];
	uint32_t w[16];
	for (int i = 0; i < 8; i++)
	{
		ah[i] = h[i];
	}
	for (int i = 0; i < 16; i++)
	{
		uint32_t wx = p[i];
		wx = (wx >> 24) | ((wx >> 8) & 0xff00) | ((wx << 8) & 0xff0000) | (wx << 24);
		uint32_t s1 = SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 6) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 11) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 25);
		uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
		uint32_t temp1 = ah[7] + s1 + ch + Sha256InternalConstantTableK[i] + wx;
		uint32_t s0 = SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 2) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 13) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 22);
		uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
		uint32_t temp2 = s0 + maj;
		w[i] = wx;
		ah[7] = ah[6];
		ah[6] = ah[5];
		ah[5] = ah[4];
		ah[4] = ah[3] + temp1;
		ah[3] = ah[2];
		ah[2] = ah[1];
		ah[1] = ah[0];
		ah[0] = temp1 + temp2;
	}
	for (int i = 0; i < 48; i++)
	{
		int j = i & 0xf;
		uint32_t s0 = SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 1) & 0xf], 7) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 1) & 0xf], 18) ^ (w[(j + 1) & 0xf] >> 3);
		uint32_t s1 = SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 14) & 0xf], 17) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 14) & 0xf], 19) ^ (w[(j + 14) & 0xf] >> 10);
		uint32_t wx = w[j] + s0 + w[(j + 9) & 0xf] + s1;
		uint32_t s3 = SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 6) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 11) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 25);
		uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
		uint32_t temp1 = ah[7] + s3 + ch + Sha256InternalConstantTableK[((i & 0x30) + 16) | j] + wx;
		uint32_t s2 = SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 2) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 13) ^ SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 22);
		uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
		uint32_t temp2 = s2 + maj;
		w[j] = wx;
		ah[7] = ah[6];
		ah[6] = ah[5];
		ah[5] = ah[4];
		ah[4] = ah[3] + temp1;
		ah[3] = ah[2];
		ah[2] = ah[1];
		ah[1] = ah[0];
		ah[0] = temp1 + temp2;
	}
	for (int i = 0; i < 8; i++)
	{
		h[i] += ah[i];
	}
}

void Sha256Initialize(Sha256Context* Context)
{
	Context->Size = 0;
	Context->Buffer[0] = 0x6a09e667lu;
	Context->Buffer[1] = 0xbb67ae85lu;
	Context->Buffer[2] = 0x3c6ef372lu;
	Context->Buffer[3] = 0xa54ff53alu;
	Context->Buffer[4] = 0x510e527flu;
	Context->Buffer[5] = 0x9b05688clu;
	Context->Buffer[6] = 0x1f83d9ablu;
	Context->Buffer[7] = 0x5be0cd19lu;
	memset(&Context->Input, 0, 64);
}

void Sha256Update(Sha256Context* Context, size_t InputSize, const void* InputData)
{
	const uint8_t* Input = (const uint8_t*)InputData;
	int InitialStepOffset = (int)(Context->Size & 0x3F);
	Context->Size += InputSize;
	int InitialStepInputSize = 64 - InitialStepOffset;
	if ((size_t)InitialStepInputSize > InputSize)
	{
		InitialStepInputSize = (int)InputSize;
	}
	memcpy(Context->Input + InitialStepOffset, InputData, InitialStepInputSize);
	if (InitialStepOffset + InitialStepInputSize < 64)
	{
		return;
	}
	Sha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
	Input += (size_t)InitialStepInputSize;
	InputSize -= (size_t)InitialStepInputSize;
	while (InputSize >= 64)
	{
		memcpy(Context->Input, Input, 64);
		Sha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
		Input += 64;
		InputSize -= 64;
	}
	memcpy(Context->Input, Input, InputSize);
}

void Sha256Finalize(Sha256Context* Context, void* Digest)
{
	size_t ChunkIndex = Context->Size & 0x3F;
	Context->Input[ChunkIndex] = 0x80;
	ChunkIndex++;
	if (ChunkIndex > 56)
	{
		memset(Context->Input + ChunkIndex, 0, 64 - ChunkIndex);
		Sha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
		ChunkIndex = 0;
	}
	memset(Context->Input + ChunkIndex, 0, 56 - ChunkIndex);
	uint64_t BitSize = Context->Size << 3;
	for (int i = 0; i < 8; i++)
	{
		Context->Input[56 + i] = (uint8_t)(BitSize >> ((7 - i) << 3));
	}
	Sha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
	for (int i = 0; i < 8; i++)
	{
		uint32_t swap = Context->Buffer[i];
		swap = (swap >> 24) | ((swap >> 8) & 0xff00) | ((swap << 8) & 0xff0000) | (swap << 24);
		Context->Buffer[i] = swap;
	}
	memcpy(Digest, Context->Buffer, SHA256_DIGEST_SIZE);
}

#ifdef __cplusplus
}
#endif // __cplusplus
