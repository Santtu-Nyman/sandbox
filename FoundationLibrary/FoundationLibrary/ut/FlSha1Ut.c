/*
	SHA1 unit tests by Santtu S. Nyman.

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

		Unit tests for FlSha1CreateHash, FlSha1HashData and FlSha1FinishHash.

		Reference digests were obtained with "certutil -hashfile <file> SHA1" on
		Windows.  The well-known empty-input digest (FIPS 180-4) was verified
		separately using the .NET SHA1 implementation because certutil rejects
		zero-length files.
*/

#include "FlUt.h"
#include "../include/FlSha1.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Expected SHA1 digests for known inputs (verified with certutil / FIPS 180-4).
// ---------------------------------------------------------------------------

// SHA1("") = da39a3ee5e6b4b0d3255bfef95601890afd80709  (FIPS 180-4 example)
static const uint8_t FL_SHA1_UT_EXPECTED_EMPTY[FL_SHA1_DIGEST_SIZE] =
{
	0xDA, 0x39, 0xA3, 0xEE, 0x5E, 0x6B, 0x4B, 0x0D,
	0x32, 0x55, 0xBF, 0xEF, 0x95, 0x60, 0x18, 0x90,
	0xAF, 0xD8, 0x07, 0x09
};

// SHA1("The quick brown fox jumps over the lazy dog") = 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12
static const uint8_t FL_SHA1_UT_EXPECTED_QUICK_BROWN_FOX[FL_SHA1_DIGEST_SIZE] =
{
	0x2F, 0xD4, 0xE1, 0xC6, 0x7A, 0x2D, 0x28, 0xFC,
	0xED, 0x84, 0x9E, 0xE1, 0xBB, 0x76, 0xE7, 0x39,
	0x1B, 0x93, 0xEB, 0x12
};

// SHA1("Hello world") = 7b502c3a1f48c8609ae212cdfb639dee39673f5e
static const uint8_t FL_SHA1_UT_EXPECTED_HELLO_WORLD[FL_SHA1_DIGEST_SIZE] =
{
	0x7B, 0x50, 0x2C, 0x3A, 0x1F, 0x48, 0xC8, 0x60,
	0x9A, 0xE2, 0x12, 0xCD, 0xFB, 0x63, 0x9D, 0xEE,
	0x39, 0x67, 0x3F, 0x5E
};

// SHA1("a") = 86f7e437faa5a7fce15d1ddcb9eaeaea377667b8
static const uint8_t FL_SHA1_UT_EXPECTED_BYTE_A[FL_SHA1_DIGEST_SIZE] =
{
	0x86, 0xF7, 0xE4, 0x37, 0xFA, 0xA5, 0xA7, 0xFC,
	0xE1, 0x5D, 0x1D, 0xDC, 0xB9, 0xEA, 0xEA, 0xEA,
	0x37, 0x76, 0x67, 0xB8
};

// SHA1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d  (FIPS 180-4 example)
static const uint8_t FL_SHA1_UT_EXPECTED_ABC[FL_SHA1_DIGEST_SIZE] =
{
	0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A,
	0xBA, 0x3E, 0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C,
	0x9C, 0xD0, 0xD8, 0x9D
};

// SHA1("123456789") = f7c3bc1d808e04732adf679965ccc34ca7ae3441
static const uint8_t FL_SHA1_UT_EXPECTED_123456789[FL_SHA1_DIGEST_SIZE] =
{
	0xF7, 0xC3, 0xBC, 0x1D, 0x80, 0x8E, 0x04, 0x73,
	0x2A, 0xDF, 0x67, 0x99, 0x65, 0xCC, 0xC3, 0x4C,
	0xA7, 0xAE, 0x34, 0x41
};

// SHA1("abcdefghijklmnopqrstuvwxyz") = 32d10c7b8cf96570ca04ce37f2a19d84240d3a89
static const uint8_t FL_SHA1_UT_EXPECTED_LOWERCASE_ALPHABET[FL_SHA1_DIGEST_SIZE] =
{
	0x32, 0xD1, 0x0C, 0x7B, 0x8C, 0xF9, 0x65, 0x70,
	0xCA, 0x04, 0xCE, 0x37, 0xF2, 0xA1, 0x9D, 0x84,
	0x24, 0x0D, 0x3A, 0x89
};

