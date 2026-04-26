/*
	CRC32 unit tests by Santtu S. Nyman.

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

		Unit tests for FlCrc32Create, FlCrc32Data and FlCrc32Finish.

		Test vectors are taken from publicly documented CRC32/ISO-HDLC
		(IEEE 802.3 / zlib) reference values. The canonical check value
		0xCBF43926 for the input "123456789" is the primary correctness anchor.
*/

#include "FlUt.h"
#include "../include/FlCrc32.h"
#include <stddef.h>
#include <stdint.h>

// Expected CRC32 values for known inputs (CRC32/ISO-HDLC, polynomial 0xEDB88320).
#define FL_CRC32_UT_EXPECTED_EMPTY              0x00000000ul
#define FL_CRC32_UT_EXPECTED_123456789          0xCBF43926ul
#define FL_CRC32_UT_EXPECTED_BYTE_A             0xE8B7BE43ul
#define FL_CRC32_UT_EXPECTED_ABC                0x352441C2ul
#define FL_CRC32_UT_EXPECTED_QUICK_BROWN_FOX    0x414FA339ul
#define FL_CRC32_UT_EXPECTED_HELLO_WORLD        0x8BD69E52ul
#define FL_CRC32_UT_EXPECTED_LOWERCASE_ALPHABET 0x4C2750BDul
#define FL_CRC32_UT_EXPECTED_SANTTU_TEST_DATA   0xD7E37FBAul
#define FL_CRC32_UT_EXPECTED_1024_ZERO_BYTES    0xEFB5AF2Eul
#define FL_CRC32_UT_EXPECTED_64KIB_ZERO_BYTES   0xD7978EEBul
#define FL_CRC32_UT_EXPECTED_64KIB_FF_BYTES     0xDEAB7E4Eul

// Convenience wrapper: compute CRC32 of a buffer in a single operation.
static uint32_t FlCrc32Compute(_In_ size_t data_size, _In_reads_bytes_(data_size) const void* data)
{
	return FlCrc32Finish(FlCrc32Data(FlCrc32Create(), data_size, data));
}

// CRC32 of empty input must be 0x00000000.
static void FlCrc32UtEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t emptyData[1] = { 0 };
	uint32_t result = FlCrc32Compute(0, emptyData);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_EMPTY, "FlCrc32UtEmptyInput");
}

// "123456789" is the universal CRC32 standard check value.
// Every conforming CRC32/ISO-HDLC implementation must return 0xCBF43926.
static void FlCrc32UtCanonicalCheckValue(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint32_t result = FlCrc32Compute(sizeof data, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_123456789, "FlCrc32UtCanonicalCheckValue");
}

// CRC32 of the single ASCII byte 'a' (0x61) must be 0xE8B7BE43.
static void FlCrc32UtSingleByteA(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { 0x61 };
	uint32_t result = FlCrc32Compute(sizeof data, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_BYTE_A, "FlCrc32UtSingleByteA");
}

// CRC32 of "abc" must be 0x352441C2.
static void FlCrc32UtStringAbc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { 'a', 'b', 'c' };
	uint32_t result = FlCrc32Compute(sizeof data, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_ABC, "FlCrc32UtStringAbc");
}

// CRC32 of "The quick brown fox jumps over the lazy dog" must be 0x414FA339.
static void FlCrc32UtQuickBrownFox(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// Array initialized from string literal; sizeof includes null terminator so subtract 1.
	static const uint8_t data[] = "The quick brown fox jumps over the lazy dog";
	uint32_t result = FlCrc32Compute(sizeof(data) - 1, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_QUICK_BROWN_FOX, "FlCrc32UtQuickBrownFox");
}

// Feeding data in multiple chunks must produce the same result as a single call.
static void FlCrc32UtStreamingEquality(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint32_t wholeResult = FlCrc32Compute(sizeof data, data);

	// Split 1 + 8.
	{
		uint32_t ctx = FlCrc32Create();
		ctx = FlCrc32Data(ctx, 1, data);
		ctx = FlCrc32Data(ctx, 8, data + 1);
		FL_UT_CHECK(FlCrc32Finish(ctx) == wholeResult, "FlCrc32UtStreamingEquality_1_8");
	}

	// Split 5 + 4.
	{
		uint32_t ctx = FlCrc32Create();
		ctx = FlCrc32Data(ctx, 5, data);
		ctx = FlCrc32Data(ctx, 4, data + 5);
		FL_UT_CHECK(FlCrc32Finish(ctx) == wholeResult, "FlCrc32UtStreamingEquality_5_4");
	}

	// One byte at a time.
	{
		uint32_t ctx = FlCrc32Create();
		for (size_t n = sizeof data, i = 0; i < n; i++)
			ctx = FlCrc32Data(ctx, 1, data + i);
		FL_UT_CHECK(FlCrc32Finish(ctx) == wholeResult, "FlCrc32UtStreamingEquality_1x9");
	}

	// Split at every possible position within a 4-byte input.
	{
		static const uint8_t shortData[] = { 'a', 'b', 'c', 'd' };
		uint32_t expectedShort = FlCrc32Compute(sizeof shortData, shortData);
		for (size_t n = sizeof shortData, splitAt = 1; splitAt < n; splitAt++)
		{
			uint32_t ctx = FlCrc32Create();
			ctx = FlCrc32Data(ctx, splitAt, shortData);
			ctx = FlCrc32Data(ctx, n - splitAt, shortData + splitAt);
			FL_UT_CHECK(FlCrc32Finish(ctx) == expectedShort, "FlCrc32UtStreamingEquality_AllSplits");
		}
	}
}

