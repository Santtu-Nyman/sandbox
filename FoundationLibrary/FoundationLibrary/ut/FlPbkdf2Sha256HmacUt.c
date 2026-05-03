/*
	PBKDF2-HMAC-SHA256 unit tests by Santtu S. Nyman.

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

		Unit tests for FlPbkdf2Sha256Hmac.

		Reference vectors are computed with a manual PBKDF2-HMAC-SHA256
		implementation in PowerShell using System.Security.Cryptography.HMACSHA256,
		cross-verified against RFC 7914 (scrypt) Appendix B PBKDF2-HMAC-SHA256 vectors.
		Single-block outputs (dkLen=32, c=1) were independently confirmed by direct
		HMAC-SHA256(password, salt || INT(1)) computation.

		Special case: IterationCount==0 causes the implementation to zero the output
		buffer immediately without any HMAC work.
*/

#include "FlUt.h"
#include "../include/FlPbkdf2Sha256Hmac.h"
#include "../include/FlSha256.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Expected derived key bytes for known inputs.
// ---------------------------------------------------------------------------

// RFC 7914 TC1: P="passwd", S="salt", c=1, dkLen=64
static const uint8_t FL_PBKDF2_UT_EXPECTED_RFC7914_TC1[64] =
{
	0x55, 0xAC, 0x04, 0x6E, 0x56, 0xE3, 0x08, 0x9F, 0xEC, 0x16, 0x91, 0xC2, 0x25, 0x44, 0xB6, 0x05,
	0xF9, 0x41, 0x85, 0x21, 0x6D, 0xDE, 0x04, 0x65, 0xE6, 0x8B, 0x9D, 0x57, 0xC2, 0x0D, 0xAC, 0xBC,
	0x49, 0xCA, 0x9C, 0xCC, 0xF1, 0x79, 0xB6, 0x45, 0x99, 0x16, 0x64, 0xB3, 0x9D, 0x77, 0xEF, 0x31,
	0x7C, 0x71, 0xB8, 0x45, 0xB1, 0xE3, 0x0B, 0xD5, 0x09, 0x11, 0x20, 0x41, 0xD3, 0xA1, 0x97, 0x83
};

// RFC 7914 TC2: P="Password", S="NaCl", c=80000, dkLen=64
static const uint8_t FL_PBKDF2_UT_EXPECTED_RFC7914_TC2[64] =
{
	0x4D, 0xDC, 0xD8, 0xF6, 0x0B, 0x98, 0xBE, 0x21, 0x83, 0x0C, 0xEE, 0x5E, 0xF2, 0x27, 0x01, 0xF9,
	0x64, 0x1A, 0x44, 0x18, 0xD0, 0x4C, 0x04, 0x14, 0xAE, 0xFF, 0x08, 0x87, 0x6B, 0x34, 0xAB, 0x56,
	0xA1, 0xD4, 0x25, 0xA1, 0x22, 0x58, 0x33, 0x54, 0x9A, 0xDB, 0x84, 0x1B, 0x51, 0xC9, 0xB3, 0x17,
	0x6A, 0x27, 0x2B, 0xDE, 0xBB, 0xA1, 0xD0, 0x78, 0x47, 0x8F, 0x62, 0xB3, 0x97, 0xF3, 0x3C, 0x8D
};

// Custom 1: P="password", S="salt", c=1, dkLen=32 (one full block, c=1)
// Verified: equals HMAC-SHA256("password", "salt" || 0x00000001)
static const uint8_t FL_PBKDF2_UT_EXPECTED_SINGLE_BLOCK[FL_SHA256_DIGEST_SIZE] =
{
	0x12, 0x0F, 0xB6, 0xCF, 0xFC, 0xF8, 0xB3, 0x2C, 0x43, 0xE7, 0x22, 0x52, 0x56, 0xC4, 0xF8, 0x37,
	0xA8, 0x65, 0x48, 0xC9, 0x2C, 0xCC, 0x35, 0x48, 0x08, 0x05, 0x98, 0x7C, 0xB7, 0x0B, 0xE1, 0x7B
};

