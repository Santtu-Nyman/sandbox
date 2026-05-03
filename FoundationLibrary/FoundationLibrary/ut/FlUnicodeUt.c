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

		Unit tests for FlConvertUtf8ToUtf16, FlConvertUtf16ToUtf8,
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
#include "../include/FlUnicode.h"
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
static void FlUnicodeUtAsciiToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[5];
	size_t n = FlConvertUtf8ToUtf16(FlUtf8Len[0], (const char*)FlUtf8V0, 5, buf);
	FL_UT_CHECK(n == FlUtf16Len[0] && memcmp(buf, FlUtf16LeV0, FlUtf16Len[0] * sizeof(WCHAR)) == 0,
	            "FlUnicodeUtAsciiToUtf16Le");
}

// Two-byte (U+00E9) and three-byte (U+4E16, U+20AC) sequences -> BMP WCHARs.
static void FlUnicodeUtBmpToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[3];
	size_t n = FlConvertUtf8ToUtf16(FlUtf8Len[1], (const char*)FlUtf8V1, 3, buf);
	FL_UT_CHECK(n == FlUtf16Len[1] && memcmp(buf, FlUtf16LeV1, FlUtf16Len[1] * sizeof(WCHAR)) == 0,
	            "FlUnicodeUtBmpToUtf16Le");
}

// Four-byte sequences -> surrogate pairs: U+1F600 and U+10FFFF.
static void FlUnicodeUtSurrogateToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf2[2], buf3[2];
	size_t n2 = FlConvertUtf8ToUtf16(FlUtf8Len[2], (const char*)FlUtf8V2, 2, buf2);
	size_t n3 = FlConvertUtf8ToUtf16(FlUtf8Len[3], (const char*)FlUtf8V3, 2, buf3);
	FL_UT_CHECK(
	    n2 == FlUtf16Len[2] && memcmp(buf2, FlUtf16LeV2, FlUtf16Len[2] * sizeof(WCHAR)) == 0 &&
	    n3 == FlUtf16Len[3] && memcmp(buf3, FlUtf16LeV3, FlUtf16Len[3] * sizeof(WCHAR)) == 0,
	    "FlUnicodeUtSurrogateToUtf16Le");
}

// Mixed content: ASCII + 3-byte + ASCII + surrogate + ASCII -> 6 WCHARs.
static void FlUnicodeUtMixedToUtf16Le(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[6];
	size_t n = FlConvertUtf8ToUtf16(FlUtf8Len[4], (const char*)FlUtf8V4, 6, buf);
	FL_UT_CHECK(n == FlUtf16Len[4] && memcmp(buf, FlUtf16LeV4, FlUtf16Len[4] * sizeof(WCHAR)) == 0,
	            "FlUnicodeUtMixedToUtf16Le");
}

// ---------------------------------------------------------------------------
// UTF-16 LE -> UTF-8 tests
// ---------------------------------------------------------------------------

// Five ASCII WCHARs -> "Hello" UTF-8 bytes.
static void FlUnicodeUtAsciiToUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t buf[5];
	size_t n = FlConvertUtf16ToUtf8(FlUtf16Len[0], FlUtf16LeV0, 5, (char*)buf);
	FL_UT_CHECK(n == FlUtf8Len[0] && memcmp(buf, FlUtf8V0, FlUtf8Len[0]) == 0,
	            "FlUnicodeUtAsciiToUtf8");
}

// BMP WCHARs -> 2-byte and 3-byte UTF-8 sequences.
static void FlUnicodeUtBmpToUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t buf[8];
	size_t n = FlConvertUtf16ToUtf8(FlUtf16Len[1], FlUtf16LeV1, 8, (char*)buf);
	FL_UT_CHECK(n == FlUtf8Len[1] && memcmp(buf, FlUtf8V1, FlUtf8Len[1]) == 0,
	            "FlUnicodeUtBmpToUtf8");
}

// Surrogate pairs -> four-byte UTF-8: U+1F600 and U+10FFFF.
static void FlUnicodeUtSurrogateToUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t buf2[4], buf3[4];
	size_t n2 = FlConvertUtf16ToUtf8(FlUtf16Len[2], FlUtf16LeV2, 4, (char*)buf2);
	size_t n3 = FlConvertUtf16ToUtf8(FlUtf16Len[3], FlUtf16LeV3, 4, (char*)buf3);
	FL_UT_CHECK(
	    n2 == FlUtf8Len[2] && memcmp(buf2, FlUtf8V2, FlUtf8Len[2]) == 0 &&
	    n3 == FlUtf8Len[3] && memcmp(buf3, FlUtf8V3, FlUtf8Len[3]) == 0,
	    "FlUnicodeUtSurrogateToUtf8");
}

// ---------------------------------------------------------------------------
// Empty input
// ---------------------------------------------------------------------------

// All four functions return 0 and write nothing for length-zero input.
static void FlUnicodeUtEmptyInput(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR   wBuf[1] = { 0xFFFF };
	uint8_t cBuf[1] = { 0xFF };
	size_t r0 = FlConvertUtf8ToUtf16(0, "",     0, wBuf);
	size_t r1 = FlConvertUtf16ToUtf8(0, wBuf,   0, (char*)cBuf);
	FL_UT_CHECK(r0 == 0 && r1 == 0,
	            "FlUnicodeUtEmptyInput");
}

// ---------------------------------------------------------------------------
// Invalid input
// ---------------------------------------------------------------------------

// Invalid UTF-8 bytes (0xFF and 0x80) are each replaced by U+FFFD (WCHAR 0xFFFD).
static void FlUnicodeUtInvalidUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t badFF[] = { 0xFF };
	static const uint8_t bad80[] = { 0x80 };
	WCHAR bufFF[1], buf80[1];
	size_t nFF = FlConvertUtf8ToUtf16(1, (const char*)badFF, 1, bufFF);
	size_t n80 = FlConvertUtf8ToUtf16(1, (const char*)bad80, 1, buf80);
	FL_UT_CHECK(
	    nFF == 1 && bufFF[0] == 0xFFFD &&
	    n80 == 1 && buf80[0] == 0xFFFD,
	    "FlUnicodeUtInvalidUtf8");
}

