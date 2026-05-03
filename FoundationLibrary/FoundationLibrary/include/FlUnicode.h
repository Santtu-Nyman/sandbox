/*
	Unicode string library by Santtu S. Nyman.

	Version history
		version 1.2.0 2026-05-03
			Removed big endian UTF-16 support and added UTF-32 support.
		version 1.1.1 2026-04-12
			Added UTF-8 support for string comparison.
		version 1.1.0 2024-04-01
			Initial UTF-16 only string comparison version.
		version 1.0.0 2023-02-25
			First publicly available version.

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

#ifndef FL_UNICODE_H
#define FL_UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>
#include "FlSAL.h"

size_t FlConvertUtf8ToUtf16(_In_ size_t utf8Length, _In_reads_(utf8Length) const char* utf8Data, _In_ size_t utf16BufferLength, _Out_writes_to_(utf16BufferLength, return) WCHAR* utf16Buffer);
/*
	Function:
		FlConvertUtf8ToUtf16

	Description:
		Converts a UTF-8 encoded string to a UTF-16 encoded string.
		The output is not null-terminated.
		Invalid UTF-8 byte sequences are replaced with the Unicode replacement
		character U+FFFD in the output.
		To determine the required output buffer size, call this function with
		utf16BufferLength set to 0 and utf16Buffer set to NULL; the return value
		gives the required number of WCHAR code units, after which the caller can
		allocate the buffer and call the function again to perform the conversion.
		This function cannot fail.

	Parameters:
		utf8Length:
			Length of utf8Data in bytes (UTF-8 code units), not including any null
			terminator.

		utf8Data:
			Pointer to the non-null-terminated UTF-8 string to convert.

		utf16BufferLength:
			Capacity of utf16Buffer in WCHAR code units.  Pass 0 to query the
			required output size without writing any output.

		utf16Buffer:
			Pointer to the buffer that receives the converted UTF-16 string.
			The output is not null-terminated.  May be NULL when
			utf16BufferLength is 0.

	Return:
		Returns the number of WCHAR code units required to represent the full
		converted string.  If utf16BufferLength is less than this value, only
		the leading WCHAR code units that fit within the buffer are written.
*/

size_t FlConvertUtf16ToUtf8(_In_ size_t utf16Length, _In_reads_(utf16Length) const WCHAR* utf16Data, _In_ size_t utf8BufferLength, _Out_writes_to_(utf8BufferLength, return) char* utf8Buffer);
/*
	Function:
		FlConvertUtf16ToUtf8

	Description:
		Converts a UTF-16 encoded string to a UTF-8 encoded string.
		The output is not null-terminated.
		Invalid UTF-16 sequences, including unpaired surrogate code units, are
		replaced with the Unicode replacement character U+FFFD in the output.
		To determine the required output buffer size, call this function with
		utf8BufferLength set to 0 and utf8Buffer set to NULL; the return value
		gives the required number of bytes, after which the caller can allocate
		the buffer and call the function again to perform the conversion.
		This function cannot fail.

	Parameters:
		utf16Length:
			Length of utf16Data in WCHAR code units (UTF-16 code units), not
			including any null terminator.

		utf16Data:
			Pointer to the non-null-terminated UTF-16 string to convert.

		utf8BufferLength:
			Capacity of utf8Buffer in bytes.  Pass 0 to query the required
			output size without writing any output.

		utf8Buffer:
			Pointer to the buffer that receives the converted UTF-8 string.
			The output is not null-terminated.  May be NULL when
			utf8BufferLength is 0.

	Return:
		Returns the number of bytes required to represent the full converted
		string.  If utf8BufferLength is less than this value, only the leading
		bytes that fit within the buffer are written.
*/