// Passing a zero-length chunk must not alter the context value.
static void FlCrc32UtZeroLengthChunkNoEffect(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint32_t expected = FlCrc32Compute(sizeof data, data);

	// Zero-length call before actual data.
	{
		uint32_t ctx = FlCrc32Create();
		ctx = FlCrc32Data(ctx, 0, data);
		ctx = FlCrc32Data(ctx, sizeof data, data);
		FL_UT_CHECK(FlCrc32Finish(ctx) == expected, "FlCrc32UtZeroLengthChunkNoEffect_Before");
	}

	// Zero-length call after actual data.
	{
		uint32_t ctx = FlCrc32Create();
		ctx = FlCrc32Data(ctx, sizeof data, data);
		ctx = FlCrc32Data(ctx, 0, data);
		FL_UT_CHECK(FlCrc32Finish(ctx) == expected, "FlCrc32UtZeroLengthChunkNoEffect_After");
	}

	// Zero-length call in the middle of actual data.
	{
		uint32_t ctx = FlCrc32Create();
		ctx = FlCrc32Data(ctx, 4, data);
		ctx = FlCrc32Data(ctx, 0, data);
		ctx = FlCrc32Data(ctx, 5, data + 4);
		FL_UT_CHECK(FlCrc32Finish(ctx) == expected, "FlCrc32UtZeroLengthChunkNoEffect_Middle");
	}
}

// Same input must always produce the same output (no hidden mutable state).
static void FlCrc32UtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint32_t firstResult = FlCrc32Compute(sizeof data, data);
	uint32_t secondResult = FlCrc32Compute(sizeof data, data);
	FL_UT_CHECK(firstResult == secondResult, "FlCrc32UtDeterminism");
}

// Two known-distinct inputs must produce different CRC32 values.
static void FlCrc32UtDifferentInputsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = { 'a' };
	static const uint8_t dataB[] = { 'b' };
	uint32_t resultA = FlCrc32Compute(sizeof dataA, dataA);
	uint32_t resultB = FlCrc32Compute(sizeof dataB, dataB);
	FL_UT_CHECK(resultA != resultB, "FlCrc32UtDifferentInputsDifferentOutput_1byte");

	static const uint8_t dataC[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	static const uint8_t dataD[] = { '9', '8', '7', '6', '5', '4', '3', '2', '1' };
	uint32_t resultC = FlCrc32Compute(sizeof dataC, dataC);
	uint32_t resultD = FlCrc32Compute(sizeof dataD, dataD);
	FL_UT_CHECK(resultC != resultD, "FlCrc32UtDifferentInputsDifferentOutput_Reversed");
}

// Two interleaved CRC32 computations must not interfere with each other.
static void FlCrc32UtContextIsolation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t dataA[] = { 'a', 'b', 'c' };
	static const uint8_t dataB[] = { '1', '2', '3' };
	uint32_t expectedA = FlCrc32Compute(sizeof dataA, dataA);
	uint32_t expectedB = FlCrc32Compute(sizeof dataB, dataB);

	// Interleave byte-by-byte updates to both contexts.
	uint32_t ctxA = FlCrc32Create();
	uint32_t ctxB = FlCrc32Create();
	for (size_t n = sizeof dataA, i = 0; i < n; i++)
	{
		ctxA = FlCrc32Data(ctxA, 1, dataA + i);
		ctxB = FlCrc32Data(ctxB, 1, dataB + i);
	}

	FL_UT_CHECK(FlCrc32Finish(ctxA) == expectedA, "FlCrc32UtContextIsolation_A");
	FL_UT_CHECK(FlCrc32Finish(ctxB) == expectedB, "FlCrc32UtContextIsolation_B");
}