// Custom 2: P="password", S="salt", c=1000, dkLen=32 (iteration correctness)
static const uint8_t FL_PBKDF2_UT_EXPECTED_MULTI_ITER[FL_SHA256_DIGEST_SIZE] =
{
	0x63, 0x2C, 0x28, 0x12, 0xE4, 0x6D, 0x46, 0x04, 0x10, 0x2B, 0xA7, 0x61, 0x8E, 0x9D, 0x6D, 0x7D,
	0x2F, 0x81, 0x28, 0xF6, 0x26, 0x6B, 0x4A, 0x03, 0x26, 0x4D, 0x2A, 0x04, 0x60, 0xB7, 0xDC, 0xB3
};

// Custom 3: P="password", S="salt", c=1, dkLen=1 (sub-block output — first byte of custom 1)
static const uint8_t FL_PBKDF2_UT_EXPECTED_SUB_BLOCK[1] = { 0x12 };

// Custom 4: P="password", S="salt", c=1, dkLen=33 (one full block + 1 byte of second block)
static const uint8_t FL_PBKDF2_UT_EXPECTED_CROSS_BLOCK[33] =
{
	0x12, 0x0F, 0xB6, 0xCF, 0xFC, 0xF8, 0xB3, 0x2C, 0x43, 0xE7, 0x22, 0x52, 0x56, 0xC4, 0xF8, 0x37,
	0xA8, 0x65, 0x48, 0xC9, 0x2C, 0xCC, 0x35, 0x48, 0x08, 0x05, 0x98, 0x7C, 0xB7, 0x0B, 0xE1, 0x7B,
	0x4D
};

// Custom 5: P="" (empty), S="salt", c=1, dkLen=32
static const uint8_t FL_PBKDF2_UT_EXPECTED_EMPTY_PASSWORD[FL_SHA256_DIGEST_SIZE] =
{
	0xF1, 0x35, 0xC2, 0x79, 0x93, 0xBA, 0xF9, 0x87, 0x73, 0xC5, 0xCD, 0xB4, 0x0A, 0x57, 0x06, 0xCE,
	0x6A, 0x34, 0x5C, 0xDE, 0x61, 0xB0, 0x00, 0xA6, 0x78, 0x58, 0x65, 0x0C, 0xD6, 0xA3, 0x24, 0xD7
};

// Custom 6: P="password", S="" (empty), c=1, dkLen=32
static const uint8_t FL_PBKDF2_UT_EXPECTED_EMPTY_SALT[FL_SHA256_DIGEST_SIZE] =
{
	0xC1, 0x23, 0x2F, 0x10, 0xF6, 0x27, 0x15, 0xFD, 0xA0, 0x6A, 0xE7, 0xC0, 0xA2, 0x03, 0x7C, 0xA1,
	0x9B, 0x33, 0xCF, 0x10, 0x3B, 0x72, 0x7B, 0xA5, 0x6D, 0x87, 0x0C, 0x11, 0xF2, 0x90, 0xA2, 0xAB
};

// Custom 7: P="password", S="Test data of Santtu S. Nyman", c=1000, dkLen=32
static const uint8_t FL_PBKDF2_UT_EXPECTED_SANTTU_TEST_DATA[FL_SHA256_DIGEST_SIZE] =
{
	0xBA, 0xAE, 0xC1, 0xCA, 0xFD, 0x85, 0x32, 0x0B, 0xE6, 0x01, 0xAA, 0x09, 0x5C, 0xEB, 0x36, 0x7C,
	0xC1, 0x79, 0x25, 0xA7, 0xD2, 0x45, 0x7B, 0x3C, 0x34, 0x6F, 0xD2, 0x5D, 0xB6, 0x63, 0x43, 0xE3
};

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

// RFC 7914 TC1: P="passwd", S="salt", c=1, dkLen=64.
static void FlPbkdf2Sha256HmacUtRfc7914Tc1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "passwd";
	static const uint8_t salt[]     = "salt";
	uint8_t dk[64];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_RFC7914_TC1, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtRfc7914Tc1");
}

// P="password", S="salt", c=1, dkLen=32: exactly one block, single iteration.
static void FlPbkdf2Sha256HmacUtSingleBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_SINGLE_BLOCK, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtSingleBlock");
}

// P="password", S="salt", c=1000, dkLen=32: iteration XOR accumulation is correct.
static void FlPbkdf2Sha256HmacUtMultipleIterations(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1000, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_MULTI_ITER, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtMultipleIterations");
}

// P="password", S="salt", c=1, dkLen=1: only the first output byte is written.
static void FlPbkdf2Sha256HmacUtSubBlockOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk[1];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_SUB_BLOCK, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtSubBlockOutput");
}