size_t FlConvertUtf8ToUtf32(_In_ size_t utf8Length, _In_reads_(utf8Length) const char* utf8Data, _In_ size_t utf32BufferLength, _Out_writes_to_(utf32BufferLength, return) int* utf32Buffer);
/*
	Function:
		FlConvertUtf8ToUtf32

	Description:
		Converts a UTF-8 encoded string to a UTF-32 encoded string.
		Each element of the output array is a single Unicode code point stored
		as a 32-bit integer.
		The output is not null-terminated.
		Invalid UTF-8 byte sequences are replaced with the Unicode replacement
		character U+FFFD in the output.
		To determine the required output buffer size, call this function with
		utf32BufferLength set to 0 and utf32Buffer set to NULL; the return value
		gives the required number of int elements, after which the caller can
		allocate the buffer and call the function again to perform the conversion.
		This function cannot fail.

	Parameters:
		utf8Length:
			Length of utf8Data in bytes (UTF-8 code units), not including any null
			terminator.

		utf8Data:
			Pointer to the non-null-terminated UTF-8 string to convert.

		utf32BufferLength:
			Capacity of utf32Buffer in int elements (UTF-32 code units).  Pass 0
			to query the required output size without writing any output.

		utf32Buffer:
			Pointer to the buffer that receives the converted UTF-32 string.
			The output is not null-terminated.  May be NULL when
			utf32BufferLength is 0.

	Return:
		Returns the number of int elements required to represent the full
		converted string.  If utf32BufferLength is less than this value, only
		the leading elements that fit within the buffer are written.
*/

size_t FlConvertUtf32ToUtf8(_In_ size_t utf32Length, _In_reads_(utf32Length) const int* utf32Data, _In_ size_t utf8BufferLength, _Out_writes_to_(utf8BufferLength, return) char* utf8Buffer);
/*
	Function:
		FlConvertUtf32ToUtf8

	Description:
		Converts a UTF-32 encoded string to a UTF-8 encoded string.
		Each element of the input array is interpreted as a single Unicode code
		point stored as a 32-bit integer.
		The output is not null-terminated.
		Code points that cannot be represented in a 4-byte UTF-8 sequence
		(greater than 0x1FFFFF) are replaced with the Unicode replacement
		character U+FFFD in the output.  Code points in the range
		0x110000–0x1FFFFF lie outside the valid Unicode range but still fit
		in a 4-byte UTF-8 sequence; they are encoded as-is rather than
		replaced.
		To determine the required output buffer size, call this function with
		utf8BufferLength set to 0 and utf8Buffer set to NULL; the return value
		gives the required number of bytes, after which the caller can allocate
		the buffer and call the function again to perform the conversion.
		This function cannot fail.

	Parameters:
		utf32Length:
			Length of utf32Data in int elements (UTF-32 code units), not including
			any null terminator.

		utf32Data:
			Pointer to the non-null-terminated UTF-32 string to convert.

		utf8BufferLength:
			Capacity of utf8Buffer in bytes.  Pass 0 to query the required
			output size without writing any output.

		utf8Buffer:
			Pointer to the buffer that receives the converted UTF-8 string.
			The output is not null-terminated.  May be NULL when
			utf8BufferLength is 0.

	Return:
		Returns the number of bytes required to represent the full converted
		string.  If utf8BufferLength is less than this value, only the leading
		bytes that fit within the buffer are written.
*/

size_t FlConvertUtf32ToUtf16(_In_ size_t utf32Length, _In_reads_(utf32Length) const int* utf32Data, _In_ size_t utf16BufferLength, _Out_writes_to_(utf16BufferLength, return) WCHAR* utf16Buffer);
/*
	Function:
		FlConvertUtf32ToUtf16

	Description:
		Converts a UTF-32 encoded string to a UTF-16 encoded string.
		Each element of the input array is interpreted as a single Unicode code
		point stored as a 32-bit integer.
		Supplementary characters (code points greater than U+FFFF) are encoded
		as surrogate pairs and therefore occupy two WCHAR code units in the output.
		The output is not null-terminated.
		Code points outside the valid Unicode range (greater than 0x10FFFF) are
		replaced with the Unicode replacement character U+FFFD in the output.
		To determine the required output buffer size, call this function with
		utf16BufferLength set to 0 and utf16Buffer set to NULL; the return value
		gives the required number of WCHAR code units, after which the caller can
		allocate the buffer and call the function again to perform the conversion.
		This function cannot fail.

	Parameters:
		utf32Length:
			Length of utf32Data in int elements (UTF-32 code units), not including
			any null terminator.

		utf32Data:
			Pointer to the non-null-terminated UTF-32 string to convert.

		utf16BufferLength:
			Capacity of utf16Buffer in WCHAR code units.  Pass 0 to query the
			required output size without writing any output.

		utf16Buffer:
			Pointer to the buffer that receives the converted UTF-16 string.
			The output is not null-terminated.  May be NULL when
			utf16BufferLength is 0.

	Return:
		Returns the number of WCHAR code units required to represent the full
		converted string.  If utf16BufferLength is less than this value, only
		the leading WCHAR code units that fit within the buffer are written.
*/