// All 256 distinct byte values must each produce a non-zero and unique CRC32.
// This exercises that the lookup table covers all byte positions correctly.
static void FlCrc32UtAllByteValues(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint32_t singleByteResults[256];

	for (size_t i = 0; i < 256; i++)
	{
		uint8_t byte = (uint8_t)i;
		singleByteResults[i] = FlCrc32Compute(1, &byte);
		// CRC32 of any single byte is non-zero under this polynomial.
		FL_UT_CHECK(singleByteResults[i] != 0x00000000ul, "FlCrc32UtAllByteValues_NonZero");
	}

	// Each distinct single byte must produce a distinct CRC32 value.
	for (size_t i = 0; i < 256; i++)
	{
		for (size_t j = i + 1; j < 256; j++)
		{
			FL_UT_CHECK(singleByteResults[i] != singleByteResults[j], "FlCrc32UtAllByteValues_Unique");
		}
	}
}

// CRC32 of "Hello world" must be 0x8BD69E52.
static void FlCrc32UtHelloWorld(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Hello world";
	uint32_t result = FlCrc32Compute(sizeof(data) - 1, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_HELLO_WORLD, "FlCrc32UtHelloWorld");
}

// CRC32 of "abcdefghijklmnopqrstuvwxyz" must be 0x4C2750BD.
static void FlCrc32UtLowercaseAlphabet(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "abcdefghijklmnopqrstuvwxyz";
	uint32_t result = FlCrc32Compute(sizeof(data) - 1, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_LOWERCASE_ALPHABET, "FlCrc32UtLowercaseAlphabet");
}

// CRC32 of "Test data of Santtu S. Nyman" must be 0xD7E37FBA.
static void FlCrc32UtSanttuTestData(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[] = "Test data of Santtu S. Nyman";
	uint32_t result = FlCrc32Compute(sizeof(data) - 1, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_SANTTU_TEST_DATA, "FlCrc32UtSanttuTestData");
}

// CRC32 of 1024 zero bytes must be 0xEFB5AF2E.
static void FlCrc32Ut1024ZeroBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t data[1024] = { 0 };
	uint32_t result = FlCrc32Compute(sizeof data, data);
	FL_UT_CHECK(result == FL_CRC32_UT_EXPECTED_1024_ZERO_BYTES, "FlCrc32Ut1024ZeroBytes");
}

// CRC32 of 64 KiB of zero bytes must be 0xD7978EEB.
static void FlCrc32Ut64KiBZeroBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t zeroByte = 0x00;
	uint32_t ctx = FlCrc32Create();
	for (size_t i = 0; i < 65536; i++)
		ctx = FlCrc32Data(ctx, 1, &zeroByte);
	FL_UT_CHECK(FlCrc32Finish(ctx) == FL_CRC32_UT_EXPECTED_64KIB_ZERO_BYTES, "FlCrc32Ut64KiBZeroBytes");
}

// CRC32 of 64 KiB of 0xFF bytes must be 0xDEAB7E4E.
static void FlCrc32Ut64KiBFfBytes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t ffByte = 0xFF;
	uint32_t ctx = FlCrc32Create();
	for (size_t i = 0; i < 65536; i++)
		ctx = FlCrc32Data(ctx, 1, &ffByte);
	FL_UT_CHECK(FlCrc32Finish(ctx) == FL_CRC32_UT_EXPECTED_64KIB_FF_BYTES, "FlCrc32Ut64KiBFfBytes");
}

void FlCrc32UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlCrc32UtEmptyInput(testCount, failCount);
	FlCrc32UtCanonicalCheckValue(testCount, failCount);
	FlCrc32UtSingleByteA(testCount, failCount);
	FlCrc32UtStringAbc(testCount, failCount);
	FlCrc32UtQuickBrownFox(testCount, failCount);
	FlCrc32UtStreamingEquality(testCount, failCount);
	FlCrc32UtZeroLengthChunkNoEffect(testCount, failCount);
	FlCrc32UtDeterminism(testCount, failCount);
	FlCrc32UtDifferentInputsDifferentOutput(testCount, failCount);
	FlCrc32UtContextIsolation(testCount, failCount);
	FlCrc32UtAllByteValues(testCount, failCount);
	FlCrc32Ut1024ZeroBytes(testCount, failCount);
	FlCrc32UtHelloWorld(testCount, failCount);
	FlCrc32UtLowercaseAlphabet(testCount, failCount);
	FlCrc32UtSanttuTestData(testCount, failCount);
	FlCrc32Ut64KiBZeroBytes(testCount, failCount);
	FlCrc32Ut64KiBFfBytes(testCount, failCount);
}
