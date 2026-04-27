/*
	Santtu S. Nyman's 2024 public domain AES256 implementation.

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

#include "FlAes256.h"
#include <stdint.h>
#include <assert.h>

#if defined(_MSC_VER)
#include <intrin.h>
#define FL_AES256_BYTE_SWAP_16(X) _byteswap_ushort((X))
#define FL_AES256_BYTE_SWAP_32(X) _byteswap_ulong((X))
#define FL_AES256_BYTE_SWAP_64(X) _byteswap_uint64((X))
#elif defined(__GNUC__) || defined(__clang__)
#define FL_AES256_BYTE_SWAP_16(X) __builtin_bswap16((X))
#define FL_AES256_BYTE_SWAP_32(X) __builtin_bswap32((X))
#define FL_AES256_BYTE_SWAP_64(X) __builtin_bswap64((X))
#else
static inline uint16_t FlAesInlineByteSwap16(uint16_t x)
{
	return (x >> 8) | (x << 8);
}
static inline uint32_t FlAesInlineByteSwap32(uint32_t x)
{
	return
		((x & 0xff000000ul) >> 24) |
		((x & 0x00ff0000ul) >>  8) |
		((x & 0x0000ff00ul) <<  8) |
		((x & 0x000000fful) << 24);
}
static inline uint64_t FlAesInlineByteSwap64(uint64_t x)
{
	return
		((x & 0xff00000000000000ull) >> 56) |
		((x & 0x00ff000000000000ull) >> 40) |
		((x & 0x0000ff0000000000ull) >> 24) |
		((x & 0x000000ff00000000ull) >>  8) |
		((x & 0x00000000ff000000ull) <<  8) |
		((x & 0x0000000000ff0000ull) << 24) |
		((x & 0x000000000000ff00ull) << 40) |
		((x & 0x00000000000000ffull) << 56);
}
#define FL_AES256_BYTE_SWAP_16(X) FlAesInlineByteSwap16((X))
#define FL_AES256_BYTE_SWAP_32(X) FlAesInlineByteSwap32((X))
#define FL_AES256_BYTE_SWAP_64(X) FlAesInlineByteSwap64((X))
#endif

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <immintrin.h>
#endif
#define FL_AES256_ADD_CARRY_64(I,A,B,O) _addcarry_u64((I), (A), (B), (O))
#else
#if defined(__GNUC__) || defined(__clang__)
__attribute__((always_inline)) unsigned char FLAes256AddCarry64(unsigned char carry, uint64_t a, uint64_t b, uint64_t* out)
{
	if (carry > 1)
	{
		__builtin_unreachable();
	}
	unsigned long long int o;
	uint64_t c = __builtin_addcll(a, b, carry, &o);
	*out = c;
	return (unsigned char)o;
}
#define FL_AES256_ADD_CARRY_64(I,A,B,O) FLAes256AddCarry64((I), (A), (B), (O))
#else
inline unsigned char FLAes256AddCarry64(unsigned char carry, uint64_t a, uint64_t b, uint64_t* out)
{
	uint64_t c = a + b + carry;
	carry = (((a | b) & ~c) | (a & b)) >> 63;
	*out = c;
	return carry;
}
#define FL_AES256_ADD_CARRY_64(I,A,B,O) FLAes256AddCarry64((I), (A), (B), (O))
#endif
#endif

#ifdef _MSC_VER
#define FL_AES256_ALIGN(N) __declspec(align(N))
#elif defined(__GNUC__) || defined(__clang__)
#define FL_AES256_ALIGN(N) __attribute__((aligned (N)))
#else
#define FL_AES256_ALIGN(N)
#endif

FL_AES256_ALIGN(256) static const uint8_t FlAes256SBoxTable[256] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
	0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
	0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
	0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
	0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
	0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
	0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
	0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
	0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
	0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
	0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
	0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
	0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
	0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
	0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
	0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16 };

FL_AES256_ALIGN(256) static const uint8_t FlAes256IBoxTable[256] = {
	0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
	0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
	0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
	0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
	0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
	0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
	0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
	0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
	0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
	0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
	0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
	0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
	0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
	0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
	0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D };

FL_AES256_ALIGN(8) static const uint8_t FlAes256RTable[8] = { 0x8D, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40 };

void FlAes256KeyExpansion(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _Out_writes_bytes_all_(FL_AES256_ROUND_KEY_SIZE) void* roundKey)
{
	uint8_t* keyBuffer = (uint8_t*)key;
	uint8_t* roundKeyBuffer = (uint8_t*)roundKey;
	for (int i = 0; i < 32; i++)
	{
		roundKeyBuffer[i] = keyBuffer[i];
	}
	for (int i = 32; i < FL_AES256_ROUND_KEY_SIZE; i += 4)
	{
		uint8_t temporal[4] = {
			roundKeyBuffer[i - 4],
			roundKeyBuffer[i - 3],
			roundKeyBuffer[i - 2],
			roundKeyBuffer[i - 1] };
		if (!(i & 0x1F))
		{
			uint8_t temporalMove = temporal[0];
			temporal[0] = temporal[1];
			temporal[1] = temporal[2];
			temporal[2] = temporal[3];
			temporal[3] = temporalMove;
			temporal[0] = FlAes256SBoxTable[temporal[0]];
			temporal[1] = FlAes256SBoxTable[temporal[1]];
			temporal[2] = FlAes256SBoxTable[temporal[2]];
			temporal[3] = FlAes256SBoxTable[temporal[3]];
			temporal[0] = temporal[0] ^ FlAes256RTable[i >> 5];
		}
		if ((i & 0x1F) == 0x10)
		{
			temporal[0] = FlAes256SBoxTable[temporal[0]];
			temporal[1] = FlAes256SBoxTable[temporal[1]];
			temporal[2] = FlAes256SBoxTable[temporal[2]];
			temporal[3] = FlAes256SBoxTable[temporal[3]];
		}
		int seekBackIndex = i - 32;
		roundKeyBuffer[i + 0] = roundKeyBuffer[seekBackIndex + 0] ^ temporal[0];
		roundKeyBuffer[i + 1] = roundKeyBuffer[seekBackIndex + 1] ^ temporal[1];
		roundKeyBuffer[i + 2] = roundKeyBuffer[seekBackIndex + 2] ^ temporal[2];
		roundKeyBuffer[i + 3] = roundKeyBuffer[seekBackIndex + 3] ^ temporal[3];
	}
}

static void FlAesAddRoundKey(const uint8_t* roundKey, int round, uint8_t* state)
{
#if defined(_MSC_VER)
	__assume(round >= 0 && round <= 14);
#elif defined(__GNUC__) || defined(__clang__)
	if (round < 0 || round > 14)
	{
		__builtin_unreachable();
	}
#else
	assert(round >= 0 && round <= 14);
#endif
	roundKey += (round << 4);
	for (int i = 0; i < 16; i++)
	{
		state[i] ^= roundKey[i];
	}
}

static void FlAesSubstituteBytes(uint8_t* state)
{
	for (int i = 0; i < 16; i++)
	{
		state[i] = FlAes256SBoxTable[state[i]];
	}
}

static void FlAesShiftRows(uint8_t* state)
{
	uint8_t state1 = state[1];
	state[1] = state[5];
	state[5] = state[9];
	state[9] = state[13];
	state[13] = state1;
	uint8_t state2 = state[2];
	state[2] = state[10];
	state[10] = state2;
	uint8_t state6 = state[6];
	state[6] = state[14];
	state[14] = state6;
	uint8_t state3 = state[3];
	state[3] = state[15];
	state[15] = state[11];
	state[11] = state[7];
	state[7] = state3;
}

static void FlAesMixColumns(uint8_t* state)
{
	for (int i = 0; i < 16; i += 4)
	{
		uint8_t state0 = state[i];
		uint8_t temporal1 = state0 ^ state[i + 1];
		uint8_t temporal0 = temporal1 ^ state[i + 2] ^ state[i + 3];
		state[i + 0] ^= (temporal1 << 1) ^ ((0 - (temporal1 >> 7)) & 0x1B) ^ temporal0;
		uint8_t temporal2 = state[i + 1] ^ state[i + 2];
		state[i + 1] ^= (temporal2 << 1) ^ ((0 - (temporal2 >> 7)) & 0x1B) ^ temporal0;
		uint8_t temporal3 = state[i + 2] ^ state[i + 3];
		state[i + 2] ^= (temporal3 << 1) ^ ((0 - (temporal3 >> 7)) & 0x1B) ^ temporal0;
		uint8_t temporal4 = state[i + 3] ^ state0;
		state[i + 3] ^= (temporal4 << 1) ^ ((0 - (temporal4 >> 7)) & 0x1B) ^ temporal0;
	}
}

void FlAes256Encrypt(_In_reads_bytes_(FL_AES256_ROUND_KEY_SIZE) const void* roundKey, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* plainText, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) void* cipherText)
{
	FL_AES256_ALIGN(16) uint8_t state[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		state[i] = ((const uint8_t*)plainText)[i];
	}

	FlAesAddRoundKey((const uint8_t*)roundKey, 0, (uint8_t*)(&state[0]));
	for (int round = 1; round < 14; round++)
	{
		FlAesSubstituteBytes((uint8_t*)(&state[0]));
		FlAesShiftRows((uint8_t*)(&state[0]));
		FlAesMixColumns((uint8_t*)(&state[0]));
		FlAesAddRoundKey((const uint8_t*)roundKey, round, (uint8_t*)(&state[0]));
	}
	FlAesSubstituteBytes((uint8_t*)(&state[0]));
	FlAesShiftRows((uint8_t*)(&state[0]));
	FlAesAddRoundKey((const uint8_t*)roundKey, 14, (uint8_t*)(&state[0]));

	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		((uint8_t*)cipherText)[i] = state[i];
	}
}

static void FlAesInverseShiftRows(uint8_t* state)
{
	uint8_t state13 = state[13];
	state[13] = state[9];
	state[9] = state[5];
	state[5] = state[1];
	state[1] = state13;
	uint8_t state2 = state[2];
	state[2] = state[10];
	state[10] = state2;
	uint8_t state6 = state[6];
	state[6] = state[14];
	state[14] = state6;
	uint8_t state3 = state[3];
	state[3] = state[7];
	state[7] = state[11];
	state[11] = state[15];
	state[15] = state3;
}

static void FlAesInverseSubstituteBytes(uint8_t* state)
{
	for (int i = 0; i < 16; i++)
	{
		state[i] = FlAes256IBoxTable[state[i]];
	}
}

static void FlAesInverseMixColumns(uint8_t* state)
{
	for (int i = 0; i < 16; i += 4)
	{
		uint8_t x = state[i + 0];
		uint8_t y = state[i + 1];
		uint8_t z = state[i + 2];
		uint8_t w = state[i + 3];
		uint8_t x2 = ((x << 1) ^ (((uint8_t)(0) - (x >> 7)) & (uint8_t)(0x1B)));
		uint8_t x3 = ((x2 << 1) ^ (((uint8_t)(0) - (x2 >> 7)) & (uint8_t)(0x1B)));
		uint8_t x4 = ((x3 << 1) ^ (((uint8_t)(0) - (x3 >> 7)) & (uint8_t)(0x1B)));
		uint8_t y2 = ((y << 1) ^ (((uint8_t)(0) - (y >> 7)) & (uint8_t)(0x1B)));
		uint8_t y3 = ((y2 << 1) ^ (((uint8_t)(0) - (y2 >> 7)) & (uint8_t)(0x1B)));
		uint8_t y4 = ((y3 << 1) ^ (((uint8_t)(0) - (y3 >> 7)) & (uint8_t)(0x1B)));
		uint8_t z2 = ((z << 1) ^ (((uint8_t)(0) - (z >> 7)) & (uint8_t)(0x1B)));
		uint8_t z3 = ((z2 << 1) ^ (((uint8_t)(0) - (z2 >> 7)) & (uint8_t)(0x1B)));
		uint8_t z4 = ((z3 << 1) ^ (((uint8_t)(0) - (z3 >> 7)) & (uint8_t)(0x1B)));
		uint8_t w2 = ((w << 1) ^ (((uint8_t)(0) - (w >> 7)) & (uint8_t)(0x1B)));
		uint8_t w3 = ((w2 << 1) ^ (((uint8_t)(0) - (w2 >> 7)) & (uint8_t)(0x1B)));
		uint8_t w4 = ((w3 << 1) ^ (((uint8_t)(0) - (w3 >> 7)) & (uint8_t)(0x1B)));
		state[i + 0] = x2 ^ x3 ^ x4 ^ y ^ y2 ^ y4 ^ z ^ z3 ^ z4 ^ w ^ w4;
		state[i + 1] = x ^ x4 ^ y2 ^ y3 ^ y4 ^ z ^ z2 ^ z4 ^ w ^ w3 ^ w4;
		state[i + 2] = x ^ x3 ^ x4 ^ y ^ y4 ^ z2 ^ z3 ^ z4 ^ w ^ w2 ^ w4;
		state[i + 3] = x ^ x2 ^ x4 ^ y ^ y3 ^ y4 ^ z ^ z4 ^ w2 ^ w3 ^ w4;
	}
}

void FlAes256Decrypt(_In_reads_bytes_(FL_AES256_ROUND_KEY_SIZE) const void* roundKey, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* cipherText, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) void* plainText)
{
	FL_AES256_ALIGN(16) uint8_t state[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		state[i] = ((const uint8_t*)cipherText)[i];
	}

	FlAesAddRoundKey((const uint8_t*)roundKey, 14, (uint8_t*)(&state[0]));
	for (int round = 13; round > 0; round--)
	{
		FlAesInverseShiftRows((uint8_t*)(&state[0]));
		FlAesInverseSubstituteBytes((uint8_t*)(&state[0]));
		FlAesAddRoundKey((const uint8_t*)roundKey, round, (uint8_t*)(&state[0]));
		FlAesInverseMixColumns((&state[0]));
	}
	FlAesInverseShiftRows((uint8_t*)(&state[0]));
	FlAesInverseSubstituteBytes((uint8_t*)(&state[0]));
	FlAesAddRoundKey((const uint8_t*)roundKey, 0, (uint8_t*)(&state[0]));

	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		((uint8_t*)plainText)[i] = state[i];
	}
}

void FlAes256EcbEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText)
{
	const uint8_t* plainTextBuffer = (const uint8_t*)plainText;
	uint8_t* cipherTextBuffer = (uint8_t*)cipherText;
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	FlAes256KeyExpansion(key, &roundKey[0]);
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		FlAes256Encrypt(&roundKey[0], plainTextBuffer + offset, cipherTextBuffer + offset);
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		uint8_t block[FL_AES256_BLOCK_SIZE];
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = 0;
		}
		for (int i = 0; i < remaining; i++)
		{
			block[i] = plainTextBuffer[offset + i];
		}
		FlAes256Encrypt(&roundKey[0], &block[0], &block[0]);
		for (int i = 0; i < remaining; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
	}
}

void FlAes256EcbDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* cipherText, _Out_writes_bytes_all_(textSize) void* plainText)
{
	const uint8_t* cipherTextBuffer = (uint8_t*)cipherText;
	uint8_t* plainTextBuffer = (uint8_t*)plainText;
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	FlAes256KeyExpansion(key, &roundKey[0]);
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		FlAes256Decrypt(&roundKey[0], cipherTextBuffer + offset, plainTextBuffer + offset);
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		uint8_t block[FL_AES256_BLOCK_SIZE];
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = 0;
		}
		for (int i = 0; i < remaining; i++)
		{
			block[i] = cipherTextBuffer[offset + i];
		}
		FlAes256Decrypt(&roundKey[0], &block[0], &block[0]);
		for (int i = 0; i < remaining; i++)
		{
			plainTextBuffer[offset + i] = block[i];
		}
	}
}

void FlAes256CtrEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* iv, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText)
{
	uint8_t* cipherTextBuffer = (uint8_t*)cipherText;
	const uint8_t* plainTextBuffer = (const uint8_t*)plainText;
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	uint64_t counterBlock[2];
	uint8_t keystreamBlock[FL_AES256_BLOCK_SIZE];
	uint8_t block[FL_AES256_BLOCK_SIZE];
	counterBlock[0] =
		((uint64_t)((const uint8_t*)iv)[0]) |
		((uint64_t)((const uint8_t*)iv)[1] << 8) |
		((uint64_t)((const uint8_t*)iv)[2] << 16) |
		((uint64_t)((const uint8_t*)iv)[3] << 24) |
		((uint64_t)((const uint8_t*)iv)[4] << 32) |
		((uint64_t)((const uint8_t*)iv)[5] << 40) |
		((uint64_t)((const uint8_t*)iv)[6] << 48) |
		((uint64_t)((const uint8_t*)iv)[7] << 56);
	counterBlock[1] =
		((uint64_t)((const uint8_t*)iv)[8]) |
		((uint64_t)((const uint8_t*)iv)[9] << 8) |
		((uint64_t)((const uint8_t*)iv)[10] << 16) |
		((uint64_t)((const uint8_t*)iv)[11] << 24) |
		((uint64_t)((const uint8_t*)iv)[12] << 32) |
		((uint64_t)((const uint8_t*)iv)[13] << 40) |
		((uint64_t)((const uint8_t*)iv)[14] << 48) |
		((uint64_t)((const uint8_t*)iv)[15] << 56);
	FlAes256KeyExpansion(key, &roundKey[0]);
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		FlAes256Encrypt(&roundKey[0], &counterBlock[0], &keystreamBlock[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = plainTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= keystreamBlock[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
		uint64_t counterBlockLow;
		unsigned char counterCarry = FL_AES256_ADD_CARRY_64(0, FL_AES256_BYTE_SWAP_64(counterBlock[1]), 1, &counterBlockLow);
		uint64_t counterBlockHigh = FL_AES256_BYTE_SWAP_64(counterBlock[0]) + counterCarry;
		counterBlock[0] = FL_AES256_BYTE_SWAP_64(counterBlockHigh);
		counterBlock[1] = FL_AES256_BYTE_SWAP_64(counterBlockLow);
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		FlAes256Encrypt(&roundKey[0], &counterBlock[0], &keystreamBlock[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = 0;
		}
		for (int i = 0; i < remaining; i++)
		{
			block[i] = plainTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= keystreamBlock[i];
		}
		for (int i = 0; i < remaining; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
	}
}

void FlAes256CbcEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* iv, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText)
{
	const uint8_t* plainTextBuffer = (const uint8_t*)plainText;
	uint8_t* cipherTextBuffer = (uint8_t*)cipherText;
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	uint8_t plainTextBlock[FL_AES256_BLOCK_SIZE];
	uint8_t block[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		block[i] = ((const uint8_t*)iv)[i];
	}
	FlAes256KeyExpansion(key, &roundKey[0]);
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			plainTextBlock[i] = plainTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= plainTextBlock[i];
		}
		FlAes256Encrypt(&roundKey[0], &block[0], &block[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			plainTextBlock[i] = 0;
		}
		for (int i = 0; i < remaining; i++)
		{
			plainTextBlock[i] = plainTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= plainTextBlock[i];
		}
		FlAes256Encrypt(&roundKey[0], &block[0], &block[0]);
		for (int i = 0; i < remaining; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
	}
}

void FlAes256CbcDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* iv, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* cipherText, _Out_writes_bytes_all_(textSize) void* plainText)
{
	const uint8_t* cipherTextBuffer = (const uint8_t*)cipherText;
	uint8_t* plainTextBuffer = (uint8_t*)plainText;
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	uint8_t cipherTextBlock[FL_AES256_BLOCK_SIZE];
	uint8_t previousBlock[FL_AES256_BLOCK_SIZE];
	uint8_t block[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		previousBlock[i] = ((const uint8_t*)iv)[i];
	}
	FlAes256KeyExpansion(key, &roundKey[0]);
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			cipherTextBlock[i] = cipherTextBuffer[offset + i];
		}
		FlAes256Decrypt(&roundKey[0], &cipherTextBlock[0], &block[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= previousBlock[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			plainTextBuffer[offset + i] = block[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			previousBlock[i] = cipherTextBlock[i];
		}
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			cipherTextBlock[i] = 0;
		}
		for (int i = 0; i < remaining; i++)
		{
			cipherTextBlock[i] = cipherTextBuffer[offset + i];
		}
		FlAes256Decrypt(&roundKey[0], &cipherTextBlock[0], &block[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= previousBlock[i];
		}
		for (int i = 0; i < remaining; i++)
		{
			plainTextBuffer[offset + i] = block[i];
		}
	}
}

static void FlAes256GcmPrecomputeGHASHLookupTable(_In_reads_bytes_(16) const uint8_t* hashKey, _Out_writes_(32) uint64_t* hashKeyLookupTable)
{
	uint64_t hashKeyBlock[2] = {
		((uint64_t)hashKey[0] << 56) | ((uint64_t)hashKey[1] << 48) |
		((uint64_t)hashKey[2] << 40) | ((uint64_t)hashKey[3] << 32) |
		((uint64_t)hashKey[4] << 24) | ((uint64_t)hashKey[5] << 16) |
		((uint64_t)hashKey[6] << 8) | (uint64_t)hashKey[7],
		((uint64_t)hashKey[8] << 56) | ((uint64_t)hashKey[9] << 48) |
		((uint64_t)hashKey[10] << 40) | ((uint64_t)hashKey[11] << 32) |
		((uint64_t)hashKey[12] << 24) | ((uint64_t)hashKey[13] << 16) |
		((uint64_t)hashKey[14] << 8) | (uint64_t)hashKey[15] };

	hashKeyLookupTable[(0 << 1) | 0] = 0;
	hashKeyLookupTable[(0 << 1) | 1] = 0;
	hashKeyLookupTable[(8 << 1) | 0] = hashKeyBlock[0];
	hashKeyLookupTable[(8 << 1) | 1] = hashKeyBlock[1];

	uint64_t carry = hashKeyBlock[1] & 1;
	hashKeyLookupTable[(4 << 1) | 1] = (hashKeyBlock[1] >> 1) | (hashKeyBlock[0] << 63);
	hashKeyLookupTable[(4 << 1) | 0] = (hashKeyBlock[0] >> 1) ^ ((uint64_t)(0xE1 & (0 - carry)) << 56);

	hashKeyBlock[0] = hashKeyLookupTable[(4 << 1) | 0];
	hashKeyBlock[1] = hashKeyLookupTable[(4 << 1) | 1];
	carry = hashKeyBlock[1] & 1;
	hashKeyLookupTable[(2 << 1) | 1] = (hashKeyBlock[1] >> 1) | (hashKeyBlock[0] << 63);
	hashKeyLookupTable[(2 << 1) | 0] = (hashKeyBlock[0] >> 1) ^ ((uint64_t)(0xE1 & (0 - carry)) << 56);

	hashKeyBlock[0] = hashKeyLookupTable[(2 << 1) | 0];
	hashKeyBlock[1] = hashKeyLookupTable[(2 << 1) | 1];
	carry = hashKeyBlock[1] & 1;
	hashKeyLookupTable[(1 << 1) | 1] = (hashKeyBlock[1] >> 1) | (hashKeyBlock[0] << 63);
	hashKeyLookupTable[(1 << 1) | 0] = (hashKeyBlock[0] >> 1) ^ ((uint64_t)(0xE1 & (0 - carry)) << 56);

	hashKeyLookupTable[(3 << 1) | 0] = hashKeyLookupTable[(2 << 1) | 0] ^ hashKeyLookupTable[(1 << 1) | 0];
	hashKeyLookupTable[(3 << 1) | 1] = hashKeyLookupTable[(2 << 1) | 1] ^ hashKeyLookupTable[(1 << 1) | 1];

	hashKeyLookupTable[(5 << 1) | 0] = hashKeyLookupTable[(4 << 1) | 0] ^ hashKeyLookupTable[(1 << 1) | 0];
	hashKeyLookupTable[(5 << 1) | 1] = hashKeyLookupTable[(4 << 1) | 1] ^ hashKeyLookupTable[(1 << 1) | 1];

	hashKeyLookupTable[(6 << 1) | 0] = hashKeyLookupTable[(4 << 1) | 0] ^ hashKeyLookupTable[(2 << 1) | 0];
	hashKeyLookupTable[(6 << 1) | 1] = hashKeyLookupTable[(4 << 1) | 1] ^ hashKeyLookupTable[(2 << 1) | 1];

	hashKeyLookupTable[(7 << 1) | 0] = hashKeyLookupTable[(4 << 1) | 0] ^ hashKeyLookupTable[(3 << 1) | 0];
	hashKeyLookupTable[(7 << 1) | 1] = hashKeyLookupTable[(4 << 1) | 1] ^ hashKeyLookupTable[(3 << 1) | 1];

	hashKeyLookupTable[(9 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(1 << 1) | 0];
	hashKeyLookupTable[(9 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(1 << 1) | 1];

	hashKeyLookupTable[(10 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(2 << 1) | 0];
	hashKeyLookupTable[(10 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(2 << 1) | 1];

	hashKeyLookupTable[(11 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(3 << 1) | 0];
	hashKeyLookupTable[(11 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(3 << 1) | 1];

	hashKeyLookupTable[(12 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(4 << 1) | 0];
	hashKeyLookupTable[(12 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(4 << 1) | 1];

	hashKeyLookupTable[(13 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(5 << 1) | 0];
	hashKeyLookupTable[(13 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(5 << 1) | 1];

	hashKeyLookupTable[(14 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(6 << 1) | 0];
	hashKeyLookupTable[(14 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(6 << 1) | 1];

	hashKeyLookupTable[(15 << 1) | 0] = hashKeyLookupTable[(8 << 1) | 0] ^ hashKeyLookupTable[(7 << 1) | 0];
	hashKeyLookupTable[(15 << 1) | 1] = hashKeyLookupTable[(8 << 1) | 1] ^ hashKeyLookupTable[(7 << 1) | 1];
}

FL_AES256_ALIGN(32) static const uint16_t FlAes256ReductionConstant4bitTable[16] = {
	0x0000, 0x1C20, 0x3840, 0x2460,
	0x7080, 0x6CA0, 0x48C0, 0x54E0,
	0xE100, 0xFD20, 0xD940, 0xC560,
	0x9180, 0x8DA0, 0xA9C0, 0xB5E0 };
    
static void FlAes256GcmGHASH(_In_reads_(32) const uint64_t* hashKeyLookupTable, _Inout_updates_(16) uint8_t* hash)
{
	uint8_t l = hash[15] & 0xF;
	uint8_t h = hash[15] >> 4;
	uint64_t z[2] = { hashKeyLookupTable[(l << 1) | 0], hashKeyLookupTable[(l << 1) | 1] };

	uint64_t r = z[1] & 0xF;
	z[1] = (z[1] >> 4) | (z[0] << 60);
	z[0] = (z[0] >> 4);
	z[0] ^= (uint64_t)FlAes256ReductionConstant4bitTable[r] << 48;
	z[0] ^= hashKeyLookupTable[(h << 1) | 0];
	z[1] ^= hashKeyLookupTable[(h << 1) | 1];

	for (int i = 15; i--; i)
	{
		l = hash[i] & 0xF;
		h = hash[i] >> 4;

		uint64_t rl = z[1] & 0xF;
		z[1] = (z[1] >> 4) | (z[0] << 60);
		z[0] = (z[0] >> 4);
		z[0] ^= (uint64_t)FlAes256ReductionConstant4bitTable[rl] << 48;
		z[0] ^= hashKeyLookupTable[(l << 1) | 0];
		z[1] ^= hashKeyLookupTable[(l << 1) | 1];

		uint64_t rh = z[1] & 0xF;
		z[1] = (z[1] >> 4) | (z[0] << 60);
		z[0] = (z[0] >> 4);
		z[0] ^= (uint64_t)FlAes256ReductionConstant4bitTable[rh] << 48;
		z[0] ^= hashKeyLookupTable[(h << 1) | 0];
		z[1] ^= hashKeyLookupTable[(h << 1) | 1];
	}

	hash[0] = (uint8_t)(z[0] >> 56);
	hash[1] = (uint8_t)(z[0] >> 48);
	hash[2] = (uint8_t)(z[0] >> 40);
	hash[3] = (uint8_t)(z[0] >> 32);
	hash[4] = (uint8_t)(z[0] >> 24);
	hash[5] = (uint8_t)(z[0] >> 16);
	hash[6] = (uint8_t)(z[0] >> 8);
	hash[7] = (uint8_t)(z[0]);
	hash[8] = (uint8_t)(z[1] >> 56);
	hash[9] = (uint8_t)(z[1] >> 48);
	hash[10] = (uint8_t)(z[1] >> 40);
	hash[11] = (uint8_t)(z[1] >> 32);
	hash[12] = (uint8_t)(z[1] >> 24);
	hash[13] = (uint8_t)(z[1] >> 16);
	hash[14] = (uint8_t)(z[1] >> 8);
	hash[15] = (uint8_t)(z[1]);
}

static void FlAes256GcmComputeInitialCounter(_In_reads_(32) const uint64_t* hashKeyLookupTable, _In_ size_t nonceSize, _In_reads_bytes_(nonceSize) const void* nonce, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) uint8_t* initialCounter)
{
	const uint8_t* nonceBuffer = (const uint8_t*)nonce;
	if (nonceSize == FL_AES256_GCM_NONCE_SIZE)
	{
		for (int i = 0; i < FL_AES256_GCM_NONCE_SIZE; i++)
		{
			initialCounter[i] = nonceBuffer[i];
		}
		initialCounter[12] = 0;
		initialCounter[13] = 0;
		initialCounter[14] = 0;
		initialCounter[15] = 1;
	}
	else
	{
		uint64_t nonceBitSize = (uint64_t)nonceSize << 3;
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			initialCounter[i] = 0;
		}
		while (nonceSize >= FL_AES256_BLOCK_SIZE)
		{
			for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
			{
				initialCounter[i] ^= nonceBuffer[i];
			}
			FlAes256GcmGHASH(hashKeyLookupTable, &initialCounter[0]);
			nonceBuffer += FL_AES256_BLOCK_SIZE;
			nonceSize -= FL_AES256_BLOCK_SIZE;
		}
		if (nonceSize > 0)
		{
			for (size_t i = 0; i < nonceSize; i++)
			{
				initialCounter[i] ^= nonceBuffer[i];
			}
			FlAes256GcmGHASH(hashKeyLookupTable, &initialCounter[0]);
		}
		initialCounter[8]  ^= (uint8_t)(nonceBitSize >> 56);
		initialCounter[9]  ^= (uint8_t)(nonceBitSize >> 48);
		initialCounter[10] ^= (uint8_t)(nonceBitSize >> 40);
		initialCounter[11] ^= (uint8_t)(nonceBitSize >> 32);
		initialCounter[12] ^= (uint8_t)(nonceBitSize >> 24);
		initialCounter[13] ^= (uint8_t)(nonceBitSize >> 16);
		initialCounter[14] ^= (uint8_t)(nonceBitSize >> 8);
		initialCounter[15] ^= (uint8_t)(nonceBitSize);
		FlAes256GcmGHASH(hashKeyLookupTable, &initialCounter[0]);
	}
}

void FlAes256GcmEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t nonceSize, _In_reads_bytes_(nonceSize) const void* nonce, _In_ size_t aadSize, _In_reads_bytes_(aadSize) const void* aad, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText, _Out_writes_bytes_all_(FL_AES256_GCM_TAG_SIZE) void* tag)
{
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	FlAes256KeyExpansion(key, &roundKey[0]);

	uint8_t hashKey[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		hashKey[i] = 0;
	}
	FlAes256Encrypt(&roundKey[0], &hashKey[0], &hashKey[0]);

	uint64_t hashKeyLookupTable[32];
	FlAes256GcmPrecomputeGHASHLookupTable(&hashKey[0], &hashKeyLookupTable[0]);

	uint8_t initialCounter[FL_AES256_BLOCK_SIZE];
	FlAes256GcmComputeInitialCounter(&hashKeyLookupTable[0], nonceSize, nonce, &initialCounter[0]);

	uint8_t encryptedInitialCounter[FL_AES256_BLOCK_SIZE];
	FlAes256Encrypt(&roundKey[0], &initialCounter[0], &encryptedInitialCounter[0]);

	uint8_t tagBlock[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		tagBlock[i] = 0;
	}

	uint8_t keystreamBlock[FL_AES256_BLOCK_SIZE];
	uint8_t block[FL_AES256_BLOCK_SIZE];

	const uint8_t* aadBuffer = (const uint8_t*)aad;
	size_t aadOffset = 0;
	for (size_t end = aadSize & ~((size_t)0xF); aadOffset < end; aadOffset += FL_AES256_BLOCK_SIZE)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = aadBuffer[aadOffset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	}
	int aadRemaining = (int)(aadSize & 0xF);
	if (aadRemaining)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = 0;
		}
		for (int i = 0; i < aadRemaining; i++)
		{
			block[i] = aadBuffer[aadOffset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	}

	uint32_t counter[4] = {
		((uint32_t)initialCounter[0]) |
		((uint32_t)initialCounter[1] << 8) |
		((uint32_t)initialCounter[2] << 16) |
		((uint32_t)initialCounter[3] << 24),
		((uint32_t)initialCounter[4]) |
		((uint32_t)initialCounter[5] << 8) |
		((uint32_t)initialCounter[6] << 16) |
		((uint32_t)initialCounter[7] << 24),
		((uint32_t)initialCounter[8]) |
		((uint32_t)initialCounter[9] << 8) |
		((uint32_t)initialCounter[10] << 16) |
		((uint32_t)initialCounter[11] << 24),
		((uint32_t)initialCounter[12]) |
		((uint32_t)initialCounter[13] << 8) |
		((uint32_t)initialCounter[14] << 16) |
		((uint32_t)initialCounter[15] << 24) };

	const uint8_t* plainTextBuffer = (const uint8_t*)plainText;
	uint8_t* cipherTextBuffer = (uint8_t*)cipherText;
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		counter[3] = FL_AES256_BYTE_SWAP_32(FL_AES256_BYTE_SWAP_32(counter[3]) + 1);
		FlAes256Encrypt(&roundKey[0], &counter[0], &keystreamBlock[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = plainTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= keystreamBlock[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		counter[3] = FL_AES256_BYTE_SWAP_32(FL_AES256_BYTE_SWAP_32(counter[3]) + 1);
		FlAes256Encrypt(&roundKey[0], &counter[0], &keystreamBlock[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = keystreamBlock[i];
		}
		for (int i = 0; i < remaining; i++)
		{
			block[i] = plainTextBuffer[offset +i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= keystreamBlock[i];
		}
		for (int i = 0; i < remaining; i++)
		{
			cipherTextBuffer[offset + i] = block[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	}

	uint64_t aadBits = (uint64_t)aadSize << 3;
	uint64_t textBits = (uint64_t)textSize << 3;
	uint8_t lengthBlock[FL_AES256_BLOCK_SIZE];
	lengthBlock[0]  = (uint8_t)(aadBits >> 56);
	lengthBlock[1]  = (uint8_t)(aadBits >> 48);
	lengthBlock[2]  = (uint8_t)(aadBits >> 40);
	lengthBlock[3]  = (uint8_t)(aadBits >> 32);
	lengthBlock[4]  = (uint8_t)(aadBits >> 24);
	lengthBlock[5]  = (uint8_t)(aadBits >> 16);
	lengthBlock[6]  = (uint8_t)(aadBits >> 8);
	lengthBlock[7]  = (uint8_t)(aadBits);
	lengthBlock[8]  = (uint8_t)(textBits >> 56);
	lengthBlock[9]  = (uint8_t)(textBits >> 48);
	lengthBlock[10] = (uint8_t)(textBits >> 40);
	lengthBlock[11] = (uint8_t)(textBits >> 32);
	lengthBlock[12] = (uint8_t)(textBits >> 24);
	lengthBlock[13] = (uint8_t)(textBits >> 16);
	lengthBlock[14] = (uint8_t)(textBits >> 8);
	lengthBlock[15] = (uint8_t)(textBits);
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		tagBlock[i] ^= lengthBlock[i];
	}
	FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);

	for (int i = 0; i < FL_AES256_GCM_TAG_SIZE; i++)
	{
		tagBlock[i] ^= encryptedInitialCounter[i];
	}
	uint8_t* tagBuffer = (uint8_t*)tag;
	for (int i = 0; i < FL_AES256_GCM_TAG_SIZE; i++)
	{
		tagBuffer[i] = tagBlock[i];
	}
}

int FlAes256GcmDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t nonceSize, _In_reads_bytes_(nonceSize) const void* nonce, _In_ size_t aadSize, _In_reads_bytes_(aadSize) const void* aad, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* cipherText, _Out_writes_bytes_all_(textSize) void* plainText, _In_reads_bytes_(FL_AES256_GCM_TAG_SIZE) const void* tag)
{
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	FlAes256KeyExpansion(key, &roundKey[0]);

	uint8_t hashKey[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		hashKey[i] = 0;
	}
	FlAes256Encrypt(&roundKey[0], &hashKey[0], &hashKey[0]);

	uint64_t hashKeyLookupTable[32];
	FlAes256GcmPrecomputeGHASHLookupTable(&hashKey[0], &hashKeyLookupTable[0]);

	uint8_t initialCounter[FL_AES256_BLOCK_SIZE];
	FlAes256GcmComputeInitialCounter(&hashKeyLookupTable[0], nonceSize, nonce, &initialCounter[0]);

	uint8_t encryptedInitialCounter[FL_AES256_BLOCK_SIZE];
	FlAes256Encrypt(&roundKey[0], &initialCounter[0], &encryptedInitialCounter[0]);

	uint8_t keystreamBlock[FL_AES256_BLOCK_SIZE];
	uint8_t block[FL_AES256_BLOCK_SIZE];

	uint8_t tagBlock[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		tagBlock[i] = 0;
	}

	const uint8_t* aadBuffer = (const uint8_t*)aad;
	size_t aadOffset = 0;
	for (size_t end = aadSize & ~((size_t)0xF); aadOffset < end; aadOffset += FL_AES256_BLOCK_SIZE)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = aadBuffer[aadOffset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	}
	int aadRemaining = (int)(aadSize & 0xF);
	if (aadRemaining)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = 0;
		}
		for (int i = 0; i < aadRemaining; i++)
		{
			block[i] = aadBuffer[aadOffset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	}

	uint32_t counter[4] = {
		((uint32_t)initialCounter[0]) |
		((uint32_t)initialCounter[1] << 8) |
		((uint32_t)initialCounter[2] << 16) |
		((uint32_t)initialCounter[3] << 24),
		((uint32_t)initialCounter[4]) |
		((uint32_t)initialCounter[5] << 8) |
		((uint32_t)initialCounter[6] << 16) |
		((uint32_t)initialCounter[7] << 24),
		((uint32_t)initialCounter[8]) |
		((uint32_t)initialCounter[9] << 8) |
		((uint32_t)initialCounter[10] << 16) |
		((uint32_t)initialCounter[11] << 24),
		((uint32_t)initialCounter[12]) |
		((uint32_t)initialCounter[13] << 8) |
		((uint32_t)initialCounter[14] << 16) |
		((uint32_t)initialCounter[15] << 24) };

	const uint8_t* cipherTextBuffer = (const uint8_t*)cipherText;
	uint8_t* plainTextBuffer = (uint8_t*)plainText;
	size_t offset = 0;
	for (size_t end = textSize & ~((size_t)0xF); offset < end; offset += FL_AES256_BLOCK_SIZE)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = cipherTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
		counter[3] = FL_AES256_BYTE_SWAP_32(FL_AES256_BYTE_SWAP_32(counter[3]) + 1);
		FlAes256Encrypt(&roundKey[0], &counter[0], &keystreamBlock[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= keystreamBlock[i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			plainTextBuffer[offset + i] = block[i];
		}
	}
	int remaining = (int)(textSize & 0xF);
	if (remaining)
	{
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] = 0;
		}
		for (int i = 0; i < remaining; i++)
		{
			block[i] = cipherTextBuffer[offset + i];
		}
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			tagBlock[i] ^= block[i];
		}
		FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
		counter[3] = FL_AES256_BYTE_SWAP_32(FL_AES256_BYTE_SWAP_32(counter[3]) + 1);
		FlAes256Encrypt(&roundKey[0], &counter[0], &keystreamBlock[0]);
		for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		{
			block[i] ^= keystreamBlock[i];
		}
		for (int i = 0; i < remaining; i++)
		{
			plainTextBuffer[offset + i] = block[i];
		}
	}

	uint64_t aadBits = (uint64_t)aadSize << 3;
	uint64_t textBits = (uint64_t)textSize << 3;
	uint8_t lengthBlock[FL_AES256_BLOCK_SIZE];
	lengthBlock[0] = (uint8_t)(aadBits >> 56);
	lengthBlock[1] = (uint8_t)(aadBits >> 48);
	lengthBlock[2] = (uint8_t)(aadBits >> 40);
	lengthBlock[3] = (uint8_t)(aadBits >> 32);
	lengthBlock[4] = (uint8_t)(aadBits >> 24);
	lengthBlock[5] = (uint8_t)(aadBits >> 16);
	lengthBlock[6] = (uint8_t)(aadBits >> 8);
	lengthBlock[7] = (uint8_t)(aadBits);
	lengthBlock[8] = (uint8_t)(textBits >> 56);
	lengthBlock[9] = (uint8_t)(textBits >> 48);
	lengthBlock[10] = (uint8_t)(textBits >> 40);
	lengthBlock[11] = (uint8_t)(textBits >> 32);
	lengthBlock[12] = (uint8_t)(textBits >> 24);
	lengthBlock[13] = (uint8_t)(textBits >> 16);
	lengthBlock[14] = (uint8_t)(textBits >> 8);
	lengthBlock[15] = (uint8_t)(textBits);
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		tagBlock[i] ^= lengthBlock[i];
	}
	FlAes256GcmGHASH(&hashKeyLookupTable[0], &tagBlock[0]);
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
	{
		tagBlock[i] ^= encryptedInitialCounter[i];
	}

	uint8_t tagDifference = 0;
	for (int i = 0; i < FL_AES256_GCM_TAG_SIZE; i++)
	{
		block[i] = ((const uint8_t*)tag)[i];
	}
	for (int i = 0; i < FL_AES256_GCM_TAG_SIZE; i++)
	{
		tagDifference |= tagBlock[i] ^ block[i];
	}
	return !tagDifference;
}

#ifdef __cplusplus
}
#endif // __cplusplus
