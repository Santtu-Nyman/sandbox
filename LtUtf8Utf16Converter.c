/*
	UTF-16 - UTF-8 conversion library version 1.0.0 2023-02-25 by Santtu S. Nyman.

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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define WIN32_LEAN_AND_MEAN
#include "LtUtf8Utf16Converter.h"

#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
#define LT_UTF8_UTF16_CONVERTER_LOAD_U16LE(P) (*((const uint16_t*)(P)))
#define LT_UTF8_UTF16_CONVERTER_STORE_U16LE(P,D) *((uint16_t*)(P)) = (uint16_t)(D)
#else
#define LT_UTF8_UTF16_CONVERTER_LOAD_U16LE(P) (((uint16_t)(*((const uint8_t*)(P)))) | ((uint16_t)(*((const uint8_t*)((uintptr_t)(P) + 1))) << 8))
#define LT_UTF8_UTF16_CONVERTER_STORE_U16LE(P,D) *((uint8_t*)(P)) = (uint8_t)((uint16_t)(D)); *((uint8_t*)((uintptr_t)(P) + 1)) = (uint8_t)((uint16_t)(D) >> 8)
#endif

#if defined(_MSC_VER)
#define LT_UTF8_UTF16_CONVERTER_UNREACHABLE() __assume(0)
#elif defined(__clang__)
#define LT_UTF8_UTF16_CONVERTER_UNREACHABLE() __builtin_unreachable()
#elif defined(__GNUC__) || defined(__GNUG__)
#define LT_UTF8_UTF16_CONVERTER_UNREACHABLE() __builtin_unreachable()
#else
#define LT_UTF8_UTF16_CONVERTER_UNREACHABLE() do {} while (0)
#endif

size_t LtConvertUtf8ToUtf16Le(size_t Utf8Length, const char* Utf8Data, size_t Utf16BufferLength, WCHAR* Utf16Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN WELL TESTED
	const uint8_t* utf8 = (const uint8_t*)Utf8Data;
	uint16_t* utf16 = (uint16_t*)Utf16Buffer;
	size_t utf16_length = 0;
	for (size_t utf8_index = 0; utf8_index != Utf8Length;)
	{
		size_t code_unit_data_size_limit = Utf8Length - utf8_index;
		uint32_t code_unit_data;
		if (code_unit_data_size_limit > 3)
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]) | (((uint32_t)utf8[utf8_index + 1]) << 8) | (((uint32_t)utf8[utf8_index + 2]) << 16) | (((uint32_t)utf8[utf8_index + 3]) << 24);
		}
		else if (code_unit_data_size_limit == 3)
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]) | (((uint32_t)utf8[utf8_index + 1]) << 8) | (((uint32_t)utf8[utf8_index + 2]) << 16);
		}
		else if (code_unit_data_size_limit == 2)
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]) | (((uint32_t)utf8[utf8_index + 1]) << 8);
		}
		else
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]);
		}
		int code_unit_byte_count;
		uint32_t utf_code_point;
		if (!(code_unit_data & 0x80))
		{
			code_unit_byte_count = 1;
			utf_code_point = code_unit_data & 0x7F;
		}
		else if ((code_unit_data & (uint32_t)0xC0E0) == (uint32_t)0x80C0)
		{
			code_unit_byte_count = 2;
			utf_code_point = ((code_unit_data & 0x1F) << 6) | ((code_unit_data >> 8) & 0x3F);
		}
		else if ((code_unit_data & (uint32_t)0xC0C0F0) == (uint32_t)0x8080E0)
		{
			code_unit_byte_count = 3;
			utf_code_point = ((code_unit_data & 0x0F) << 12) | (((code_unit_data >> 8) & 0x3F) << 6) | ((code_unit_data >> 16) & 0x3F);
		}
		else if ((code_unit_data & (uint32_t)0xC0C0C0F8) == (uint32_t)0x808080F0)
		{
			code_unit_byte_count = 4;
			utf_code_point = ((code_unit_data & 0x0F) << 18) | (((code_unit_data >> 8) & 0x3F) << 12) | (((code_unit_data >> 16) & 0x3F) << 6) | ((code_unit_data >> 24) & 0x3F);
		}
		else
		{
			// The data is too corrupted to determine what is even supposed the code point data
			int max_code_unit_byte_count;
			if ((code_unit_data & 0xE0) == 0xC0)
			{
				max_code_unit_byte_count = 2;
			}
			else if ((code_unit_data & 0xF0) == 0xE0)
			{
				max_code_unit_byte_count = 3;
			}
			else if ((code_unit_data & 0xF8) == 0xF0)
			{
				max_code_unit_byte_count = 4;
			}
			else
			{
				max_code_unit_byte_count = 1;
			}
			code_unit_byte_count = 1;
			while ((code_unit_byte_count < max_code_unit_byte_count) && (((code_unit_data >> (code_unit_byte_count * 8)) & 0xC0) == 0x80))
			{
				code_unit_byte_count++;
			}
			utf_code_point = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
		}
		utf8_index += (size_t)code_unit_byte_count;
		if (utf_code_point > (uint32_t)0x10FFFF)
		{
			utf_code_point = 0xFFFD;// Unknown, unrecognized, or (unrepresentable) character code point
		}
		if (utf_code_point < (uint32_t)0x10000)
		{
			if (utf16_length + 1 <= Utf16BufferLength)
			{
				LT_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16_length, (uint16_t)utf_code_point);
			}
			utf16_length += 1;
		}
		else
		{
			if (utf16_length + 2 <= Utf16BufferLength)
			{
				uint32_t temporal = utf_code_point - (uint32_t)0x10000;
				uint16_t leading_utf16_surrogate = (uint16_t)((temporal >> 10) | (uint32_t)0xD800);
				uint16_t trailing_utf16_surrogate = (uint16_t)((temporal & 0x3FF) | (uint32_t)0xDC00);
				LT_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16_length, leading_utf16_surrogate);
				LT_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16_length + 1, trailing_utf16_surrogate);
			}
			utf16_length += 2;
		}
	}
	return utf16_length;
}

size_t LtConvertUtf16LeToUtf8(size_t Utf16Length, const WCHAR* Utf16Data, size_t Utf8BufferLength, char* Utf8Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN WELL TESTED
	const uint16_t* utf16 = (const uint16_t*)Utf16Data;
	uint8_t* utf8 = (uint8_t*)Utf8Buffer;
	size_t utf8_length = 0;
	for (size_t utf16_index = 0; utf16_index != Utf16Length;)
	{
		uint16_t first_utf16_code_unit = LT_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16_index);
		utf16_index++;
		uint16_t second_utf16_code_unit = 0;
		if (((first_utf16_code_unit >> 10) == 0x36) && (utf16_index != Utf16Length))
		{
			uint16_t possible_second_utf16_code_unit = LT_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16_index);
			if ((possible_second_utf16_code_unit >> 10) == 0x37)
			{
				second_utf16_code_unit = possible_second_utf16_code_unit;
				utf16_index++;
			}
		}
		uint32_t utf_code_point;
		if ((first_utf16_code_unit >> 11) != 0x1B)
		{
			utf_code_point = (uint32_t)first_utf16_code_unit;
		}
		else
		{
			if (((first_utf16_code_unit >> 10) == 0x36) && ((second_utf16_code_unit >> 10) == 0x37))
			{
				utf_code_point = ((((uint32_t)first_utf16_code_unit & 0x3FF) << 10) | ((uint32_t)second_utf16_code_unit & 0x3FF)) + (uint32_t)0x10000;
			}
			else
			{
				utf_code_point = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
			}
		}
		// Encode utf code point as utf-8
		if (utf_code_point < 0x80)
		{
			// Encode 7 bit code point as 1 byte
			if (utf8_length + 1 <= Utf8BufferLength)
			{
				utf8[utf8_length] = (uint8_t)utf_code_point;
			}
			utf8_length += 1;
		}
		else if (utf_code_point < 0x800)
		{
			// Encode 13 bit code point as 2 bytes
			if (utf8_length + 2 <= Utf8BufferLength)
			{
				utf8[utf8_length] = 0xC0 | (uint8_t)(utf_code_point >> 6);
				utf8[utf8_length + 1] = 0x80 | (uint8_t)(utf_code_point & 0x3F);
			}
			utf8_length += 2;
		}
		else if (utf_code_point < 0x10000)
		{
			//  Encode 16 bit code point as 3 bytes
			if (utf8_length + 3 <= Utf8BufferLength)
			{
				utf8[utf8_length] = 0xE0 | (uint8_t)(utf_code_point >> 12);
				utf8[utf8_length + 1] = 0x80 | (uint8_t)((utf_code_point >> 6) & 0x3F);
				utf8[utf8_length + 2] = 0x80 | (uint8_t)(utf_code_point & 0x3F);
			}
			utf8_length += 3;
		}
		else
		{
			//  Encode 21 bit code point as 4 bytes. Any code point from utf-16 will fit
			if (utf8_length + 4 <= Utf8BufferLength)
			{
				utf8[utf8_length] = 0xF0 | (uint8_t)(utf_code_point >> 18);
				utf8[utf8_length + 1] = 0x80 | (uint8_t)((utf_code_point >> 12) & 0x3F);
				utf8[utf8_length + 2] = 0x80 | (uint8_t)((utf_code_point >> 6) & 0x3F);
				utf8[utf8_length + 3] = 0x80 | (uint8_t)(utf_code_point & 0x3F);
			}
			utf8_length += 4;
		}
	}
	return utf8_length;
}

size_t LtConvertUtf8ToUtf16Be(size_t Utf8Length, const char* Utf8Data, size_t Utf16BufferLength, WCHAR* Utf16Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN MOSTLY COPIED FROM THE UTF-16BE VERSION THAT HAS BEEN WELL TESTED
	const uint8_t* utf8 = (const uint8_t*)Utf8Data;
	uint16_t* utf16 = (uint16_t*)Utf16Buffer;
	size_t utf16_length = 0;
	for (size_t utf8_index = 0; utf8_index != Utf8Length;)
	{
		size_t code_unit_data_size_limit = Utf8Length - utf8_index;
		uint32_t code_unit_data;
		if (code_unit_data_size_limit > 3)
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]) | (((uint32_t)utf8[utf8_index + 1]) << 8) | (((uint32_t)utf8[utf8_index + 2]) << 16) | (((uint32_t)utf8[utf8_index + 3]) << 24);
		}
		else if (code_unit_data_size_limit == 3)
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]) | (((uint32_t)utf8[utf8_index + 1]) << 8) | (((uint32_t)utf8[utf8_index + 2]) << 16);
		}
		else if (code_unit_data_size_limit == 2)
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]) | (((uint32_t)utf8[utf8_index + 1]) << 8);
		}
		else
		{
			code_unit_data = ((uint32_t)utf8[utf8_index]);
		}
		int code_unit_byte_count;
		uint32_t utf_code_point;
		if (!(code_unit_data & 0x80))
		{
			code_unit_byte_count = 1;
			utf_code_point = code_unit_data & 0x7F;
		}
		else if ((code_unit_data & (uint32_t)0xC0E0) == (uint32_t)0x80C0)
		{
			code_unit_byte_count = 2;
			utf_code_point = ((code_unit_data & 0x1F) << 6) | ((code_unit_data >> 8) & 0x3F);
		}
		else if ((code_unit_data & (uint32_t)0xC0C0F0) == (uint32_t)0x8080E0)
		{
			code_unit_byte_count = 3;
			utf_code_point = ((code_unit_data & 0x0F) << 12) | (((code_unit_data >> 8) & 0x3F) << 6) | ((code_unit_data >> 16) & 0x3F);
		}
		else if ((code_unit_data & (uint32_t)0xC0C0C0F8) == (uint32_t)0x808080F0)
		{
			code_unit_byte_count = 4;
			utf_code_point = ((code_unit_data & 0x0F) << 18) | (((code_unit_data >> 8) & 0x3F) << 12) | (((code_unit_data >> 16) & 0x3F) << 6) | ((code_unit_data >> 24) & 0x3F);
		}
		else
		{
			// The data is too corrupted to determine what is even supposed the code point data
			int max_code_unit_byte_count;
			if ((code_unit_data & 0xE0) == 0xC0)
			{
				max_code_unit_byte_count = 2;
			}
			else if ((code_unit_data & 0xF0) == 0xE0)
			{
				max_code_unit_byte_count = 3;
			}
			else if ((code_unit_data & 0xF8) == 0xF0)
			{
				max_code_unit_byte_count = 4;
			}
			else
			{
				max_code_unit_byte_count = 1;
			}
			code_unit_byte_count = 1;
			while ((code_unit_byte_count < max_code_unit_byte_count) && (((code_unit_data >> (code_unit_byte_count * 8)) & 0xC0) == 0x80))
			{
				code_unit_byte_count++;
			}
			utf_code_point = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
		}
		utf8_index += (size_t)code_unit_byte_count;
		if (utf_code_point > (uint32_t)0x10FFFF)
		{
			utf_code_point = 0xFFFD;// Unknown, unrecognized, or (unrepresentable) character code point
		}
		if (utf_code_point < (uint32_t)0x10000)
		{
			if (utf16_length + 1 <= Utf16BufferLength)
			{
				utf_code_point = (utf_code_point << 8) | (utf_code_point >> 8);
				LT_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16_length, (uint16_t)utf_code_point);
			}
			utf16_length += 1;
		}
		else
		{
			if (utf16_length + 2 <= Utf16BufferLength)
			{
				uint32_t temporal = utf_code_point - (uint32_t)0x10000;
				uint16_t leading_utf16_surrogate = (uint16_t)((temporal >> 10) | (uint32_t)0xD800);
				uint16_t trailing_utf16_surrogate = (uint16_t)((temporal & 0x3FF) | (uint32_t)0xDC00);
				leading_utf16_surrogate = (leading_utf16_surrogate << 8) | (leading_utf16_surrogate >> 8);
				LT_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16_length, leading_utf16_surrogate);
				trailing_utf16_surrogate = (trailing_utf16_surrogate << 8) | (trailing_utf16_surrogate >> 8);
				LT_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16_length + 1, trailing_utf16_surrogate);
			}
			utf16_length += 2;
		}
	}
	return utf16_length;
}

size_t LtConvertUtf16BeToUtf8(size_t Utf16Length, const WCHAR* Utf16Data, size_t Utf8BufferLength, char* Utf8Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN MOSTLY COPIED FROM THE UTF-16BE VERSION THAT HAS BEEN WELL TESTED
	const uint16_t* utf16 = (const uint16_t*)Utf16Data;
	uint8_t* utf8 = (uint8_t*)Utf8Buffer;
	size_t utf8_length = 0;
	for (size_t utf16_index = 0; utf16_index != Utf16Length;)
	{
		uint16_t first_utf16_code_unit = LT_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16_index);
		first_utf16_code_unit = (first_utf16_code_unit << 8) | (first_utf16_code_unit >> 8);
		utf16_index++;
		uint16_t second_utf16_code_unit = 0;
		if (((first_utf16_code_unit >> 10) == 0x36) && (utf16_index != Utf16Length))
		{
			uint16_t possible_second_utf16_code_unit = LT_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16_index);
			possible_second_utf16_code_unit = (possible_second_utf16_code_unit << 8) | (possible_second_utf16_code_unit >> 8);
			if ((possible_second_utf16_code_unit >> 10) == 0x37)
			{
				second_utf16_code_unit = possible_second_utf16_code_unit;
				utf16_index++;
			}
		}
		uint32_t utf_code_point;
		if ((first_utf16_code_unit >> 11) != 0x1B)
		{
			utf_code_point = (uint32_t)first_utf16_code_unit;
		}
		else
		{
			if (((first_utf16_code_unit >> 10) == 0x36) && ((second_utf16_code_unit >> 10) == 0x37))
			{
				utf_code_point = ((((uint32_t)first_utf16_code_unit & 0x3FF) << 10) | ((uint32_t)second_utf16_code_unit & 0x3FF)) + (uint32_t)0x10000;
			}
			else
			{
				utf_code_point = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
			}
		}
		// Encode utf code point as utf-8
		if (utf_code_point < 0x80)
		{
			// Encode 7 bit code point as 1 byte
			if (utf8_length + 1 <= Utf8BufferLength)
			{
				utf8[utf8_length] = (uint8_t)utf_code_point;
			}
			utf8_length += 1;
		}
		else if (utf_code_point < 0x800)
		{
			// Encode 13 bit code point as 2 bytes
			if (utf8_length + 2 <= Utf8BufferLength)
			{
				utf8[utf8_length] = 0xC0 | (uint8_t)(utf_code_point >> 6);
				utf8[utf8_length + 1] = 0x80 | (uint8_t)(utf_code_point & 0x3F);
			}
			utf8_length += 2;
		}
		else if (utf_code_point < 0x10000)
		{
			//  Encode 16 bit code point as 3 bytes
			if (utf8_length + 3 <= Utf8BufferLength)
			{
				utf8[utf8_length] = 0xE0 | (uint8_t)(utf_code_point >> 12);
				utf8[utf8_length + 1] = 0x80 | (uint8_t)((utf_code_point >> 6) & 0x3F);
				utf8[utf8_length + 2] = 0x80 | (uint8_t)(utf_code_point & 0x3F);
			}
			utf8_length += 3;
		}
		else
		{
			//  Encode 21 bit code point as 4 bytes. Any code point from utf-16 will fit
			if (utf8_length + 4 <= Utf8BufferLength)
			{
				utf8[utf8_length] = 0xF0 | (uint8_t)(utf_code_point >> 18);
				utf8[utf8_length + 1] = 0x80 | (uint8_t)((utf_code_point >> 12) & 0x3F);
				utf8[utf8_length + 2] = 0x80 | (uint8_t)((utf_code_point >> 6) & 0x3F);
				utf8[utf8_length + 3] = 0x80 | (uint8_t)(utf_code_point & 0x3F);
			}
			utf8_length += 4;
		}
	}
	return utf8_length;
}

#ifdef __cplusplus
}
#endif // __cplusplus