// SHA1("Test data of Santtu S. Nyman") = 193ed843c41fbfdc6e48114939c70e940ee68399
static const uint8_t FL_SHA1_UT_EXPECTED_SANTTU_TEST_DATA[FL_SHA1_DIGEST_SIZE] =
{
	0x19, 0x3E, 0xD8, 0x43, 0xC4, 0x1F, 0xBF, 0xDC,
	0x6E, 0x48, 0x11, 0x49, 0x39, 0xC7, 0x0E, 0x94,
	0x0E, 0xE6, 0x83, 0x99
};

// SHA1(64 KiB of 0x00 bytes) = 1adc95bebe9eea8c112d40cd04ab7a8d75c4f961
static const uint8_t FL_SHA1_UT_EXPECTED_64KIB_ZERO_BYTES[FL_SHA1_DIGEST_SIZE] =
{
	0x1A, 0xDC, 0x95, 0xBE, 0xBE, 0x9E, 0xEA, 0x8C,
	0x11, 0x2D, 0x40, 0xCD, 0x04, 0xAB, 0x7A, 0x8D,
	0x75, 0xC4, 0xF9, 0x61
};

// SHA1(64 KiB of 0xFF bytes) = 472a55b0ba289b0f4e538bb4c8b826dede3a40bb
static const uint8_t FL_SHA1_UT_EXPECTED_64KIB_FF_BYTES[FL_SHA1_DIGEST_SIZE] =
{
	0x47, 0x2A, 0x55, 0xB0, 0xBA, 0x28, 0x9B, 0x0F,
	0x4E, 0x53, 0x8B, 0xB4, 0xC8, 0xB8, 0x26, 0xDE,
	0xDE, 0x3A, 0x40, 0xBB
};

// ---------------------------------------------------------------------------
// Helper: compute SHA1 of a buffer in a single shot.
// ---------------------------------------------------------------------------
static void FlSha1Compute(_In_ size_t dataSize, _In_reads_bytes_(dataSize) const void* data, _Out_writes_bytes_all_(FL_SHA1_DIGEST_SIZE) uint8_t* digest)
{
	FlSha1Context ctx;
	FlSha1CreateHash(&ctx);
	FlSha1HashData(&ctx, dataSize, data);
	FlSha1FinishHash(&ctx, digest);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

// SHA1 of empty input must equal the FIPS 180-4 reference value.
static void FlSha1UtEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t emptyData[1] = { 0 };
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(0, emptyData, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_EMPTY, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtEmptyInput");
}

// SHA1("The quick brown fox jumps over the lazy dog").
static void FlSha1UtQuickBrownFox(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_QUICK_BROWN_FOX, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtQuickBrownFox");
}

// SHA1("Hello world").
static void FlSha1UtHelloWorld(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Hello world";
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_HELLO_WORLD, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtHelloWorld");
}

// SHA1("a").
static void FlSha1UtSingleByteA(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { 'a' };
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof data, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_BYTE_A, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtSingleByteA");
}

// SHA1("abc") — FIPS 180-4 reference value.
static void FlSha1UtStringAbc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abc";
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_ABC, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtStringAbc");
}

// SHA1("123456789").
static void FlSha1UtDigitString(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "123456789";
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_123456789, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtDigitString");
}

// SHA1("abcdefghijklmnopqrstuvwxyz").
static void FlSha1UtLowercaseAlphabet(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abcdefghijklmnopqrstuvwxyz";
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_LOWERCASE_ALPHABET, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtLowercaseAlphabet");
}

// SHA1("Test data of Santtu S. Nyman").
static void FlSha1UtSanttuTestData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Test data of Santtu S. Nyman";
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_SANTTU_TEST_DATA, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtSanttuTestData");
}

// SHA1(64 KiB of 0x00 bytes).
static void FlSha1Ut64KiBZeroBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t zeroByte = 0x00;
	FlSha1Context ctx;
	FlSha1CreateHash(&ctx);
	for (size_t i = 0; i < 65536; i++)
		FlSha1HashData(&ctx, 1, &zeroByte);
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1FinishHash(&ctx, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_64KIB_ZERO_BYTES, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1Ut64KiBZeroBytes");
}

