/*
	Win32 command line parsing unit tests by Santtu S. Nyman.

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

		Unit tests for FlWin32CommandLineToArgumentsUtf16 and
		FlWin32CommandLineToArgumentsUtf8.

		The same command line cases are exercised for both functions.
		All test command lines use ASCII characters so the UTF-16 and
		UTF-8 argument text is identical.
*/

#define WIN32_LEAN_AND_MEAN
#include "FlUt.h"
#include "../include/FlWin32CommandLine.h"
#include <stddef.h>
#include <string.h>
#include <wchar.h>

// Returns 1 when args matches expectedCount and every string equals the
// corresponding entry in expected[].  Also verifies the null sentinel
// at args[count].
static int flCmdLineArgsEqW(WCHAR** args, size_t count, size_t expectedCount, const WCHAR* const* expected)
{
	if (count != expectedCount || args[count] != NULL)
		return 0;
	for (size_t i = 0; i < count; i++)
		if (wcscmp(args[i], expected[i]) != 0)
			return 0;
	return 1;
}

static int flCmdLineArgsEqU8(char** args, size_t count, size_t expectedCount, const char* const* expected)
{
	if (count != expectedCount || args[count] != NULL)
		return 0;
	for (size_t i = 0; i < count; i++)
		if (strcmp(args[i], expected[i]) != 0)
			return 0;
	return 1;
}

// ============================================================================
// FlWin32CommandLineToArgumentsUtf16
// ============================================================================

