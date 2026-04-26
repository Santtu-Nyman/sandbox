/*
	Random file name unit tests by Santtu S. Nyman.

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

		Unit tests for FlCreateRandomFileName (UTF-16) and FlCreateRandomFileNameUtf8.

		Both functions must:
		  - Return exactly FL_RANDOM_FILE_NAME_LENGTH (8) on every call.
		  - Write only characters from the 52-character valid set
		    (digits 0-9, uppercase A-Z, and the specials ! # $ % & ' ( ) - @ ^ _ ` { } ~).
		  - Produce a different name on each call (verified by generating 64 names
		    and confirming all pairs are distinct; the probability of a false failure
		    with 52^8 ~= 5.4 * 10^13 possible names is negligible).
*/

#define WIN32_LEAN_AND_MEAN
#include "FlUt.h"
#include "../include/FlRandomFileName.h"
#include <stddef.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Valid-character check helper
// ---------------------------------------------------------------------------

// The 52 characters that the implementation may produce, in the same order as
// FlInternalRandomFileNameCharacterTable in FlRandomFileName.c.
static const char FlRandomFileNameUtValidChars[52] = {
	'0', '1',  '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L',  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V',  'W', 'X', 'Y', 'Z', '!', '#', '$', '%',
	'&', '\'', '(', ')', '-', '@', '^', '_', '`', '{',
	'}', '~'
};

static int FlRandomFileNameUtIsValidChar(char c)
{
	for (size_t i = 0; i < sizeof FlRandomFileNameUtValidChars; i++)
	{
		if (c == FlRandomFileNameUtValidChars[i])
			return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Test cases — wide (UTF-16) version
// ---------------------------------------------------------------------------

// FlCreateRandomFileName must return FL_RANDOM_FILE_NAME_LENGTH on every call.
static void FlRandomFileNameUtWideReturnValue(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < 64; i++)
	{
		WCHAR buf[FL_RANDOM_FILE_NAME_LENGTH];
		size_t len = FlCreateRandomFileName(buf);
		if (len != FL_RANDOM_FILE_NAME_LENGTH)
			allCorrect = 0;
	}
	FL_UT_CHECK(allCorrect, "FlRandomFileNameUtWideReturnValue");
}

// FlCreateRandomFileName must write exactly FL_RANDOM_FILE_NAME_LENGTH characters
// (the return value is the written length, verified by checking each character is
// non-zero, which is always true for any member of the valid set).
static void FlRandomFileNameUtWideLength(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < 64; i++)
	{
		WCHAR buf[FL_RANDOM_FILE_NAME_LENGTH + 1];
		buf[FL_RANDOM_FILE_NAME_LENGTH] = L'\0';
		size_t len = FlCreateRandomFileName(buf);
		if (len != FL_RANDOM_FILE_NAME_LENGTH)
		{
			allCorrect = 0;
			break;
		}
		// All FL_RANDOM_FILE_NAME_LENGTH positions must be filled (non-zero).
		for (size_t j = 0; j < FL_RANDOM_FILE_NAME_LENGTH; j++)
		{
			if (buf[j] == L'\0')
			{
				allCorrect = 0;
				break;
			}
		}
		if (!allCorrect)
			break;
	}
	FL_UT_CHECK(allCorrect, "FlRandomFileNameUtWideLength");
}

// Every character written by FlCreateRandomFileName must belong to the valid set.
static void FlRandomFileNameUtWideValidChars(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allValid = 1;
	for (int i = 0; i < 64; i++)
	{
		WCHAR buf[FL_RANDOM_FILE_NAME_LENGTH];
		FlCreateRandomFileName(buf);
		for (size_t j = 0; j < FL_RANDOM_FILE_NAME_LENGTH; j++)
		{
			// Every character must fit in ASCII range and be in the valid set.
			if (buf[j] > 127 || !FlRandomFileNameUtIsValidChar((char)buf[j]))
			{
				allValid = 0;
				break;
			}
		}
		if (!allValid)
			break;
	}
	FL_UT_CHECK(allValid, "FlRandomFileNameUtWideValidChars");
}