// P="password", S="salt", c=1, dkLen=33: output straddles the first/second block boundary.
static void FlPbkdf2Sha256HmacUtCrossBlockBoundary(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk[33];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_CROSS_BLOCK, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtCrossBlockBoundary");
}

// Empty password: P="", S="salt", c=1, dkLen=32.
static void FlPbkdf2Sha256HmacUtEmptyPassword(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t emptyPassword[1] = { 0 };
	static const uint8_t salt[] = "salt";
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, 0, emptyPassword, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_EMPTY_PASSWORD, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtEmptyPassword");
}

// Empty salt: P="password", S="", c=1, dkLen=32.
static void FlPbkdf2Sha256HmacUtEmptySalt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t emptySalt[1] = { 0 };
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, 0, emptySalt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_EMPTY_SALT, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtEmptySalt");
}

// Custom string vector: P="password", S="Test data of Santtu S. Nyman", c=1000, dkLen=32.
static void FlPbkdf2Sha256HmacUtSanttuTestData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "Test data of Santtu S. Nyman";
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1000, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_EXPECTED_SANTTU_TEST_DATA, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtSanttuTestData");
}

// IterationCount=0: output buffer must be entirely zeroed, regardless of password/salt.
static void FlPbkdf2Sha256HmacUtZeroIterations(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	static const uint8_t zeros[FL_SHA256_DIGEST_SIZE] = { 0 };
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	memset(dk, 0xAB, sizeof dk);
	FlPbkdf2Sha256Hmac(0, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, zeros, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtZeroIterations");
}

// Same inputs must always produce the same derived key (no hidden mutable state).
static void FlPbkdf2Sha256HmacUtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk1[FL_SHA256_DIGEST_SIZE];
	uint8_t dk2[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk1, dk1);
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk2, dk2);
	FL_UT_CHECK(memcmp(dk1, dk2, sizeof dk1) == 0, "FlPbkdf2Sha256HmacUtDeterminism");
}

// Different passwords must produce different derived keys.
static void FlPbkdf2Sha256HmacUtDifferentPasswordsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t passwordA[] = "password";
	static const uint8_t passwordB[] = "Password";
	static const uint8_t salt[]      = "salt";
	uint8_t dkA[FL_SHA256_DIGEST_SIZE];
	uint8_t dkB[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, sizeof(passwordA) - 1, passwordA, sizeof(salt) - 1, salt, sizeof dkA, dkA);
	FlPbkdf2Sha256Hmac(1, sizeof(passwordB) - 1, passwordB, sizeof(salt) - 1, salt, sizeof dkB, dkB);
	FL_UT_CHECK(memcmp(dkA, dkB, sizeof dkA) != 0, "FlPbkdf2Sha256HmacUtDifferentPasswordsDifferentOutput");
}

// Different salts must produce different derived keys.
static void FlPbkdf2Sha256HmacUtDifferentSaltsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t saltA[]    = "salt";
	static const uint8_t saltB[]    = "pepper";
	uint8_t dkA[FL_SHA256_DIGEST_SIZE];
	uint8_t dkB[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(saltA) - 1, saltA, sizeof dkA, dkA);
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(saltB) - 1, saltB, sizeof dkB, dkB);
	FL_UT_CHECK(memcmp(dkA, dkB, sizeof dkA) != 0, "FlPbkdf2Sha256HmacUtDifferentSaltsDifferentOutput");
}

// Different iteration counts must produce different derived keys.
static void FlPbkdf2Sha256HmacUtDifferentIterationsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk1[FL_SHA256_DIGEST_SIZE];
	uint8_t dk2[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1,    sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk1, dk1);
	FlPbkdf2Sha256Hmac(1000, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk2, dk2);
	FL_UT_CHECK(memcmp(dk1, dk2, sizeof dk1) != 0, "FlPbkdf2Sha256HmacUtDifferentIterationsDifferentOutput");
}

// The first 32 bytes of a 64-byte derivation must equal the 32-byte derivation with the same inputs.
// This verifies that the block counter and truncation are correct.
static void FlPbkdf2Sha256HmacUtOutputPrefixConsistency(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t password[] = "password";
	static const uint8_t salt[]     = "salt";
	uint8_t dk32[FL_SHA256_DIGEST_SIZE];
	uint8_t dk64[64];
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk32, dk32);
	FlPbkdf2Sha256Hmac(1, sizeof(password) - 1, password, sizeof(salt) - 1, salt, sizeof dk64, dk64);
	FL_UT_CHECK(memcmp(dk32, dk64, FL_SHA256_DIGEST_SIZE) == 0, "FlPbkdf2Sha256HmacUtOutputPrefixConsistency");
}