// A single token with no spaces or quotes produces one argument.
static void FlWin32CommandLineUtUtf16SingleArg(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"hello";
	static const WCHAR* const expected[] = { L"hello" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_SingleArg_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_SingleArg_Args");
}

// A space between two tokens splits them into two arguments.
static void FlWin32CommandLineUtUtf16TwoArgs(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo bar";
	static const WCHAR* const expected[] = { L"foo", L"bar" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_TwoArgs_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_TwoArgs_Args");
}

// Three space-separated tokens produce three arguments.
static void FlWin32CommandLineUtUtf16ThreeArgs(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"a b c";
	static const WCHAR* const expected[] = { L"a", L"b", L"c" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_ThreeArgs_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 3, expected), "Utf16_ThreeArgs_Args");
}

// A tab character is a valid argument separator.
static void FlWin32CommandLineUtUtf16TabSeparator(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo\tbar";
	static const WCHAR* const expected[] = { L"foo", L"bar" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_TabSeparator_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_TabSeparator_Args");
}

// Trailing spaces after the last token are ignored.
static void FlWin32CommandLineUtUtf16TrailingSpaces(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo  ";
	static const WCHAR* const expected[] = { L"foo" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_TrailingSpaces_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_TrailingSpaces_Args");
}

// Multiple consecutive spaces between tokens count as a single separator.
static void FlWin32CommandLineUtUtf16MultipleSpacesBetween(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo   bar";
	static const WCHAR* const expected[] = { L"foo", L"bar" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_MultipleSpaces_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_MultipleSpaces_Args");
}

// Quotes preserve an embedded space as part of a single argument.
static void FlWin32CommandLineUtUtf16QuotedWithSpace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"foo bar\"";
	static const WCHAR* const expected[] = { L"foo bar" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_QuotedWithSpace_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_QuotedWithSpace_Args");
}

// An adjacent pair of quotes "" produces one empty-string argument.
static void FlWin32CommandLineUtUtf16EmptyQuotedArg(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"\"";
	static const WCHAR* const expected[] = { L"" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_EmptyQuoted_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_EmptyQuoted_Args");
}

// A quoted argument followed by an unquoted argument produces two arguments.
static void FlWin32CommandLineUtUtf16QuotedAndUnquoted(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"hello world\" foo";
	static const WCHAR* const expected[] = { L"hello world", L"foo" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_QuotedAndUnquoted_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QuotedAndUnquoted_Args");
}

// An odd number of backslashes immediately before a quote: (n-1)/2 literal
// backslashes plus one literal quote character, no toggle of quoting.
// Command line: \"hello  ->  arg: "hello
static void FlWin32CommandLineUtUtf16EscapedQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\\\"hello";
	static const WCHAR* const expected[] = { L"\"hello" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_EscapedQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_EscapedQuote_Args");
}

// An even number of backslashes immediately before a quote: n/2 literal
// backslashes and the quote toggles quoting mode as normal.
// Command line: \\"hello"  ->  arg: \hello
static void FlWin32CommandLineUtUtf16EvenBackslashesBeforeQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\\\\\"hello\"";
	static const WCHAR* const expected[] = { L"\\hello" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_EvenBsBeforeQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_EvenBsBeforeQuote_Args");
}

// Three backslashes before a quote (odd): one literal backslash plus one
// literal quote, no toggle of quoting.
// Command line: \\\"hello  ->  arg: \"hello
static void FlWin32CommandLineUtUtf16OddBackslashesBeforeQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\\\\\\\"hello";
	static const WCHAR* const expected[] = { L"\\\"hello" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_OddBsBeforeQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_OddBsBeforeQuote_Args");
}

// Backslashes not immediately before a quote are treated as literal characters.
// Command line: foo\\bar  ->  arg: foo\\bar
static void FlWin32CommandLineUtUtf16BackslashBeforeNonQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo\\\\bar";
	static const WCHAR* const expected[] = { L"foo\\\\bar" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_BsBeforeNonQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_BsBeforeNonQuote_Args");
}

// A "" sequence inside an open quoted region produces one literal quote
// character and keeps the quoted region open.
// Command line: "foo""bar"  ->  arg: foo"bar
static void FlWin32CommandLineUtUtf16QuotedDoubleQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"foo\"\"bar\"";
	static const WCHAR* const expected[] = { L"foo\"bar" };
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_QuotedDblQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_QuotedDblQuote_Args");
}

// Calling with bufferSize 0 and NULL buffer returns ERROR_INSUFFICIENT_BUFFER
// with the required size.  A second call with that exact size succeeds.
static void FlWin32CommandLineUtUtf16SizeQuery(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo bar";
	static const WCHAR* const expected[] = { L"foo", L"bar" };
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, 0, NULL, &required, &count);
	FL_UT_CHECK(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "Utf16_SizeQuery_ReturnCode");
	FL_UT_CHECK(required > 0, "Utf16_SizeQuery_RequiredNonZero");
	FL_UT_CHECK(count == 2, "Utf16_SizeQuery_Count");
	WCHAR* buf[64];
	FL_UT_CHECK(required <= sizeof(buf), "Utf16_SizeQuery_FitsInStack");
	if (required <= sizeof(buf))
	{
		size_t required2 = 0, count2 = 0;
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, required, buf, &required2, &count2);
		FL_UT_CHECK(hr == S_OK, "Utf16_SizeQuery_SecondCall_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count2, 2, expected), "Utf16_SizeQuery_SecondCall_Args");
	}
}

// A buffer that is one byte too small causes ERROR_INSUFFICIENT_BUFFER and
// writes the required size.
static void FlWin32CommandLineUtUtf16BufferTooSmall(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo";
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(cmd, 1, buf, &required, &count);
	FL_UT_CHECK(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "Utf16_BufTooSmall_ReturnCode");
	FL_UT_CHECK(required > 1, "Utf16_BufTooSmall_RequiredSize");
}

// An empty command line causes the function to substitute the current
// executable path, producing exactly one non-empty argument.
static void FlWin32CommandLineUtUtf16EmptyCommandLine(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR* buf[256];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf16(L"", sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf16_EmptyCmd_S_OK");
	FL_UT_CHECK(count == 1, "Utf16_EmptyCmd_Count");
	FL_UT_CHECK(buf[0] != NULL && buf[0][0] != L'\0', "Utf16_EmptyCmd_ArgNonEmpty");
	FL_UT_CHECK(buf[1] == NULL, "Utf16_EmptyCmd_NullTerminator");
}

// ============================================================================
// FlWin32CommandLineToArgumentsUtf8
// ============================================================================

static void FlWin32CommandLineUtUtf8SingleArg(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"hello";
	static const char* const expected[] = { "hello" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_SingleArg_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_SingleArg_Args");
}

static void FlWin32CommandLineUtUtf8TwoArgs(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo bar";
	static const char* const expected[] = { "foo", "bar" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_TwoArgs_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_TwoArgs_Args");
}

static void FlWin32CommandLineUtUtf8ThreeArgs(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"a b c";
	static const char* const expected[] = { "a", "b", "c" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_ThreeArgs_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 3, expected), "Utf8_ThreeArgs_Args");
}

static void FlWin32CommandLineUtUtf8TabSeparator(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo\tbar";
	static const char* const expected[] = { "foo", "bar" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_TabSeparator_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_TabSeparator_Args");
}

static void FlWin32CommandLineUtUtf8TrailingSpaces(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo  ";
	static const char* const expected[] = { "foo" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_TrailingSpaces_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_TrailingSpaces_Args");
}

static void FlWin32CommandLineUtUtf8MultipleSpacesBetween(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo   bar";
	static const char* const expected[] = { "foo", "bar" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_MultipleSpaces_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_MultipleSpaces_Args");
}

static void FlWin32CommandLineUtUtf8QuotedWithSpace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"foo bar\"";
	static const char* const expected[] = { "foo bar" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_QuotedWithSpace_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_QuotedWithSpace_Args");
}

static void FlWin32CommandLineUtUtf8EmptyQuotedArg(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"\"";
	static const char* const expected[] = { "" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_EmptyQuoted_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_EmptyQuoted_Args");
}

static void FlWin32CommandLineUtUtf8QuotedAndUnquoted(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"hello world\" foo";
	static const char* const expected[] = { "hello world", "foo" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_QuotedAndUnquoted_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QuotedAndUnquoted_Args");
}

// Command line: \"hello  ->  arg: "hello
static void FlWin32CommandLineUtUtf8EscapedQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\\\"hello";
	static const char* const expected[] = { "\"hello" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_EscapedQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_EscapedQuote_Args");
}

// Command line: \\"hello"  ->  arg: \hello
static void FlWin32CommandLineUtUtf8EvenBackslashesBeforeQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\\\\\"hello\"";
	static const char* const expected[] = { "\\hello" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_EvenBsBeforeQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_EvenBsBeforeQuote_Args");
}

// Command line: \\\"hello  ->  arg: \"hello
static void FlWin32CommandLineUtUtf8OddBackslashesBeforeQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\\\\\\\"hello";
	static const char* const expected[] = { "\\\"hello" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_OddBsBeforeQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_OddBsBeforeQuote_Args");
}

// Command line: foo\\bar  ->  arg: foo\\bar
static void FlWin32CommandLineUtUtf8BackslashBeforeNonQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo\\\\bar";
	static const char* const expected[] = { "foo\\\\bar" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_BsBeforeNonQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_BsBeforeNonQuote_Args");
}

// Command line: "foo""bar"  ->  arg: foo"bar
static void FlWin32CommandLineUtUtf8QuotedDoubleQuote(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"\"foo\"\"bar\"";
	static const char* const expected[] = { "foo\"bar" };
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_QuotedDblQuote_S_OK");
	FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_QuotedDblQuote_Args");
}

static void FlWin32CommandLineUtUtf8SizeQuery(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo bar";
	static const char* const expected[] = { "foo", "bar" };
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, 0, NULL, &required, &count);
	FL_UT_CHECK(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "Utf8_SizeQuery_ReturnCode");
	FL_UT_CHECK(required > 0, "Utf8_SizeQuery_RequiredNonZero");
	FL_UT_CHECK(count == 2, "Utf8_SizeQuery_Count");
	char* buf[64];
	FL_UT_CHECK(required <= sizeof(buf), "Utf8_SizeQuery_FitsInStack");
	if (required <= sizeof(buf))
	{
		size_t required2 = 0, count2 = 0;
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, required, buf, &required2, &count2);
		FL_UT_CHECK(hr == S_OK, "Utf8_SizeQuery_SecondCall_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count2, 2, expected), "Utf8_SizeQuery_SecondCall_Args");
	}
}

static void FlWin32CommandLineUtUtf8BufferTooSmall(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR cmd[] = L"foo";
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(cmd, 1, buf, &required, &count);
	FL_UT_CHECK(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "Utf8_BufTooSmall_ReturnCode");
	FL_UT_CHECK(required > 1, "Utf8_BufTooSmall_RequiredSize");
}

static void FlWin32CommandLineUtUtf8EmptyCommandLine(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char* buf[256];
	size_t required = 0, count = 0;
	HRESULT hr = FlWin32CommandLineToArgumentsUtf8(L"", sizeof(buf), buf, &required, &count);
	FL_UT_CHECK(hr == S_OK, "Utf8_EmptyCmd_S_OK");
	FL_UT_CHECK(count == 1, "Utf8_EmptyCmd_Count");
	FL_UT_CHECK(buf[0] != NULL && buf[0][0] != '\0', "Utf8_EmptyCmd_ArgNonEmpty");
	FL_UT_CHECK(buf[1] == NULL, "Utf8_EmptyCmd_NullTerminator");
}

// Tests 11 representative command lines that combine quotation marks and
// backslashes, covering: literal backslash not before a quote, quoted spaces,
// leading/trailing whitespace trimming, backslash inside quotes, even/odd
// backslash counts before a closing quote, the "" escape for a literal quote
// inside a quoted region, an empty quoted argument, and a fully quoted argv[0].
static void FlWin32CommandLineUtUtf16QuotesAndBackslashes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr;

	// "FlUt.exe x\\y"  ->  ["FlUt.exe", "x\y"]
	// Backslash not before a quote is a literal character.
	{
		static const WCHAR cmd[] = L"FlUt.exe x\\y";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"x\\y" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C01_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C01_Args");
	}

	// "FlUt.exe \"x y\""  ->  ["FlUt.exe", "x y"]
	// Quoted region preserves an embedded space.
	{
		static const WCHAR cmd[] = L"FlUt.exe \"x y\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"x y" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C02_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C02_Args");
	}

	// "FlUt.exe  \"x y\"  "  ->  ["FlUt.exe", "x y"]
	// Extra surrounding spaces are stripped.
	{
		static const WCHAR cmd[] = L"FlUt.exe  \"x y\"  ";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"x y" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C03_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C03_Args");
	}

	// "FlUt.exe \"Hello world.txt\""  ->  ["FlUt.exe", "Hello world.txt"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello world.txt\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"Hello world.txt" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C04_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C04_Args");
	}

	// "FlUt.exe \"C:\\Hello world.txt\""  ->  ["FlUt.exe", "C:\Hello world.txt"]
	// Backslash inside quotes not followed by another quote is literal.
	{
		static const WCHAR cmd[] = L"FlUt.exe \"C:\\Hello world.txt\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"C:\\Hello world.txt" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C05_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C05_Args");
	}

	// "FlUt.exe \"Hello\\\"world\""  ->  ["FlUt.exe", "Hello\"world"]
	// One backslash before a quote (odd count): literal quote, no mode toggle.
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello\\\"world\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"Hello\"world" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C06_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C06_Args");
	}

	// "FlUt.exe \"Hello\\\\\""  ->  ["FlUt.exe", "Hello\"]
	// Two backslashes before closing quote (even count): one literal backslash, quote closes.
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello\\\\\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"Hello\\" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C07_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C07_Args");
	}

	// "FlUt.exe \"Hello\\\\\\\\\""  ->  ["FlUt.exe", "Hello\\"]
	// Four backslashes before closing quote (even count): two literal backslashes, quote closes.
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello\\\\\\\\\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"Hello\\\\" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C08_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C08_Args");
	}

	// "FlUt.exe \"\"\"Hello\""  ->  ["FlUt.exe", "\"Hello"]
	// "" inside a quoted region yields one literal quote, keeps the region open.
	{
		static const WCHAR cmd[] = L"FlUt.exe \"\"\"Hello\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"\"Hello" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C09_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 2, expected), "Utf16_QnB_C09_Args");
	}

	// "FlUt.exe   \"\"   \"Hello\""  ->  ["FlUt.exe", "", "Hello"]
	// Empty quoted argument followed by a normal quoted argument.
	{
		static const WCHAR cmd[] = L"FlUt.exe   \"\"   \"Hello\"";
		static const WCHAR* const expected[] = { L"FlUt.exe", L"", L"Hello" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C10_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 3, expected), "Utf16_QnB_C10_Args");
	}

	// "\"FlUt.exe\""  ->  ["FlUt.exe"]
	// Fully quoted argv[0].
	{
		static const WCHAR cmd[] = L"\"FlUt.exe\"";
		static const WCHAR* const expected[] = { L"FlUt.exe" };
		hr = FlWin32CommandLineToArgumentsUtf16(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf16_QnB_C11_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqW(buf, count, 1, expected), "Utf16_QnB_C11_Args");
	}
}

