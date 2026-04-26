/*
	SHA-256 unit tests by Santtu S. Nyman.

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

		Unit tests for FlSha256CreateHash, FlSha256HashData and FlSha256FinishHash.

		Reference digests were obtained with "certutil -hashfile <file> SHA256" on
		Windows.  The well-known empty-input digest (FIPS 180-4) was verified
		separately using the .NET SHA256 implementation because certutil rejects
		zero-length files.
*/

#include "FlUt.h"
#include "../include/FlSha256.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Expected SHA-256 digests for known inputs (verified with certutil / FIPS 180-4).
// ---------------------------------------------------------------------------

// SHA256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  (FIPS 180-4)
static const uint8_t FL_SHA256_UT_EXPECTED_EMPTY[FL_SHA256_DIGEST_SIZE] =
{
	0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14,
	0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24,
	0x27, 0xAE, 0x41, 0xE4, 0x64, 0x9B, 0x93, 0x4C,
	0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55
};

// SHA256("The quick brown fox jumps over the lazy dog") = d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592
static const uint8_t FL_SHA256_UT_EXPECTED_QUICK_BROWN_FOX[FL_SHA256_DIGEST_SIZE] =
{
	0xD7, 0xA8, 0xFB, 0xB3, 0x07, 0xD7, 0x80, 0x94,
	0x69, 0xCA, 0x9A, 0xBC, 0xB0, 0x08, 0x2E, 0x4F,
	0x8D, 0x56, 0x51, 0xE4, 0x6D, 0x3C, 0xDB, 0x76,
	0x2D, 0x02, 0xD0, 0xBF, 0x37, 0xC9, 0xE5, 0x92
};

// SHA256("Hello world") = 64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c
static const uint8_t FL_SHA256_UT_EXPECTED_HELLO_WORLD[FL_SHA256_DIGEST_SIZE] =
{
	0x64, 0xEC, 0x88, 0xCA, 0x00, 0xB2, 0x68, 0xE5,
	0xBA, 0x1A, 0x35, 0x67, 0x8A, 0x1B, 0x53, 0x16,
	0xD2, 0x12, 0xF4, 0xF3, 0x66, 0xB2, 0x47, 0x72,
	0x32, 0x53, 0x4A, 0x8A, 0xEC, 0xA3, 0x7F, 0x3C
};

// SHA256("a") = ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb
static const uint8_t FL_SHA256_UT_EXPECTED_BYTE_A[FL_SHA256_DIGEST_SIZE] =
{
	0xCA, 0x97, 0x81, 0x12, 0xCA, 0x1B, 0xBD, 0xCA,
	0xFA, 0xC2, 0x31, 0xB3, 0x9A, 0x23, 0xDC, 0x4D,
	0xA7, 0x86, 0xEF, 0xF8, 0x14, 0x7C, 0x4E, 0x72,
	0xB9, 0x80, 0x77, 0x85, 0xAF, 0xEE, 0x48, 0xBB
};

// SHA256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad  (FIPS 180-4)
static const uint8_t FL_SHA256_UT_EXPECTED_ABC[FL_SHA256_DIGEST_SIZE] =
{
	0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
	0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
	0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
	0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
};

// SHA256("123456789") = 15e2b0d3c33891ebb0f1ef609ec419420c20e320ce94c65fbc8c3312448eb225
static const uint8_t FL_SHA256_UT_EXPECTED_123456789[FL_SHA256_DIGEST_SIZE] =
{
	0x15, 0xE2, 0xB0, 0xD3, 0xC3, 0x38, 0x91, 0xEB,
	0xB0, 0xF1, 0xEF, 0x60, 0x9E, 0xC4, 0x19, 0x42,
	0x0C, 0x20, 0xE3, 0x20, 0xCE, 0x94, 0xC6, 0x5F,
	0xBC, 0x8C, 0x33, 0x12, 0x44, 0x8E, 0xB2, 0x25
};

