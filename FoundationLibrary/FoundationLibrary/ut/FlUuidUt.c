/*
	UUID unit tests by Santtu S. Nyman.

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

		Unit tests for FlUuidCreateRandomId, FlUuidEncodeStringUtf8,
		FlUuidEncodeStringUtf16, FlUuidDecodeStringUtf8, and
		FlUuidDecodeStringUtf16.

		Encode/decode test vectors verify the Windows UUID byte order
		(little-endian Data1/Data2/Data3, big-endian Data4):

		  Bytes                                              String
		  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   {00000000-0000-0000-0000-000000000000}
		  00 00 00 00 00 00 00 40 80 00 00 00 00 00 00 00   {00000000-0000-4000-8000-000000000000}
		  F7 DE BB 3A 1E 05 16 4C 85 B6 13 3A C7 74 93 97   {3ABBDEF7-051E-4C16-85B6-133AC7749397}
		  C4 FC 25 ED 71 78 21 44 90 50 BE 3B E3 5E A8 87   {ED25FCC4-7871-4421-9050-BE3BE35EA887}
		  E3 AE 09 F9 8D 5D C3 42 A9 18 60 DF 9D 10 66 8E   {F909AEE3-5D8D-42C3-A918-60DF9D10668E}
		  0A 96 E4 58 EB CA DB 4F B6 A5 7C A6 65 30 B5 B2   {58E4960A-CAEB-4FDB-B6A5-7CA66530B5B2}
*/

#define WIN32_LEAN_AND_MEAN
#include "FlUt.h"
#include "../include/FlUuid.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Shared test vectors
// ---------------------------------------------------------------------------

#define FL_UUID_UT_VECTOR_COUNT 6

static const uint8_t FlUuidUtBytes[FL_UUID_UT_VECTOR_COUNT][FL_UUID_SIZE] =
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xF7, 0xDE, 0xBB, 0x3A, 0x1E, 0x05, 0x16, 0x4C, 0x85, 0xB6, 0x13, 0x3A, 0xC7, 0x74, 0x93, 0x97 },
	{ 0xC4, 0xFC, 0x25, 0xED, 0x71, 0x78, 0x21, 0x44, 0x90, 0x50, 0xBE, 0x3B, 0xE3, 0x5E, 0xA8, 0x87 },
	{ 0xE3, 0xAE, 0x09, 0xF9, 0x8D, 0x5D, 0xC3, 0x42, 0xA9, 0x18, 0x60, 0xDF, 0x9D, 0x10, 0x66, 0x8E },
	{ 0x0A, 0x96, 0xE4, 0x58, 0xEB, 0xCA, 0xDB, 0x4F, 0xB6, 0xA5, 0x7C, 0xA6, 0x65, 0x30, 0xB5, 0xB2 }
};

static const char FlUuidUtStrings[FL_UUID_UT_VECTOR_COUNT][39] =
{
	"{00000000-0000-0000-0000-000000000000}",
	"{00000000-0000-4000-8000-000000000000}",
	"{3ABBDEF7-051E-4C16-85B6-133AC7749397}",
	"{ED25FCC4-7871-4421-9050-BE3BE35EA887}",
	"{F909AEE3-5D8D-42C3-A918-60DF9D10668E}",
	"{58E4960A-CAEB-4FDB-B6A5-7CA66530B5B2}"
};

// ---------------------------------------------------------------------------
// FlUuidCreateRandomId tests
// ---------------------------------------------------------------------------

// FlUuidCreateRandomId must not produce the nil UUID (all zero bytes) or
// the specific near-nil value {00000000-0000-4000-8000-000000000000}.
// P(either match) is (2 / 2^128) ~= 5.9e-39.
static void FlUuidUtNotNil(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t id[FL_UUID_SIZE];
	FlUuidCreateRandomId(id);
	int notNil = (memcmp(id, FlUuidUtBytes[0], FL_UUID_SIZE) != 0) &&
	             (memcmp(id, FlUuidUtBytes[1], FL_UUID_SIZE) != 0);
	FL_UT_CHECK(notNil, "FlUuidUtNotNil");
}

// 256 calls to FlUuidCreateRandomId must all produce distinct UUIDs.
// P(any collision among 256 128-bit values) ~= 256^2 / (2 * 2^128) ~= 2.4e-34.
static void FlUuidUtCreateRandomUnique(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t ids[256][FL_UUID_SIZE];
	for (int i = 0; i < 256; i++)
		FlUuidCreateRandomId(ids[i]);

	int allUnique = 1;
	for (int i = 0; i < 256 && allUnique; i++)
		for (int j = i + 1; j < 256 && allUnique; j++)
			if (memcmp(ids[i], ids[j], FL_UUID_SIZE) == 0)
				allUnique = 0;

	FL_UT_CHECK(allUnique, "FlUuidUtCreateRandomUnique");
}

// ---------------------------------------------------------------------------
// FlUuidEncodeStringUtf8 tests
// ---------------------------------------------------------------------------

