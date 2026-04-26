/*
	SHA-256 HMAC unit tests by Santtu S. Nyman.

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

	Description:

		Unit tests for FlSha256HmacCreateHmac, FlSha256HmacHashData and
		FlSha256Hmac256FinishHmac.

		Primary reference: RFC 4231 test vectors for HMAC-SHA-256.
		Additional custom vectors computed with .NET System.Security.Cryptography.HMACSHA256.
		TC1, TC3, TC4, TC6 and TC7 match the RFC exactly.  TC2 and custom vectors
		were verified with .NET HMACSHA256 (key/data bytes confirmed against RFC hex).
		TC5 (truncation) is omitted as the API always produces the full 32-byte digest.
*/

#include "FlUt.h"
#include "../include/FlSha256Hmac.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Expected HMAC-SHA-256 digests for known inputs.
// ---------------------------------------------------------------------------

// RFC 4231 TC1: key = 20x 0x0B, data = "Hi There"
// = b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC1[FL_SHA256_DIGEST_SIZE] =
{
	0xB0, 0x34, 0x4C, 0x61, 0xD8, 0xDB, 0x38, 0x53,
	0x5C, 0xA8, 0xAF, 0xCE, 0xAF, 0x0B, 0xF1, 0x2B,
	0x88, 0x1D, 0xC2, 0x00, 0xC9, 0x83, 0x3D, 0xA7,
	0x26, 0xE9, 0x37, 0x6C, 0x2E, 0x32, 0xCF, 0xF7
};

// RFC 4231 TC2: key = "Jefe", data = "what do ya want for nothing?"
// = 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC2[FL_SHA256_DIGEST_SIZE] =
{
	0x5B, 0xDC, 0xC1, 0x46, 0xBF, 0x60, 0x75, 0x4E,
	0x6A, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xC7,
	0x5A, 0x00, 0x3F, 0x08, 0x9D, 0x27, 0x39, 0x83,
	0x9D, 0xEC, 0x58, 0xB9, 0x64, 0xEC, 0x38, 0x43
};

// RFC 4231 TC3: key = 20x 0xAA, data = 50x 0xDD
// = 773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC3[FL_SHA256_DIGEST_SIZE] =
{
	0x77, 0x3E, 0xA9, 0x1E, 0x36, 0x80, 0x0E, 0x46,
	0x85, 0x4D, 0xB8, 0xEB, 0xD0, 0x91, 0x81, 0xA7,
	0x29, 0x59, 0x09, 0x8B, 0x3E, 0xF8, 0xC1, 0x22,
	0xD9, 0x63, 0x55, 0x14, 0xCE, 0xD5, 0x65, 0xFE
};

// RFC 4231 TC4: key = 0x01..0x19 (25 bytes), data = 50x 0xCD
// = 82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC4[FL_SHA256_DIGEST_SIZE] =
{
	0x82, 0x55, 0x8A, 0x38, 0x9A, 0x44, 0x3C, 0x0E,
	0xA4, 0xCC, 0x81, 0x98, 0x99, 0xF2, 0x08, 0x3A,
	0x85, 0xF0, 0xFA, 0xA3, 0xE5, 0x78, 0xF8, 0x07,
	0x7A, 0x2E, 0x3F, 0xF4, 0x67, 0x29, 0x66, 0x5B
};

// RFC 4231 TC6: key = 131x 0xAA (key > block size, hashed first),
//              data = "Test Using Larger Than Block-Size Key - Hash Key First"
// = 60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC6[FL_SHA256_DIGEST_SIZE] =
{
	0x60, 0xE4, 0x31, 0x59, 0x1E, 0xE0, 0xB6, 0x7F,
	0x0D, 0x8A, 0x26, 0xAA, 0xCB, 0xF5, 0xB7, 0x7F,
	0x8E, 0x0B, 0xC6, 0x21, 0x37, 0x28, 0xC5, 0x14,
	0x05, 0x46, 0x04, 0x0F, 0x0E, 0xE3, 0x7F, 0x54
};

