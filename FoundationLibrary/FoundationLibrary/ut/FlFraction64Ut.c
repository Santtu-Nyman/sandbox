/*
	FlFraction64 unit tests by Santtu S. Nyman.

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

		Unit tests for FlDecodeFraction64 and FlEncodeFraction64.

		The 64-bit fraction type represents a value in [0, 1) as a fixed-point
		binary fraction: stored value N corresponds to N / 2^64.

		  0x0000000000000000 = 0.0
		  0x8000000000000000 = 0.5   (1/2)
		  0x4000000000000000 = 0.25  (1/4)
		  0x2000000000000000 = 0.125 (1/8)
		  0x1000000000000000 = 0.0625 (1/16)

		Values that are exact in both binary and decimal (multiples of 2^-n
		whose decimal representations terminate) are used as primary test
		vectors because they survive decode-encode round-trips without error.
*/

#include "FlUt.h"
#include "../include/FlFraction64.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Binary fractions that are exactly representable in the 64-bit format and
// whose decimal representations are also exact (no rounding on either path).
#define FL_FRACTION64_UT_ZERO      0x0000000000000000ull
#define FL_FRACTION64_UT_HALF      0x8000000000000000ull  // 0.5    = 1/2
#define FL_FRACTION64_UT_QUARTER   0x4000000000000000ull  // 0.25   = 1/4
#define FL_FRACTION64_UT_EIGHTH    0x2000000000000000ull  // 0.125  = 1/8
#define FL_FRACTION64_UT_SIXTEENTH 0x1000000000000000ull  // 0.0625 = 1/16

// Returns 1 if the first length characters of text equal the null-terminated
// expected string, 0 otherwise.  Used to validate FlEncodeFraction64 output
// without requiring null termination of the output buffer.
static int FlFraction64UtTextEqual(
	_In_ size_t length,
	_In_reads_(length) const char* text,
	_In_z_ const char* expected)
{
	size_t expectedLength = strlen(expected);
	if (length != expectedLength)
		return 0;
	return memcmp(text, expected, length) == 0;
}

// ──────────────────────────────────────────────────────────────
// FlDecodeFraction64 tests
// ──────────────────────────────────────────────────────────────

// Decoding zero length input must return 0.
static void FlFraction64UtDecodeEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '0' };
	uint64_t result = FlDecodeFraction64(0, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_ZERO, "FlFraction64UtDecodeEmptyInput");
}

// Decoding the single digit "0" must return 0.
static void FlFraction64UtDecodeDigitZero(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '0' };
	uint64_t result = FlDecodeFraction64(1, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_ZERO, "FlFraction64UtDecodeDigitZero");
}

// Decoding "5" must return the exact representation of 0.5.
static void FlFraction64UtDecodeHalf(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '5' };
	uint64_t result = FlDecodeFraction64(1, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_HALF, "FlFraction64UtDecodeHalf");
}

// Decoding "25" must return the exact representation of 0.25.
static void FlFraction64UtDecodeQuarter(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '2', '5' };
	uint64_t result = FlDecodeFraction64(2, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_QUARTER, "FlFraction64UtDecodeQuarter");
}

// Decoding "125" must return the exact representation of 0.125.
static void FlFraction64UtDecodeEighth(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '1', '2', '5' };
	uint64_t result = FlDecodeFraction64(3, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_EIGHTH, "FlFraction64UtDecodeEighth");
}

// Decoding "0625" must return the exact representation of 0.0625.
static void FlFraction64UtDecodeSixteenth(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '0', '6', '2', '5' };
	uint64_t result = FlDecodeFraction64(4, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_SIXTEENTH, "FlFraction64UtDecodeSixteenth");
}

// Decoding a longer string that begins with "5" and has extra trailing digits
// must still produce the same value as decoding "5" (the extra precision is
// subsumed within the 64-bit resolution).
static void FlFraction64UtDecodeExtraDigitsPastResolution(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "500000000000000000000" — 20 zeros after 5 is well past 64-bit resolution.
	static const char data[] = "500000000000000000000";
	uint64_t result = FlDecodeFraction64(sizeof(data) - 1, data);
	FL_UT_CHECK(result == FL_FRACTION64_UT_HALF, "FlFraction64UtDecodeExtraDigitsPastResolution");
}

// Decoding is deterministic: the same input always produces the same output.
static void FlFraction64UtDecodeDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char data[] = { '2', '5' };
	uint64_t firstResult = FlDecodeFraction64(2, data);
	uint64_t secondResult = FlDecodeFraction64(2, data);
	FL_UT_CHECK(firstResult == secondResult, "FlFraction64UtDecodeDeterminism");
}

