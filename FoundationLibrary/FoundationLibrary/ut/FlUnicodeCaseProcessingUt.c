/*
	Unicode case processing unit tests by Santtu S. Nyman.

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

		Unit tests for FlCodepointToUpperCase, FlCodepointToLowerCase,
		FlCompareStringOrdinalUtf8 and FlCompareStringOrdinalUtf16.

		Non-ASCII UTF-8 byte sequences are expressed as explicit uint8_t arrays
		so the tests do not depend on the source-file encoding.  Unicode code
		points used in comments are given as U+XXXX.

		Non-ASCII UTF-16 code units are expressed as explicit WCHAR arrays for
		the same reason.  Lone surrogate tests use explicit WCHAR code-unit
		values rather than L"..." string literals.
*/

#include "FlUt.h"
#include "../include/FlUnicodeCaseProcessing.h"
#include <stddef.h>
#include <stdint.h>

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
static void FlUnicodeCaseProcessingUtUpperCaseAsciiLowercase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtUpperCaseAsciiLowercase");
}

// ASCII uppercase letters must be returned unchanged.
static void FlUnicodeCaseProcessingUtUpperCaseAsciiUppercaseUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtUpperCaseAsciiUppercaseUnchanged");
}

// ASCII non-alpha codepoints (digits, punctuation, space) must be returned unchanged.
static void FlUnicodeCaseProcessingUtUpperCaseAsciiNonAlphaUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtUpperCaseAsciiNonAlphaUnchanged");
}

// Non-ASCII codepoints that have uppercase equivalents must map correctly.
static void FlUnicodeCaseProcessingUtUpperCaseNonAscii(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtUpperCaseNonAscii");
}

// Codepoint values outside the valid Unicode range must be returned unchanged.
static void FlUnicodeCaseProcessingUtUpperCaseOutOfRange(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FL_UT_CHECK(FlCodepointToUpperCase(-1)       == -1,       "FlUnicodeCaseProcessingUtUpperCaseOutOfRange_neg");
	FL_UT_CHECK(FlCodepointToUpperCase(0x110000) == 0x110000, "FlUnicodeCaseProcessingUtUpperCaseOutOfRange_high");
}

// ---------------------------------------------------------------------------
// FlCodepointToLowerCase
// ---------------------------------------------------------------------------

// ASCII uppercase letters must map to their lowercase equivalents.
static void FlUnicodeCaseProcessingUtLowerCaseAsciiUppercase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtLowerCaseAsciiUppercase");
}

// ASCII lowercase letters must be returned unchanged.
static void FlUnicodeCaseProcessingUtLowerCaseAsciiLowercaseUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtLowerCaseAsciiLowercaseUnchanged");
}

// ASCII non-alpha codepoints must be returned unchanged.
static void FlUnicodeCaseProcessingUtLowerCaseAsciiNonAlphaUnchanged(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtLowerCaseAsciiNonAlphaUnchanged");
}

// Non-ASCII codepoints that have lowercase equivalents must map correctly.
static void FlUnicodeCaseProcessingUtLowerCaseNonAscii(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
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
	FL_UT_CHECK(caseMappingSucceeded, "FlUnicodeCaseProcessingUtLowerCaseNonAscii");
}

// Codepoint values outside the valid Unicode range must be returned unchanged.
static void FlUnicodeCaseProcessingUtLowerCaseOutOfRange(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FL_UT_CHECK(FlCodepointToLowerCase(-1) == -1, "FlUnicodeCaseProcessingUtLowerCaseOutOfRange_neg");
	FL_UT_CHECK(FlCodepointToLowerCase(0x110000) == 0x110000, "FlUnicodeCaseProcessingUtLowerCaseOutOfRange_high");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf8 — ASCII
// ---------------------------------------------------------------------------

// Identical ASCII strings must compare as equal (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf8CaseSensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "hello";
	static const char strB[] = "hello";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseSensitiveEqual");
}

// "abc" must compare as less than "abd" (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf8CaseSensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "abc";
	static const char strB[] = "abd";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf8CaseSensitiveLess");
}

// "abd" must compare as greater than "abc" (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf8CaseSensitiveGreater(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "abd";
	static const char strB[] = "abc";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeCaseProcessingUtUtf8CaseSensitiveGreater");
}