size_t FlConvertUtf16ToUtf32(_In_ size_t utf16Length, _In_reads_(utf16Length) const WCHAR * utf16Data, _In_ size_t utf32BufferLength, _Out_writes_to_(utf32BufferLength, return) int* utf32Buffer);
/*
	Function:
		FlConvertUtf16ToUtf32

	Description:
		Converts a UTF-16 encoded string to a UTF-32 encoded string.
		Each element of the output array is a single Unicode code point stored
		as a 32-bit integer.
		Surrogate pairs in the input are decoded into a single code point in the
		output.
		The output is not null-terminated.
		Invalid UTF-16 sequences, including unpaired surrogate code units, are
		replaced with the Unicode replacement character U+FFFD in the output.
		To determine the required output buffer size, call this function with
		utf32BufferLength set to 0 and utf32Buffer set to NULL; the return value
		gives the required number of int elements, after which the caller can
		allocate the buffer and call the function again to perform the conversion.
		This function cannot fail.

	Parameters:
		utf16Length:
			Length of utf16Data in WCHAR code units (UTF-16 code units), not
			including any null terminator.

		utf16Data:
			Pointer to the non-null-terminated UTF-16 string to convert.

		utf32BufferLength:
			Capacity of utf32Buffer in int elements (UTF-32 code units).  Pass 0
			to query the required output size without writing any output.

		utf32Buffer:
			Pointer to the buffer that receives the converted UTF-32 string.
			The output is not null-terminated.  May be NULL when
			utf32BufferLength is 0.

	Return:
		Returns the number of int elements required to represent the full
		converted string.  If utf32BufferLength is less than this value, only
		the leading elements that fit within the buffer are written.
*/

int FlCodepointToUpperCase(_In_ int codepoint);
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
		codepoint:
			The Unicode code point to convert.
			Valid values are 0 through 0x10FFFF inclusive.
			Values outside this range are returned unchanged.

	Return:
		Returns the uppercase equivalent of codepoint, or codepoint itself if
		there is no uppercase mapping.
*/

int FlCodepointToLowerCase(_In_ int codepoint);
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
		codepoint:
			The Unicode code point to convert.
			Valid values are 0 through 0x10FFFF inclusive.
			Values outside this range are returned unchanged.

	Return:
		Returns the lowercase equivalent of codepoint, or codepoint itself if
		there is no lowercase mapping.
*/

int FlCompareStringOrdinalUtf8(_In_NLS_string_(string1Length) const char* string1, _In_ size_t string1Length, _In_NLS_string_(string2Length) const char* string2, _In_ size_t string2Length, _In_ BOOL ignoreCase);
/*
	Function:
		FlCompareStringOrdinalUtf8

	Description:
		Compares two UTF-8 strings strings lexicographically.
		This function is the UTF-8 equivalent of the Win32 API CompareStringOrdinal.

		When an input string is invalid the function can't order the strings,
		but if the input strings are not identical the return value will never be CSTR_LESS_THAN.

	Parameters:
		string1:
			Pointer to the first non-null-terminated UTF-8 string.
			If string1Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		string1Length:
			Length of string1 in bytes (UTF-8 code units), not including any null
			terminator.  Pass (size_t)-1 to indicate that string1 is null
			terminated.

		string2:
			Pointer to the second non-null-terminated UTF-8 string.
			If string2Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		string2Length:
			Length of string2 in bytes (UTF-8 code units), not including any null
			terminator.  Pass (size_t)-1 to indicate that string2 is null
			terminated.

		ignoreCase:
			If TRUE the function is to perform a case-insensitive comparison,
			using hard-coded locale invariant casing table information.

	Return:
		Returns one of the following constants:
		  CSTR_LESS_THAN    (1) if string1 appears before string2 in lexicographical order.
		  CSTR_EQUAL        (2) if string1 and string2 compare equal.
		  CSTR_GREATER_THAN (3) if string1 appears after string2 in lexicographical order.
		To maintain the C runtime convention of comparing strings,
		the value 2 can be subtracted from a return value. Then, the meaning of <0, ==0, and >0 is consistent with the C runtime.
*/