// SHA256("abcdefghijklmnopqrstuvwxyz") = 71c480df93d6ae2f1efad1447c66c9525e316218cf51fc8d9ed832f2daf18b73
static const uint8_t FL_SHA256_UT_EXPECTED_LOWERCASE_ALPHABET[FL_SHA256_DIGEST_SIZE] =
{
	0x71, 0xC4, 0x80, 0xDF, 0x93, 0xD6, 0xAE, 0x2F,
	0x1E, 0xFA, 0xD1, 0x44, 0x7C, 0x66, 0xC9, 0x52,
	0x5E, 0x31, 0x62, 0x18, 0xCF, 0x51, 0xFC, 0x8D,
	0x9E, 0xD8, 0x32, 0xF2, 0xDA, 0xF1, 0x8B, 0x73
};

// SHA256("Test data of Santtu S. Nyman") = 2c36705757186d5d8bac6a885a5649854452e20c29a359cc0324fc6192002dbc
static const uint8_t FL_SHA256_UT_EXPECTED_SANTTU_TEST_DATA[FL_SHA256_DIGEST_SIZE] =
{
	0x2C, 0x36, 0x70, 0x57, 0x57, 0x18, 0x6D, 0x5D,
	0x8B, 0xAC, 0x6A, 0x88, 0x5A, 0x56, 0x49, 0x85,
	0x44, 0x52, 0xE2, 0x0C, 0x29, 0xA3, 0x59, 0xCC,
	0x03, 0x24, 0xFC, 0x61, 0x92, 0x00, 0x2D, 0xBC
};

// SHA256(64 KiB of 0x00 bytes) = de2f256064a0af797747c2b97505dc0b9f3df0de4f489eac731c23ae9ca9cc31
static const uint8_t FL_SHA256_UT_EXPECTED_64KIB_ZERO_BYTES[FL_SHA256_DIGEST_SIZE] =
{
	0xDE, 0x2F, 0x25, 0x60, 0x64, 0xA0, 0xAF, 0x79,
	0x77, 0x47, 0xC2, 0xB9, 0x75, 0x05, 0xDC, 0x0B,
	0x9F, 0x3D, 0xF0, 0xDE, 0x4F, 0x48, 0x9E, 0xAC,
	0x73, 0x1C, 0x23, 0xAE, 0x9C, 0xA9, 0xCC, 0x31
};

// SHA256(64 KiB of 0xFF bytes) = 71189f7fb6aed638640078fba3a35fda6c39c8962e74dcc75935aac948da9063
static const uint8_t FL_SHA256_UT_EXPECTED_64KIB_FF_BYTES[FL_SHA256_DIGEST_SIZE] =
{
	0x71, 0x18, 0x9F, 0x7F, 0xB6, 0xAE, 0xD6, 0x38,
	0x64, 0x00, 0x78, 0xFB, 0xA3, 0xA3, 0x5F, 0xDA,
	0x6C, 0x39, 0xC8, 0x96, 0x2E, 0x74, 0xDC, 0xC7,
	0x59, 0x35, 0xAA, 0xC9, 0x48, 0xDA, 0x90, 0x63
};

// ---------------------------------------------------------------------------
// Helper: compute SHA-256 of a buffer in a single shot.
// ---------------------------------------------------------------------------
static void FlSha256Compute(_In_ size_t dataSize, _In_reads_bytes_(dataSize) const void* data, _Out_writes_bytes_all_(FL_SHA256_DIGEST_SIZE) uint8_t* digest)
{
	FlSha256Context ctx;
	FlSha256CreateHash(&ctx);
	FlSha256HashData(&ctx, dataSize, data);
	FlSha256FinishHash(&ctx, digest);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

// SHA-256 of empty input must equal the FIPS 180-4 reference value.
static void FlSha256UtEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t emptyData[1] = { 0 };
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(0, emptyData, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_EMPTY, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtEmptyInput");
}

// SHA-256("The quick brown fox jumps over the lazy dog").
static void FlSha256UtQuickBrownFox(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_QUICK_BROWN_FOX, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtQuickBrownFox");
}

// SHA-256("Hello world").
static void FlSha256UtHelloWorld(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Hello world";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_HELLO_WORLD, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtHelloWorld");
}

// SHA-256("a").
static void FlSha256UtSingleByteA(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { 'a' };
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof data, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_BYTE_A, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtSingleByteA");
}

// SHA-256("abc") — FIPS 180-4 reference value.
static void FlSha256UtStringAbc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abc";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_ABC, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtStringAbc");
}

// SHA-256("123456789").
static void FlSha256UtDigitString(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "123456789";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_123456789, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtDigitString");
}

