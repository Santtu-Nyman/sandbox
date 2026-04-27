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

#include "FlMd5.h"
#include <string.h>

#ifdef _MSC_VER
#define FL_MD5_ALIGN(N) __declspec(align(N))
#elif defined(__GNUC__) || defined(__clang__)
#define FL_MD5_ALIGN(N) __attribute__((aligned (N)))
#else
#define FL_MD5_ALIGN(N)
#endif

#define FL_MD5_INTERNAL_ROTATE_LEFT_32(X, N) (((X) << (N)) | ((X) >> (32 - (N))))

#define FL_MD5_INTERNAL_STEP_F(X, Y, Z) ((X & Y) | (~X & Z))
#define FL_MD5_INTERNAL_STEP_G(X, Y, Z) ((X & Z) | (Y & ~Z))
#define FL_MD5_INTERNAL_STEP_H(X, Y, Z) (X ^ Y ^ Z)
#define FL_MD5_INTERNAL_STEP_I(X, Y, Z) (Y ^ (X | ~Z))

FL_MD5_ALIGN(256) static uint32_t FlMd5InternalConstantTableK[64] = {
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

FL_MD5_ALIGN(64) static uint8_t FlMd5InternalConstantTableS[64] = {
	0x07, 0x0c, 0x11, 0x16, 0x07, 0x0c, 0x11, 0x16, 0x07, 0x0c, 0x11, 0x16, 0x07, 0x0c, 0x11, 0x16,
	0x05, 0x09, 0x0e, 0x14, 0x05, 0x09, 0x0e, 0x14, 0x05, 0x09, 0x0e, 0x14, 0x05, 0x09, 0x0e, 0x14,
	0x04, 0x0b, 0x10, 0x17, 0x04, 0x0b, 0x10, 0x17, 0x04, 0x0b, 0x10, 0x17, 0x04, 0x0b, 0x10, 0x17,
	0x06, 0x0a, 0x0f, 0x15, 0x06, 0x0a, 0x0f, 0x15, 0x06, 0x0a, 0x0f, 0x15, 0x06, 0x0a, 0x0f, 0x15 };

static void FlMd5InternalConsumeChunk(uint32_t* buffer, const uint32_t* input)
{
	uint32_t aa = buffer[0];
	uint32_t bb = buffer[1];
	uint32_t cc = buffer[2];
	uint32_t dd = buffer[3];
	for (int i = 0; i < 64; i++)
	{
		uint32_t e;
		int j;
		switch (i >> 4)
		{
			case 0:
				e = FL_MD5_INTERNAL_STEP_F(bb, cc, dd);
				j = i;
				break;
			case 1:
				e = FL_MD5_INTERNAL_STEP_G(bb, cc, dd);
				j = ((i * 5) + 1) & 0xF;
				break;
			case 2:
				e = FL_MD5_INTERNAL_STEP_H(bb, cc, dd);
				j = ((i * 3) + 5) & 0xF;
				break;
			default:
				e = FL_MD5_INTERNAL_STEP_I(bb, cc, dd);
				j = (i * 7) & 0xF;
				break;
		}
		uint32_t t0 = dd;
		uint32_t t1 = aa + e + FlMd5InternalConstantTableK[i] + input[j];
		int shift = (int)FlMd5InternalConstantTableS[i];
		dd = cc;
		cc = bb;
		bb = bb + FL_MD5_INTERNAL_ROTATE_LEFT_32(t1, shift);
		aa = t0;
	}
	buffer[0] += aa;
	buffer[1] += bb;
	buffer[2] += cc;
	buffer[3] += dd;
}

void FlMd5CreateHash(_Out_ FlMd5Context* context)
{
	context->size = 0;
	context->buffer[0] = 0x67452301lu;
	context->buffer[1] = 0xefcdab89lu;
	context->buffer[2] = 0x98badcfelu;
	context->buffer[3] = 0x10325476lu;
	memset(context->input, 0, 64);
}

void FlMd5HashData(_Inout_ FlMd5Context* context, _In_ size_t inputSize, _In_reads_bytes_(inputSize) const void* inputData)
{
	const uint8_t* input = (const uint8_t*)inputData;
	int initialStepOffset = (int)(context->size & 0x3F);
	context->size += (uint64_t)inputSize;
	int initialStepInputSize = 64 - initialStepOffset;
	if ((size_t)initialStepInputSize > inputSize)
	{
		initialStepInputSize = (int)inputSize;
	}
	memcpy(context->input + initialStepOffset, input, initialStepInputSize);
	if (initialStepOffset + initialStepInputSize < 64)
	{
		return;
	}
	FlMd5InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
	input += (size_t)initialStepInputSize;
	inputSize -= (size_t)initialStepInputSize;
	while (inputSize >= 64)
	{
		memcpy(context->input, input, 64);
		FlMd5InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
		input += 64;
		inputSize -= 64;
	}
	memcpy(context->input, input, inputSize);
}

void FlMd5FinishHash(_Inout_ FlMd5Context* context, _Out_writes_bytes_all_(FL_MD5_DIGEST_SIZE) void* digest)
{
	uint8_t padding[64];
	padding[0] = 0x80;
	memset(&padding[1], 0, 63);
	int offset = (int)(context->size & 0x3F);
	int paddingLength = (offset < 56) ? (56 - offset) : ((56 + 64) - offset);
	FlMd5HashData(context, (size_t)paddingLength, padding);
	context->size -= (uint64_t)paddingLength;
	uint64_t sizeInBits = context->size << 3;
	*(uint64_t*)&context->input[56] = sizeInBits;
	FlMd5InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
	memcpy(digest, context->buffer, FL_MD5_DIGEST_SIZE);
}

#ifdef __cplusplus
}
#endif // __cplusplus