// RFC 4231 TC7: key = 131x 0xAA (key > block size, hashed first),
//              data = "This is a test using a larger than block-size key and a larger
//                      than block-size data. The key needs to be hashed before being
//                      used by the HMAC algorithm."
// = 9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC7[FL_SHA256_DIGEST_SIZE] =
{
	0x9B, 0x09, 0xFF, 0xA7, 0x1B, 0x94, 0x2F, 0xCB,
	0x27, 0x63, 0x5F, 0xBC, 0xD5, 0xB0, 0xE9, 0x44,
	0xBF, 0xDC, 0x63, 0x64, 0x4F, 0x07, 0x13, 0x93,
	0x8A, 0x7F, 0x51, 0x53, 0x5C, 0x3A, 0x35, 0xE2
};

// Custom: key = 20x 0x0B, data = "" (empty)
// = 999a901219f032cd497cadb5e6051e97b6a29ab297bd6ae722bd6062a2f59542
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_EMPTY_DATA[FL_SHA256_DIGEST_SIZE] =
{
	0x99, 0x9A, 0x90, 0x12, 0x19, 0xF0, 0x32, 0xCD,
	0x49, 0x7C, 0xAD, 0xB5, 0xE6, 0x05, 0x1E, 0x97,
	0xB6, 0xA2, 0x9A, 0xB2, 0x97, 0xBD, 0x6A, 0xE7,
	0x22, 0xBD, 0x60, 0x62, 0xA2, 0xF5, 0x95, 0x42
};

// Custom: key = "" (0 bytes, zero-padded to block size), data = "abc"
// = fd7adb152c05ef80dccf50a1fa4c05d5a3ec6da95575fc312ae7c5d091836351
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_EMPTY_KEY[FL_SHA256_DIGEST_SIZE] =
{
	0xFD, 0x7A, 0xDB, 0x15, 0x2C, 0x05, 0xEF, 0x80,
	0xDC, 0xCF, 0x50, 0xA1, 0xFA, 0x4C, 0x05, 0xD5,
	0xA3, 0xEC, 0x6D, 0xA9, 0x55, 0x75, 0xFC, 0x31,
	0x2A, 0xE7, 0xC5, 0xD0, 0x91, 0x83, 0x63, 0x51
};

// Custom: key = 64x 0x01 (exactly block size, no padding or hashing needed), data = "abc"
// = 0c462fe9789424c45ebeb37b90d6e879e8e0580849c91a84198b267a90c77250
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_BLOCK_SIZE_KEY[FL_SHA256_DIGEST_SIZE] =
{
	0x0C, 0x46, 0x2F, 0xE9, 0x78, 0x94, 0x24, 0xC4,
	0x5E, 0xBE, 0xB3, 0x7B, 0x90, 0xD6, 0xE8, 0x79,
	0xE8, 0xE0, 0x58, 0x08, 0x49, 0xC9, 0x1A, 0x84,
	0x19, 0x8B, 0x26, 0x7A, 0x90, 0xC7, 0x72, 0x50
};

// Custom: key = "key", data = "Test data of Santtu S. Nyman"
// = bf044cacc4a566f5cbedb295a84222f584b242920b8579ef81822de6f7425af9
static const uint8_t FL_SHA256_HMAC_UT_EXPECTED_SANTTU_TEST_DATA[FL_SHA256_DIGEST_SIZE] =
{
	0xBF, 0x04, 0x4C, 0xAC, 0xC4, 0xA5, 0x66, 0xF5,
	0xCB, 0xED, 0xB2, 0x95, 0xA8, 0x42, 0x22, 0xF5,
	0x84, 0xB2, 0x42, 0x92, 0x0B, 0x85, 0x79, 0xEF,
	0x81, 0x82, 0x2D, 0xE6, 0xF7, 0x42, 0x5A, 0xF9
};