// Different decimal strings that represent distinct values must decode to
// distinct 64-bit fractions.
static void FlFraction64UtDecodeDifferentInputsDifferentOutput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char dataA[] = { '2', '5' };
	static const char dataB[] = { '7', '5' };
	uint64_t resultA = FlDecodeFraction64(2, dataA);
	uint64_t resultB = FlDecodeFraction64(2, dataB);
	FL_UT_CHECK(resultA != resultB, "FlFraction64UtDecodeDifferentInputsDifferentOutput");
}

// ──────────────────────────────────────────────────────────────
// FlEncodeFraction64 tests
// ──────────────────────────────────────────────────────────────

// Encoding zero must produce "0" with length 1.
static void FlFraction64UtEncodeZero(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_ZERO, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "0"), "FlFraction64UtEncodeZero");
}

// Encoding 0.5 with full precision must produce "5" with length 1.
static void FlFraction64UtEncodeHalf(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_HALF, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "5"), "FlFraction64UtEncodeHalf");
}

// Encoding 0.25 with full precision must produce "25" with length 2.
static void FlFraction64UtEncodeQuarter(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_QUARTER, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "25"), "FlFraction64UtEncodeQuarter");
}

// Encoding 0.125 with full precision must produce "125" with length 3.
static void FlFraction64UtEncodeEighth(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_EIGHTH, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "125"), "FlFraction64UtEncodeEighth");
}

// Encoding 0.0625 with full precision must produce "0625" with length 4.
static void FlFraction64UtEncodeSixteenth(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_SIXTEENTH, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "0625"), "FlFraction64UtEncodeSixteenth");
}

// The returned length must never exceed FL_FRACTION_64_MAX_LENGTH for any
// value encoded with FL_FRACTION_64_FULL_PRECISION.
static void FlFraction64UtEncodeLengthBounded(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint64_t testValues[] = {
		FL_FRACTION64_UT_ZERO,
		FL_FRACTION64_UT_HALF,
		FL_FRACTION64_UT_QUARTER,
		FL_FRACTION64_UT_EIGHTH,
		FL_FRACTION64_UT_SIXTEENTH,
		0x1999999999999999ull,  // approx 0.1
		0x3333333333333333ull,  // approx 0.2
		0xCCCCCCCCCCCCCCCCull,  // approx 0.8
		0xFFFFFFFFFFFFFFFFull   // maximum value
	};
	char text[FL_FRACTION_64_MAX_LENGTH];
	for (size_t n = sizeof(testValues) / sizeof(testValues[0]), i = 0; i < n; i++)
	{
		size_t length = FlEncodeFraction64(testValues[i], FL_FRACTION_64_FULL_PRECISION, text);
		FL_UT_CHECK(
			length <= FL_FRACTION_64_MAX_LENGTH || length == 0,
			"FlFraction64UtEncodeLengthBounded");
	}
}

// All characters written to the output buffer must be decimal digit characters.
static void FlFraction64UtEncodeOutputDigitsValid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint64_t testValues[] = {
		FL_FRACTION64_UT_ZERO,
		FL_FRACTION64_UT_HALF,
		FL_FRACTION64_UT_QUARTER,
		FL_FRACTION64_UT_EIGHTH,
		FL_FRACTION64_UT_SIXTEENTH,
		0x1999999999999999ull,
		0x3333333333333333ull,
		0xCCCCCCCCCCCCCCCCull
	};
	char text[FL_FRACTION_64_MAX_LENGTH];
	for (size_t n = sizeof(testValues) / sizeof(testValues[0]), i = 0; i < n; i++)
	{
		size_t length = FlEncodeFraction64(testValues[i], FL_FRACTION_64_FULL_PRECISION, text);
		if (length == 0)
			continue;
		for (size_t j = 0; j < length; j++)
			FL_UT_CHECK(text[j] >= '0' && text[j] <= '9', "FlFraction64UtEncodeOutputDigitsValid");
	}
}

// Encoding is deterministic: the same input always produces the same output.
static void FlFraction64UtEncodeDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char textA[FL_FRACTION_64_MAX_LENGTH];
	char textB[FL_FRACTION_64_MAX_LENGTH];
	size_t lengthA = FlEncodeFraction64(FL_FRACTION64_UT_QUARTER, FL_FRACTION_64_FULL_PRECISION, textA);
	size_t lengthB = FlEncodeFraction64(FL_FRACTION64_UT_QUARTER, FL_FRACTION_64_FULL_PRECISION, textB);
	FL_UT_CHECK(lengthA == lengthB, "FlFraction64UtEncodeDeterminism_Length");
	FL_UT_CHECK(memcmp(textA, textB, lengthA) == 0, "FlFraction64UtEncodeDeterminism_Content");
}

