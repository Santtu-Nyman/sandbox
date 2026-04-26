/*
	UTF-8 / UTF-16 converter unit tests by Santtu S. Nyman.

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

		Unit tests for FlConvertUtf8ToUtf16Le, FlConvertUtf16LeToUtf8,
		FlConvertUtf8ToUtf16Be, and FlConvertUtf16BeToUtf8.

		Test vectors cover all four UTF-8 sequence lengths, surrogate pairs
		for supplementary code points, the maximum valid code point (U+10FFFF),
		mixed content, empty input, invalid UTF-8, and lone surrogates.

		UTF-16 BE variant stores each 16-bit code unit with bytes swapped
		relative to the LE value, i.e. WCHAR_be = (v << 8) | (v >> 8).

		Encode vectors (UTF-8 bytes  ->  UTF-16 LE WCHARs):
		  #0  48 65 6C 6C 6F                               ("Hello", ASCII)
		        -> 0048 0065 006C 006C 006F
		  #1  C3 A9 E4 B8 96 E2 82 AC                      (U+00E9 U+4E16 U+20AC)
		        -> 00E9 4E16 20AC
		  #2  F0 9F 98 80                                   (U+1F600, 4-byte -> surrogate pair)
		        -> D83D DE00
		  #3  F4 8F BF BF                                   (U+10FFFF, maximum code point)
		        -> DBFF DFFF
		  #4  41 E2 82 AC 42 F0 9F 98 80 43                 (A U+20AC B U+1F600 C, mixed)
		        -> 0041 20AC 0042 D83D DE00 0043
		  #5  (empty)  ->  (empty)
*/

#define WIN32_LEAN_AND_MEAN
#include "FlUt.h"
#include "../include/FlUtf8Utf16Converter.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Test vectors
// ---------------------------------------------------------------------------

// Vector 0: ASCII "Hello"
static const uint8_t FlUtf8V0[]    = { 0x48, 0x65, 0x6C, 0x6C, 0x6F };
static const WCHAR   FlUtf16LeV0[] = { 0x0048, 0x0065, 0x006C, 0x006C, 0x006F };
static const WCHAR   FlUtf16BeV0[] = { 0x4800, 0x6500, 0x6C00, 0x6C00, 0x6F00 };

// Vector 1: U+00E9 (é, 2-byte) + U+4E16 (世, 3-byte) + U+20AC (€, 3-byte)
static const uint8_t FlUtf8V1[]    = { 0xC3, 0xA9, 0xE4, 0xB8, 0x96, 0xE2, 0x82, 0xAC };
static const WCHAR   FlUtf16LeV1[] = { 0x00E9, 0x4E16, 0x20AC };
static const WCHAR   FlUtf16BeV1[] = { 0xE900, 0x164E, 0xAC20 };

// Vector 2: U+1F600 (😀, 4-byte UTF-8 -> surrogate pair D83D DE00)
static const uint8_t FlUtf8V2[]    = { 0xF0, 0x9F, 0x98, 0x80 };
static const WCHAR   FlUtf16LeV2[] = { 0xD83D, 0xDE00 };
static const WCHAR   FlUtf16BeV2[] = { 0x3DD8, 0x00DE };

// Vector 3: U+10FFFF (maximum valid code point, -> surrogate pair DBFF DFFF)
static const uint8_t FlUtf8V3[]    = { 0xF4, 0x8F, 0xBF, 0xBF };
static const WCHAR   FlUtf16LeV3[] = { 0xDBFF, 0xDFFF };
static const WCHAR   FlUtf16BeV3[] = { 0xFFDB, 0xFFDF };

// Vector 4: mixed — 'A' + U+20AC + 'B' + U+1F600 + 'C'
static const uint8_t FlUtf8V4[]    = { 0x41, 0xE2, 0x82, 0xAC, 0x42, 0xF0, 0x9F, 0x98, 0x80, 0x43 };
static const WCHAR   FlUtf16LeV4[] = { 0x0041, 0x20AC, 0x0042, 0xD83D, 0xDE00, 0x0043 };
static const WCHAR   FlUtf16BeV4[] = { 0x4100, 0xAC20, 0x4200, 0x3DD8, 0x00DE, 0x4300 };

