/*
	MD5 unit tests by Santtu S. Nyman.

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

		Unit tests for FlMd5CreateHash, FlMd5HashData and FlMd5FinishHash.

		Reference digests were obtained with "certutil -hashfile <file> MD5" on
		Windows.  The well-known empty-input digest (RFC 1321 Appendix A.5) was
		verified separately because certutil rejects zero-length files.
*/

#include "FlUt.h"
#include "../include/FlMd5.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Expected MD5 digests for known inputs (verified with certutil / RFC 1321).
// ---------------------------------------------------------------------------

// MD5("") = d41d8cd98f00b204e9800998ecf8427e  (RFC 1321 Appendix A.5)
static const uint8_t FL_MD5_UT_EXPECTED_EMPTY[FL_MD5_DIGEST_SIZE] =
{
	0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04,
	0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E
};

// MD5("The quick brown fox jumps over the lazy dog") = 9e107d9d372bb6826bd81d3542a419d6
static const uint8_t FL_MD5_UT_EXPECTED_QUICK_BROWN_FOX[FL_MD5_DIGEST_SIZE] =
{
	0x9E, 0x10, 0x7D, 0x9D, 0x37, 0x2B, 0xB6, 0x82,
	0x6B, 0xD8, 0x1D, 0x35, 0x42, 0xA4, 0x19, 0xD6
};

// MD5("Hello world") = 3e25960a79dbc69b674cd4ec67a72c62
static const uint8_t FL_MD5_UT_EXPECTED_HELLO_WORLD[FL_MD5_DIGEST_SIZE] =
{
	0x3E, 0x25, 0x96, 0x0A, 0x79, 0xDB, 0xC6, 0x9B,
	0x67, 0x4C, 0xD4, 0xEC, 0x67, 0xA7, 0x2C, 0x62
};

// MD5("a") = 0cc175b9c0f1b6a831c399e269772661
static const uint8_t FL_MD5_UT_EXPECTED_BYTE_A[FL_MD5_DIGEST_SIZE] =
{
	0x0C, 0xC1, 0x75, 0xB9, 0xC0, 0xF1, 0xB6, 0xA8,
	0x31, 0xC3, 0x99, 0xE2, 0x69, 0x77, 0x26, 0x61
};

// MD5("abc") = 900150983cd24fb0d6963f7d28e17f72  (RFC 1321 Appendix A.5)
static const uint8_t FL_MD5_UT_EXPECTED_ABC[FL_MD5_DIGEST_SIZE] =
{
	0x90, 0x01, 0x50, 0x98, 0x3C, 0xD2, 0x4F, 0xB0,
	0xD6, 0x96, 0x3F, 0x7D, 0x28, 0xE1, 0x7F, 0x72
};

// MD5("123456789") = 25f9e794323b453885f5181f1b624d0b
static const uint8_t FL_MD5_UT_EXPECTED_123456789[FL_MD5_DIGEST_SIZE] =
{
	0x25, 0xF9, 0xE7, 0x94, 0x32, 0x3B, 0x45, 0x38,
	0x85, 0xF5, 0x18, 0x1F, 0x1B, 0x62, 0x4D, 0x0B
};

// MD5("abcdefghijklmnopqrstuvwxyz") = c3fcd3d76192e4007dfb496cca67e13b  (RFC 1321 Appendix A.5)
static const uint8_t FL_MD5_UT_EXPECTED_LOWERCASE_ALPHABET[FL_MD5_DIGEST_SIZE] =
{
	0xC3, 0xFC, 0xD3, 0xD7, 0x61, 0x92, 0xE4, 0x00,
	0x7D, 0xFB, 0x49, 0x6C, 0xCA, 0x67, 0xE1, 0x3B
};

// MD5("Test data of Santtu S. Nyman") = e33fe4590a98b03a9800bb5905566d75
static const uint8_t FL_MD5_UT_EXPECTED_SANTTU_TEST_DATA[FL_MD5_DIGEST_SIZE] =
{
	0xE3, 0x3F, 0xE4, 0x59, 0x0A, 0x98, 0xB0, 0x3A,
	0x98, 0x00, 0xBB, 0x59, 0x05, 0x56, 0x6D, 0x75
};