// 64 consecutive calls to FlCreateRandomFileName must all produce distinct names.
// With 52^8 ~= 5.4 * 10^13 possible names the probability of any collision is
// approximately 64^2 / (2 * 52^8) ~= 3.8 * 10^-11, so a spurious failure is
// practically impossible.
static void FlRandomFileNameUtWideUniqueness(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR names[64][FL_RANDOM_FILE_NAME_LENGTH];
	for (int i = 0; i < 64; i++)
		FlCreateRandomFileName(names[i]);

	int allUnique = 1;
	for (int i = 0; i < 64 && allUnique; i++)
		for (int j = i + 1; j < 64 && allUnique; j++)
			if (memcmp(names[i], names[j], FL_RANDOM_FILE_NAME_LENGTH * sizeof(WCHAR)) == 0)
				allUnique = 0;

	FL_UT_CHECK(allUnique, "FlRandomFileNameUtWideUniqueness");
}

// ---------------------------------------------------------------------------
// Test cases — UTF-8 version
// ---------------------------------------------------------------------------

// FlCreateRandomFileNameUtf8 must return FL_RANDOM_FILE_NAME_LENGTH on every call.
static void FlRandomFileNameUtUtf8ReturnValue(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < 64; i++)
	{
		char buf[FL_RANDOM_FILE_NAME_LENGTH];
		size_t len = FlCreateRandomFileNameUtf8(buf);
		if (len != FL_RANDOM_FILE_NAME_LENGTH)
			allCorrect = 0;
	}
	FL_UT_CHECK(allCorrect, "FlRandomFileNameUtUtf8ReturnValue");
}

// FlCreateRandomFileNameUtf8 must write exactly FL_RANDOM_FILE_NAME_LENGTH characters.
static void FlRandomFileNameUtUtf8Length(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < 64; i++)
	{
		char buf[FL_RANDOM_FILE_NAME_LENGTH + 1];
		buf[FL_RANDOM_FILE_NAME_LENGTH] = '\0';
		size_t len = FlCreateRandomFileNameUtf8(buf);
		if (len != FL_RANDOM_FILE_NAME_LENGTH)
		{
			allCorrect = 0;
			break;
		}
		for (size_t j = 0; j < FL_RANDOM_FILE_NAME_LENGTH; j++)
		{
			if (buf[j] == '\0')
			{
				allCorrect = 0;
				break;
			}
		}
		if (!allCorrect)
			break;
	}
	FL_UT_CHECK(allCorrect, "FlRandomFileNameUtUtf8Length");
}

// Every character written by FlCreateRandomFileNameUtf8 must belong to the valid set.
static void FlRandomFileNameUtUtf8ValidChars(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allValid = 1;
	for (int i = 0; i < 64; i++)
	{
		char buf[FL_RANDOM_FILE_NAME_LENGTH];
		FlCreateRandomFileNameUtf8(buf);
		for (size_t j = 0; j < FL_RANDOM_FILE_NAME_LENGTH; j++)
		{
			if (!FlRandomFileNameUtIsValidChar(buf[j]))
			{
				allValid = 0;
				break;
			}
		}
		if (!allValid)
			break;
	}
	FL_UT_CHECK(allValid, "FlRandomFileNameUtUtf8ValidChars");
}

// 64 consecutive calls to FlCreateRandomFileNameUtf8 must all produce distinct names.
static void FlRandomFileNameUtUtf8Uniqueness(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char names[64][FL_RANDOM_FILE_NAME_LENGTH];
	for (int i = 0; i < 64; i++)
		FlCreateRandomFileNameUtf8(names[i]);

	int allUnique = 1;
	for (int i = 0; i < 64 && allUnique; i++)
		for (int j = i + 1; j < 64 && allUnique; j++)
			if (memcmp(names[i], names[j], FL_RANDOM_FILE_NAME_LENGTH) == 0)
				allUnique = 0;

	FL_UT_CHECK(allUnique, "FlRandomFileNameUtUtf8Uniqueness");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlRandomFileNameUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlRandomFileNameUtWideReturnValue(testCount, failCount);
	FlRandomFileNameUtWideLength(testCount, failCount);
	FlRandomFileNameUtWideValidChars(testCount, failCount);
	FlRandomFileNameUtWideUniqueness(testCount, failCount);
	FlRandomFileNameUtUtf8ReturnValue(testCount, failCount);
	FlRandomFileNameUtUtf8Length(testCount, failCount);
	FlRandomFileNameUtUtf8ValidChars(testCount, failCount);
	FlRandomFileNameUtUtf8Uniqueness(testCount, failCount);
}