// SHA1(64 KiB of 0xFF bytes).
static void FlSha1Ut64KiBFfBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t ffByte = 0xFF;
	FlSha1Context ctx;
	FlSha1CreateHash(&ctx);
	for (size_t i = 0; i < 65536; i++)
		FlSha1HashData(&ctx, 1, &ffByte);
	uint8_t digest[FL_SHA1_DIGEST_SIZE];
	FlSha1FinishHash(&ctx, digest);
	FL_UT_CHECK(memcmp(digest, FL_SHA1_UT_EXPECTED_64KIB_FF_BYTES, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1Ut64KiBFfBytes");
}

// Feeding data in multiple chunks must produce the same digest as a single call.
static void FlSha1UtStreamingEquality(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t wholeDigest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, wholeDigest);

	// Split 1 + remainder.
	{
		FlSha1Context ctx;
		FlSha1CreateHash(&ctx);
		FlSha1HashData(&ctx, 1, data);
		FlSha1HashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_SHA1_DIGEST_SIZE];
		FlSha1FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtStreamingEquality_1_rest");
	}

	// Split at midpoint.
	{
		size_t mid = (sizeof(data) - 1) / 2;
		FlSha1Context ctx;
		FlSha1CreateHash(&ctx);
		FlSha1HashData(&ctx, mid, data);
		FlSha1HashData(&ctx, sizeof(data) - 1 - mid, data + mid);
		uint8_t digest[FL_SHA1_DIGEST_SIZE];
		FlSha1FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtStreamingEquality_mid");
	}

	// One byte at a time.
	{
		FlSha1Context ctx;
		FlSha1CreateHash(&ctx);
		for (size_t n = sizeof(data) - 1, i = 0; i < n; i++)
			FlSha1HashData(&ctx, 1, data + i);
		uint8_t digest[FL_SHA1_DIGEST_SIZE];
		FlSha1FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtStreamingEquality_1x43");
	}

	// Split at every possible position within a short input.
	{
		static const uint8_t shortData[] = "abcd";
		uint8_t expectedShort[FL_SHA1_DIGEST_SIZE];
		FlSha1Compute(sizeof(shortData) - 1, shortData, expectedShort);
		for (size_t n = sizeof(shortData) - 1, splitAt = 1; splitAt < n; splitAt++)
		{
			FlSha1Context ctx;
			FlSha1CreateHash(&ctx);
			FlSha1HashData(&ctx, splitAt, shortData);
			FlSha1HashData(&ctx, n - splitAt, shortData + splitAt);
			uint8_t digest[FL_SHA1_DIGEST_SIZE];
			FlSha1FinishHash(&ctx, digest);
			FL_UT_CHECK(memcmp(digest, expectedShort, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtStreamingEquality_AllSplits");
		}
	}
}

// Passing a zero-length chunk must not alter the digest.
static void FlSha1UtZeroLengthChunkNoEffect(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abc";
	uint8_t expected[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, expected);

	// Zero-length call before actual data.
	{
		FlSha1Context ctx;
		FlSha1CreateHash(&ctx);
		FlSha1HashData(&ctx, 0, data);
		FlSha1HashData(&ctx, sizeof(data) - 1, data);
		uint8_t digest[FL_SHA1_DIGEST_SIZE];
		FlSha1FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtZeroLengthChunkNoEffect_Before");
	}

	// Zero-length call after actual data.
	{
		FlSha1Context ctx;
		FlSha1CreateHash(&ctx);
		FlSha1HashData(&ctx, sizeof(data) - 1, data);
		FlSha1HashData(&ctx, 0, data);
		uint8_t digest[FL_SHA1_DIGEST_SIZE];
		FlSha1FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtZeroLengthChunkNoEffect_After");
	}

	// Zero-length call in the middle of actual data.
	{
		FlSha1Context ctx;
		FlSha1CreateHash(&ctx);
		FlSha1HashData(&ctx, 1, data);
		FlSha1HashData(&ctx, 0, data);
		FlSha1HashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_SHA1_DIGEST_SIZE];
		FlSha1FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtZeroLengthChunkNoEffect_Middle");
	}
}