static void FlWin32CommandLineUtUtf8QuotesAndBackslashes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char* buf[64];
	size_t required = 0, count = 0;
	HRESULT hr;

	// "FlUt.exe x\\y"  ->  ["FlUt.exe", "x\y"]
	{
		static const WCHAR cmd[] = L"FlUt.exe x\\y";
		static const char* const expected[] = { "FlUt.exe", "x\\y" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C01_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C01_Args");
	}

	// "FlUt.exe \"x y\""  ->  ["FlUt.exe", "x y"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"x y\"";
		static const char* const expected[] = { "FlUt.exe", "x y" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C02_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C02_Args");
	}

	// "FlUt.exe  \"x y\"  "  ->  ["FlUt.exe", "x y"]
	{
		static const WCHAR cmd[] = L"FlUt.exe  \"x y\"  ";
		static const char* const expected[] = { "FlUt.exe", "x y" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C03_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C03_Args");
	}

	// "FlUt.exe \"Hello world.txt\""  ->  ["FlUt.exe", "Hello world.txt"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello world.txt\"";
		static const char* const expected[] = { "FlUt.exe", "Hello world.txt" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C04_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C04_Args");
	}

	// "FlUt.exe \"C:\\Hello world.txt\""  ->  ["FlUt.exe", "C:\Hello world.txt"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"C:\\Hello world.txt\"";
		static const char* const expected[] = { "FlUt.exe", "C:\\Hello world.txt" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C05_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C05_Args");
	}

	// "FlUt.exe \"Hello\\\"world\""  ->  ["FlUt.exe", "Hello\"world"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello\\\"world\"";
		static const char* const expected[] = { "FlUt.exe", "Hello\"world" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C06_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C06_Args");
	}

	// "FlUt.exe \"Hello\\\\\""  ->  ["FlUt.exe", "Hello\"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello\\\\\"";
		static const char* const expected[] = { "FlUt.exe", "Hello\\" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C07_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C07_Args");
	}

	// "FlUt.exe \"Hello\\\\\\\\\""  ->  ["FlUt.exe", "Hello\\"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"Hello\\\\\\\\\"";
		static const char* const expected[] = { "FlUt.exe", "Hello\\\\" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C08_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C08_Args");
	}

	// "FlUt.exe \"\"\"Hello\""  ->  ["FlUt.exe", "\"Hello"]
	{
		static const WCHAR cmd[] = L"FlUt.exe \"\"\"Hello\"";
		static const char* const expected[] = { "FlUt.exe", "\"Hello" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C09_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 2, expected), "Utf8_QnB_C09_Args");
	}

	// "FlUt.exe   \"\"   \"Hello\""  ->  ["FlUt.exe", "", "Hello"]
	{
		static const WCHAR cmd[] = L"FlUt.exe   \"\"   \"Hello\"";
		static const char* const expected[] = { "FlUt.exe", "", "Hello" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C10_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 3, expected), "Utf8_QnB_C10_Args");
	}

	// "\"FlUt.exe\""  ->  ["FlUt.exe"]
	{
		static const WCHAR cmd[] = L"\"FlUt.exe\"";
		static const char* const expected[] = { "FlUt.exe" };
		hr = FlWin32CommandLineToArgumentsUtf8(cmd, sizeof(buf), buf, &required, &count);
		FL_UT_CHECK(hr == S_OK, "Utf8_QnB_C11_S_OK");
		FL_UT_CHECK(flCmdLineArgsEqU8(buf, count, 1, expected), "Utf8_QnB_C11_Args");
	}
}