int FlCompareStringOrdinalUtf16(_In_NLS_string_(string1Length) const WCHAR* string1, _In_ size_t string1Length, _In_NLS_string_(string2Length) const WCHAR* string2, _In_ size_t string2Length, _In_ BOOL ignoreCase);
/*
	Function:
		FlCompareStringOrdinalUtf16

	Description:
		Compares two UTF-16 strings strings lexicographically.
		This function is the equivalent of the Win32 API CompareStringOrdinal.

		When an input string is invalid the function can't order the strings,
		but if the input strings are not identical the return value will never be CSTR_LESS_THAN.

	Parameters:
		string1:
			Pointer to the first non-null-terminated UTF-16 string.
			If string1Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		string1Length:
			Length of string1 in WCHARs (UTF-16 code units), not including any
			null terminator.  Pass (size_t)-1 to indicate that string1 is null
			terminated.

		string2:
			Pointer to the second non-null-terminated UTF-16 string.
			If string2Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		string2Length:
			Length of string2 in WCHARs (UTF-16 code units), not including any
			null terminator.  Pass (size_t)-1 to indicate that string2 is null
			terminated.

		ignoreCase:
			If TRUE the function is to perform a case-insensitive comparison,
			using hard-coded locale invariant casing table information.

	Return:
		Returns one of the following constants:
		  CSTR_LESS_THAN    (1) if string1 appears before string2 in lexicographical order.
		  CSTR_EQUAL        (2) if string1 and string2 compare equal.
		  CSTR_GREATER_THAN (3) if string1 appears after string2 in lexicographical order.
		To maintain the C runtime convention of comparing strings,
		the value 2 can be subtracted from a return value. Then, the meaning of <0, ==0, and >0 is consistent with the C runtime.
*/

int FlCompareStringOrdinalUtf32(_In_NLS_string_(string1Length) const int* string1, _In_ size_t string1Length, _In_NLS_string_(string2Length) const int* string2, _In_ size_t string2Length, _In_ BOOL ignoreCase);
/*
	Function:
		FlCompareStringOrdinalUtf32

	Description:
		Compares two UTF-32 strings strings lexicographically.
		This function is the UTF-32 equivalent of the Win32 API CompareStringOrdinal.

		When an input string is invalid the function can't order the strings,
		but if the input strings are not identical the return value will never be CSTR_LESS_THAN.

	Parameters:
		string1:
			Pointer to the first non-null-terminated UTF-32 string.
			If string1Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		string1Length:
			Length of string1 in bytes (UTF-32 code units), not including any null
			terminator.  Pass (size_t)-1 to indicate that string1 is null
			terminated.

		string2:
			Pointer to the second non-null-terminated UTF-32 string.
			If string2Length is (size_t)-1 the string must be null terminated and
			its length is computed automatically.

		string2Length:
			Length of string2 in bytes (UTF-32 code units), not including any null
			terminator.  Pass (size_t)-1 to indicate that string2 is null
			terminated.

		ignoreCase:
			If TRUE the function is to perform a case-insensitive comparison,
			using hard-coded locale invariant casing table information.

	Return:
		Returns one of the following constants:
		  CSTR_LESS_THAN    (1) if string1 appears before string2 in lexicographical order.
		  CSTR_EQUAL        (2) if string1 and string2 compare equal.
		  CSTR_GREATER_THAN (3) if string1 appears after string2 in lexicographical order.
		To maintain the C runtime convention of comparing strings,
		the value 2 can be subtracted from a return value. Then, the meaning of <0, ==0, and >0 is consistent with the C runtime.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_UNICODE_H