// ---------------------------------------------------------------------------
// Helper: compute HMAC-SHA-256 in a single shot.
// ---------------------------------------------------------------------------
static void FlSha256HmacCompute(
	_In_ size_t keySize, _In_reads_bytes_(keySize) const void* key,
	_In_ size_t dataSize, _In_reads_bytes_(dataSize) const void* data,
	_Out_writes_bytes_all_(FL_SHA256_DIGEST_SIZE) uint8_t* digest)
{
	FlSha256HmacContext ctx;
	FlSha256HmacCreateHmac(&ctx, keySize, key);
	FlSha256HmacHashData(&ctx, dataSize, data);
	FlSha256Hmac256FinishHmac(&ctx, digest);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

// RFC 4231 TC1: key = 20x 0x0B, data = "Hi There".
static void FlSha256HmacUtRfc4231Tc1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[]  = { 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	                                0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B };
	static const uint8_t data[] = "Hi There";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC1, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtRfc4231Tc1");
}

// RFC 4231 TC2: key = "Jefe", data = "what do ya want for nothing?".
static void FlSha256HmacUtRfc4231Tc2(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[]  = "Jefe";
	static const uint8_t data[] = "what do ya want for nothing?";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof(key) - 1, key, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC2, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtRfc4231Tc2");
}

// RFC 4231 TC3: key = 20x 0xAA, data = 50x 0xDD.
static void FlSha256HmacUtRfc4231Tc3(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[20]  = { 0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
	                                   0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA };
	static const uint8_t data[50] = { 0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,
	                                   0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,
	                                   0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,
	                                   0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,
	                                   0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD,0xDD };
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof data, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC3, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtRfc4231Tc3");
}

// RFC 4231 TC4: key = 0x01..0x19 (25 bytes), data = 50x 0xCD.
static void FlSha256HmacUtRfc4231Tc4(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[25]  = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,
	                                   0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,
	                                   0x15,0x16,0x17,0x18,0x19 };
	static const uint8_t data[50] = { 0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,
	                                   0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,
	                                   0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,
	                                   0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,
	                                   0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD,0xCD };
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof data, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC4, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtRfc4231Tc4");
}

// RFC 4231 TC6: key = 131x 0xAA (key > block size, hashed first),
//              data = "Test Using Larger Than Block-Size Key - Hash Key First".
static void FlSha256HmacUtRfc4231Tc6(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[131] =
	{
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA
	};
	static const uint8_t data[] = "Test Using Larger Than Block-Size Key - Hash Key First";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC6, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtRfc4231Tc6");
}

// RFC 4231 TC7: key = 131x 0xAA, large data string.
static void FlSha256HmacUtRfc4231Tc7(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[131] =
	{
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
		0xAA,0xAA,0xAA
	};
	static const uint8_t data[] =
		"This is a test using a larger than block-size key and a larger than block-size data. "
		"The key needs to be hashed before being used by the HMAC algorithm.";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_RFC4231_TC7, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtRfc4231Tc7");
}

// Custom: key = 20x 0x0B, data = empty.  Exercises that finish on empty data is correct.
static void FlSha256HmacUtEmptyData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[] = { 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	                                0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B };
	static const uint8_t emptyData[1] = { 0 };
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, 0, emptyData, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_EMPTY_DATA, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtEmptyData");
}

// Custom: key = empty (0 bytes, zero-padded to block size), data = "abc".
static void FlSha256HmacUtEmptyKey(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t emptyKey[1] = { 0 };
	static const uint8_t data[] = "abc";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(0, emptyKey, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_EMPTY_KEY, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtEmptyKey");
}

// Custom: key = 64x 0x01 (exactly block size, no padding or hashing needed), data = "abc".
static void FlSha256HmacUtBlockSizeKey(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[FL_SHA256_HMAC_BLOCK_SIZE] =
	{
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01
	};
	static const uint8_t data[] = "abc";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_BLOCK_SIZE_KEY, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtBlockSizeKey");
}

// Custom: key = "key", data = "Test data of Santtu S. Nyman".
static void FlSha256HmacUtSanttuTestData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[]  = "key";
	static const uint8_t data[] = "Test data of Santtu S. Nyman";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof(key) - 1, key, sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_HMAC_UT_EXPECTED_SANTTU_TEST_DATA, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtSanttuTestData");
}