// Lengths
static const size_t FlUtf8Len[]    = { 5, 8, 4, 4, 10 }; // byte counts
static const size_t FlUtf16Len[]   = { 5, 3, 2, 2,  6 }; // WCHAR counts

// ---------------------------------------------------------------------------
// UTF-8 -> UTF-16 LE tests
// ---------------------------------------------------------------------------

// ASCII "Hello": 5 one-byte sequences -> 5 WCHARs, return 5.
static void FlUtf8Utf16ConverterUtAsciiToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[5];
	size_t n = FlConvertUtf8ToUtf16Le(FlUtf8Len[0], (const char*)FlUtf8V0, 5, buf);
	FL_UT_CHECK(n == FlUtf16Len[0] && memcmp(buf, FlUtf16LeV0, FlUtf16Len[0] * sizeof(WCHAR)) == 0,
	            "FlUtf8Utf16ConverterUtAsciiToUtf16Le");
}

// Two-byte (U+00E9) and three-byte (U+4E16, U+20AC) sequences -> BMP WCHARs.
static void FlUtf8Utf16ConverterUtBmpToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[3];
	size_t n = FlConvertUtf8ToUtf16Le(FlUtf8Len[1], (const char*)FlUtf8V1, 3, buf);
	FL_UT_CHECK(n == FlUtf16Len[1] && memcmp(buf, FlUtf16LeV1, FlUtf16Len[1] * sizeof(WCHAR)) == 0,
	            "FlUtf8Utf16ConverterUtBmpToUtf16Le");
}

// Four-byte sequences -> surrogate pairs: U+1F600 and U+10FFFF.
static void FlUtf8Utf16ConverterUtSurrogateToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf2[2], buf3[2];
	size_t n2 = FlConvertUtf8ToUtf16Le(FlUtf8Len[2], (const char*)FlUtf8V2, 2, buf2);
	size_t n3 = FlConvertUtf8ToUtf16Le(FlUtf8Len[3], (const char*)FlUtf8V3, 2, buf3);
	FL_UT_CHECK(
	    n2 == FlUtf16Len[2] && memcmp(buf2, FlUtf16LeV2, FlUtf16Len[2] * sizeof(WCHAR)) == 0 &&
	    n3 == FlUtf16Len[3] && memcmp(buf3, FlUtf16LeV3, FlUtf16Len[3] * sizeof(WCHAR)) == 0,
	    "FlUtf8Utf16ConverterUtSurrogateToUtf16Le");
}

// Mixed content: ASCII + 3-byte + ASCII + surrogate + ASCII -> 6 WCHARs.
static void FlUtf8Utf16ConverterUtMixedToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[6];
	size_t n = FlConvertUtf8ToUtf16Le(FlUtf8Len[4], (const char*)FlUtf8V4, 6, buf);
	FL_UT_CHECK(n == FlUtf16Len[4] && memcmp(buf, FlUtf16LeV4, FlUtf16Len[4] * sizeof(WCHAR)) == 0,
	            "FlUtf8Utf16ConverterUtMixedToUtf16Le");
}

// ---------------------------------------------------------------------------
// UTF-16 LE -> UTF-8 tests
// ---------------------------------------------------------------------------

// Five ASCII WCHARs -> "Hello" UTF-8 bytes.
static void FlUtf8Utf16ConverterUtAsciiToUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t buf[5];
	size_t n = FlConvertUtf16LeToUtf8(FlUtf16Len[0], FlUtf16LeV0, 5, (char*)buf);
	FL_UT_CHECK(n == FlUtf8Len[0] && memcmp(buf, FlUtf8V0, FlUtf8Len[0]) == 0,
	            "FlUtf8Utf16ConverterUtAsciiToUtf8");
}

// BMP WCHARs -> 2-byte and 3-byte UTF-8 sequences.
static void FlUtf8Utf16ConverterUtBmpToUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t buf[8];
	size_t n = FlConvertUtf16LeToUtf8(FlUtf16Len[1], FlUtf16LeV1, 8, (char*)buf);
	FL_UT_CHECK(n == FlUtf8Len[1] && memcmp(buf, FlUtf8V1, FlUtf8Len[1]) == 0,
	            "FlUtf8Utf16ConverterUtBmpToUtf8");
}