// SHA-256("abcdefghijklmnopqrstuvwxyz").
static void FlSha256UtLowercaseAlphabet(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abcdefghijklmnopqrstuvwxyz";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_LOWERCASE_ALPHABET, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtLowercaseAlphabet");
}

// SHA-256("Test data of Santtu S. Nyman").
static void FlSha256UtSanttuTestData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Test data of Santtu S. Nyman";
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_SANTTU_TEST_DATA, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtSanttuTestData");
}

// SHA-256(64 KiB of 0x00 bytes).
static void FlSha256Ut64KiBZeroBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t zeroByte = 0x00;
	FlSha256Context ctx;
	FlSha256CreateHash(&ctx);
	for (size_t i = 0; i < 65536; i++)
		FlSha256HashData(&ctx, 1, &zeroByte);
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256FinishHash(&ctx, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_64KIB_ZERO_BYTES, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256Ut64KiBZeroBytes");
}

// SHA-256(64 KiB of 0xFF bytes).
static void FlSha256Ut64KiBFfBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t ffByte = 0xFF;
	FlSha256Context ctx;
	FlSha256CreateHash(&ctx);
	for (size_t i = 0; i < 65536; i++)
		FlSha256HashData(&ctx, 1, &ffByte);
	uint8_t digest[FL_SHA256_DIGEST_SIZE];
	FlSha256FinishHash(&ctx, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA256_UT_EXPECTED_64KIB_FF_BYTES, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256Ut64KiBFfBytes");
}

// Feeding data in multiple chunks must produce the same digest as a single call.
static void FlSha256UtStreamingEquality(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t wholeDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, wholeDigest);

	// Split 1 + remainder.
	{
		FlSha256Context ctx;
		FlSha256CreateHash(&ctx);
		FlSha256HashData(&ctx, 1, data);
		FlSha256HashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtStreamingEquality_1_rest");
	}

	// Split at midpoint.
	{
		size_t mid = (sizeof(data) - 1) / 2;
		FlSha256Context ctx;
		FlSha256CreateHash(&ctx);
		FlSha256HashData(&ctx, mid, data);
		FlSha256HashData(&ctx, sizeof(data) - 1 - mid, data + mid);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtStreamingEquality_mid");
	}

	// One byte at a time.
	{
		FlSha256Context ctx;
		FlSha256CreateHash(&ctx);
		for (size_t n = sizeof(data) - 1, i = 0; i < n; i++)
			FlSha256HashData(&ctx, 1, data + i);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtStreamingEquality_1x43");
	}

	// Split at every possible position within a short input.
	{
		static const uint8_t shortData[] = "abcd";
		uint8_t expectedShort[FL_SHA256_DIGEST_SIZE];
		FlSha256Compute(sizeof(shortData) - 1, shortData, expectedShort);
		for (size_t n = sizeof(shortData) - 1, splitAt = 1; splitAt < n; splitAt++)
		{
			FlSha256Context ctx;
			FlSha256CreateHash(&ctx);
			FlSha256HashData(&ctx, splitAt, shortData);
			FlSha256HashData(&ctx, n - splitAt, shortData + splitAt);
			uint8_t digest[FL_SHA256_DIGEST_SIZE];
			FlSha256FinishHash(&ctx, digest);
			FL_UT_CHECK(memcmp(digest, expectedShort, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtStreamingEquality_AllSplits");
		}
	}
}

// Passing a zero-length chunk must not alter the digest.
static void FlSha256UtZeroLengthChunkNoEffect(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abc";
	uint8_t expected[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, expected);

	// Zero-length call before actual data.
	{
		FlSha256Context ctx;
		FlSha256CreateHash(&ctx);
		FlSha256HashData(&ctx, 0, data);
		FlSha256HashData(&ctx, sizeof(data) - 1, data);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtZeroLengthChunkNoEffect_Before");
	}

	// Zero-length call after actual data.
	{
		FlSha256Context ctx;
		FlSha256CreateHash(&ctx);
		FlSha256HashData(&ctx, sizeof(data) - 1, data);
		FlSha256HashData(&ctx, 0, data);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtZeroLengthChunkNoEffect_After");
	}

	// Zero-length call in the middle of actual data.
	{
		FlSha256Context ctx;
		FlSha256CreateHash(&ctx);
		FlSha256HashData(&ctx, 1, data);
		FlSha256HashData(&ctx, 0, data);
		FlSha256HashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_SHA256_DIGEST_SIZE];
		FlSha256FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtZeroLengthChunkNoEffect_Middle");
	}
}