// "abc" must compare as greater than "ABC" in case-sensitive mode because the
// raw byte value of 'a' (0x61) is greater than 'A' (0x41).
static void FlUnicodeCaseProcessingUtUtf8CaseSensitiveDifferentCase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strLower[] = "abc";
	static const char strUpper[] = "ABC";
	int result = FlCompareStringOrdinalUtf8(strLower, sizeof(strLower) - 1, strUpper, sizeof(strUpper) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeCaseProcessingUtUtf8CaseSensitiveDifferentCase");
}

// "hello" and "HELLO" must compare as equal when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strLower[] = "hello";
	static const char strUpper[] = "HELLO";
	int result = FlCompareStringOrdinalUtf8(strLower, sizeof(strLower) - 1, strUpper, sizeof(strUpper) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveEqual");
}

// "abc" must compare as less than "abd" in case-insensitive mode.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "abc";
	static const char strB[] = "abd";
	int result = FlCompareStringOrdinalUtf8(strA, sizeof(strA) - 1, strB, sizeof(strB) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveLess");
}

// "001000" must compare as less than "002000".
static void FlUnicodeCaseProcessingUtUtf8CaseNumberLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char str001[] = "001000";
	static const char str002[] = "002000";
	int result = FlCompareStringOrdinalUtf8(str001, sizeof(str001) - 1, str002, sizeof(str002) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf8CaseNumberLess");
}

// Two empty strings must compare as equal in both modes.
static void FlUnicodeCaseProcessingUtUtf8EmptyEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char empty[] = "";
	int resultCS = FlCompareStringOrdinalUtf8(empty, 0, empty, 0, FALSE);
	int resultCI = FlCompareStringOrdinalUtf8(empty, 0, empty, 0, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8EmptyEqual_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8EmptyEqual_CI");
}

// An empty string must compare as less than a non-empty string (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf8EmptyLessThanNonEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char empty[] = "";
	static const char strA[]  = "a";
	int result = FlCompareStringOrdinalUtf8(empty, 0, strA, sizeof(strA) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf8EmptyLessThanNonEmpty");
}

// Passing (size_t)-1 as length must give the same result as passing the
// explicit byte count for a null-terminated string.
static void FlUnicodeCaseProcessingUtUtf8NullTerminated(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char strA[] = "hello";
	static const char strB[] = "hello";
	int resultCS = FlCompareStringOrdinalUtf8(strA, (size_t)-1, strB, (size_t)-1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf8(strA, (size_t)-1, strB, (size_t)-1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8NullTerminated_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8NullTerminated_CI");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf8 — Non-ASCII
// ---------------------------------------------------------------------------

// U+00E4 (ä, UTF-8: 0xC3 0xA4) must compare equal to U+00C4 (Ä, UTF-8: 0xC3 0x84)
// when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveAeEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xA4 }; // ä  U+00E4
	static const uint8_t upper[] = { 0xC3, 0x84 }; // Ä  U+00C4
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveAeEqual");
}

// U+00F6 (ö, UTF-8: 0xC3 0xB6) must compare equal to U+00D6 (Ö, UTF-8: 0xC3 0x96)
// when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveOeEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xB6 }; // ö  U+00F6
	static const uint8_t upper[] = { 0xC3, 0x96 }; // Ö  U+00D6
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveOeEqual");
}

// U+00FC (ü, UTF-8: 0xC3 0xBC) must compare equal to U+00DC (Ü, UTF-8: 0xC3 0x9C)
// when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveUeEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xBC }; // ü  U+00FC
	static const uint8_t upper[] = { 0xC3, 0x9C }; // Ü  U+00DC
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveUeEqual");
}

// U+00E5 (å, UTF-8: 0xC3 0xA5) must compare equal to U+00C5 (Å, UTF-8: 0xC3 0x85)
// when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveAaEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t lower[] = { 0xC3, 0xA5 }; // å  U+00E5
	static const uint8_t upper[] = { 0xC3, 0x85 }; // Å  U+00C5
	int result = FlCompareStringOrdinalUtf8((const char*)lower, sizeof lower, (const char*)upper, sizeof upper, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveAaEqual");
}