// Surrogate pairs -> four-byte UTF-8: U+1F600 and U+10FFFF.
static void FlUtf8Utf16ConverterUtSurrogateToUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t buf2[4], buf3[4];
	size_t n2 = FlConvertUtf16LeToUtf8(FlUtf16Len[2], FlUtf16LeV2, 4, (char*)buf2);
	size_t n3 = FlConvertUtf16LeToUtf8(FlUtf16Len[3], FlUtf16LeV3, 4, (char*)buf3);
	FL_UT_CHECK(
	    n2 == FlUtf8Len[2] && memcmp(buf2, FlUtf8V2, FlUtf8Len[2]) == 0 &&
	    n3 == FlUtf8Len[3] && memcmp(buf3, FlUtf8V3, FlUtf8Len[3]) == 0,
	    "FlUtf8Utf16ConverterUtSurrogateToUtf8");
}

// ---------------------------------------------------------------------------
// Empty input
// ---------------------------------------------------------------------------

// All four functions return 0 and write nothing for length-zero input.
static void FlUtf8Utf16ConverterUtEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR   wBuf[1] = { 0xFFFF };
	uint8_t cBuf[1] = { 0xFF };
	size_t r0 = FlConvertUtf8ToUtf16Le(0, "",     0, wBuf);
	size_t r1 = FlConvertUtf16LeToUtf8(0, wBuf,   0, (char*)cBuf);
	size_t r2 = FlConvertUtf8ToUtf16Be(0, "",     0, wBuf);
	size_t r3 = FlConvertUtf16BeToUtf8(0, wBuf,   0, (char*)cBuf);
	FL_UT_CHECK(r0 == 0 && r1 == 0 && r2 == 0 && r3 == 0,
	            "FlUtf8Utf16ConverterUtEmptyInput");
}

// ---------------------------------------------------------------------------
// Invalid input
// ---------------------------------------------------------------------------

// Invalid UTF-8 bytes (0xFF and 0x80) are each replaced by U+FFFD (WCHAR 0xFFFD).
static void FlUtf8Utf16ConverterUtInvalidUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t badFF[] = { 0xFF };
	static const uint8_t bad80[] = { 0x80 };
	WCHAR bufFF[1], buf80[1];
	size_t nFF = FlConvertUtf8ToUtf16Le(1, (const char*)badFF, 1, bufFF);
	size_t n80 = FlConvertUtf8ToUtf16Le(1, (const char*)bad80, 1, buf80);
	FL_UT_CHECK(
	    nFF == 1 && bufFF[0] == 0xFFFD &&
	    n80 == 1 && buf80[0] == 0xFFFD,
	    "FlUtf8Utf16ConverterUtInvalidUtf8");
}

// A lone high surrogate (0xD83D) and a lone low surrogate (0xDE00) in a UTF-16 LE
// stream are each replaced with U+FFFD = UTF-8 bytes EF BF BD.
static void FlUtf8Utf16ConverterUtLoneSurrogate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR loneHigh[] = { 0xD83D };
	static const WCHAR loneLow[]  = { 0xDE00 };
	static const uint8_t expectedFFFD[] = { 0xEF, 0xBF, 0xBD };
	uint8_t bufHigh[3], bufLow[3];
	size_t nHigh = FlConvertUtf16LeToUtf8(1, loneHigh, 3, (char*)bufHigh);
	size_t nLow  = FlConvertUtf16LeToUtf8(1, loneLow,  3, (char*)bufLow);
	FL_UT_CHECK(
	    nHigh == 3 && memcmp(bufHigh, expectedFFFD, 3) == 0 &&
	    nLow  == 3 && memcmp(bufLow,  expectedFFFD, 3) == 0,
	    "FlUtf8Utf16ConverterUtLoneSurrogate");
}

// ---------------------------------------------------------------------------
// Round-trip tests (using the mixed vector, which exercises all sequence lengths)
// ---------------------------------------------------------------------------