// A lone high surrogate (0xD83D) and a lone low surrogate (0xDE00) in a UTF-16 LE
// stream are each replaced with U+FFFD = UTF-8 bytes EF BF BD.
static void FlUnicodeUtLoneSurrogate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR loneHigh[] = { 0xD83D };
	static const WCHAR loneLow[]  = { 0xDE00 };
	static const uint8_t expectedFFFD[] = { 0xEF, 0xBF, 0xBD };
	uint8_t bufHigh[3], bufLow[3];
	size_t nHigh = FlConvertUtf16ToUtf8(1, loneHigh, 3, (char*)bufHigh);
	size_t nLow  = FlConvertUtf16ToUtf8(1, loneLow,  3, (char*)bufLow);
	FL_UT_CHECK(
	    nHigh == 3 && memcmp(bufHigh, expectedFFFD, 3) == 0 &&
	    nLow  == 3 && memcmp(bufLow,  expectedFFFD, 3) == 0,
	    "FlUnicodeUtLoneSurrogate");
}

// ---------------------------------------------------------------------------
// Round-trip tests (using the mixed vector, which exercises all sequence lengths)
// ---------------------------------------------------------------------------

// UTF-8 -> UTF-16 LE -> UTF-8 restores the original bytes exactly.
static void FlUnicodeUtRoundTripLe(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR   intermediate[6];
	uint8_t result[10];
	size_t nWide = FlConvertUtf8ToUtf16(FlUtf8Len[4], (const char*)FlUtf8V4, 6, intermediate);
	size_t nUtf8 = FlConvertUtf16ToUtf8(nWide, intermediate, 10, (char*)result);
	FL_UT_CHECK(nUtf8 == FlUtf8Len[4] && memcmp(result, FlUtf8V4, FlUtf8Len[4]) == 0,
	            "FlUnicodeUtRoundTripLe");
}


static const int FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[26] = { 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A };
static const int FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[26] = { 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A };

static const int FL_UNICODE_UT_UPPER_CASE_GERMAN_ALPHABET[30] = { 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00DC, 0x1E9E };
static const int FL_UNICODE_UT_LOWER_CASE_GERMAN_ALPHABET[30] = { 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00FC, 0x00DF };

static const int FL_UNICODE_UT_UPPER_CASE_FINNISH_ALPHABET[29] = { 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00C5, 0x00C4, 0x00D6 };
static const int FL_UNICODE_UT_LOWER_CASE_FINNISH_ALPHABET[29] = { 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x00E5, 0x00E4, 0x00F6 };

static const int FL_UNICODE_UT_UPPER_CASE_RUSSIAN_ALPHABET[33] = { 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0401, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F };
static const int FL_UNICODE_UT_LOWER_CASE_RUSSIAN_ALPHABET[33] = { 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0451, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F };

static const int FL_UNICODE_UT_UPPER_CASE_GREEK_ALPHABET[24] = { 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F, 0x03A0, 0x03A1, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9 };
static const int FL_UNICODE_UT_LOWER_CASE_GREEK_ALPHABET[24] = { 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF, 0x03C0, 0x03C1, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9 };

// ---------------------------------------------------------------------------
// FlCodepointToUpperCase
// ---------------------------------------------------------------------------