// Case 0: P="Raine" (binary), S="1234" (binary), c=1, dkLen=32
static const uint8_t FL_PBKDF2_UT_CASE0_PASSWORD[5] =
{
	0x52, 0x61, 0x69, 0x6E, 0x65
};

static const uint8_t FL_PBKDF2_UT_CASE0_SALT[4] =
{
	0x31, 0x32, 0x33, 0x34
};

static const uint8_t FL_PBKDF2_UT_CASE0_EXPECTED[FL_SHA256_DIGEST_SIZE] =
{
	0x36, 0x41, 0x5F, 0x73, 0x55, 0xC4, 0x11, 0x58, 0x9C, 0x29, 0xD8, 0x22, 0x83, 0x5D, 0x9F, 0x80,
	0xF4, 0xC6, 0x12, 0x3C, 0x56, 0xE2, 0x84, 0x4B, 0xDF, 0xA2, 0x86, 0x5E, 0x5E, 0x56, 0x48, 0xA7
};

// Case 1-3 share password and salt: P="Hello world" (binary), S="random16bytesalt" (binary), c=100
static const uint8_t FL_PBKDF2_UT_CASE1_PASSWORD[11] =
{
	0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64
};

static const uint8_t FL_PBKDF2_UT_CASE1_SALT[16] =
{
	0x72, 0x61, 0x6E, 0x64, 0x6F, 0x6D, 0x31, 0x36, 0x62, 0x79, 0x74, 0x65, 0x73, 0x61, 0x6C, 0x74
};

static const uint8_t FL_PBKDF2_UT_CASE1_EXPECTED[1] =
{
	0xE8
};

static const uint8_t FL_PBKDF2_UT_CASE2_EXPECTED[16] =
{
	0xE8, 0xE4, 0xA5, 0x98, 0x89, 0xDC, 0x82, 0x69, 0xDB, 0xD4, 0x55, 0x79, 0x7A, 0x18, 0x65, 0x8D
};

static const uint8_t FL_PBKDF2_UT_CASE3_EXPECTED[64] =
{
	0xE8, 0xE4, 0xA5, 0x98, 0x89, 0xDC, 0x82, 0x69, 0xDB, 0xD4, 0x55, 0x79, 0x7A, 0x18, 0x65, 0x8D,
	0x20, 0x34, 0x39, 0x2E, 0x11, 0x6C, 0xDE, 0x07, 0x6E, 0x5C, 0x4D, 0xEE, 0xE1, 0x6D, 0x8E, 0x2A,
	0x20, 0x34, 0x08, 0x3E, 0x8B, 0xC6, 0xF5, 0x0D, 0xC1, 0xF4, 0x3F, 0x03, 0x9C, 0xFA, 0xE6, 0xB0,
	0x15, 0x28, 0x0E, 0x2A, 0x42, 0xCA, 0x57, 0x14, 0xF3, 0xD3, 0x4B, 0x1F, 0x43, 0x13, 0xA1, 0x60
};

// Case 4: P="Santtu's test data" (binary), S="12345678" (binary), c=1000, dkLen=32
static const uint8_t FL_PBKDF2_UT_CASE4_PASSWORD[18] =
{
	0x53, 0x61, 0x6E, 0x74, 0x74, 0x75, 0x27, 0x73, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61,
	0x74, 0x61
};

static const uint8_t FL_PBKDF2_UT_CASE4_SALT[8] =
{
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38
};

static const uint8_t FL_PBKDF2_UT_CASE4_EXPECTED[FL_SHA256_DIGEST_SIZE] =
{
	0x46, 0x35, 0xAF, 0x2E, 0x07, 0x36, 0xDA, 0xB9, 0x80, 0x25, 0x99, 0xE8, 0xE6, 0xDA, 0x84, 0xB4,
	0x13, 0x5C, 0x8D, 0xA7, 0x61, 0xC9, 0xEE, 0xFD, 0x42, 0xF8, 0xE2, 0x72, 0xBC, 0x39, 0x19, 0xBB
};