// FlUuidEncodeStringUtf8 must return 38 on every call.
static void FlUuidUtEncodeUtf8Length(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < FL_UUID_UT_VECTOR_COUNT; i++)
	{
		char buf[38];
		size_t len = FlUuidEncodeStringUtf8(FlUuidUtBytes[i], buf);
		if (len != 38)
		{
			allCorrect = 0;
			break;
		}
	}
	FL_UT_CHECK(allCorrect, "FlUuidUtEncodeUtf8Length");
}

// FlUuidEncodeStringUtf8 must encode each test vector to the expected string.
static void FlUuidUtEncodeUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < FL_UUID_UT_VECTOR_COUNT; i++)
	{
		char buf[38];
		FlUuidEncodeStringUtf8(FlUuidUtBytes[i], buf);
		if (memcmp(buf, FlUuidUtStrings[i], 38) != 0)
		{
			allCorrect = 0;
			break;
		}
	}
	FL_UT_CHECK(allCorrect, "FlUuidUtEncodeUtf8");
}

// ---------------------------------------------------------------------------
// FlUuidEncodeStringUtf16 tests
// ---------------------------------------------------------------------------

// FlUuidEncodeStringUtf16 must return 38 on every call.
static void FlUuidUtEncodeUtf16Length(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < FL_UUID_UT_VECTOR_COUNT; i++)
	{
		WCHAR buf[38];
		size_t len = FlUuidEncodeStringUtf16(FlUuidUtBytes[i], buf);
		if (len != 38)
		{
			allCorrect = 0;
			break;
		}
	}
	FL_UT_CHECK(allCorrect, "FlUuidUtEncodeUtf16Length");
}

// FlUuidEncodeStringUtf16 must encode each test vector to the expected string.
static void FlUuidUtEncodeUtf16(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < FL_UUID_UT_VECTOR_COUNT; i++)
	{
		WCHAR buf[38];
		FlUuidEncodeStringUtf16(FlUuidUtBytes[i], buf);
		// All expected characters are ASCII so each WCHAR equals the char value.
		for (int j = 0; j < 38; j++)
		{
			if (buf[j] != (WCHAR)(unsigned char)FlUuidUtStrings[i][j])
			{
				allCorrect = 0;
				break;
			}
		}
		if (!allCorrect)
			break;
	}
	FL_UT_CHECK(allCorrect, "FlUuidUtEncodeUtf16");
}

// ---------------------------------------------------------------------------
// FlUuidDecodeStringUtf8 tests
// ---------------------------------------------------------------------------

// FlUuidDecodeStringUtf8 must decode each test string to the expected bytes
// and return 38 (the length of the decoded UUID string).
static void FlUuidUtDecodeUtf8(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int allCorrect = 1;
	for (int i = 0; i < FL_UUID_UT_VECTOR_COUNT; i++)
	{
		uint8_t id[FL_UUID_SIZE];
		size_t result = FlUuidDecodeStringUtf8(id, 38, FlUuidUtStrings[i]);
		if (result != 38 || memcmp(id, FlUuidUtBytes[i], FL_UUID_SIZE) != 0)
		{
			allCorrect = 0;
			break;
		}
	}
	FL_UT_CHECK(allCorrect, "FlUuidUtDecodeUtf8");
}

// ---------------------------------------------------------------------------
// FlUuidDecodeStringUtf16 tests
// ---------------------------------------------------------------------------

// FlUuidDecodeStringUtf16 must decode each test string to the expected bytes
// and return 38 (the length of the decoded UUID string).
static void FlUuidUtDecodeUtf16(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// Build wide versions of the test strings at compile time.
	static const WCHAR wideStrings[FL_UUID_UT_VECTOR_COUNT][39] =
	{
		L"{00000000-0000-0000-0000-000000000000}",
		L"{00000000-0000-4000-8000-000000000000}",
		L"{3ABBDEF7-051E-4C16-85B6-133AC7749397}",
		L"{ED25FCC4-7871-4421-9050-BE3BE35EA887}",
		L"{F909AEE3-5D8D-42C3-A918-60DF9D10668E}",
		L"{58E4960A-CAEB-4FDB-B6A5-7CA66530B5B2}"
	};

	int allCorrect = 1;
	for (int i = 0; i < FL_UUID_UT_VECTOR_COUNT; i++)
	{
		uint8_t id[FL_UUID_SIZE];
		size_t result = FlUuidDecodeStringUtf16(id, 38, wideStrings[i]);
		if (result != 38 || memcmp(id, FlUuidUtBytes[i], FL_UUID_SIZE) != 0)
		{
			allCorrect = 0;
			break;
		}
	}
	FL_UT_CHECK(allCorrect, "FlUuidUtDecodeUtf16");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlUuidUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlUuidUtNotNil(testCount, failCount);
	FlUuidUtCreateRandomUnique(testCount, failCount);
	FlUuidUtEncodeUtf8Length(testCount, failCount);
	FlUuidUtEncodeUtf8(testCount, failCount);
	FlUuidUtEncodeUtf16Length(testCount, failCount);
	FlUuidUtEncodeUtf16(testCount, failCount);
	FlUuidUtDecodeUtf8(testCount, failCount);
	FlUuidUtDecodeUtf16(testCount, failCount);
}