// ASCII lowercase letters must map to their uppercase equivalents.
static void FlUnicodeUtUpperCaseAsciiLowercase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET) / sizeof(FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[0])); i++)
	{
		if (FlCodepointToUpperCase(FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[i]) != FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtUpperCaseAsciiLowercase");
}

// ASCII uppercase letters must be returned unchanged.
static void FlUnicodeUtUpperCaseAsciiUppercaseUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET) / sizeof(FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[0])); i++)
	{
		if (FlCodepointToUpperCase(FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[i]) != FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtUpperCaseAsciiUppercaseUnchanged");
}

// ASCII non-alpha codepoints (digits, punctuation, space) must be returned unchanged.
static void FlUnicodeUtUpperCaseAsciiNonAlphaUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	if (FlCodepointToUpperCase(' ') != ' ' || FlCodepointToUpperCase('!') != '!' || FlCodepointToUpperCase('?') != '?' || FlCodepointToUpperCase('@') != '@' || FlCodepointToUpperCase('_') != '_')
	{
		caseMappingSucceeded = 0;
	}
	for (int digit = 0; digit < 10; digit++)
	{
		if (FlCodepointToUpperCase(0x30 + digit) != 0x30 + digit)
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtUpperCaseAsciiNonAlphaUnchanged");
}

// Non-ASCII codepoints that have uppercase equivalents must map correctly.
static void FlUnicodeUtUpperCaseNonAscii(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_LOWER_CASE_GERMAN_ALPHABET) / sizeof(FL_UNICODE_UT_LOWER_CASE_GERMAN_ALPHABET[0])) - 1; i++)
	{
		if (FlCodepointToUpperCase(FL_UNICODE_UT_LOWER_CASE_GERMAN_ALPHABET[i]) != FL_UNICODE_UT_UPPER_CASE_GERMAN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_LOWER_CASE_FINNISH_ALPHABET) / sizeof(FL_UNICODE_UT_LOWER_CASE_FINNISH_ALPHABET[0])); i++)
	{
		if (FlCodepointToUpperCase(FL_UNICODE_UT_LOWER_CASE_FINNISH_ALPHABET[i]) != FL_UNICODE_UT_UPPER_CASE_FINNISH_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_LOWER_CASE_RUSSIAN_ALPHABET) / sizeof(FL_UNICODE_UT_LOWER_CASE_RUSSIAN_ALPHABET[0])); i++)
	{
		if (FlCodepointToUpperCase(FL_UNICODE_UT_LOWER_CASE_RUSSIAN_ALPHABET[i]) != FL_UNICODE_UT_UPPER_CASE_RUSSIAN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_LOWER_CASE_GREEK_ALPHABET) / sizeof(FL_UNICODE_UT_LOWER_CASE_GREEK_ALPHABET[0])); i++)
	{
		if (FlCodepointToUpperCase(FL_UNICODE_UT_LOWER_CASE_GREEK_ALPHABET[i]) != FL_UNICODE_UT_UPPER_CASE_GREEK_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtUpperCaseNonAscii");
}

// Codepoint values outside the valid Unicode range must be returned unchanged.
static void FlUnicodeUtUpperCaseOutOfRange(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FL_UT_CHECK(FlCodepointToUpperCase(-1) == -1, "FlUnicodeUtUpperCaseOutOfRange_neg");
	FL_UT_CHECK(FlCodepointToUpperCase(0x110000) == 0x110000, "FlUnicodeUtUpperCaseOutOfRange_high");
}

// ---------------------------------------------------------------------------
// FlCodepointToLowerCase
// ---------------------------------------------------------------------------

// ASCII uppercase letters must map to their lowercase equivalents.
static void FlUnicodeUtLowerCaseAsciiUppercase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET) / sizeof(FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[0])); i++)
	{
		if (FlCodepointToLowerCase(FL_UNICODE_UT_UPPER_CASE_LATIN_ALPHABET[i]) != FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtLowerCaseAsciiUppercase");
}

// ASCII lowercase letters must be returned unchanged.
static void FlUnicodeUtLowerCaseAsciiLowercaseUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET) / sizeof(FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[0])); i++)
	{
		if (FlCodepointToLowerCase(FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[i]) != FL_UNICODE_UT_LOWER_CASE_LATIN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtLowerCaseAsciiLowercaseUnchanged");
}

// ASCII non-alpha codepoints must be returned unchanged.
static void FlUnicodeUtLowerCaseAsciiNonAlphaUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	if (FlCodepointToLowerCase(' ') != ' ' || FlCodepointToLowerCase('!') != '!' || FlCodepointToLowerCase('?') != '?' || FlCodepointToLowerCase('@') != '@' || FlCodepointToLowerCase('_') != '_')
	{
		caseMappingSucceeded = 0;
	}
	for (int digit = 0; digit < 10; digit++)
	{
		if (FlCodepointToLowerCase(0x30 + digit) != 0x30 + digit)
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtLowerCaseAsciiNonAlphaUnchanged");
}

// Non-ASCII codepoints that have lowercase equivalents must map correctly.
static void FlUnicodeUtLowerCaseNonAscii(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int caseMappingSucceeded = 1;
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_UPPER_CASE_GERMAN_ALPHABET) / sizeof(FL_UNICODE_UT_UPPER_CASE_GERMAN_ALPHABET[0])) - 1; i++)
	{
		if (FlCodepointToLowerCase(FL_UNICODE_UT_UPPER_CASE_GERMAN_ALPHABET[i]) != FL_UNICODE_UT_LOWER_CASE_GERMAN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_UPPER_CASE_FINNISH_ALPHABET) / sizeof(FL_UNICODE_UT_UPPER_CASE_FINNISH_ALPHABET[0])); i++)
	{
		if (FlCodepointToLowerCase(FL_UNICODE_UT_UPPER_CASE_FINNISH_ALPHABET[i]) != FL_UNICODE_UT_LOWER_CASE_FINNISH_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_UPPER_CASE_RUSSIAN_ALPHABET) / sizeof(FL_UNICODE_UT_UPPER_CASE_RUSSIAN_ALPHABET[0])); i++)
	{
		if (FlCodepointToLowerCase(FL_UNICODE_UT_UPPER_CASE_RUSSIAN_ALPHABET[i]) != FL_UNICODE_UT_LOWER_CASE_RUSSIAN_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	for (size_t i = 0; i < (sizeof(FL_UNICODE_UT_UPPER_CASE_GREEK_ALPHABET) / sizeof(FL_UNICODE_UT_UPPER_CASE_GREEK_ALPHABET[0])); i++)
	{
		if (FlCodepointToLowerCase(FL_UNICODE_UT_UPPER_CASE_GREEK_ALPHABET[i]) != FL_UNICODE_UT_LOWER_CASE_GREEK_ALPHABET[i])
		{
			caseMappingSucceeded = 0;
			break;
		}
	}
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeUtLowerCaseNonAscii");
}

// Codepoint values outside the valid Unicode range must be returned unchanged.
static void FlUnicodeUtLowerCaseOutOfRange(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FL_UT_CHECK(FlCodepointToLowerCase(-1) == -1, "FlUnicodeUtLowerCaseOutOfRange_neg");
	FL_UT_CHECK(FlCodepointToLowerCase(0x110000) == 0x110000, "FlUnicodeUtLowerCaseOutOfRange_high");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf8 — ASCII
// ---------------------------------------------------------------------------

// Identical ASCII strings must compare as equal (case-sensitive).
static void FlUnicodeUtUtf8CaseSensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "hello";
	static const char strB[] = "hello";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8CaseSensitiveEqual");
}

// "abc" must compare as less than "abd" (case-sensitive).
static void FlUnicodeUtUtf8CaseSensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "abc";
	static const char strB[] = "abd";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf8CaseSensitiveLess");
}

// "abd" must compare as greater than "abc" (case-sensitive).
static void FlUnicodeUtUtf8CaseSensitiveGreater(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "abd";
	static const char strB[] = "abc";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeUtUtf8CaseSensitiveGreater");
}

// "abc" must compare as greater than "ABC" in case-sensitive mode because the
// raw byte value of 'a' (0x61) is greater than 'A' (0x41).
static void FlUnicodeUtUtf8CaseSensitiveDifferentCase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strLower[] = "abc";
	static const char strUpper[] = "ABC";
	int result = FlCompareStringOrdinalUtf8(strLower, sizeof(strLower) - 1, strUpper, sizeof(strUpper) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeUtUtf8CaseSensitiveDifferentCase");
}

// "hello" and "HELLO" must compare as equal when case is ignored.
static void FlUnicodeUtUtf8CaseInsensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strLower[] = "hello";
	static const char strUpper[] = "HELLO";
	int result = FlCompareStringOrdinalUtf8(strLower, sizeof(strLower) - 1, strUpper, sizeof(strUpper) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveEqual");
}

// "abc" must compare as less than "abd" in case-insensitive mode.
static void FlUnicodeUtUtf8CaseInsensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "abc";
	static const char strB[] = "abd";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf8CaseInsensitiveLess");
}

// "001000" must compare as less than "002000".
static void FlUnicodeUtUtf8CaseNumberLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char str001[] = "001000";
	static const char str002[] = "002000";
	int result = FlCompareStringOrdinalUtf8(str001, sizeof(str001) - 1, str002, sizeof(str002) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf8CaseNumberLess");
}