// MD5(64 KiB of 0x00 bytes) = fcd6bcb56c1689fcef28b57c22475bad
static const uint8_t FL_MD5_UT_EXPECTED_64KIB_ZERO_BYTES[FL_MD5_DIGEST_SIZE] =
{
	0xFC, 0xD6, 0xBC, 0xB5, 0x6C, 0x16, 0x89, 0xFC,
	0xEF, 0x28, 0xB5, 0x7C, 0x22, 0x47, 0x5B, 0xAD
};

// MD5(64 KiB of 0xFF bytes) = ecb99e6ffea7be1e5419350f725da86b
static const uint8_t FL_MD5_UT_EXPECTED_64KIB_FF_BYTES[FL_MD5_DIGEST_SIZE] =
{
	0xEC, 0xB9, 0x9E, 0x6F, 0xFE, 0xA7, 0xBE, 0x1E,
	0x54, 0x19, 0x35, 0x0F, 0x72, 0x5D, 0xA8, 0x6B
};

// ---------------------------------------------------------------------------
// Helper: compute MD5 of a buffer in a single shot.
// ---------------------------------------------------------------------------
static void FlMd5Compute(_In_ size_t dataSize, _In_reads_bytes_(dataSize) const void* data, _Out_writes_bytes_all_(FL_MD5_DIGEST_SIZE) uint8_t* digest)
{
	FlMd5Context ctx;
	FlMd5CreateHash(&ctx);
	FlMd5HashData(&ctx, dataSize, data);
	FlMd5FinishHash(&ctx, digest);
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

// MD5 of empty input must equal the RFC 1321 reference value.
static void FlMd5UtEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t emptyData[1] = { 0 };
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(0, emptyData, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_EMPTY, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtEmptyInput");
}

// MD5("The quick brown fox jumps over the lazy dog").
static void FlMd5UtQuickBrownFox(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_QUICK_BROWN_FOX, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtQuickBrownFox");
}

// MD5("Hello world").
static void FlMd5UtHelloWorld(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Hello world";
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_HELLO_WORLD, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtHelloWorld");
}

// MD5("a").
static void FlMd5UtSingleByteA(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { 'a' };
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof data, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_BYTE_A, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtSingleByteA");
}

// MD5("abc") — RFC 1321 reference value.
static void FlMd5UtStringAbc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abc";
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_ABC, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtStringAbc");
}

// MD5("123456789").
static void FlMd5UtDigitString(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "123456789";
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_123456789, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtDigitString");
}

// MD5("abcdefghijklmnopqrstuvwxyz") — RFC 1321 reference value.
static void FlMd5UtLowercaseAlphabet(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abcdefghijklmnopqrstuvwxyz";
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_LOWERCASE_ALPHABET, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtLowercaseAlphabet");
}

// MD5("Test data of Santtu S. Nyman").
static void FlMd5UtSanttuTestData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Test data of Santtu S. Nyman";
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_SANTTU_TEST_DATA, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtSanttuTestData");
}

// MD5(64 KiB of 0x00 bytes).
static void FlMd5Ut64KiBZeroBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t zeroByte = 0x00;
	FlMd5Context ctx;
	FlMd5CreateHash(&ctx);
	for (size_t i = 0; i < 65536; i++)
		FlMd5HashData(&ctx, 1, &zeroByte);
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5FinishHash(&ctx, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_64KIB_ZERO_BYTES, FL_MD5_DIGEST_SIZE) == 0, "FlMd5Ut64KiBZeroBytes");
}