// ──────────────────────────────────────────────────────────────
// FlEncodeFraction64 precision and rounding tests
// ──────────────────────────────────────────────────────────────

// Encoding 0.25 with precision 1 must round up to "3" (0.25 → 0.3).
static void FlFraction64UtEncodePrecisionRoundsUp(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_QUARTER, 1, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "3"), "FlFraction64UtEncodePrecisionRoundsUp");
}

// Encoding 0.125 with precision 1 must truncate to "1" (0.125 → 0.1, next digit is 2).
static void FlFraction64UtEncodePrecisionTruncates(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_EIGHTH, 1, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "1"), "FlFraction64UtEncodePrecisionTruncates");
}

// Encoding 0.5 with precision 2 must produce "5" with length 1:
// the exact result is "5" and the trailing digit would be "0" which is stripped.
static void FlFraction64UtEncodeTrailingZeroStripped(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(FL_FRACTION64_UT_HALF, 2, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, "5"), "FlFraction64UtEncodeTrailingZeroStripped");
}

// Encoding the 0x07FFFFFFFFFFFFFFull value with precision 1 must return
// 1 with string "0" because the value rounds up to 0.
static void FlFraction64UtEncodeRoundToZero(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(0x07FFFFFFFFFFFFFFull, 1, &text[0]);
	FL_UT_CHECK(length == 1 && text[0] == '0', "FlFraction64UtEncodeRoundToZero");

	int loopSucceeded = 1;
	for (int i = 1; i < 19; i++)
	{
		memset(&text[0], '0', sizeof(text));
		text[i] = '1';
		uint64_t decoded = FlDecodeFraction64(i + 1, &text[0]);
		memset(&text[0], 0, sizeof(text));
		length = FlEncodeFraction64(decoded, i, &text[0]);
		if (length != 1 || text[0] != '0')
		{
			loopSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(loopSucceeded, "FlFraction64UtEncodeRoundToZero_loop");
}

// Encoding the maximum uint64_t value with precision 1 must return
// 0 because the value rounds up to 1.
static void FlFraction64UtEncodeRoundToOne(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length = FlEncodeFraction64(0xFFFFFFFFFFFFFFFFull, 1, text);
	FL_UT_CHECK(length == 0, "FlFraction64UtEncodeRoundToOne");

	int loopSucceeded = 1;
	for (int i = 1; i < 19; i++)
	{
		memset(&text[0], '9', sizeof(text));
		text[i] = '9';
		uint64_t decoded = FlDecodeFraction64(i + 1, &text[0]);
		memset(&text[0], 0, sizeof(text));
		length = FlEncodeFraction64(decoded, i, &text[0]);
		if (length != 0)
		{
			loopSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(loopSucceeded, "FlFraction64UtEncodeRoundToOne_loop");
}

// ──────────────────────────────────────────────────────────────
// Decode-encode round-trip tests
// ──────────────────────────────────────────────────────────────

// For fractions that are exact in binary, decoding their decimal representation
// and then re-encoding must produce the original decimal string unchanged.
static void FlFraction64UtDecodeEncodeRoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const struct
	{
		const char* text;
		size_t      length;
	} cases[] = {
		{ "0",    1 },
		{ "5",    1 },
		{ "25",   2 },
		{ "125",  3 },
		{ "0625", 4 },
	};
	char encodedText[FL_FRACTION_64_MAX_LENGTH];
	for (size_t n = sizeof(cases) / sizeof(cases[0]), i = 0; i < n; i++)
	{
		uint64_t decoded = FlDecodeFraction64(cases[i].length, cases[i].text);
		size_t encodedLength = FlEncodeFraction64(decoded, FL_FRACTION_64_FULL_PRECISION, encodedText);
		FL_UT_CHECK(
			FlFraction64UtTextEqual(encodedLength, encodedText, cases[i].text),
			"FlFraction64UtDecodeEncodeRoundTrip");
	}
}

// Encoding a known value and then decoding the result must recover the
// original value (for exact binary fractions).
static void FlFraction64UtEncodeDecodeRoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint64_t testValues[] = {
		FL_FRACTION64_UT_ZERO,
		FL_FRACTION64_UT_HALF,
		FL_FRACTION64_UT_QUARTER,
		FL_FRACTION64_UT_EIGHTH,
		FL_FRACTION64_UT_SIXTEENTH,
	};
	char text[FL_FRACTION_64_MAX_LENGTH];
	for (size_t n = sizeof(testValues) / sizeof(testValues[0]), i = 0; i < n; i++)
	{
		size_t length = FlEncodeFraction64(testValues[i], FL_FRACTION_64_FULL_PRECISION, text);
		uint64_t decoded = FlDecodeFraction64(length, text);
		FL_UT_CHECK(decoded == testValues[i], "FlFraction64UtEncodeDecodeRoundTrip");
	}
}

// Encoding 0x0000000000000001 and 0xFFFFFFFFFFFFFFFF with full precision must
// produce their exact 64-digit decimal representations.
static void FlFraction64UtEncodeFullPrecisionBoundaryValues(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char expectedMin[] = "0000000000000000000542101086242752217003726400434970855712890625";
	static const char expectedMax[] = "9999999999999999999457898913757247782996273599565029144287109375";
	char text[FL_FRACTION_64_MAX_LENGTH];
	size_t length;

	length = FlEncodeFraction64(0x0000000000000001ull, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, expectedMin), "FlFraction64UtEncodeFullPrecisionBoundaryValues_Min");

	length = FlEncodeFraction64(0xFFFFFFFFFFFFFFFFull, FL_FRACTION_64_FULL_PRECISION, text);
	FL_UT_CHECK(FlFraction64UtTextEqual(length, text, expectedMax), "FlFraction64UtEncodeFullPrecisionBoundaryValues_Max");
}

// The 64-digit decimal strings that are the exact decimal representations of
// the smallest non-zero value (1/2^64) and the largest value ((2^64-1)/2^64)
// in the 64-bit fixed-point format must decode to 0x0000000000000001 and
// 0xFFFFFFFFFFFFFFFF respectively.
static void FlFraction64UtDecodeFullPrecisionBoundaryValues(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char minNonZero[] = "0000000000000000000542101086242752217003726400434970855712890625";
	static const char maxValue[]   = "9999999999999999999457898913757247782996273599565029144287109375";
	FL_UT_CHECK(FlDecodeFraction64(sizeof(minNonZero) - 1, minNonZero) == 0x0000000000000001ull, "FlFraction64UtDecodeFullPrecisionBoundaryValues_Min");
	FL_UT_CHECK(FlDecodeFraction64(sizeof(maxValue)   - 1, maxValue)   == 0xFFFFFFFFFFFFFFFFull, "FlFraction64UtDecodeFullPrecisionBoundaryValues_Max");
}

void FlFraction64UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlFraction64UtDecodeEmptyInput(testCount, failCount);
	FlFraction64UtDecodeDigitZero(testCount, failCount);
	FlFraction64UtDecodeHalf(testCount, failCount);
	FlFraction64UtDecodeQuarter(testCount, failCount);
	FlFraction64UtDecodeEighth(testCount, failCount);
	FlFraction64UtDecodeSixteenth(testCount, failCount);
	FlFraction64UtDecodeExtraDigitsPastResolution(testCount, failCount);
	FlFraction64UtDecodeDeterminism(testCount, failCount);
	FlFraction64UtDecodeDifferentInputsDifferentOutput(testCount, failCount);
	FlFraction64UtEncodeZero(testCount, failCount);
	FlFraction64UtEncodeHalf(testCount, failCount);
	FlFraction64UtEncodeQuarter(testCount, failCount);
	FlFraction64UtEncodeEighth(testCount, failCount);
	FlFraction64UtEncodeSixteenth(testCount, failCount);
	FlFraction64UtEncodeLengthBounded(testCount, failCount);
	FlFraction64UtEncodeOutputDigitsValid(testCount, failCount);
	FlFraction64UtEncodeDeterminism(testCount, failCount);
	FlFraction64UtEncodePrecisionRoundsUp(testCount, failCount);
	FlFraction64UtEncodePrecisionTruncates(testCount, failCount);
	FlFraction64UtEncodeTrailingZeroStripped(testCount, failCount);
	FlFraction64UtEncodeRoundToZero(testCount, failCount);
	FlFraction64UtEncodeRoundToOne(testCount, failCount);
	FlFraction64UtDecodeEncodeRoundTrip(testCount, failCount);
	FlFraction64UtEncodeDecodeRoundTrip(testCount, failCount);
	FlFraction64UtDecodeFullPrecisionBoundaryValues(testCount, failCount);
	FlFraction64UtEncodeFullPrecisionBoundaryValues(testCount, failCount);
}