// Two empty strings must compare as equal in both modes.
static void FlUnicodeUtUtf8EmptyEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char empty[] = "";
	int resultCS = FlCompareStringOrdinalUtf8(empty, 0, empty, 0, FALSE);
	int resultCI = FlCompareStringOrdinalUtf8(empty, 0, empty, 0, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeUtUtf8EmptyEqual_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeUtUtf8EmptyEqual_CI");
}

// An empty string must compare as less than a non-empty string (case-sensitive).
static void FlUnicodeUtUtf8EmptyLessThanNonEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char empty[] = "";
	static const char strA[] = "a";
	int result = FlCompareStringOrdinalUtf8(empty, 0, strA, sizeof(strA) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf8EmptyLessThanNonEmpty");
}

// Passing (size_t)-1 as length must give the same result as passing the
// explicit byte count for a null-terminated string.
static void FlUnicodeUtUtf8NullTerminated(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "hello";
	static const char strB[] = "hello";
	int resultCS = FlCompareStringOrdinalUtf8(strA, (size_t)-1, strB, (size_t)-1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf8(strA, (size_t)-1, strB, (size_t)-1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeUtUtf8NullTerminated_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeUtUtf8NullTerminated_CI");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf8 — Non-ASCII
// ---------------------------------------------------------------------------

// U+00E4 (ä, UTF-8: 0xC3 0xA4) must compare equal to U+00C4 (Ä, UTF-8: 0xC3 0x84)
// when case is ignored.
static void FlUnicodeUtUtf8CaseInsensitiveAeEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xA4 }; // ä  U+00E4
	static const uint8_t upper[] = { 0xC3, 0x84 }; // Ä  U+00C4
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveAeEqual");
}

// U+00F6 (ö, UTF-8: 0xC3 0xB6) must compare equal to U+00D6 (Ö, UTF-8: 0xC3 0x96)
// when case is ignored.
static void FlUnicodeUtUtf8CaseInsensitiveOeEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xB6 }; // ö  U+00F6
	static const uint8_t upper[] = { 0xC3, 0x96 }; // Ö  U+00D6
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveOeEqual");
}

// U+00FC (ü, UTF-8: 0xC3 0xBC) must compare equal to U+00DC (Ü, UTF-8: 0xC3 0x9C)
// when case is ignored.
static void FlUnicodeUtUtf8CaseInsensitiveUeEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xBC }; // ü  U+00FC
	static const uint8_t upper[] = { 0xC3, 0x9C }; // Ü  U+00DC
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveUeEqual");
}

// U+00E5 (å, UTF-8: 0xC3 0xA5) must compare equal to U+00C5 (Å, UTF-8: 0xC3 0x85)
// when case is ignored.
static void FlUnicodeUtUtf8CaseInsensitiveAaEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xA5 }; // å  U+00E5
	static const uint8_t upper[] = { 0xC3, 0x85 }; // Å  U+00C5
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveAaEqual");
}

// ä (U+00E4) and ö (U+00F6) are distinct characters; they must not compare as
// equal in either case-sensitive or case-insensitive mode.
static void FlUnicodeUtUtf8CaseInsensitiveNonEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t ae[] = { 0xC3, 0xA4 }; // ä  U+00E4
	static const uint8_t oe[] = { 0xC3, 0xB6 }; // ö  U+00F6
	int resultCI = FlCompareStringOrdinalUtf8((const char*)ae, sizeof ae, (const char*)oe, sizeof oe, TRUE);
	int resultCS = FlCompareStringOrdinalUtf8((const char*)ae, sizeof ae, (const char*)oe, sizeof oe, FALSE);
	FL_UT_CHECK(resultCI != CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveNonEqual_CI");
	FL_UT_CHECK(resultCS != CSTR_EQUAL, "FlUnicodeUtUtf8CaseInsensitiveNonEqual_CS");
}

// German: "schön" (s c h U+00F6 n) must compare equal to "SCHÖN" (S C H U+00D6 N)
// when case is ignored, and must not compare equal to "schon" (plain o, no umlaut)
// even when case is ignored.
static void FlUnicodeUtUtf8GermanCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "schön": s  c     h     ö           n
	static const uint8_t schoen[] = { 0x73, 0x63, 0x68, 0xC3, 0xB6, 0x6E };
	// "SCHÖN": S  C     H     Ö           N
	static const uint8_t SCHOEN[] = { 0x53, 0x43, 0x48, 0xC3, 0x96, 0x4E };
	// "schon": s  c     h     o     n  (plain o, no umlaut)
	static const uint8_t schon[] = { 0x73, 0x63, 0x68, 0x6F, 0x6E };

	int resultEqual = FlCompareStringOrdinalUtf8((const char*)schoen, sizeof schoen, (const char*)SCHOEN, sizeof SCHOEN, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf8((const char*)schoen, sizeof schoen, (const char*)schon, sizeof schon, TRUE);

	FL_UT_CHECK(resultEqual == CSTR_EQUAL, "FlUnicodeUtUtf8GermanCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeUtUtf8GermanCaseInsensitive_NotEqual");
}

// Finnish: "äiti" (U+00E4 i t i) must compare equal to "ÄITI" (U+00C4 I T I)
// when case is ignored, and must not compare equal to "aiti" (plain a, no umlaut)
// even when case is ignored.
static void FlUnicodeUtUtf8FinnishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "äiti": ä           i     t     i
	static const uint8_t aiti_lower[] = { 0xC3, 0xA4, 0x69, 0x74, 0x69 };
	// "ÄITI": Ä           I     T     I
	static const uint8_t AITI_upper[] = { 0xC3, 0x84, 0x49, 0x54, 0x49 };
	// "aiti": a  i  t  i  (plain a, no umlaut)
	static const uint8_t aiti_plain[] = { 0x61, 0x69, 0x74, 0x69 };

	int resultEqual = FlCompareStringOrdinalUtf8((const char*)aiti_lower, sizeof aiti_lower, (const char*)AITI_upper, sizeof AITI_upper, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf8((const char*)aiti_lower, sizeof aiti_lower, (const char*)aiti_plain, sizeof aiti_plain, TRUE);

	FL_UT_CHECK(resultEqual == CSTR_EQUAL, "FlUnicodeUtUtf8FinnishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeUtUtf8FinnishCaseInsensitive_NotEqual");
}

