/*
	Santtu S. Nyman's 2024 version of Bryce Wilson's public domain MD5 implementation ( https://github.com/Zunawe/md5-c Tuesday, 5 March 2024 ).
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

#include "md5.h"
#include <string.h>

#define MD5_INTERNAL_ROTATE_LEFT_32(X, N) ((X << N) | (X >> (32 - N)))

#define MD5_INTERNAL_STEP_F(X, Y, Z) ((X & Y) | (~X & Z))
#define MD5_INTERNAL_STEP_G(X, Y, Z) ((X & Z) | (Y & ~Z))
#define MD5_INTERNAL_STEP_H(X, Y, Z) (X ^ Y ^ Z)
#define MD5_INTERNAL_STEP_I(X, Y, Z) (Y ^ (X | ~Z))

static uint8_t Md5InternalConstantTableS[64] = {
	0x07, 0x0c, 0x11, 0x16, 0x07, 0x0c, 0x11, 0x16, 0x07, 0x0c, 0x11, 0x16, 0x07, 0x0c, 0x11, 0x16,
	0x05, 0x09, 0x0e, 0x14, 0x05, 0x09, 0x0e, 0x14, 0x05, 0x09, 0x0e, 0x14, 0x05, 0x09, 0x0e, 0x14,
	0x04, 0x0b, 0x10, 0x17, 0x04, 0x0b, 0x10, 0x17, 0x04, 0x0b, 0x10, 0x17, 0x04, 0x0b, 0x10, 0x17,
	0x06, 0x0a, 0x0f, 0x15, 0x06, 0x0a, 0x0f, 0x15, 0x06, 0x0a, 0x0f, 0x15, 0x06, 0x0a, 0x0f, 0x15 };

static uint32_t Md5InternalConstantTableK[64] = {
	0xd76aa478lu, 0xe8c7b756lu, 0x242070dblu, 0xc1bdceeelu,
	0xf57c0faflu, 0x4787c62alu, 0xa8304613lu, 0xfd469501lu,
	0x698098d8lu, 0x8b44f7aflu, 0xffff5bb1lu, 0x895cd7belu,
	0x6b901122lu, 0xfd987193lu, 0xa679438elu, 0x49b40821lu,
	0xf61e2562lu, 0xc040b340lu, 0x265e5a51lu, 0xe9b6c7aalu,
	0xd62f105dlu, 0x02441453lu, 0xd8a1e681lu, 0xe7d3fbc8lu,
	0x21e1cde6lu, 0xc33707d6lu, 0xf4d50d87lu, 0x455a14edlu,
	0xa9e3e905lu, 0xfcefa3f8lu, 0x676f02d9lu, 0x8d2a4c8alu,
	0xfffa3942lu, 0x8771f681lu, 0x6d9d6122lu, 0xfde5380clu,
	0xa4beea44lu, 0x4bdecfa9lu, 0xf6bb4b60lu, 0xbebfbc70lu,
	0x289b7ec6lu, 0xeaa127falu, 0xd4ef3085lu, 0x04881d05lu,
	0xd9d4d039lu, 0xe6db99e5lu, 0x1fa27cf8lu, 0xc4ac5665lu,
	0xf4292244lu, 0x432aff97lu, 0xab9423a7lu, 0xfc93a039lu,
	0x655b59c3lu, 0x8f0ccc92lu, 0xffeff47dlu, 0x85845dd1lu,
	0x6fa87e4flu, 0xfe2ce6e0lu, 0xa3014314lu, 0x4e0811a1lu,
	0xf7537e82lu, 0xbd3af235lu, 0x2ad7d2bblu, 0xeb86d391lu };

static void Md5InternalConsumeChunk(uint32_t* Buffer, const uint32_t* Input)
{
	uint32_t AA = Buffer[0];
	uint32_t BB = Buffer[1];
	uint32_t CC = Buffer[2];
	uint32_t DD = Buffer[3];
	for (int i = 0; i < 64; i++)
	{
		uint32_t E;
		int j;
		switch (i >> 4)
		{
			case 0:
				E = MD5_INTERNAL_STEP_F(BB, CC, DD);
				j = i;
				break;
			case 1:
				E = MD5_INTERNAL_STEP_G(BB, CC, DD);
				j = ((i * 5) + 1) & 0xF;
				break;
			case 2:
				E = MD5_INTERNAL_STEP_H(BB, CC, DD);
				j = ((i * 3) + 5) & 0xF;
				break;
			default:
				E = MD5_INTERNAL_STEP_I(BB, CC, DD);
				j = (i * 7) & 0xF;
				break;
		}
		uint32_t T0 = DD;
		uint32_t T1 = AA + E + Md5InternalConstantTableK[i] + Input[j];
		int Shift = (int)Md5InternalConstantTableS[i];
		DD = CC;
		CC = BB;
		BB = BB + MD5_INTERNAL_ROTATE_LEFT_32(T1, Shift);
		AA = T0;
	}
	Buffer[0] += AA;
	Buffer[1] += BB;
	Buffer[2] += CC;
	Buffer[3] += DD;
}

void Md5Initialize(Md5Context* Context)
{
	Context->Size = 0;
	Context->Buffer[0] = 0x67452301lu;
	Context->Buffer[1] = 0xefcdab89lu;
	Context->Buffer[2] = 0x98badcfelu;
	Context->Buffer[3] = 0x10325476lu;
	memset(Context->Input, 0, 64);
}

void Md5Update(Md5Context* Context, size_t InputSize, const void* InputData)
{
	const uint8_t* Input = (const uint8_t*)InputData;
	int InitialStepOffset = (int)(Context->Size & 0x3F);
	Context->Size += (uint64_t)InputSize;
	int InitialStepInputSize = 64 - InitialStepOffset;
	if ((size_t)InitialStepInputSize > InputSize)
	{
		InitialStepInputSize = (int)InputSize;
	}
	memcpy(Context->Input + InitialStepOffset, Input, InitialStepInputSize);
	if (InitialStepOffset + InitialStepInputSize < 64)
	{
		return;
	}
	Md5InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
	Input += (size_t)InitialStepInputSize;
	InputSize -= (size_t)InitialStepInputSize;
	while (InputSize >= 64)
	{
		memcpy(Context->Input, Input, 64);
		Md5InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
		Input += 64;
		InputSize -= 64;
	}
	memcpy(Context->Input, Input, InputSize);
}

void Md5Finalize(Md5Context* Context, void* Digest)
{
	uint8_t Padding[64];
	Padding[0] = 0x80;
	memset(&Padding[1], 0, 63);
	int Offset = (int)(Context->Size & 0x3F);
	int PaddingLength = (Offset < 56) ? (56 - Offset) : ((56 + 64) - Offset);
	Md5Update(Context, (size_t)PaddingLength, Padding);
	Context->Size -= (uint64_t)PaddingLength;
	uint64_t SizeInBits = Context->Size << 3;
	*(uint64_t*)&Context->Input[56] = SizeInBits;
	Md5InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
	memcpy(Digest, Context->Buffer, MD5_DIGEST_SIZE);
}

#ifdef __cplusplus
}
#endif // __cplusplus