// P="Raine", S="1234", c=1, dkLen=32.
static void FlPbkdf2Sha256HmacUtCase0(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1, sizeof FL_PBKDF2_UT_CASE0_PASSWORD, FL_PBKDF2_UT_CASE0_PASSWORD,
	                   sizeof FL_PBKDF2_UT_CASE0_SALT, FL_PBKDF2_UT_CASE0_SALT, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_CASE0_EXPECTED, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtCase0");
}

// P="Hello world", S="random16bytesalt", c=100, dkLen=1: sub-block output.
static void FlPbkdf2Sha256HmacUtCase1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t dk[1];
	FlPbkdf2Sha256Hmac(100, sizeof FL_PBKDF2_UT_CASE1_PASSWORD, FL_PBKDF2_UT_CASE1_PASSWORD,
	                   sizeof FL_PBKDF2_UT_CASE1_SALT, FL_PBKDF2_UT_CASE1_SALT, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_CASE1_EXPECTED, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtCase1");
}

// P="Hello world", S="random16bytesalt", c=100, dkLen=16: half-block output.
static void FlPbkdf2Sha256HmacUtCase2(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t dk[16];
	FlPbkdf2Sha256Hmac(100, sizeof FL_PBKDF2_UT_CASE1_PASSWORD, FL_PBKDF2_UT_CASE1_PASSWORD,
	                   sizeof FL_PBKDF2_UT_CASE1_SALT, FL_PBKDF2_UT_CASE1_SALT, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_CASE2_EXPECTED, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtCase2");
}

// P="Hello world", S="random16bytesalt", c=100, dkLen=64: two full blocks.
static void FlPbkdf2Sha256HmacUtCase3(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t dk[64];
	FlPbkdf2Sha256Hmac(100, sizeof FL_PBKDF2_UT_CASE1_PASSWORD, FL_PBKDF2_UT_CASE1_PASSWORD,
	                   sizeof FL_PBKDF2_UT_CASE1_SALT, FL_PBKDF2_UT_CASE1_SALT, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_CASE3_EXPECTED, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtCase3");
}

// P="Santtu's test data", S="12345678", c=1000, dkLen=32.
static void FlPbkdf2Sha256HmacUtCase4(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t dk[FL_SHA256_DIGEST_SIZE];
	FlPbkdf2Sha256Hmac(1000, sizeof FL_PBKDF2_UT_CASE4_PASSWORD, FL_PBKDF2_UT_CASE4_PASSWORD,
	                   sizeof FL_PBKDF2_UT_CASE4_SALT, FL_PBKDF2_UT_CASE4_SALT, sizeof dk, dk);
	FL_UT_CHECK(memcmp(dk, FL_PBKDF2_UT_CASE4_EXPECTED, sizeof dk) == 0, "FlPbkdf2Sha256HmacUtCase4");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlPbkdf2Sha256HmacUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlPbkdf2Sha256HmacUtRfc7914Tc1(testCount, failCount);
	FlPbkdf2Sha256HmacUtSingleBlock(testCount, failCount);
	FlPbkdf2Sha256HmacUtMultipleIterations(testCount, failCount);
	FlPbkdf2Sha256HmacUtSubBlockOutput(testCount, failCount);
	FlPbkdf2Sha256HmacUtCrossBlockBoundary(testCount, failCount);
	FlPbkdf2Sha256HmacUtEmptyPassword(testCount, failCount);
	FlPbkdf2Sha256HmacUtEmptySalt(testCount, failCount);
	FlPbkdf2Sha256HmacUtSanttuTestData(testCount, failCount);
	FlPbkdf2Sha256HmacUtZeroIterations(testCount, failCount);
	FlPbkdf2Sha256HmacUtDeterminism(testCount, failCount);
	FlPbkdf2Sha256HmacUtDifferentPasswordsDifferentOutput(testCount, failCount);
	FlPbkdf2Sha256HmacUtDifferentSaltsDifferentOutput(testCount, failCount);
	FlPbkdf2Sha256HmacUtDifferentIterationsDifferentOutput(testCount, failCount);
	FlPbkdf2Sha256HmacUtOutputPrefixConsistency(testCount, failCount);
	FlPbkdf2Sha256HmacUtCase0(testCount, failCount);
	FlPbkdf2Sha256HmacUtCase1(testCount, failCount);
	FlPbkdf2Sha256HmacUtCase2(testCount, failCount);
	FlPbkdf2Sha256HmacUtCase3(testCount, failCount);
	FlPbkdf2Sha256HmacUtCase4(testCount, failCount);
}