// Swedish: "ålder" (U+00E5 l d e r) must compare equal to "ÅLDER" (U+00C5 L D E R)
// when case is ignored, and must not compare equal to "alder" (plain a, no ring)
// even when case is ignored.
static void FlUnicodeUtUtf8SwedishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "ålder": å           l     d     e     r
	static const uint8_t alder_lower[] = { 0xC3, 0xA5, 0x6C, 0x64, 0x65, 0x72 };
	// "ÅLDER": Å           L     D     E     R
	static const uint8_t ALDER_upper[] = { 0xC3, 0x85, 0x4C, 0x44, 0x45, 0x52 };
	// "alder": a  l  d  e  r  (plain a, no ring)
	static const uint8_t alder_plain[] = { 0x61, 0x6C, 0x64, 0x65, 0x72 };

	int resultEqual = FlCompareStringOrdinalUtf8((const char*)alder_lower, sizeof alder_lower, (const char*)ALDER_upper, sizeof ALDER_upper, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf8((const char*)alder_lower, sizeof alder_lower, (const char*)alder_plain, sizeof alder_plain, TRUE);

	FL_UT_CHECK(resultEqual == CSTR_EQUAL, "FlUnicodeUtUtf8SwedishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeUtUtf8SwedishCaseInsensitive_NotEqual");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf8 — Invalid sequences
// ---------------------------------------------------------------------------

// Two strings that consist of the same invalid byte must compare as equal
// (case-sensitive).  Invalid bytes at the same position are compared by raw value.
static void FlUnicodeUtUtf8InvalidEqualSame(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalid[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalid, sizeof invalid, (const char*)invalid, sizeof invalid, FALSE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8InvalidEqualSame");
}

// Same invalid byte must also compare as equal in case-insensitive mode.
static void FlUnicodeUtUtf8InvalidEqualSameCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalid[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalid, sizeof invalid, (const char*)invalid, sizeof invalid, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf8InvalidEqualSameCaseInsensitive");
}

// A string with invalid byte 0xFE must compare as not equal to a string with
// invalid byte 0xFF (case-sensitive).
static void FlUnicodeUtUtf8InvalidNotEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalidLow[] = { 0xFE };
	static const uint8_t invalidHigh[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalidLow, sizeof invalidLow, (const char*)invalidHigh, sizeof invalidHigh, FALSE);
	FL_UT_CHECK(result != CSTR_EQUAL, "FlUnicodeUtUtf8InvalidNotEqual");
}

// The same raw-bytes must compare as not equal in case-insensitive mode.
static void FlUnicodeUtUtf8InvalidNotEqualCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalidLow[] = { 0xFE };
	static const uint8_t invalidHigh[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalidLow, sizeof invalidLow, (const char*)invalidHigh, sizeof invalidHigh, TRUE);
	FL_UT_CHECK(result != CSTR_EQUAL, "FlUnicodeUtUtf8InvalidNotEqualCaseInsensitive");
}

// Ordering of strings most still be valid after chunk of invalid data if that data is the same in both strings
static void FlUnicodeUtUtf8ValidAfterInvalid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalidAStr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x41 };
	static const uint8_t invalidBStr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x42 };

	int resultCI = FlCompareStringOrdinalUtf8((const char*)invalidAStr, sizeof invalidAStr, (const char*)invalidBStr, sizeof invalidBStr, TRUE);
	int resultCS = FlCompareStringOrdinalUtf8((const char*)invalidAStr, sizeof invalidAStr, (const char*)invalidBStr, sizeof invalidBStr, FALSE);

	FL_UT_CHECK(resultCI == CSTR_LESS_THAN && resultCS == CSTR_LESS_THAN, "FlUnicodeUtUtf8ValidAfterInvalid");
}

// Comparison between an invalid and a valid strings must be not equal.
static void FlUnicodeUtUtf8ValidVsInvalid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char    validStr[] = "a";
	static const uint8_t invalidStr[] = { 0xFF };

	int resultValidCS = FlCompareStringOrdinalUtf8(validStr, sizeof(validStr) - 1, (const char*)invalidStr, sizeof invalidStr, FALSE);
	int resultValidCI = FlCompareStringOrdinalUtf8(validStr, sizeof(validStr) - 1, (const char*)invalidStr, sizeof invalidStr, TRUE);
	int resultInvalidCS = FlCompareStringOrdinalUtf8((const char*)invalidStr, sizeof invalidStr, validStr, sizeof(validStr) - 1, FALSE);
	int resultInvalidCI = FlCompareStringOrdinalUtf8((const char*)invalidStr, sizeof invalidStr, validStr, sizeof(validStr) - 1, TRUE);

	// Case-sensitive: raw byte 0x61 ('a') != 0xFF.
	FL_UT_CHECK(resultValidCS != CSTR_EQUAL, "FlUnicodeUtUtf8ValidVsInvalid_Valid_CS");
	FL_UT_CHECK(resultInvalidCS != CSTR_EQUAL, "FlUnicodeUtUtf8ValidVsInvalid_Invalid_CS");
	// Case-insensitive: valid code point position != invalid sequence position.
	FL_UT_CHECK(resultValidCI != CSTR_EQUAL, "FlUnicodeUtUtf8ValidVsInvalid_Valid_CI");
	FL_UT_CHECK(resultInvalidCI != CSTR_EQUAL, "FlUnicodeUtUtf8ValidVsInvalid_Invalid_CI");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf16 — ASCII
// ---------------------------------------------------------------------------

// Identical ASCII strings must compare as equal (case-sensitive).
static void FlUnicodeUtUtf16CaseSensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"hello";
	static const WCHAR strB[] = L"hello";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf16CaseSensitiveEqual");
}

// "abc" must compare as less than "abd" (case-sensitive).
static void FlUnicodeUtUtf16CaseSensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"abc";
	static const WCHAR strB[] = L"abd";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf16CaseSensitiveLess");
}