// Same input must always produce the same digest (no hidden mutable state).
static void FlSha256UtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t firstDigest[FL_SHA256_DIGEST_SIZE];
	uint8_t secondDigest[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(data) - 1, data, firstDigest);
	FlSha256Compute(sizeof(data) - 1, data, secondDigest);
	FL_UT_CHECK(memcmp(firstDigest, secondDigest, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtDeterminism");
}

// Two known-distinct inputs must produce different digests.
static void FlSha256UtDifferentInputsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = { 'a' };
	static const uint8_t dataB[] = { 'b' };
	uint8_t digestA[FL_SHA256_DIGEST_SIZE];
	uint8_t digestB[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof dataA, dataA, digestA);
	FlSha256Compute(sizeof dataB, dataB, digestB);
	FL_UT_CHECK(memcmp(digestA, digestB, FL_SHA256_DIGEST_SIZE) != 0, "FlSha256UtDifferentInputsDifferentOutput_1byte");

	static const uint8_t dataC[] = "abc";
	static const uint8_t dataD[] = "cba";
	uint8_t digestC[FL_SHA256_DIGEST_SIZE];
	uint8_t digestD[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(dataC) - 1, dataC, digestC);
	FlSha256Compute(sizeof(dataD) - 1, dataD, digestD);
	FL_UT_CHECK(memcmp(digestC, digestD, FL_SHA256_DIGEST_SIZE) != 0, "FlSha256UtDifferentInputsDifferentOutput_Reversed");
}

// Two interleaved SHA-256 computations must not interfere with each other.
static void FlSha256UtContextIsolation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = "abc";
	static const uint8_t dataB[] = "123";
	uint8_t expectedA[FL_SHA256_DIGEST_SIZE];
	uint8_t expectedB[FL_SHA256_DIGEST_SIZE];
	FlSha256Compute(sizeof(dataA) - 1, dataA, expectedA);
	FlSha256Compute(sizeof(dataB) - 1, dataB, expectedB);

	// Interleave byte-by-byte updates to both contexts.
	FlSha256Context ctxA;
	FlSha256Context ctxB;
	FlSha256CreateHash(&ctxA);
	FlSha256CreateHash(&ctxB);
	for (size_t n = sizeof(dataA) - 1, i = 0; i < n; i++)
	{
		FlSha256HashData(&ctxA, 1, dataA + i);
		FlSha256HashData(&ctxB, 1, dataB + i);
	}

	uint8_t digestA[FL_SHA256_DIGEST_SIZE];
	uint8_t digestB[FL_SHA256_DIGEST_SIZE];
	FlSha256FinishHash(&ctxA, digestA);
	FlSha256FinishHash(&ctxB, digestB);
	FL_UT_CHECK(memcmp(digestA, expectedA, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtContextIsolation_A");
	FL_UT_CHECK(memcmp(digestB, expectedB, FL_SHA256_DIGEST_SIZE) == 0, "FlSha256UtContextIsolation_B");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlSha256UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlSha256UtEmptyInput(testCount, failCount);
	FlSha256UtQuickBrownFox(testCount, failCount);
	FlSha256UtHelloWorld(testCount, failCount);
	FlSha256UtSingleByteA(testCount, failCount);
	FlSha256UtStringAbc(testCount, failCount);
	FlSha256UtDigitString(testCount, failCount);
	FlSha256UtLowercaseAlphabet(testCount, failCount);
	FlSha256UtSanttuTestData(testCount, failCount);
	FlSha256Ut64KiBZeroBytes(testCount, failCount);
	FlSha256Ut64KiBFfBytes(testCount, failCount);
	FlSha256UtStreamingEquality(testCount, failCount);
	FlSha256UtZeroLengthChunkNoEffect(testCount, failCount);
	FlSha256UtDeterminism(testCount, failCount);
	FlSha256UtDifferentInputsDifferentOutput(testCount, failCount);
	FlSha256UtContextIsolation(testCount, failCount);
}