// Same input must always produce the same digest (no hidden mutable state).
static void FlSha1UtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t firstDigest[FL_SHA1_DIGEST_SIZE];
	uint8_t secondDigest[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(data) - 1, data, firstDigest);
	FlSha1Compute(sizeof(data) - 1, data, secondDigest);
	FL_UT_CHECK(memcmp(firstDigest, secondDigest, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtDeterminism");
}

// Two known-distinct inputs must produce different digests.
static void FlSha1UtDifferentInputsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = { 'a' };
	static const uint8_t dataB[] = { 'b' };
	uint8_t digestA[FL_SHA1_DIGEST_SIZE];
	uint8_t digestB[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof dataA, dataA, digestA);
	FlSha1Compute(sizeof dataB, dataB, digestB);
	FL_UT_CHECK(memcmp(digestA, digestB, FL_SHA1_DIGEST_SIZE) != 0, "FlSha1UtDifferentInputsDifferentOutput_1byte");

	static const uint8_t dataC[] = "abc";
	static const uint8_t dataD[] = "cba";
	uint8_t digestC[FL_SHA1_DIGEST_SIZE];
	uint8_t digestD[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(dataC) - 1, dataC, digestC);
	FlSha1Compute(sizeof(dataD) - 1, dataD, digestD);
	FL_UT_CHECK(memcmp(digestC, digestD, FL_SHA1_DIGEST_SIZE) != 0, "FlSha1UtDifferentInputsDifferentOutput_Reversed");
}

// Two interleaved SHA1 computations must not interfere with each other.
static void FlSha1UtContextIsolation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = "abc";
	static const uint8_t dataB[] = "123";
	uint8_t expectedA[FL_SHA1_DIGEST_SIZE];
	uint8_t expectedB[FL_SHA1_DIGEST_SIZE];
	FlSha1Compute(sizeof(dataA) - 1, dataA, expectedA);
	FlSha1Compute(sizeof(dataB) - 1, dataB, expectedB);

	// Interleave byte-by-byte updates to both contexts.
	FlSha1Context ctxA;
	FlSha1Context ctxB;
	FlSha1CreateHash(&ctxA);
	FlSha1CreateHash(&ctxB);
	for (size_t n = sizeof(dataA) - 1, i = 0; i < n; i++)
	{
		FlSha1HashData(&ctxA, 1, dataA + i);
		FlSha1HashData(&ctxB, 1, dataB + i);
	}

	uint8_t digestA[FL_SHA1_DIGEST_SIZE];
	uint8_t digestB[FL_SHA1_DIGEST_SIZE];
	FlSha1FinishHash(&ctxA, digestA);
	FlSha1FinishHash(&ctxB, digestB);
	FL_UT_CHECK(memcmp(digestA, expectedA, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtContextIsolation_A");
	FL_UT_CHECK(memcmp(digestB, expectedB, FL_SHA1_DIGEST_SIZE) == 0, "FlSha1UtContextIsolation_B");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlSha1UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlSha1UtEmptyInput(testCount, failCount);
	FlSha1UtQuickBrownFox(testCount, failCount);
	FlSha1UtHelloWorld(testCount, failCount);
	FlSha1UtSingleByteA(testCount, failCount);
	FlSha1UtStringAbc(testCount, failCount);
	FlSha1UtDigitString(testCount, failCount);
	FlSha1UtLowercaseAlphabet(testCount, failCount);
	FlSha1UtSanttuTestData(testCount, failCount);
	FlSha1Ut64KiBZeroBytes(testCount, failCount);
	FlSha1Ut64KiBFfBytes(testCount, failCount);
	FlSha1UtStreamingEquality(testCount, failCount);
	FlSha1UtZeroLengthChunkNoEffect(testCount, failCount);
	FlSha1UtDeterminism(testCount, failCount);
	FlSha1UtDifferentInputsDifferentOutput(testCount, failCount);
	FlSha1UtContextIsolation(testCount, failCount);
}