// "abc" must compare as greater than "ABC" in case-sensitive mode because the
// raw WCHAR value of 'a' (0x0061) is greater than 'A' (0x0041).
static void FlUnicodeUtUtf16CaseSensitiveDifferentCase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strLower[] = L"abc";
	static const WCHAR strUpper[] = L"ABC";
	int result = FlCompareStringOrdinalUtf16(strLower, (sizeof(strLower) / sizeof(strLower[0])) - 1, strUpper, (sizeof(strUpper) / sizeof(strUpper[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeUtUtf16CaseSensitiveDifferentCase");
}

// "hello" and "HELLO" must compare as equal when case is ignored.
static void FlUnicodeUtUtf16CaseInsensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strLower[] = L"hello";
	static const WCHAR strUpper[] = L"HELLO";
	int result = FlCompareStringOrdinalUtf16(strLower, (sizeof(strLower) / sizeof(strLower[0])) - 1, strUpper, (sizeof(strUpper) / sizeof(strUpper[0])) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveEqual");
}

// Two empty UTF-16 strings must compare as equal in both modes.
static void FlUnicodeUtUtf16EmptyEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR empty[] = L"";
	int resultCS = FlCompareStringOrdinalUtf16(empty, 0, empty, 0, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(empty, 0, empty, 0, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeUtUtf16EmptyEqual_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeUtUtf16EmptyEqual_CI");
}

// "abd" must compare as greater than "abc" (case-sensitive).
static void FlUnicodeUtUtf16CaseSensitiveGreater(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"abd";
	static const WCHAR strB[] = L"abc";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeUtUtf16CaseSensitiveGreater");
}

// "abc" must compare as less than "abd" in case-insensitive mode.
static void FlUnicodeUtUtf16CaseInsensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"abc";
	static const WCHAR strB[] = L"abd";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf16CaseInsensitiveLess");
}

// "001000" must compare as less than "002000".
static void FlUnicodeUtUtf16CaseNumberLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR str001[] = L"001000";
	static const WCHAR str002[] = L"002000";
	int result = FlCompareStringOrdinalUtf16(str001, (sizeof(str001) / sizeof(str001[0])) - 1, str002, (sizeof(str002) / sizeof(str002[0])) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf16CaseNumberLess");
}

// An empty string must compare as less than a non-empty string (case-sensitive).
static void FlUnicodeUtUtf16EmptyLessThanNonEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR empty[] = L"";
	static const WCHAR strA[] = L"a";
	int result = FlCompareStringOrdinalUtf16(empty, 0, strA, (sizeof(strA) / sizeof(strA[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeUtUtf16EmptyLessThanNonEmpty");
}

// Passing (size_t)-1 as length must give the same result as passing the
// explicit code-unit count for a null-terminated string.
static void FlUnicodeUtUtf16NullTerminated(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"hello";
	static const WCHAR strB[] = L"hello";
	int resultCS = FlCompareStringOrdinalUtf16(strA, (size_t)-1, strB, (size_t)-1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(strA, (size_t)-1, strB, (size_t)-1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeUtUtf16NullTerminated_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeUtUtf16NullTerminated_CI");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf16 — Non-ASCII
// ---------------------------------------------------------------------------

// Non-ASCII letters with case pairs must compare as equal when case is ignored.
// U+00E4 (ä) == U+00C4 (Ä), U+00F6 (ö) == U+00D6 (Ö), U+00FC (ü) == U+00DC (Ü),
// U+00E5 (å) == U+00C5 (Å).
static void FlUnicodeUtUtf16CaseInsensitiveNonAsciiEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR ae_lower[] = { 0x00E4, 0 }; // ä  U+00E4
	static const WCHAR ae_upper[] = { 0x00C4, 0 }; // Ä  U+00C4
	static const WCHAR oe_lower[] = { 0x00F6, 0 }; // ö  U+00F6
	static const WCHAR oe_upper[] = { 0x00D6, 0 }; // Ö  U+00D6
	static const WCHAR ue_lower[] = { 0x00FC, 0 }; // ü  U+00FC
	static const WCHAR ue_upper[] = { 0x00DC, 0 }; // Ü  U+00DC
	static const WCHAR aa_lower[] = { 0x00E5, 0 }; // å  U+00E5
	static const WCHAR aa_upper[] = { 0x00C5, 0 }; // Å  U+00C5

	FL_UT_CHECK(FlCompareStringOrdinalUtf16(ae_lower, 1, ae_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveNonAsciiEqual_ae");
	FL_UT_CHECK(FlCompareStringOrdinalUtf16(oe_lower, 1, oe_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveNonAsciiEqual_oe");
	FL_UT_CHECK(FlCompareStringOrdinalUtf16(ue_lower, 1, ue_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveNonAsciiEqual_ue");
	FL_UT_CHECK(FlCompareStringOrdinalUtf16(aa_lower, 1, aa_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveNonAsciiEqual_aa");
}

// ä (U+00E4) and ö (U+00F6) are distinct characters; they must not compare as
// equal in either case-sensitive or case-insensitive mode.
static void FlUnicodeUtUtf16CaseInsensitiveNonEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR ae[] = { 0x00E4, 0 }; // ä  U+00E4
	static const WCHAR oe[] = { 0x00F6, 0 }; // ö  U+00F6
	int resultCI = FlCompareStringOrdinalUtf16(ae, 1, oe, 1, TRUE);
	int resultCS = FlCompareStringOrdinalUtf16(ae, 1, oe, 1, FALSE);
	FL_UT_CHECK(resultCI != CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveNonEqual_CI");
	FL_UT_CHECK(resultCS != CSTR_EQUAL, "FlUnicodeUtUtf16CaseInsensitiveNonEqual_CS");
}

// German: "schön" (s c h U+00F6 n) must compare equal to "SCHÖN" (S C H U+00D6 N)
// when case is ignored, and must not compare equal to "schon" (plain o) even when
// case is ignored.
static void FlUnicodeUtUtf16GermanCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "schön": s       c       h       ö        n
	static const WCHAR schoen[] = { L's', L'c', L'h', 0x00F6, L'n', 0 };
	// "SCHÖN": S       C       H       Ö        N
	static const WCHAR SCHOEN[] = { L'S', L'C', L'H', 0x00D6, L'N', 0 };
	static const WCHAR schon[] = L"schon";

	int resultEqual = FlCompareStringOrdinalUtf16(schoen, (sizeof(schoen) / sizeof(schoen[0])) - 1, SCHOEN, (sizeof(SCHOEN) / sizeof(SCHOEN[0])) - 1, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf16(schoen, (sizeof(schoen) / sizeof(schoen[0])) - 1, schon, (sizeof(schon) / sizeof(schon[0])) - 1, TRUE);

	FL_UT_CHECK(resultEqual == CSTR_EQUAL, "FlUnicodeUtUtf16GermanCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeUtUtf16GermanCaseInsensitive_NotEqual");
}

// Finnish: "äiti" (U+00E4 i t i) must compare equal to "ÄITI" (U+00C4 I T I)
// when case is ignored, and must not compare equal to "aiti" (plain a) even when
// case is ignored.
static void FlUnicodeUtUtf16FinnishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "äiti": ä        i       t       i
	static const WCHAR aiti_lower[] = { 0x00E4, L'i', L't', L'i', 0 };
	// "ÄITI": Ä        I       T       I
	static const WCHAR AITI_upper[] = { 0x00C4, L'I', L'T', L'I', 0 };
	static const WCHAR aiti_plain[] = L"aiti";

	int resultEqual = FlCompareStringOrdinalUtf16(aiti_lower, (sizeof(aiti_lower) / sizeof(aiti_lower[0])) - 1, AITI_upper, (sizeof(AITI_upper) / sizeof(AITI_upper[0])) - 1, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf16(aiti_lower, (sizeof(aiti_lower) / sizeof(aiti_lower[0])) - 1, aiti_plain, (sizeof(aiti_plain) / sizeof(aiti_plain[0])) - 1, TRUE);

	FL_UT_CHECK(resultEqual == CSTR_EQUAL, "FlUnicodeUtUtf16FinnishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeUtUtf16FinnishCaseInsensitive_NotEqual");
}

// Swedish: "ålder" (U+00E5 l d e r) must compare equal to "ÅLDER" (U+00C5 L D E R)
// when case is ignored, and must not compare equal to "alder" (plain a) even when
// case is ignored.
static void FlUnicodeUtUtf16SwedishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "ålder": å        l       d       e       r
	static const WCHAR alder_lower[] = { 0x00E5, L'l', L'd', L'e', L'r', 0 };
	// "ÅLDER": Å        L       D       E       R
	static const WCHAR ALDER_upper[] = { 0x00C5, L'L', L'D', L'E', L'R', 0 };
	static const WCHAR alder_plain[] = L"alder";

	int resultEqual = FlCompareStringOrdinalUtf16(alder_lower, (sizeof(alder_lower) / sizeof(alder_lower[0])) - 1, ALDER_upper, (sizeof(ALDER_upper) / sizeof(ALDER_upper[0])) - 1, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf16(alder_lower, (sizeof(alder_lower) / sizeof(alder_lower[0])) - 1, alder_plain, (sizeof(alder_plain) / sizeof(alder_plain[0])) - 1, TRUE);

	FL_UT_CHECK(resultEqual == CSTR_EQUAL, "FlUnicodeUtUtf16SwedishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeUtUtf16SwedishCaseInsensitive_NotEqual");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf16 — Invalid (lone surrogates)
// ---------------------------------------------------------------------------

// A lone high surrogate has no valid pair partner and is treated as U+FFFD.
// The same lone surrogate compared to itself must be equal in both modes.
static void FlUnicodeUtUtf16LoneSurrogateEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR loneSurrogate[] = { 0xD800, 0 }; // lone high surrogate
	int resultCS = FlCompareStringOrdinalUtf16(loneSurrogate, 1, loneSurrogate, 1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(loneSurrogate, 1, loneSurrogate, 1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeUtUtf16LoneSurrogateEqual_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeUtUtf16LoneSurrogateEqual_CI");
}

// In case-insensitive mode both a lone high surrogate (0xD800) and a lone low
// surrogate (0xDC00) are mapped to U+FFFD before comparison, so they compare as
// equal.  In case-sensitive mode the raw WCHAR values 0xD800 and 0xDC00 differ,
// so the result must not be equal.
static void FlUnicodeUtUtf16DifferentLoneSurrogatesEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR highSurrogate[] = { 0xD800, 0 }; // lone high surrogate
	static const WCHAR lowSurrogate[] = { 0xDC00, 0 }; // lone low surrogate
	int resultCI = FlCompareStringOrdinalUtf16(highSurrogate, 1, lowSurrogate, 1, TRUE);
	int resultCS = FlCompareStringOrdinalUtf16(highSurrogate, 1, lowSurrogate, 1, FALSE);
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeUtUtf16DifferentLoneSurrogatesEqual_CI");
	FL_UT_CHECK(resultCS != CSTR_EQUAL, "FlUnicodeUtUtf16DifferentLoneSurrogatesEqual_CS");
}

// Ordering of strings must still be valid after a shared prefix of lone surrogates.
static void FlUnicodeUtUtf16ValidAfterLoneSurrogate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR surrogateAStr[] = { 0xD800, 0xD800, 0xD800, 0xD800, L'A', 0 };
	static const WCHAR surrogateBStr[] = { 0xD800, 0xD800, 0xD800, 0xD800, L'B', 0 };

	int resultCI = FlCompareStringOrdinalUtf16(surrogateAStr, 5, surrogateBStr, 5, TRUE);
	int resultCS = FlCompareStringOrdinalUtf16(surrogateAStr, 5, surrogateBStr, 5, FALSE);

	FL_UT_CHECK(resultCI == CSTR_LESS_THAN && resultCS == CSTR_LESS_THAN, "FlUnicodeUtUtf16ValidAfterLoneSurrogate");
}

// A lone surrogate must compare as greater than L"A" in both modes.
// Case-sensitive: raw 0xD800 (55296) > raw 0x0041 (65).
// Case-insensitive: U+FFFD (65533) > uppercase(U+0041) = U+0041 (65).
static void FlUnicodeUtUtf16LoneSurrogateVsValidChar(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR loneSurrogate[] = { 0xD800, 0 };
	static const WCHAR charA[] = L"A";
	int resultCS = FlCompareStringOrdinalUtf16(loneSurrogate, 1, charA, 1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(loneSurrogate, 1, charA, 1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_GREATER_THAN, "FlUnicodeUtUtf16LoneSurrogateVsValidChar_CS");
	FL_UT_CHECK(resultCI == CSTR_GREATER_THAN, "FlUnicodeUtUtf16LoneSurrogateVsValidChar_CI");
}

// The symmetric case: L"A" must compare as less than a lone surrogate in both modes.
static void FlUnicodeUtUtf16ValidCharVsLoneSurrogate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR charA[] = L"A";
	static const WCHAR loneSurrogate[] = { 0xD800, 0 };
	int resultCS = FlCompareStringOrdinalUtf16(charA, 1, loneSurrogate, 1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(charA, 1, loneSurrogate, 1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_LESS_THAN, "FlUnicodeUtUtf16ValidCharVsLoneSurrogate_CS");
	FL_UT_CHECK(resultCI == CSTR_LESS_THAN, "FlUnicodeUtUtf16ValidCharVsLoneSurrogate_CI");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlUnicodeUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// Encoding conversion
	FlUnicodeUtAsciiToUtf16Le(testCount, failCount);
	FlUnicodeUtBmpToUtf16Le(testCount, failCount);
	FlUnicodeUtSurrogateToUtf16Le(testCount, failCount);
	FlUnicodeUtMixedToUtf16Le(testCount, failCount);
	FlUnicodeUtAsciiToUtf8(testCount, failCount);
	FlUnicodeUtBmpToUtf8(testCount, failCount);
	FlUnicodeUtSurrogateToUtf8(testCount, failCount);
	FlUnicodeUtEmptyInput(testCount, failCount);
	FlUnicodeUtInvalidUtf8(testCount, failCount);
	FlUnicodeUtLoneSurrogate(testCount, failCount);
	FlUnicodeUtRoundTripLe(testCount, failCount);

	// FlCodepointToUpperCase
	FlUnicodeUtUpperCaseAsciiLowercase(testCount, failCount);
	FlUnicodeUtUpperCaseAsciiUppercaseUnchanged(testCount, failCount);
	FlUnicodeUtUpperCaseAsciiNonAlphaUnchanged(testCount, failCount);
	FlUnicodeUtUpperCaseNonAscii(testCount, failCount);
	FlUnicodeUtUpperCaseOutOfRange(testCount, failCount);

	// FlCodepointToLowerCase
	FlUnicodeUtLowerCaseAsciiUppercase(testCount, failCount);
	FlUnicodeUtLowerCaseAsciiLowercaseUnchanged(testCount, failCount);
	FlUnicodeUtLowerCaseAsciiNonAlphaUnchanged(testCount, failCount);
	FlUnicodeUtLowerCaseNonAscii(testCount, failCount);
	FlUnicodeUtLowerCaseOutOfRange(testCount, failCount);

	// FlCompareStringOrdinalUtf8 — ASCII
	FlUnicodeUtUtf8CaseSensitiveEqual(testCount, failCount);
	FlUnicodeUtUtf8CaseSensitiveLess(testCount, failCount);
	FlUnicodeUtUtf8CaseSensitiveGreater(testCount, failCount);
	FlUnicodeUtUtf8CaseSensitiveDifferentCase(testCount, failCount);
	FlUnicodeUtUtf8CaseInsensitiveEqual(testCount, failCount);
	FlUnicodeUtUtf8CaseInsensitiveLess(testCount, failCount);
	FlUnicodeUtUtf8CaseNumberLess(testCount, failCount);
	FlUnicodeUtUtf8EmptyEqual(testCount, failCount);
	FlUnicodeUtUtf8EmptyLessThanNonEmpty(testCount, failCount);
	FlUnicodeUtUtf8NullTerminated(testCount, failCount);

	// FlCompareStringOrdinalUtf8 — Non-ASCII
	FlUnicodeUtUtf8CaseInsensitiveAeEqual(testCount, failCount);
	FlUnicodeUtUtf8CaseInsensitiveOeEqual(testCount, failCount);
	FlUnicodeUtUtf8CaseInsensitiveUeEqual(testCount, failCount);
	FlUnicodeUtUtf8CaseInsensitiveAaEqual(testCount, failCount);
	FlUnicodeUtUtf8CaseInsensitiveNonEqual(testCount, failCount);
	FlUnicodeUtUtf8GermanCaseInsensitive(testCount, failCount);
	FlUnicodeUtUtf8FinnishCaseInsensitive(testCount, failCount);
	FlUnicodeUtUtf8SwedishCaseInsensitive(testCount, failCount);

	// FlCompareStringOrdinalUtf8 — Invalid sequences
	FlUnicodeUtUtf8InvalidEqualSame(testCount, failCount);
	FlUnicodeUtUtf8InvalidEqualSameCaseInsensitive(testCount, failCount);
	FlUnicodeUtUtf8InvalidNotEqual(testCount, failCount);
	FlUnicodeUtUtf8InvalidNotEqualCaseInsensitive(testCount, failCount);
	FlUnicodeUtUtf8ValidVsInvalid(testCount, failCount);

	// FlCompareStringOrdinalUtf16 — ASCII
	FlUnicodeUtUtf16CaseSensitiveEqual(testCount, failCount);
	FlUnicodeUtUtf16CaseSensitiveLess(testCount, failCount);
	FlUnicodeUtUtf16CaseSensitiveGreater(testCount, failCount);
	FlUnicodeUtUtf16CaseSensitiveDifferentCase(testCount, failCount);
	FlUnicodeUtUtf16CaseInsensitiveEqual(testCount, failCount);
	FlUnicodeUtUtf16CaseInsensitiveLess(testCount, failCount);
	FlUnicodeUtUtf16CaseNumberLess(testCount, failCount);
	FlUnicodeUtUtf16EmptyEqual(testCount, failCount);
	FlUnicodeUtUtf16EmptyLessThanNonEmpty(testCount, failCount);
	FlUnicodeUtUtf16NullTerminated(testCount, failCount);

	// FlCompareStringOrdinalUtf16 — Non-ASCII
	FlUnicodeUtUtf16CaseInsensitiveNonAsciiEqual(testCount, failCount);
	FlUnicodeUtUtf16CaseInsensitiveNonEqual(testCount, failCount);
	FlUnicodeUtUtf16GermanCaseInsensitive(testCount, failCount);
	FlUnicodeUtUtf16FinnishCaseInsensitive(testCount, failCount);
	FlUnicodeUtUtf16SwedishCaseInsensitive(testCount, failCount);

	// FlCompareStringOrdinalUtf16 — Lone surrogates
	FlUnicodeUtUtf16LoneSurrogateEqual(testCount, failCount);
	FlUnicodeUtUtf16DifferentLoneSurrogatesEqual(testCount, failCount);
	FlUnicodeUtUtf16ValidAfterLoneSurrogate(testCount, failCount);
	FlUnicodeUtUtf16LoneSurrogateVsValidChar(testCount, failCount);
	FlUnicodeUtUtf16ValidCharVsLoneSurrogate(testCount, failCount);
}