// UTF-8 -> UTF-16 LE -> UTF-8 restores the original bytes exactly.
static void FlUtf8Utf16ConverterUtRoundTripLe(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR   intermediate[6];
	uint8_t result[10];
	size_t nWide = FlConvertUtf8ToUtf16Le(FlUtf8Len[4], (const char*)FlUtf8V4, 6, intermediate);
	size_t nUtf8 = FlConvertUtf16LeToUtf8(nWide, intermediate, 10, (char*)result);
	FL_UT_CHECK(nUtf8 == FlUtf8Len[4] && memcmp(result, FlUtf8V4, FlUtf8Len[4]) == 0,
	            "FlUtf8Utf16ConverterUtRoundTripLe");
}

// UTF-8 -> UTF-16 BE -> UTF-8 restores the original bytes exactly.
static void FlUtf8Utf16ConverterUtRoundTripBe(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR   intermediate[6];
	uint8_t result[10];
	size_t nWide = FlConvertUtf8ToUtf16Be(FlUtf8Len[4], (const char*)FlUtf8V4, 6, intermediate);
	size_t nUtf8 = FlConvertUtf16BeToUtf8(nWide, intermediate, 10, (char*)result);
	FL_UT_CHECK(nUtf8 == FlUtf8Len[4] && memcmp(result, FlUtf8V4, FlUtf8Len[4]) == 0,
	            "FlUtf8Utf16ConverterUtRoundTripBe");
}

// ---------------------------------------------------------------------------
// Endianness test
// ---------------------------------------------------------------------------

// Converting the same UTF-8 to LE and to BE must yield WCHAR values that are
// byte-swapped mirror images of each other: BE[i] == (LE[i] << 8) | (LE[i] >> 8).
static void FlUtf8Utf16ConverterUtBeEndianness(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR leBuf[6], beBuf[6];
	FlConvertUtf8ToUtf16Le(FlUtf8Len[4], (const char*)FlUtf8V4, 6, leBuf);
	FlConvertUtf8ToUtf16Be(FlUtf8Len[4], (const char*)FlUtf8V4, 6, beBuf);

	// Cross-check against expected BE table to ensure both functions are correct.
	int correct = memcmp(leBuf, FlUtf16LeV4, FlUtf16Len[4] * sizeof(WCHAR)) == 0 &&
	              memcmp(beBuf, FlUtf16BeV4, FlUtf16Len[4] * sizeof(WCHAR)) == 0;

	// Additionally verify the byte-swap relationship holds element-by-element.
	for (size_t i = 0; i < FlUtf16Len[4] && correct; i++)
	{
		WCHAR swapped = (WCHAR)((leBuf[i] << 8) | (leBuf[i] >> 8));
		if (beBuf[i] != swapped)
			correct = 0;
	}
	FL_UT_CHECK(correct, "FlUtf8Utf16ConverterUtBeEndianness");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlUtf8Utf16ConverterUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlUtf8Utf16ConverterUtAsciiToUtf16Le(testCount, failCount);
	FlUtf8Utf16ConverterUtBmpToUtf16Le(testCount, failCount);
	FlUtf8Utf16ConverterUtSurrogateToUtf16Le(testCount, failCount);
	FlUtf8Utf16ConverterUtMixedToUtf16Le(testCount, failCount);
	FlUtf8Utf16ConverterUtAsciiToUtf8(testCount, failCount);
	FlUtf8Utf16ConverterUtBmpToUtf8(testCount, failCount);
	FlUtf8Utf16ConverterUtSurrogateToUtf8(testCount, failCount);
	FlUtf8Utf16ConverterUtEmptyInput(testCount, failCount);
	FlUtf8Utf16ConverterUtInvalidUtf8(testCount, failCount);
	FlUtf8Utf16ConverterUtLoneSurrogate(testCount, failCount);
	FlUtf8Utf16ConverterUtRoundTripLe(testCount, failCount);
	FlUtf8Utf16ConverterUtRoundTripBe(testCount, failCount);
	FlUtf8Utf16ConverterUtBeEndianness(testCount, failCount);
}