// Feeding data in multiple chunks must produce the same MAC as a single call.
static void FlSha256HmacUtStreamingEquality(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[]  = { 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	                                 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B };
	static const uint8_t data[] = "Hi There";
	uint8_t wholeDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, wholeDigest);

	// Split 1 + remainder.
	{
		FlSha256HmacContext ctx;
		FlSha256HmacCreateHmac(&ctx, sizeof key, key);
		FlSha256HmacHashData(&ctx, 1, data);
		FlSha256HmacHashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256Hmac256FinishHmac(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtStreamingEquality_1_rest");
	}

	// One byte at a time.
	{
		FlSha256HmacContext ctx;
		FlSha256HmacCreateHmac(&ctx, sizeof key, key);
		for (size_t n = sizeof(data) - 1, i = 0; i < n; i++)
			FlSha256HmacHashData(&ctx, 1, data + i);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256Hmac256FinishHmac(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtStreamingEquality_1xN");
	}

	// Split at every possible position within a short data.
	{
		static const uint8_t shortData[] = "abcd";
		uint8_t expectedShort[FL_SHA256_DIGEST_SIZE];
		FlSha256HmacCompute(sizeof key, key, sizeof(shortData) - 1, shortData, expectedShort);
		for (size_t n = sizeof(shortData) - 1, splitAt = 1; splitAt < n; splitAt++)
		{
			FlSha256HmacContext ctx;
			FlSha256HmacCreateHmac(&ctx, sizeof key, key);
			FlSha256HmacHashData(&ctx, splitAt, shortData);
			FlSha256HmacHashData(&ctx, n - splitAt, shortData + splitAt);
			uint8_t digest[FL_SHA256_DIGEST_SIZE];
			FlSha256Hmac256FinishHmac(&ctx, digest);
			FL_UT_CHECK(memcmp(digest, expectedShort, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtStreamingEquality_AllSplits");
		}
	}
}

// Passing a zero-length data chunk must not alter the MAC.
static void FlSha256HmacUtZeroLengthChunkNoEffect(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[]  = "Jefe";
	static const uint8_t data[] = "abc";
	uint8_t expected[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof(key) - 1, key, sizeof(data) - 1, data, expected);

	// Zero-length call before actual data.
	{
		FlSha256HmacContext ctx;
		FlSha256HmacCreateHmac(&ctx, sizeof(key) - 1, key);
		FlSha256HmacHashData(&ctx, 0, data);
		FlSha256HmacHashData(&ctx, sizeof(data) - 1, data);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256Hmac256FinishHmac(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtZeroLengthChunkNoEffect_Before");
	}

	// Zero-length call after actual data.
	{
		FlSha256HmacContext ctx;
		FlSha256HmacCreateHmac(&ctx, sizeof(key) - 1, key);
		FlSha256HmacHashData(&ctx, sizeof(data) - 1, data);
		FlSha256HmacHashData(&ctx, 0, data);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256Hmac256FinishHmac(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtZeroLengthChunkNoEffect_After");
	}

	// Zero-length call in the middle of actual data.
	{
		FlSha256HmacContext ctx;
		FlSha256HmacCreateHmac(&ctx, sizeof(key) - 1, key);
		FlSha256HmacHashData(&ctx, 1, data);
		FlSha256HmacHashData(&ctx, 0, data);
		FlSha256HmacHashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256Hmac256FinishHmac(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtZeroLengthChunkNoEffect_Middle");
	}
}

// Same key and data must always produce the same MAC (no hidden mutable state).
static void FlSha256HmacUtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t key[]  = { 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	                                 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B };
	static const uint8_t data[] = "Hi There";
	uint8_t firstDigest[FL_SHA256_DIGEST_SIZE];
	uint8_t secondDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, firstDigest);
	FlSha256HmacCompute(sizeof key, key, sizeof(data) - 1, data, secondDigest);
	FL_UT_CHECK(memcmp(firstDigest, secondDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtDeterminism");
}

// Different keys with the same data must produce different MACs.
static void FlSha256HmacUtDifferentKeysDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Hi There";
	static const uint8_t keyA[] = { 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	                                 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B };
	static const uint8_t keyB[] = { 0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
	                                 0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C };
	uint8_t digestA[FL_SHA256_DIGEST_SIZE];
	uint8_t digestB[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof keyA, keyA, sizeof(data) - 1, data, digestA);
	FlSha256HmacCompute(sizeof keyB, keyB, sizeof(data) - 1, data, digestB);
	FL_UT_CHECK(memcmp(digestA, digestB, FL_SHA256_DIGEST_SIZE) != 0, "FlSha256HmacUtDifferentKeysDifferentOutput_Keys");

	// Same key, different data must also produce different MACs.
	static const uint8_t dataA[] = "Hi There";
	static const uint8_t dataB[] = "Hi There!";
	uint8_t digestC[FL_SHA256_DIGEST_SIZE];
	uint8_t digestD[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof keyA, keyA, sizeof(dataA) - 1, dataA, digestC);
	FlSha256HmacCompute(sizeof keyA, keyA, sizeof(dataB) - 1, dataB, digestD);
	FL_UT_CHECK(memcmp(digestC, digestD, FL_SHA256_DIGEST_SIZE) != 0, "FlSha256HmacUtDifferentKeysDifferentOutput_Data");
}

// Two interleaved HMAC computations must not interfere with each other.
static void FlSha256HmacUtContextIsolation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t keyA[]  = "Jefe";
	static const uint8_t dataA[] = "abc";
	static const uint8_t keyB[]  = { 0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,
	                                  0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B };
	static const uint8_t dataB[] = "123";
	uint8_t expectedA[FL_SHA256_DIGEST_SIZE];
	uint8_t expectedB[FL_SHA256_DIGEST_SIZE];
	FlSha256HmacCompute(sizeof(keyA) - 1, keyA, sizeof(dataA) - 1, dataA, expectedA);
	FlSha256HmacCompute(sizeof keyB,      keyB, sizeof(dataB) - 1, dataB, expectedB);

	// Interleave byte-by-byte data updates on both contexts (keys are already set).
	FlSha256HmacContext ctxA;
	FlSha256HmacContext ctxB;
	FlSha256HmacCreateHmac(&ctxA, sizeof(keyA) - 1, keyA);
	FlSha256HmacCreateHmac(&ctxB, sizeof keyB,      keyB);
	for (size_t n = sizeof(dataA) - 1, i = 0; i < n; i++)
	{
		FlSha256HmacHashData(&ctxA, 1, dataA + i);
		FlSha256HmacHashData(&ctxB, 1, dataB + i);
	}

	uint8_t digestA[FL_SHA256_DIGEST_SIZE];
	uint8_t digestB[FL_SHA256_DIGEST_SIZE];
	FlSha256Hmac256FinishHmac(&ctxA, digestA);
	FlSha256Hmac256FinishHmac(&ctxB, digestB);
	FL_UT_CHECK(memcmp(digestA, expectedA, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtContextIsolation_A");
	FL_UT_CHECK(memcmp(digestB, expectedB, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256HmacUtContextIsolation_B");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlSha256HmacUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlSha256HmacUtRfc4231Tc1(testCount, failCount);
	FlSha256HmacUtRfc4231Tc2(testCount, failCount);
	FlSha256HmacUtRfc4231Tc3(testCount, failCount);
	FlSha256HmacUtRfc4231Tc4(testCount, failCount);
	FlSha256HmacUtRfc4231Tc6(testCount, failCount);
	FlSha256HmacUtRfc4231Tc7(testCount, failCount);
	FlSha256HmacUtEmptyData(testCount, failCount);
	FlSha256HmacUtEmptyKey(testCount, failCount);
	FlSha256HmacUtBlockSizeKey(testCount, failCount);
	FlSha256HmacUtSanttuTestData(testCount, failCount);
	FlSha256HmacUtStreamingEquality(testCount, failCount);
	FlSha256HmacUtZeroLengthChunkNoEffect(testCount, failCount);
	FlSha256HmacUtDeterminism(testCount, failCount);
	FlSha256HmacUtDifferentKeysDifferentOutput(testCount, failCount);
	FlSha256HmacUtContextIsolation(testCount, failCount);
}