// ============================================================================
// Run function
// ============================================================================

void FlWin32CommandLineUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// FlWin32CommandLineToArgumentsUtf16
	FlWin32CommandLineUtUtf16SingleArg(testCount, failCount);
	FlWin32CommandLineUtUtf16TwoArgs(testCount, failCount);
	FlWin32CommandLineUtUtf16ThreeArgs(testCount, failCount);
	FlWin32CommandLineUtUtf16TabSeparator(testCount, failCount);
	FlWin32CommandLineUtUtf16TrailingSpaces(testCount, failCount);
	FlWin32CommandLineUtUtf16MultipleSpacesBetween(testCount, failCount);
	FlWin32CommandLineUtUtf16QuotedWithSpace(testCount, failCount);
	FlWin32CommandLineUtUtf16EmptyQuotedArg(testCount, failCount);
	FlWin32CommandLineUtUtf16QuotedAndUnquoted(testCount, failCount);
	FlWin32CommandLineUtUtf16EscapedQuote(testCount, failCount);
	FlWin32CommandLineUtUtf16EvenBackslashesBeforeQuote(testCount, failCount);
	FlWin32CommandLineUtUtf16OddBackslashesBeforeQuote(testCount, failCount);
	FlWin32CommandLineUtUtf16BackslashBeforeNonQuote(testCount, failCount);
	FlWin32CommandLineUtUtf16QuotedDoubleQuote(testCount, failCount);
	FlWin32CommandLineUtUtf16SizeQuery(testCount, failCount);
	FlWin32CommandLineUtUtf16BufferTooSmall(testCount, failCount);
	FlWin32CommandLineUtUtf16EmptyCommandLine(testCount, failCount);
	FlWin32CommandLineUtUtf16QuotesAndBackslashes(testCount, failCount);

	// FlWin32CommandLineToArgumentsUtf8
	FlWin32CommandLineUtUtf8SingleArg(testCount, failCount);
	FlWin32CommandLineUtUtf8TwoArgs(testCount, failCount);
	FlWin32CommandLineUtUtf8ThreeArgs(testCount, failCount);
	FlWin32CommandLineUtUtf8TabSeparator(testCount, failCount);
	FlWin32CommandLineUtUtf8TrailingSpaces(testCount, failCount);
	FlWin32CommandLineUtUtf8MultipleSpacesBetween(testCount, failCount);
	FlWin32CommandLineUtUtf8QuotedWithSpace(testCount, failCount);
	FlWin32CommandLineUtUtf8EmptyQuotedArg(testCount, failCount);
	FlWin32CommandLineUtUtf8QuotedAndUnquoted(testCount, failCount);
	FlWin32CommandLineUtUtf8EscapedQuote(testCount, failCount);
	FlWin32CommandLineUtUtf8EvenBackslashesBeforeQuote(testCount, failCount);
	FlWin32CommandLineUtUtf8OddBackslashesBeforeQuote(testCount, failCount);
	FlWin32CommandLineUtUtf8BackslashBeforeNonQuote(testCount, failCount);
	FlWin32CommandLineUtUtf8QuotedDoubleQuote(testCount, failCount);
	FlWin32CommandLineUtUtf8SizeQuery(testCount, failCount);
	FlWin32CommandLineUtUtf8BufferTooSmall(testCount, failCount);
	FlWin32CommandLineUtUtf8EmptyCommandLine(testCount, failCount);
	FlWin32CommandLineUtUtf8QuotesAndBackslashes(testCount, failCount);
}