// ä (U+00E4) and ö (U+00F6) are distinct characters; they must not compare as
// equal in either case-sensitive or case-insensitive mode.
static void FlUnicodeCaseProcessingUtUtf8CaseInsensitiveNonEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t ae[] = { 0xC3, 0xA4 }; // ä  U+00E4
	static const uint8_t oe[] = { 0xC3, 0xB6 }; // ö  U+00F6
	int resultCI = FlCompareStringOrdinalUtf8((const char*)ae, sizeof ae, (const char*)oe, sizeof oe, TRUE);
	int resultCS = FlCompareStringOrdinalUtf8((const char*)ae, sizeof ae, (const char*)oe, sizeof oe, FALSE);
	FL_UT_CHECK(resultCI != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveNonEqual_CI");
	FL_UT_CHECK(resultCS != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8CaseInsensitiveNonEqual_CS");
}

// German: "schön" (s c h U+00F6 n) must compare equal to "SCHÖN" (S C H U+00D6 N)
// when case is ignored, and must not compare equal to "schon" (plain o, no umlaut)
// even when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8GermanCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "schön": s  c     h     ö           n
	static const uint8_t schoen[] = { 0x73, 0x63, 0x68, 0xC3, 0xB6, 0x6E };
	// "SCHÖN": S  C     H     Ö           N
	static const uint8_t SCHOEN[] = { 0x53, 0x43, 0x48, 0xC3, 0x96, 0x4E };
	// "schon": s  c     h     o     n  (plain o, no umlaut)
	static const uint8_t schon[]  = { 0x73, 0x63, 0x68, 0x6F, 0x6E };

	int resultEqual    = FlCompareStringOrdinalUtf8((const char*)schoen, sizeof schoen, (const char*)SCHOEN, sizeof SCHOEN, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf8((const char*)schoen, sizeof schoen, (const char*)schon,  sizeof schon,  TRUE);

	FL_UT_CHECK(resultEqual    == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8GermanCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8GermanCaseInsensitive_NotEqual");
}

// Finnish: "äiti" (U+00E4 i t i) must compare equal to "ÄITI" (U+00C4 I T I)
// when case is ignored, and must not compare equal to "aiti" (plain a, no umlaut)
// even when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8FinnishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "äiti": ä           i     t     i
	static const uint8_t aiti_lower[] = { 0xC3, 0xA4, 0x69, 0x74, 0x69 };
	// "ÄITI": Ä           I     T     I
	static const uint8_t AITI_upper[] = { 0xC3, 0x84, 0x49, 0x54, 0x49 };
	// "aiti": a  i  t  i  (plain a, no umlaut)
	static const uint8_t aiti_plain[] = { 0x61, 0x69, 0x74, 0x69 };

	int resultEqual    = FlCompareStringOrdinalUtf8((const char*)aiti_lower, sizeof aiti_lower, (const char*)AITI_upper, sizeof AITI_upper, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf8((const char*)aiti_lower, sizeof aiti_lower, (const char*)aiti_plain, sizeof aiti_plain, TRUE);

	FL_UT_CHECK(resultEqual    == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8FinnishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8FinnishCaseInsensitive_NotEqual");
}

// Swedish: "ålder" (U+00E5 l d e r) must compare equal to "ÅLDER" (U+00C5 L D E R)
// when case is ignored, and must not compare equal to "alder" (plain a, no ring)
// even when case is ignored.
static void FlUnicodeCaseProcessingUtUtf8SwedishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "ålder": å           l     d     e     r
	static const uint8_t alder_lower[] = { 0xC3, 0xA5, 0x6C, 0x64, 0x65, 0x72 };
	// "ÅLDER": Å           L     D     E     R
	static const uint8_t ALDER_upper[] = { 0xC3, 0x85, 0x4C, 0x44, 0x45, 0x52 };
	// "alder": a  l  d  e  r  (plain a, no ring)
	static const uint8_t alder_plain[] = { 0x61, 0x6C, 0x64, 0x65, 0x72 };

	int resultEqual    = FlCompareStringOrdinalUtf8((const char*)alder_lower, sizeof alder_lower, (const char*)ALDER_upper, sizeof ALDER_upper, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf8((const char*)alder_lower, sizeof alder_lower, (const char*)alder_plain, sizeof alder_plain, TRUE);

	FL_UT_CHECK(resultEqual    == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8SwedishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8SwedishCaseInsensitive_NotEqual");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf8 — Invalid sequences
// ---------------------------------------------------------------------------

// Two strings that consist of the same invalid byte must compare as equal
// (case-sensitive).  Invalid bytes at the same position are compared by raw value.
static void FlUnicodeCaseProcessingUtUtf8InvalidEqualSame(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalid[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalid, sizeof invalid, (const char*)invalid, sizeof invalid, FALSE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8InvalidEqualSame");
}

// Same invalid byte must also compare as equal in case-insensitive mode.
static void FlUnicodeCaseProcessingUtUtf8InvalidEqualSameCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalid[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalid, sizeof invalid, (const char*)invalid, sizeof invalid, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8InvalidEqualSameCaseInsensitive");
}

// A string with invalid byte 0xFE must compare as not equal to a string with
// invalid byte 0xFF (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf8InvalidNotEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalidLow[]  = { 0xFE };
	static const uint8_t invalidHigh[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalidLow, sizeof invalidLow, (const char*)invalidHigh, sizeof invalidHigh, FALSE);
	FL_UT_CHECK(result != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8InvalidNotEqual");
}

// The same raw-bytes must compare as not equal in case-insensitive mode.
static void FlUnicodeCaseProcessingUtUtf8InvalidNotEqualCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalidLow[]  = { 0xFE };
	static const uint8_t invalidHigh[] = { 0xFF };
	int result = FlCompareStringOrdinalUtf8((const char*)invalidLow, sizeof invalidLow, (const char*)invalidHigh, sizeof invalidHigh, TRUE);
	FL_UT_CHECK(result != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8InvalidNotEqualCaseInsensitive");
}

// Ordering of strings most still be valid after chunk of invalid data if that data is the same in both strings
static void FlUnicodeCaseProcessingUtUtf8ValidAfterInvalid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const uint8_t invalidAStr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x41 };
	static const uint8_t invalidBStr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x42 };

	int resultCI = FlCompareStringOrdinalUtf8((const char*)invalidAStr, sizeof invalidAStr, (const char*)invalidBStr, sizeof invalidBStr, TRUE);
	int resultCS = FlCompareStringOrdinalUtf8((const char*)invalidAStr, sizeof invalidAStr, (const char*)invalidBStr, sizeof invalidBStr, FALSE);

	FL_UT_CHECK(resultCI == CSTR_LESS_THAN && resultCS == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf8ValidAfterInvalid");
}

// Comparison between an invalid and a valid strings must be not equal.
static void FlUnicodeCaseProcessingUtUtf8ValidVsInvalid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char    validStr[]   = "a";
	static const uint8_t invalidStr[] = { 0xFF };

	int resultValidCS   = FlCompareStringOrdinalUtf8(validStr,                sizeof(validStr) - 1, (const char*)invalidStr, sizeof invalidStr,    FALSE);
	int resultValidCI   = FlCompareStringOrdinalUtf8(validStr,                sizeof(validStr) - 1, (const char*)invalidStr, sizeof invalidStr,    TRUE);
	int resultInvalidCS = FlCompareStringOrdinalUtf8((const char*)invalidStr, sizeof invalidStr,    validStr,                sizeof(validStr) - 1, FALSE);
	int resultInvalidCI = FlCompareStringOrdinalUtf8((const char*)invalidStr, sizeof invalidStr,    validStr,                sizeof(validStr) - 1, TRUE);

	// Case-sensitive: raw byte 0x61 ('a') != 0xFF.
	FL_UT_CHECK(resultValidCS   != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8ValidVsInvalid_Valid_CS");
	FL_UT_CHECK(resultInvalidCS != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8ValidVsInvalid_Invalid_CS");
	// Case-insensitive: valid code point position != invalid sequence position.
	FL_UT_CHECK(resultValidCI   != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8ValidVsInvalid_Valid_CI");
	FL_UT_CHECK(resultInvalidCI != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf8ValidVsInvalid_Invalid_CI");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf16 — ASCII
// ---------------------------------------------------------------------------

// Identical ASCII strings must compare as equal (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf16CaseSensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"hello";
	static const WCHAR strB[] = L"hello";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseSensitiveEqual");
}

// "abc" must compare as less than "abd" (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf16CaseSensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"abc";
	static const WCHAR strB[] = L"abd";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16CaseSensitiveLess");
}

// "abc" must compare as greater than "ABC" in case-sensitive mode because the
// raw WCHAR value of 'a' (0x0061) is greater than 'A' (0x0041).
static void FlUnicodeCaseProcessingUtUtf16CaseSensitiveDifferentCase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strLower[] = L"abc";
	static const WCHAR strUpper[] = L"ABC";
	int result = FlCompareStringOrdinalUtf16(strLower, (sizeof(strLower) / sizeof(strLower[0])) - 1, strUpper, (sizeof(strUpper) / sizeof(strUpper[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeCaseProcessingUtUtf16CaseSensitiveDifferentCase");
}

// "hello" and "HELLO" must compare as equal when case is ignored.
static void FlUnicodeCaseProcessingUtUtf16CaseInsensitiveEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strLower[] = L"hello";
	static const WCHAR strUpper[] = L"HELLO";
	int result = FlCompareStringOrdinalUtf16(strLower, (sizeof(strLower) / sizeof(strLower[0])) - 1, strUpper, (sizeof(strUpper) / sizeof(strUpper[0])) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveEqual");
}

// Two empty UTF-16 strings must compare as equal in both modes.
static void FlUnicodeCaseProcessingUtUtf16EmptyEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR empty[] = L"";
	int resultCS = FlCompareStringOrdinalUtf16(empty, 0, empty, 0, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(empty, 0, empty, 0, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16EmptyEqual_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16EmptyEqual_CI");
}

// "abd" must compare as greater than "abc" (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf16CaseSensitiveGreater(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"abd";
	static const WCHAR strB[] = L"abc";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_GREATER_THAN, "FlUnicodeCaseProcessingUtUtf16CaseSensitiveGreater");
}

// "abc" must compare as less than "abd" in case-insensitive mode.
static void FlUnicodeCaseProcessingUtUtf16CaseInsensitiveLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"abc";
	static const WCHAR strB[] = L"abd";
	int result = FlCompareStringOrdinalUtf16(strA, (sizeof(strA) / sizeof(strA[0])) - 1, strB, (sizeof(strB) / sizeof(strB[0])) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveLess");
}

// "001000" must compare as less than "002000".
static void FlUnicodeCaseProcessingUtUtf16CaseNumberLess(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR str001[] = L"001000";
	static const WCHAR str002[] = L"002000";
	int result = FlCompareStringOrdinalUtf16(str001, (sizeof(str001) / sizeof(str001[0])) - 1, str002, (sizeof(str002) / sizeof(str002[0])) - 1, TRUE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16CaseNumberLess");
}

// An empty string must compare as less than a non-empty string (case-sensitive).
static void FlUnicodeCaseProcessingUtUtf16EmptyLessThanNonEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR empty[] = L"";
	static const WCHAR strA[]  = L"a";
	int result = FlCompareStringOrdinalUtf16(empty, 0, strA, (sizeof(strA) / sizeof(strA[0])) - 1, FALSE);
	FL_UT_CHECK(result == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16EmptyLessThanNonEmpty");
}

// Passing (size_t)-1 as length must give the same result as passing the
// explicit code-unit count for a null-terminated string.
static void FlUnicodeCaseProcessingUtUtf16NullTerminated(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR strA[] = L"hello";
	static const WCHAR strB[] = L"hello";
	int resultCS = FlCompareStringOrdinalUtf16(strA, (size_t)-1, strB, (size_t)-1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(strA, (size_t)-1, strB, (size_t)-1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16NullTerminated_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16NullTerminated_CI");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf16 — Non-ASCII
// ---------------------------------------------------------------------------

// Non-ASCII letters with case pairs must compare as equal when case is ignored.
// U+00E4 (ä) == U+00C4 (Ä), U+00F6 (ö) == U+00D6 (Ö), U+00FC (ü) == U+00DC (Ü),
// U+00E5 (å) == U+00C5 (Å).
static void FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonAsciiEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR ae_lower[] = { 0x00E4, 0 }; // ä  U+00E4
	static const WCHAR ae_upper[] = { 0x00C4, 0 }; // Ä  U+00C4
	static const WCHAR oe_lower[] = { 0x00F6, 0 }; // ö  U+00F6
	static const WCHAR oe_upper[] = { 0x00D6, 0 }; // Ö  U+00D6
	static const WCHAR ue_lower[] = { 0x00FC, 0 }; // ü  U+00FC
	static const WCHAR ue_upper[] = { 0x00DC, 0 }; // Ü  U+00DC
	static const WCHAR aa_lower[] = { 0x00E5, 0 }; // å  U+00E5
	static const WCHAR aa_upper[] = { 0x00C5, 0 }; // Å  U+00C5

	FL_UT_CHECK(FlCompareStringOrdinalUtf16(ae_lower, 1, ae_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonAsciiEqual_ae");
	FL_UT_CHECK(FlCompareStringOrdinalUtf16(oe_lower, 1, oe_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonAsciiEqual_oe");
	FL_UT_CHECK(FlCompareStringOrdinalUtf16(ue_lower, 1, ue_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonAsciiEqual_ue");
	FL_UT_CHECK(FlCompareStringOrdinalUtf16(aa_lower, 1, aa_upper, 1, TRUE) == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonAsciiEqual_aa");
}

// ä (U+00E4) and ö (U+00F6) are distinct characters; they must not compare as
// equal in either case-sensitive or case-insensitive mode.
static void FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR ae[] = { 0x00E4, 0 }; // ä  U+00E4
	static const WCHAR oe[] = { 0x00F6, 0 }; // ö  U+00F6
	int resultCI = FlCompareStringOrdinalUtf16(ae, 1, oe, 1, TRUE);
	int resultCS = FlCompareStringOrdinalUtf16(ae, 1, oe, 1, FALSE);
	FL_UT_CHECK(resultCI != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonEqual_CI");
	FL_UT_CHECK(resultCS != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonEqual_CS");
}

// German: "schön" (s c h U+00F6 n) must compare equal to "SCHÖN" (S C H U+00D6 N)
// when case is ignored, and must not compare equal to "schon" (plain o) even when
// case is ignored.
static void FlUnicodeCaseProcessingUtUtf16GermanCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "schön": s       c       h       ö        n
	static const WCHAR schoen[] = { L's', L'c', L'h', 0x00F6, L'n', 0 };
	// "SCHÖN": S       C       H       Ö        N
	static const WCHAR SCHOEN[] = { L'S', L'C', L'H', 0x00D6, L'N', 0 };
	static const WCHAR schon[]  = L"schon";

	int resultEqual    = FlCompareStringOrdinalUtf16(schoen, (sizeof(schoen) / sizeof(schoen[0])) - 1, SCHOEN, (sizeof(SCHOEN) / sizeof(SCHOEN[0])) - 1, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf16(schoen, (sizeof(schoen) / sizeof(schoen[0])) - 1, schon,  (sizeof(schon)  / sizeof(schon[0]))  - 1, TRUE);

	FL_UT_CHECK(resultEqual    == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16GermanCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16GermanCaseInsensitive_NotEqual");
}

// Finnish: "äiti" (U+00E4 i t i) must compare equal to "ÄITI" (U+00C4 I T I)
// when case is ignored, and must not compare equal to "aiti" (plain a) even when
// case is ignored.
static void FlUnicodeCaseProcessingUtUtf16FinnishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "äiti": ä        i       t       i
	static const WCHAR aiti_lower[] = { 0x00E4, L'i', L't', L'i', 0 };
	// "ÄITI": Ä        I       T       I
	static const WCHAR AITI_upper[] = { 0x00C4, L'I', L'T', L'I', 0 };
	static const WCHAR aiti_plain[] = L"aiti";

	int resultEqual    = FlCompareStringOrdinalUtf16(aiti_lower, (sizeof(aiti_lower) / sizeof(aiti_lower[0])) - 1, AITI_upper, (sizeof(AITI_upper) / sizeof(AITI_upper[0])) - 1, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf16(aiti_lower, (sizeof(aiti_lower) / sizeof(aiti_lower[0])) - 1, aiti_plain, (sizeof(aiti_plain) / sizeof(aiti_plain[0])) - 1, TRUE);

	FL_UT_CHECK(resultEqual    == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16FinnishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16FinnishCaseInsensitive_NotEqual");
}

// Swedish: "ålder" (U+00E5 l d e r) must compare equal to "ÅLDER" (U+00C5 L D E R)
// when case is ignored, and must not compare equal to "alder" (plain a) even when
// case is ignored.
static void FlUnicodeCaseProcessingUtUtf16SwedishCaseInsensitive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// "ålder": å        l       d       e       r
	static const WCHAR alder_lower[] = { 0x00E5, L'l', L'd', L'e', L'r', 0 };
	// "ÅLDER": Å        L       D       E       R
	static const WCHAR ALDER_upper[] = { 0x00C5, L'L', L'D', L'E', L'R', 0 };
	static const WCHAR alder_plain[] = L"alder";

	int resultEqual    = FlCompareStringOrdinalUtf16(alder_lower, (sizeof(alder_lower) / sizeof(alder_lower[0])) - 1, ALDER_upper, (sizeof(ALDER_upper) / sizeof(ALDER_upper[0])) - 1, TRUE);
	int resultNotEqual = FlCompareStringOrdinalUtf16(alder_lower, (sizeof(alder_lower) / sizeof(alder_lower[0])) - 1, alder_plain, (sizeof(alder_plain) / sizeof(alder_plain[0])) - 1, TRUE);

	FL_UT_CHECK(resultEqual    == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16SwedishCaseInsensitive_Equal");
	FL_UT_CHECK(resultNotEqual != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16SwedishCaseInsensitive_NotEqual");
}

// ---------------------------------------------------------------------------
// FlCompareStringOrdinalUtf16 — Invalid (lone surrogates)
// ---------------------------------------------------------------------------

// A lone high surrogate has no valid pair partner and is treated as U+FFFD.
// The same lone surrogate compared to itself must be equal in both modes.
static void FlUnicodeCaseProcessingUtUtf16LoneSurrogateEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR loneSurrogate[] = { 0xD800, 0 }; // lone high surrogate
	int resultCS = FlCompareStringOrdinalUtf16(loneSurrogate, 1, loneSurrogate, 1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(loneSurrogate, 1, loneSurrogate, 1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16LoneSurrogateEqual_CS");
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16LoneSurrogateEqual_CI");
}

// In case-insensitive mode both a lone high surrogate (0xD800) and a lone low
// surrogate (0xDC00) are mapped to U+FFFD before comparison, so they compare as
// equal.  In case-sensitive mode the raw WCHAR values 0xD800 and 0xDC00 differ,
// so the result must not be equal.
static void FlUnicodeCaseProcessingUtUtf16DifferentLoneSurrogatesEqual(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR highSurrogate[] = { 0xD800, 0 }; // lone high surrogate
	static const WCHAR lowSurrogate[]  = { 0xDC00, 0 }; // lone low surrogate
	int resultCI = FlCompareStringOrdinalUtf16(highSurrogate, 1, lowSurrogate, 1, TRUE);
	int resultCS = FlCompareStringOrdinalUtf16(highSurrogate, 1, lowSurrogate, 1, FALSE);
	FL_UT_CHECK(resultCI == CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16DifferentLoneSurrogatesEqual_CI");
	FL_UT_CHECK(resultCS != CSTR_EQUAL, "FlUnicodeCaseProcessingUtUtf16DifferentLoneSurrogatesEqual_CS");
}

// Ordering of strings must still be valid after a shared prefix of lone surrogates.
static void FlUnicodeCaseProcessingUtUtf16ValidAfterLoneSurrogate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR surrogateAStr[] = { 0xD800, 0xD800, 0xD800, 0xD800, L'A', 0 };
	static const WCHAR surrogateBStr[] = { 0xD800, 0xD800, 0xD800, 0xD800, L'B', 0 };

	int resultCI = FlCompareStringOrdinalUtf16(surrogateAStr, 5, surrogateBStr, 5, TRUE);
	int resultCS = FlCompareStringOrdinalUtf16(surrogateAStr, 5, surrogateBStr, 5, FALSE);

	FL_UT_CHECK(resultCI == CSTR_LESS_THAN && resultCS == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16ValidAfterLoneSurrogate");
}

// A lone surrogate must compare as greater than L"A" in both modes.
// Case-sensitive: raw 0xD800 (55296) > raw 0x0041 (65).
// Case-insensitive: U+FFFD (65533) > uppercase(U+0041) = U+0041 (65).
static void FlUnicodeCaseProcessingUtUtf16LoneSurrogateVsValidChar(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR loneSurrogate[] = { 0xD800, 0 };
	static const WCHAR charA[]         = L"A";
	int resultCS = FlCompareStringOrdinalUtf16(loneSurrogate, 1, charA, 1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(loneSurrogate, 1, charA, 1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_GREATER_THAN, "FlUnicodeCaseProcessingUtUtf16LoneSurrogateVsValidChar_CS");
	FL_UT_CHECK(resultCI == CSTR_GREATER_THAN, "FlUnicodeCaseProcessingUtUtf16LoneSurrogateVsValidChar_CI");
}

// The symmetric case: L"A" must compare as less than a lone surrogate in both modes.
static void FlUnicodeCaseProcessingUtUtf16ValidCharVsLoneSurrogate(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR charA[]         = L"A";
	static const WCHAR loneSurrogate[] = { 0xD800, 0 };
	int resultCS = FlCompareStringOrdinalUtf16(charA, 1, loneSurrogate, 1, FALSE);
	int resultCI = FlCompareStringOrdinalUtf16(charA, 1, loneSurrogate, 1, TRUE);
	FL_UT_CHECK(resultCS == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16ValidCharVsLoneSurrogate_CS");
	FL_UT_CHECK(resultCI == CSTR_LESS_THAN, "FlUnicodeCaseProcessingUtUtf16ValidCharVsLoneSurrogate_CI");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void FlUnicodeCaseProcessingUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// FlCodepointToUpperCase
	FlUnicodeCaseProcessingUtUpperCaseAsciiLowercase(testCount, failCount);
	FlUnicodeCaseProcessingUtUpperCaseAsciiUppercaseUnchanged(testCount, failCount);
	FlUnicodeCaseProcessingUtUpperCaseAsciiNonAlphaUnchanged(testCount, failCount);
	FlUnicodeCaseProcessingUtUpperCaseNonAscii(testCount, failCount);
	FlUnicodeCaseProcessingUtUpperCaseOutOfRange(testCount, failCount);

	// FlCodepointToLowerCase
	FlUnicodeCaseProcessingUtLowerCaseAsciiUppercase(testCount, failCount);
	FlUnicodeCaseProcessingUtLowerCaseAsciiLowercaseUnchanged(testCount, failCount);
	FlUnicodeCaseProcessingUtLowerCaseAsciiNonAlphaUnchanged(testCount, failCount);
	FlUnicodeCaseProcessingUtLowerCaseNonAscii(testCount, failCount);
	FlUnicodeCaseProcessingUtLowerCaseOutOfRange(testCount, failCount);

	// FlCompareStringOrdinalUtf8 — ASCII
	FlUnicodeCaseProcessingUtUtf8CaseSensitiveEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseSensitiveLess(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseSensitiveGreater(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseSensitiveDifferentCase(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveLess(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseNumberLess(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8EmptyEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8EmptyLessThanNonEmpty(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8NullTerminated(testCount, failCount);

	// FlCompareStringOrdinalUtf8 — Non-ASCII
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveAeEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveOeEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveUeEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveAaEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8CaseInsensitiveNonEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8GermanCaseInsensitive(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8FinnishCaseInsensitive(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8SwedishCaseInsensitive(testCount, failCount);

	// FlCompareStringOrdinalUtf8 — Invalid sequences
	FlUnicodeCaseProcessingUtUtf8InvalidEqualSame(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8InvalidEqualSameCaseInsensitive(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8InvalidNotEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8InvalidNotEqualCaseInsensitive(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf8ValidVsInvalid(testCount, failCount);

	// FlCompareStringOrdinalUtf16 — ASCII
	FlUnicodeCaseProcessingUtUtf16CaseSensitiveEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseSensitiveLess(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseSensitiveGreater(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseSensitiveDifferentCase(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseInsensitiveEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseInsensitiveLess(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseNumberLess(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16EmptyEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16EmptyLessThanNonEmpty(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16NullTerminated(testCount, failCount);

	// FlCompareStringOrdinalUtf16 — Non-ASCII
	FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonAsciiEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16CaseInsensitiveNonEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16GermanCaseInsensitive(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16FinnishCaseInsensitive(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16SwedishCaseInsensitive(testCount, failCount);

	// FlCompareStringOrdinalUtf16 — Lone surrogates
	FlUnicodeCaseProcessingUtUtf16LoneSurrogateEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16DifferentLoneSurrogatesEqual(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16ValidAfterLoneSurrogate(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16LoneSurrogateVsValidChar(testCount, failCount);
	FlUnicodeCaseProcessingUtUtf16ValidCharVsLoneSurrogate(testCount, failCount);
}
