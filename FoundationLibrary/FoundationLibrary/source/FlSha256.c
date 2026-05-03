/*
	Santtu S. Nyman's version of Alain Mosnier's public domain SHA-256 implementation ( https://github.com/amosnier/sha-2 Sunday, 10 March 2024 ).
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

#include "FlSha256.h"
#include <string.h>

#if defined(_MSC_VER)
#include <intrin.h>
#define FL_SHA256_BYTE_SWAP_16(X) _byteswap_ushort((X))
#define FL_SHA256_BYTE_SWAP_32(X) _byteswap_ulong((X))
#define FL_SHA256_BYTE_SWAP_64(X) _byteswap_uint64((X))
#elif defined(__GNUC__) || defined(__clang__)
#define FL_SHA256_BYTE_SWAP_16(X) __builtin_bswap16((X))
#define FL_SHA256_BYTE_SWAP_32(X) __builtin_bswap32((X))
#define FL_SHA256_BYTE_SWAP_64(X) __builtin_bswap64((X))
#else
static inline uint16_t FlSha256InlineByteSwap16(uint16_t x)
{
	return (x >> 8) | (x << 8);
}
static inline uint32_t FlSha256InlineByteSwap32(uint32_t x)
{
	return
		((x & 0xff000000ul) >> 24) |
		((x & 0x00ff0000ul) >> 8) |
		((x & 0x0000ff00ul) << 8) |
		((x & 0x000000fful) << 24);
}
static inline uint64_t FlSha256InlineByteSwap64(uint64_t x)
{
	return
		((x & 0xff00000000000000ull) >> 56) |
		((x & 0x00ff000000000000ull) >> 40) |
		((x & 0x0000ff0000000000ull) >> 24) |
		((x & 0x000000ff00000000ull) >> 8) |
		((x & 0x00000000ff000000ull) << 8) |
		((x & 0x0000000000ff0000ull) << 24) |
		((x & 0x000000000000ff00ull) << 40) |
		((x & 0x00000000000000ffull) << 56);
}
#define FL_SHA256_BYTE_SWAP_16(X) FlSha256InlineByteSwap16((X))
#define FL_SHA256_BYTE_SWAP_32(X) FlSha256InlineByteSwap32((X))
#define FL_SHA256_BYTE_SWAP_64(X) FlSha256InlineByteSwap64((X))
#endif

#ifdef _MSC_VER
#define FL_SHA256_ALIGN(N) __declspec(align(N))
#elif defined(__GNUC__) || defined(__clang__)
#define FL_SHA256_ALIGN(N) __attribute__((aligned (N)))
#else
#define FL_SHA256_ALIGN(N)
#endif

#define FL_SHA256_INTERNAL_ROTATE_RIGHT_32(X, N) (((X) >> (N)) | ((X) << (32 - (N))))

FL_SHA256_ALIGN(256) static const uint32_t FlSha256InternalConstantTableK[64] = {
	0x428a2f98lu, 0x71374491lu, 0xb5c0fbcflu, 0xe9b5dba5lu, 0x3956c25blu, 0x59f111f1lu, 0x923f82a4lu, 0xab1c5ed5lu,
	0xd807aa98lu, 0x12835b01lu, 0x243185belu, 0x550c7dc3lu, 0x72be5d74lu, 0x80deb1felu, 0x9bdc06a7lu, 0xc19bf174lu,
	0xe49b69c1lu, 0xefbe4786lu, 0x0fc19dc6lu, 0x240ca1cclu, 0x2de92c6flu, 0x4a7484aalu, 0x5cb0a9dclu, 0x76f988dalu,
	0x983e5152lu, 0xa831c66dlu, 0xb00327c8lu, 0xbf597fc7lu, 0xc6e00bf3lu, 0xd5a79147lu, 0x06ca6351lu, 0x14292967lu,
	0x27b70a85lu, 0x2e1b2138lu, 0x4d2c6dfclu, 0x53380d13lu, 0x650a7354lu, 0x766a0abblu, 0x81c2c92elu, 0x92722c85lu,
	0xa2bfe8a1lu, 0xa81a664blu, 0xc24b8b70lu, 0xc76c51a3lu, 0xd192e819lu, 0xd6990624lu, 0xf40e3585lu, 0x106aa070lu,
	0x19a4c116lu, 0x1e376c08lu, 0x2748774clu, 0x34b0bcb5lu, 0x391c0cb3lu, 0x4ed8aa4alu, 0x5b9cca4flu, 0x682e6ff3lu,
	0x748f82eelu, 0x78a5636flu, 0x84c87814lu, 0x8cc70208lu, 0x90befffalu, 0xa4506ceblu, 0xbef9a3f7lu, 0xc67178f2lu };

static void FlSha256InternalConsumeChunk(_Inout_updates_(8) uint32_t* hashState, _In_reads_bytes_(16) const uint32_t* chunkData)
{
	uint32_t workingVariables[8];
	uint32_t messageSchedule[16];
	for (int i = 0; i < 8; i++)
	{
		workingVariables[i] = hashState[i];
	}
	for (int i = 0; i < 16; i++)
	{
		uint32_t inputWord = FL_SHA256_BYTE_SWAP_32(chunkData[i]);
		uint32_t roundSigma1 = FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[4], 6) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[4], 11) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[4], 25);
		uint32_t chooseValue = (workingVariables[4] & workingVariables[5]) ^ (~workingVariables[4] & workingVariables[6]);
		uint32_t temp1 = workingVariables[7] + roundSigma1 + chooseValue + FlSha256InternalConstantTableK[i] + inputWord;
		uint32_t roundSigma0 = FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[0], 2) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[0], 13) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[0], 22);
		uint32_t maj = (workingVariables[0] & workingVariables[1]) ^ (workingVariables[0] & workingVariables[2]) ^ (workingVariables[1] & workingVariables[2]);
		uint32_t temp2 = roundSigma0 + maj;
		messageSchedule[i] = inputWord;
		workingVariables[7] = workingVariables[6];
		workingVariables[6] = workingVariables[5];
		workingVariables[5] = workingVariables[4];
		workingVariables[4] = workingVariables[3] + temp1;
		workingVariables[3] = workingVariables[2];
		workingVariables[2] = workingVariables[1];
		workingVariables[1] = workingVariables[0];
		workingVariables[0] = temp1 + temp2;
	}
	for (int i = 0; i < 48; i++)
	{
		int j = i & 0xf;
		uint32_t expansionSigma0 = FL_SHA256_INTERNAL_ROTATE_RIGHT_32(messageSchedule[(j + 1) & 0xf], 7) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(messageSchedule[(j + 1) & 0xf], 18) ^ (messageSchedule[(j + 1) & 0xf] >> 3);
		uint32_t expansionSigma1 = FL_SHA256_INTERNAL_ROTATE_RIGHT_32(messageSchedule[(j + 14) & 0xf], 17) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(messageSchedule[(j + 14) & 0xf], 19) ^ (messageSchedule[(j + 14) & 0xf] >> 10);
		uint32_t scheduleWord = messageSchedule[j] + expansionSigma0 + messageSchedule[(j + 9) & 0xf] + expansionSigma1;
		uint32_t roundSigma1 = FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[4], 6) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[4], 11) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[4], 25);
		uint32_t chooseValue = (workingVariables[4] & workingVariables[5]) ^ (~workingVariables[4] & workingVariables[6]);
		uint32_t temp1 = workingVariables[7] + roundSigma1 + chooseValue + FlSha256InternalConstantTableK[((i & 0x30) + 16) | j] + scheduleWord;
		uint32_t roundSigma0 = FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[0], 2) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[0], 13) ^ FL_SHA256_INTERNAL_ROTATE_RIGHT_32(workingVariables[0], 22);
		uint32_t maj = (workingVariables[0] & workingVariables[1]) ^ (workingVariables[0] & workingVariables[2]) ^ (workingVariables[1] & workingVariables[2]);
		uint32_t temp2 = roundSigma0 + maj;
		messageSchedule[j] = scheduleWord;
		workingVariables[7] = workingVariables[6];
		workingVariables[6] = workingVariables[5];
		workingVariables[5] = workingVariables[4];
		workingVariables[4] = workingVariables[3] + temp1;
		workingVariables[3] = workingVariables[2];
		workingVariables[2] = workingVariables[1];
		workingVariables[1] = workingVariables[0];
		workingVariables[0] = temp1 + temp2;
	}
	for (int i = 0; i < 8; i++)
	{
		hashState[i] += workingVariables[i];
	}
}

void FlSha256CreateHash(_Out_ FlSha256Context* context)
{
	context->size = 0;
	context->buffer[0] = 0x6a09e667lu;
	context->buffer[1] = 0xbb67ae85lu;
	context->buffer[2] = 0x3c6ef372lu;
	context->buffer[3] = 0xa54ff53alu;
	context->buffer[4] = 0x510e527flu;
	context->buffer[5] = 0x9b05688clu;
	context->buffer[6] = 0x1f83d9ablu;
	context->buffer[7] = 0x5be0cd19lu;
	memset(&context->input, 0, 64);
}

void FlSha256HashData(_Inout_ FlSha256Context* context, _In_ size_t inputSize, _In_reads_bytes_(inputSize) const void* inputData)
{
	const uint8_t* input = (const uint8_t*)inputData;
	int initialStepOffset = (int)(context->size & 0x3F);
	context->size += inputSize;
	int initialStepInputSize = 64 - initialStepOffset;
	if ((size_t)initialStepInputSize > inputSize)
	{
		initialStepInputSize = (int)inputSize;
	}
	memcpy(context->input + initialStepOffset, inputData, initialStepInputSize);
	if (initialStepOffset + initialStepInputSize < 64)
	{
		return;
	}
	FlSha256InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
	input += (size_t)initialStepInputSize;
	inputSize -= (size_t)initialStepInputSize;
	while (inputSize >= 64)
	{
		memcpy(context->input, input, 64);
		FlSha256InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
		input += 64;
		inputSize -= 64;
	}
	memcpy(context->input, input, inputSize);
}

void FlSha256FinishHash(_Inout_ FlSha256Context* context, _Out_writes_bytes_all_(FL_SHA256_DIGEST_SIZE) void* digest)
{
	size_t chunkIndex = context->size & 0x3F;
	context->input[chunkIndex] = 0x80;
	chunkIndex++;
	if (chunkIndex > 56)
	{
		memset(context->input + chunkIndex, 0, 64 - chunkIndex);
		FlSha256InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
		chunkIndex = 0;
	}
	memset(context->input + chunkIndex, 0, 56 - chunkIndex);
	uint64_t bitSize = context->size << 3;
	*((uint64_t*)&context->input[56]) = FL_SHA256_BYTE_SWAP_64(bitSize);
	FlSha256InternalConsumeChunk(context->buffer, (const uint32_t*)&context->input);
	for (int i = 0; i < 8; i++)
	{
		context->buffer[i] = FL_SHA256_BYTE_SWAP_32(context->buffer[i]);
	}
	memcpy(digest, context->buffer, FL_SHA256_DIGEST_SIZE);
}

#ifdef __cplusplus
}
#endif // __cplusplus