// MD5(64 KiB of 0xFF bytes).
static void FlMd5Ut64KiBFfBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t ffByte = 0xFF;
	FlMd5Context ctx;
	FlMd5CreateHash(&ctx);
	for (size_t i = 0; i < 65536; i++)
		FlMd5HashData(&ctx, 1, &ffByte);
	uint8_t digest[FL_MD5_DIGEST_SIZE];
	FlMd5FinishHash(&ctx, digest);
	FL_UT_CHECK(memcmp(digest, FL_MD5_UT_EXPECTED_64KIB_FF_BYTES, FL_MD5_DIGEST_SIZE) == 0, "FlMd5Ut64KiBFfBytes");
}

// Feeding data in multiple chunks must produce the same digest as a single call.
static void FlMd5UtStreamingEquality(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t wholeDigest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, wholeDigest);

	// Split 1 + remainder.
	{
		FlMd5Context ctx;
		FlMd5CreateHash(&ctx);
		FlMd5HashData(&ctx, 1, data);
		FlMd5HashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_MD5_DIGEST_SIZE];
		FlMd5FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtStreamingEquality_1_rest");
	}

	// Split at midpoint.
	{
		size_t mid = (sizeof(data) - 1) / 2;
		FlMd5Context ctx;
		FlMd5CreateHash(&ctx);
		FlMd5HashData(&ctx, mid, data);
		FlMd5HashData(&ctx, sizeof(data) - 1 - mid, data + mid);
		uint8_t digest[FL_MD5_DIGEST_SIZE];
		FlMd5FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtStreamingEquality_mid");
	}

	// One byte at a time.
	{
		FlMd5Context ctx;
		FlMd5CreateHash(&ctx);
		for (size_t n = sizeof(data) - 1, i = 0; i < n; i++)
			FlMd5HashData(&ctx, 1, data + i);
		uint8_t digest[FL_MD5_DIGEST_SIZE];
		FlMd5FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, wholeDigest, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtStreamingEquality_1x43");
	}

	// Split at every possible position within a short input.
	{
		static const uint8_t shortData[] = "abcd";
		uint8_t expectedShort[FL_MD5_DIGEST_SIZE];
		FlMd5Compute(sizeof(shortData) - 1, shortData, expectedShort);
		for (size_t n = sizeof(shortData) - 1, splitAt = 1; splitAt < n; splitAt++)
		{
			FlMd5Context ctx;
			FlMd5CreateHash(&ctx);
			FlMd5HashData(&ctx, splitAt, shortData);
			FlMd5HashData(&ctx, n - splitAt, shortData + splitAt);
			uint8_t digest[FL_MD5_DIGEST_SIZE];
			FlMd5FinishHash(&ctx, digest);
			FL_UT_CHECK(memcmp(digest, expectedShort, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtStreamingEquality_AllSplits");
		}
	}
}

// Passing a zero-length chunk must not alter the digest.
static void FlMd5UtZeroLengthChunkNoEffect(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abc";
	uint8_t expected[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, expected);

	// Zero-length call before actual data.
	{
		FlMd5Context ctx;
		FlMd5CreateHash(&ctx);
		FlMd5HashData(&ctx, 0, data);
		FlMd5HashData(&ctx, sizeof(data) - 1, data);
		uint8_t digest[FL_MD5_DIGEST_SIZE];
		FlMd5FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtZeroLengthChunkNoEffect_Before");
	}

	// Zero-length call after actual data.
	{
		FlMd5Context ctx;
		FlMd5CreateHash(&ctx);
		FlMd5HashData(&ctx, sizeof(data) - 1, data);
		FlMd5HashData(&ctx, 0, data);
		uint8_t digest[FL_MD5_DIGEST_SIZE];
		FlMd5FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtZeroLengthChunkNoEffect_After");
	}

	// Zero-length call in the middle of actual data.
	{
		FlMd5Context ctx;
		FlMd5CreateHash(&ctx);
		FlMd5HashData(&ctx, 1, data);
		FlMd5HashData(&ctx, 0, data);
		FlMd5HashData(&ctx, sizeof(data) - 2, data + 1);
		uint8_t digest[FL_MD5_DIGEST_SIZE];
		FlMd5FinishHash(&ctx, digest);
		FL_UT_CHECK(memcmp(digest, expected, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtZeroLengthChunkNoEffect_Middle");
	}
}

// Same input must always produce the same digest (no hidden mutable state).
static void FlMd5UtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint8_t firstDigest[FL_MD5_DIGEST_SIZE];
	uint8_t secondDigest[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(data) - 1, data, firstDigest);
	FlMd5Compute(sizeof(data) - 1, data, secondDigest);
	FL_UT_CHECK(memcmp(firstDigest, secondDigest, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtDeterminism");
}

// Two known-distinct inputs must produce different digests.
static void FlMd5UtDifferentInputsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = { 'a' };
	static const uint8_t dataB[] = { 'b' };
	uint8_t digestA[FL_MD5_DIGEST_SIZE];
	uint8_t digestB[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof dataA, dataA, digestA);
	FlMd5Compute(sizeof dataB, dataB, digestB);
	FL_UT_CHECK(memcmp(digestA, digestB, FL_MD5_DIGEST_SIZE) != 0, "FlMd5UtDifferentInputsDifferentOutput_1byte");

	static const uint8_t dataC[] = "abc";
	static const uint8_t dataD[] = "cba";
	uint8_t digestC[FL_MD5_DIGEST_SIZE];
	uint8_t digestD[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(dataC) - 1, dataC, digestC);
	FlMd5Compute(sizeof(dataD) - 1, dataD, digestD);
	FL_UT_CHECK(memcmp(digestC, digestD, FL_MD5_DIGEST_SIZE) != 0, "FlMd5UtDifferentInputsDifferentOutput_Reversed");
}

// Two interleaved MD5 computations must not interfere with each other.
static void FlMd5UtContextIsolation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = "abc";
	static const uint8_t dataB[] = "123";
	uint8_t expectedA[FL_MD5_DIGEST_SIZE];
	uint8_t expectedB[FL_MD5_DIGEST_SIZE];
	FlMd5Compute(sizeof(dataA) - 1, dataA, expectedA);
	FlMd5Compute(sizeof(dataB) - 1, dataB, expectedB);

	// Interleave byte-by-byte updates to both contexts.
	FlMd5Context ctxA;
	FlMd5Context ctxB;
	FlMd5CreateHash(&ctxA);
	FlMd5CreateHash(&ctxB);
	for (size_t n = sizeof(dataA) - 1, i = 0; i < n; i++)
	{
		FlMd5HashData(&ctxA, 1, dataA + i);
		FlMd5HashData(&ctxB, 1, dataB + i);
	}

	uint8_t digestA[FL_MD5_DIGEST_SIZE];
	uint8_t digestB[FL_MD5_DIGEST_SIZE];
	FlMd5FinishHash(&ctxA, digestA);
	FlMd5FinishHash(&ctxB, digestB);
	FL_UT_CHECK(memcmp(digestA, expectedA, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtContextIsolation_A");
	FL_UT_CHECK(memcmp(digestB, expectedB, FL_MD5_DIGEST_SIZE) == 0, "FlMd5UtContextIsolation_B");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlMd5UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlMd5UtEmptyInput(testCount, failCount);
	FlMd5UtQuickBrownFox(testCount, failCount);
	FlMd5UtHelloWorld(testCount, failCount);
	FlMd5UtSingleByteA(testCount, failCount);
	FlMd5UtStringAbc(testCount, failCount);
	FlMd5UtDigitString(testCount, failCount);
	FlMd5UtLowercaseAlphabet(testCount, failCount);
	FlMd5UtSanttuTestData(testCount, failCount);
	FlMd5Ut64KiBZeroBytes(testCount, failCount);
	FlMd5Ut64KiBFfBytes(testCount, failCount);
	FlMd5UtStreamingEquality(testCount, failCount);
	FlMd5UtZeroLengthChunkNoEffect(testCount, failCount);
	FlMd5UtDeterminism(testCount, failCount);
	FlMd5UtDifferentInputsDifferentOutput(testCount, failCount);
	FlMd5UtContextIsolation(testCount, failCount);
}
