/*
	Unicode case processing library version 1.1.0 2026-04-12 by Santtu S. Nyman.

	Description
		Simple unicode case processing library.

	Version history
		version 1.1.0 2026-04-12
			Added UTF-8 support.
		version 1.0.0 2024-04-01
			Initial UTF-16 only version.

	License
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
		For more information, please refer to <https://unlicense.org>
*/

#ifndef FL_UNICODE_CASE_PROCESSING_H
#define FL_UNICODE_CASE_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>

int FlCodepointToUpperCase(_In_ int Codepoint);
/*
	Function:
		FlCodepointToUpperCase

	Description:
		Maps a Unicode code point to its uppercase equivalent.
		The mapping covers the full Unicode character repertoire, not just ASCII.
		If the given code point has no uppercase form, the code point is returned
		unchanged.
		This function cannot fail.

	Parameters:
		Codepoint:
			The Unicode code point to convert.
			Valid values are 0 through 0x10FFFF inclusive.
			Values outside this range are returned unchanged.

	Return:
		Returns the uppercase equivalent of Codepoint, or Codepoint itself if
		there is no uppercase mapping.
*/

int FlCodepointToLowerCase(_In_ int Codepoint);
/*
	Function:
		FlCodepointToLowerCase

	Description:
		Maps a Unicode code point to its lowercase equivalent.
		The mapping covers the full Unicode character repertoire, not just ASCII.
		If the given code point has no lowercase form, the code point is returned
		unchanged.
		This function cannot fail.

	Parameters:
		Codepoint:
			The Unicode code point to convert.
			Valid values are 0 through 0x10FFFF inclusive.
			Values outside this range are returned unchanged.

	Return:
		Returns the lowercase equivalent of Codepoint, or Codepoint itself if
		there is no lowercase mapping.
*/

int FlCompareStringOrdinalUtf8(_In_NLS_string_(String1Length) const char* String1, _In_ size_t String1Length, _In_NLS_string_(String2Length) const char* String2, _In_ size_t String2Length, _In_ BOOL IgnoreCase);
/*
	Function:
		FlCompareStringOrdinalUtf8

	Description:
		Compares two UTF-8 strings strings lexicographically.
		This function is the UTF-8 equivalent of the Win32 API CompareStringOrdinal.

		When an input string is invalid the function can't order the strings,
		but if the input strings are not identical the return value will never be CSTR_LESS_THAN.

	Parameters:
		String1:
			Pointer to the first non-null-terminated UTF-8 string.
			If String1Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		String1Length:
			Length of String1 in bytes (UTF-8 code units), not including any null
			terminator.  Pass (size_t)-1 to indicate that String1 is null
			terminated.

		String2:
			Pointer to the second non-null-terminated UTF-8 string.
			If String2Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		String2Length:
			Length of String2 in bytes (UTF-8 code units), not including any null
			terminator.  Pass (size_t)-1 to indicate that String2 is null
			terminated.

		IgnoreCase:
			If TRUE the function is to perform a case-insensitive comparison,
			using hard-coded locale invariant casing table information.

	Return:
		Returns one of the following constants:
		  CSTR_LESS_THAN    (1) if String1 appears before String2 in lexicographical order.
		  CSTR_EQUAL        (2) if String1 and String2 compare equal.
		  CSTR_GREATER_THAN (3) if String1 appears after String2 in lexicographical order.
		To maintain the C runtime convention of comparing strings,
		the value 2 can be subtracted from a return value. Then, the meaning of <0, ==0, and >0 is consistent with the C runtime.
*/

int FlCompareStringOrdinalUtf16(_In_NLS_string_(String1Length) const WCHAR* String1, _In_ size_t String1Length, _In_NLS_string_(String2Length) const WCHAR* String2, _In_ size_t String2Length, _In_ BOOL IgnoreCase);
/*
	Function:
		FlCompareStringOrdinalUtf16

	Description:
		Compares two UTF-16 strings strings lexicographically.
		This function is the equivalent of the Win32 API CompareStringOrdinal.

		When an input string is invalid the function can't order the strings,
		but if the input strings are not identical the return value will never be CSTR_LESS_THAN.

	Parameters:
		String1:
			Pointer to the first non-null-terminated UTF-16 string.
			If String1Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		String1Length:
			Length of String1 in WCHARs (UTF-16 code units), not including any
			null terminator.  Pass (size_t)-1 to indicate that String1 is null
			terminated.

		String2:
			Pointer to the second non-null-terminated UTF-16 string.
			If String2Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		String2Length:
			Length of String2 in WCHARs (UTF-16 code units), not including any
			null terminator.  Pass (size_t)-1 to indicate that String2 is null
			terminated.

		IgnoreCase:
			If TRUE the function is to perform a case-insensitive comparison,
			using hard-coded locale invariant casing table information.

	Return:
		Returns one of the following constants:
		  CSTR_LESS_THAN    (1) if String1 appears before String2 in lexicographical order.
		  CSTR_EQUAL        (2) if String1 and String2 compare equal.
		  CSTR_GREATER_THAN (3) if String1 appears after String2 in lexicographical order.
		To maintain the C runtime convention of comparing strings,
		the value 2 can be subtracted from a return value. Then, the meaning of <0, ==0, and >0 is consistent with the C runtime.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_UNICODE_CASE_PROCESSING_H
