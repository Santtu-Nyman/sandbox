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
#include "FlUtf8Utf16Converter.h"

#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
#define FL_UTF8_UTF16_CONVERTER_LOAD_U16LE(P) (*((const uint16_t*)(P)))
#define FL_UTF8_UTF16_CONVERTER_STORE_U16LE(P,D) *((uint16_t*)(P)) = (uint16_t)(D)
#else
#define FL_UTF8_UTF16_CONVERTER_LOAD_U16LE(P) (((uint16_t)(*((const uint8_t*)(P)))) | ((uint16_t)(*((const uint8_t*)((uintptr_t)(P) + 1))) << 8))
#define FL_UTF8_UTF16_CONVERTER_STORE_U16LE(P,D) *((uint8_t*)(P)) = (uint8_t)((uint16_t)(D)); *((uint8_t*)((uintptr_t)(P) + 1)) = (uint8_t)((uint16_t)(D) >> 8)
#endif

#if defined(_MSC_VER)
#define FL_UTF8_UTF16_CONVERTER_UNREACHABLE() __assume(0)
#elif defined(__clang__)
#define FL_UTF8_UTF16_CONVERTER_UNREACHABLE() __builtin_unreachable()
#elif defined(__GNUC__) || defined(__GNUG__)
#define FL_UTF8_UTF16_CONVERTER_UNREACHABLE() __builtin_unreachable()
#else
#define FL_UTF8_UTF16_CONVERTER_UNREACHABLE() do {} while (0)
#endif

size_t FlConvertUtf8ToUtf16Le(_In_ size_t utf8Length, _In_reads_(utf8Length) const char* utf8Data, _In_ size_t utf16BufferLength, _Out_writes_to_(utf16BufferLength,return) WCHAR* utf16Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN WELL TESTED
	const uint8_t* utf8 = (const uint8_t*)utf8Data;
	uint16_t* utf16 = (uint16_t*)utf16Buffer;
	size_t utf16Length = 0;
	for (size_t utf8Index = 0; utf8Index != utf8Length;)
	{
		size_t codeUnitDataSizeLimit = utf8Length - utf8Index;
		uint32_t codeUnitData;
		if (codeUnitDataSizeLimit > 3)
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]) | (((uint32_t)utf8[utf8Index + 1]) << 8) | (((uint32_t)utf8[utf8Index + 2]) << 16) | (((uint32_t)utf8[utf8Index + 3]) << 24);
		}
		else if (codeUnitDataSizeLimit == 3)
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]) | (((uint32_t)utf8[utf8Index + 1]) << 8) | (((uint32_t)utf8[utf8Index + 2]) << 16);
		}
		else if (codeUnitDataSizeLimit == 2)
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]) | (((uint32_t)utf8[utf8Index + 1]) << 8);
		}
		else
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]);
		}
		int codeUnitByteCount;
		uint32_t utfCodePoint;
		if (!(codeUnitData & 0x80))
		{
			codeUnitByteCount = 1;
			utfCodePoint = codeUnitData & 0x7F;
		}
		else if ((codeUnitData & (uint32_t)0xC0E0) == (uint32_t)0x80C0)
		{
			codeUnitByteCount = 2;
			utfCodePoint = ((codeUnitData & 0x1F) << 6) | ((codeUnitData >> 8) & 0x3F);
		}
		else if ((codeUnitData & (uint32_t)0xC0C0F0) == (uint32_t)0x8080E0)
		{
			codeUnitByteCount = 3;
			utfCodePoint = ((codeUnitData & 0x0F) << 12) | (((codeUnitData >> 8) & 0x3F) << 6) | ((codeUnitData >> 16) & 0x3F);
		}
		else if ((codeUnitData & (uint32_t)0xC0C0C0F8) == (uint32_t)0x808080F0)
		{
			codeUnitByteCount = 4;
			utfCodePoint = ((codeUnitData & 0x0F) << 18) | (((codeUnitData >> 8) & 0x3F) << 12) | (((codeUnitData >> 16) & 0x3F) << 6) | ((codeUnitData >> 24) & 0x3F);
		}
		else
		{
			// The data is too corrupted to determine what is even supposed the code point data
			int maxCodeUnitByteCount;
			if ((codeUnitData & 0xE0) == 0xC0)
			{
				maxCodeUnitByteCount = 2;
			}
			else if ((codeUnitData & 0xF0) == 0xE0)
			{
				maxCodeUnitByteCount = 3;
			}
			else if ((codeUnitData & 0xF8) == 0xF0)
			{
				maxCodeUnitByteCount = 4;
			}
			else
			{
				maxCodeUnitByteCount = 1;
			}
			codeUnitByteCount = 1;
			while ((codeUnitByteCount < maxCodeUnitByteCount) && (((codeUnitData >> (codeUnitByteCount * 8)) & 0xC0) == 0x80))
			{
				codeUnitByteCount++;
			}
			utfCodePoint = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
		}
		utf8Index += (size_t)codeUnitByteCount;
		if (utfCodePoint > (uint32_t)0x10FFFF)
		{
			utfCodePoint = 0xFFFD;// Unknown, unrecognized, or (unrepresentable) character code point
		}
		if (utfCodePoint < (uint32_t)0x10000)
		{
			if (utf16Length + 1 <= utf16BufferLength)
			{
				FL_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16Length, (uint16_t)utfCodePoint);
			}
			utf16Length += 1;
		}
		else
		{
			if (utf16Length + 2 <= utf16BufferLength)
			{
				uint32_t temporal = utfCodePoint - (uint32_t)0x10000;
				uint16_t leadingUtf16Surrogate = (uint16_t)((temporal >> 10) | (uint32_t)0xD800);
				uint16_t trailingUtf16Surrogate = (uint16_t)((temporal & 0x3FF) | (uint32_t)0xDC00);
				FL_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16Length, leadingUtf16Surrogate);
				FL_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16Length + 1, trailingUtf16Surrogate);
			}
			utf16Length += 2;
		}
	}
	return utf16Length;
}

size_t FlConvertUtf16LeToUtf8(_In_ size_t utf16Length, _In_reads_(utf16Length) const WCHAR* utf16Data, _In_ size_t utf8BufferLength, _Out_writes_to_(utf8BufferLength,return) char* utf8Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN WELL TESTED
	const uint16_t* utf16 = (const uint16_t*)utf16Data;
	uint8_t* utf8 = (uint8_t*)utf8Buffer;
	size_t utf8Length = 0;
	for (size_t utf16Index = 0; utf16Index != utf16Length;)
	{
		uint16_t firstUtf16CodeUnit = FL_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16Index);
		utf16Index++;
		uint16_t secondUtf16CodeUnit = 0;
		if (((firstUtf16CodeUnit >> 10) == 0x36) && (utf16Index != utf16Length))
		{
			uint16_t possibleSecondUtf16CodeUnit = FL_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16Index);
			if ((possibleSecondUtf16CodeUnit >> 10) == 0x37)
			{
				secondUtf16CodeUnit = possibleSecondUtf16CodeUnit;
				utf16Index++;
			}
		}
		uint32_t utfCodePoint;
		if ((firstUtf16CodeUnit >> 11) != 0x1B)
		{
			utfCodePoint = (uint32_t)firstUtf16CodeUnit;
		}
		else
		{
			if (((firstUtf16CodeUnit >> 10) == 0x36) && ((secondUtf16CodeUnit >> 10) == 0x37))
			{
				utfCodePoint = ((((uint32_t)firstUtf16CodeUnit & 0x3FF) << 10) | ((uint32_t)secondUtf16CodeUnit & 0x3FF)) + (uint32_t)0x10000;
			}
			else
			{
				utfCodePoint = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
			}
		}
		// Encode utf code point as utf-8
		if (utfCodePoint < 0x80)
		{
			// Encode 7 bit code point as 1 byte
			if (utf8Length + 1 <= utf8BufferLength)
			{
				utf8[utf8Length] = (uint8_t)utfCodePoint;
			}
			utf8Length += 1;
		}
		else if (utfCodePoint < 0x800)
		{
			// Encode 13 bit code point as 2 bytes
			if (utf8Length + 2 <= utf8BufferLength)
			{
				utf8[utf8Length] = 0xC0 | (uint8_t)(utfCodePoint >> 6);
				utf8[utf8Length + 1] = 0x80 | (uint8_t)(utfCodePoint & 0x3F);
			}
			utf8Length += 2;
		}
		else if (utfCodePoint < 0x10000)
		{
			//  Encode 16 bit code point as 3 bytes
			if (utf8Length + 3 <= utf8BufferLength)
			{
				utf8[utf8Length] = 0xE0 | (uint8_t)(utfCodePoint >> 12);
				utf8[utf8Length + 1] = 0x80 | (uint8_t)((utfCodePoint >> 6) & 0x3F);
				utf8[utf8Length + 2] = 0x80 | (uint8_t)(utfCodePoint & 0x3F);
			}
			utf8Length += 3;
		}
		else
		{
			//  Encode 21 bit code point as 4 bytes. Any code point from utf-16 will fit
			if (utf8Length + 4 <= utf8BufferLength)
			{
				utf8[utf8Length] = 0xF0 | (uint8_t)(utfCodePoint >> 18);
				utf8[utf8Length + 1] = 0x80 | (uint8_t)((utfCodePoint >> 12) & 0x3F);
				utf8[utf8Length + 2] = 0x80 | (uint8_t)((utfCodePoint >> 6) & 0x3F);
				utf8[utf8Length + 3] = 0x80 | (uint8_t)(utfCodePoint & 0x3F);
			}
			utf8Length += 4;
		}
	}
	return utf8Length;
}

size_t FlConvertUtf8ToUtf16Be(_In_ size_t utf8Length, _In_reads_(utf8Length) const char* utf8Data, _In_ size_t utf16BufferLength, _Out_writes_to_(utf16BufferLength,return) WCHAR* utf16Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN MOSTLY COPIED FROM THE UTF-16BE VERSION THAT HAS BEEN WELL TESTED
	const uint8_t* utf8 = (const uint8_t*)utf8Data;
	uint16_t* utf16 = (uint16_t*)utf16Buffer;
	size_t utf16Length = 0;
	for (size_t utf8Index = 0; utf8Index != utf8Length;)
	{
		size_t codeUnitDataSizeLimit = utf8Length - utf8Index;
		uint32_t codeUnitData;
		if (codeUnitDataSizeLimit > 3)
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]) | (((uint32_t)utf8[utf8Index + 1]) << 8) | (((uint32_t)utf8[utf8Index + 2]) << 16) | (((uint32_t)utf8[utf8Index + 3]) << 24);
		}
		else if (codeUnitDataSizeLimit == 3)
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]) | (((uint32_t)utf8[utf8Index + 1]) << 8) | (((uint32_t)utf8[utf8Index + 2]) << 16);
		}
		else if (codeUnitDataSizeLimit == 2)
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]) | (((uint32_t)utf8[utf8Index + 1]) << 8);
		}
		else
		{
			codeUnitData = ((uint32_t)utf8[utf8Index]);
		}
		int codeUnitByteCount;
		uint32_t utfCodePoint;
		if (!(codeUnitData & 0x80))
		{
			codeUnitByteCount = 1;
			utfCodePoint = codeUnitData & 0x7F;
		}
		else if ((codeUnitData & (uint32_t)0xC0E0) == (uint32_t)0x80C0)
		{
			codeUnitByteCount = 2;
			utfCodePoint = ((codeUnitData & 0x1F) << 6) | ((codeUnitData >> 8) & 0x3F);
		}
		else if ((codeUnitData & (uint32_t)0xC0C0F0) == (uint32_t)0x8080E0)
		{
			codeUnitByteCount = 3;
			utfCodePoint = ((codeUnitData & 0x0F) << 12) | (((codeUnitData >> 8) & 0x3F) << 6) | ((codeUnitData >> 16) & 0x3F);
		}
		else if ((codeUnitData & (uint32_t)0xC0C0C0F8) == (uint32_t)0x808080F0)
		{
			codeUnitByteCount = 4;
			utfCodePoint = ((codeUnitData & 0x0F) << 18) | (((codeUnitData >> 8) & 0x3F) << 12) | (((codeUnitData >> 16) & 0x3F) << 6) | ((codeUnitData >> 24) & 0x3F);
		}
		else
		{
			// The data is too corrupted to determine what is even supposed the code point data
			int maxCodeUnitByteCount;
			if ((codeUnitData & 0xE0) == 0xC0)
			{
				maxCodeUnitByteCount = 2;
			}
			else if ((codeUnitData & 0xF0) == 0xE0)
			{
				maxCodeUnitByteCount = 3;
			}
			else if ((codeUnitData & 0xF8) == 0xF0)
			{
				maxCodeUnitByteCount = 4;
			}
			else
			{
				maxCodeUnitByteCount = 1;
			}
			codeUnitByteCount = 1;
			while ((codeUnitByteCount < maxCodeUnitByteCount) && (((codeUnitData >> (codeUnitByteCount * 8)) & 0xC0) == 0x80))
			{
				codeUnitByteCount++;
			}
			utfCodePoint = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
		}
		utf8Index += (size_t)codeUnitByteCount;
		if (utfCodePoint > (uint32_t)0x10FFFF)
		{
			utfCodePoint = 0xFFFD;// Unknown, unrecognized, or (unrepresentable) character code point
		}
		if (utfCodePoint < (uint32_t)0x10000)
		{
			if (utf16Length + 1 <= utf16BufferLength)
			{
				utfCodePoint = (utfCodePoint << 8) | (utfCodePoint >> 8);
				FL_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16Length, (uint16_t)utfCodePoint);
			}
			utf16Length += 1;
		}
		else
		{
			if (utf16Length + 2 <= utf16BufferLength)
			{
				uint32_t temporal = utfCodePoint - (uint32_t)0x10000;
				uint16_t leadingUtf16Surrogate = (uint16_t)((temporal >> 10) | (uint32_t)0xD800);
				uint16_t trailingUtf16Surrogate = (uint16_t)((temporal & 0x3FF) | (uint32_t)0xDC00);
				leadingUtf16Surrogate = (leadingUtf16Surrogate << 8) | (leadingUtf16Surrogate >> 8);
				FL_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16Length, leadingUtf16Surrogate);
				trailingUtf16Surrogate = (trailingUtf16Surrogate << 8) | (trailingUtf16Surrogate >> 8);
				FL_UTF8_UTF16_CONVERTER_STORE_U16LE(utf16 + utf16Length + 1, trailingUtf16Surrogate);
			}
			utf16Length += 2;
		}
	}
	return utf16Length;
}

size_t FlConvertUtf16BeToUtf8(_In_ size_t utf16Length, _In_reads_(utf16Length) const WCHAR* utf16Data, _In_ size_t utf8BufferLength, _Out_writes_to_(utf8BufferLength,return) char* utf8Buffer)
{
	// WARNING BE VERY CAREFUL WHEN MODIFYING THIS PROCEDURE! IT IS EXTREMELY FINICKY AND IT HAS BEEN MOSTLY COPIED FROM THE UTF-16BE VERSION THAT HAS BEEN WELL TESTED
	const uint16_t* utf16 = (const uint16_t*)utf16Data;
	uint8_t* utf8 = (uint8_t*)utf8Buffer;
	size_t utf8Length = 0;
	for (size_t utf16Index = 0; utf16Index != utf16Length;)
	{
		uint16_t firstUtf16CodeUnit = FL_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16Index);
		firstUtf16CodeUnit = (firstUtf16CodeUnit << 8) | (firstUtf16CodeUnit >> 8);
		utf16Index++;
		uint16_t secondUtf16CodeUnit = 0;
		if (((firstUtf16CodeUnit >> 10) == 0x36) && (utf16Index != utf16Length))
		{
			uint16_t possibleSecondUtf16CodeUnit = FL_UTF8_UTF16_CONVERTER_LOAD_U16LE(utf16 + utf16Index);
			possibleSecondUtf16CodeUnit = (possibleSecondUtf16CodeUnit << 8) | (possibleSecondUtf16CodeUnit >> 8);
			if ((possibleSecondUtf16CodeUnit >> 10) == 0x37)
			{
				secondUtf16CodeUnit = possibleSecondUtf16CodeUnit;
				utf16Index++;
			}
		}
		uint32_t utfCodePoint;
		if ((firstUtf16CodeUnit >> 11) != 0x1B)
		{
			utfCodePoint = (uint32_t)firstUtf16CodeUnit;
		}
		else
		{
			if (((firstUtf16CodeUnit >> 10) == 0x36) && ((secondUtf16CodeUnit >> 10) == 0x37))
			{
				utfCodePoint = ((((uint32_t)firstUtf16CodeUnit & 0x3FF) << 10) | ((uint32_t)secondUtf16CodeUnit & 0x3FF)) + (uint32_t)0x10000;
			}
			else
			{
				utfCodePoint = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
			}
		}
		// Encode utf code point as utf-8
		if (utfCodePoint < 0x80)
		{
			// Encode 7 bit code point as 1 byte
			if (utf8Length + 1 <= utf8BufferLength)
			{
				utf8[utf8Length] = (uint8_t)utfCodePoint;
			}
			utf8Length += 1;
		}
		else if (utfCodePoint < 0x800)
		{
			// Encode 13 bit code point as 2 bytes
			if (utf8Length + 2 <= utf8BufferLength)
			{
				utf8[utf8Length] = 0xC0 | (uint8_t)(utfCodePoint >> 6);
				utf8[utf8Length + 1] = 0x80 | (uint8_t)(utfCodePoint & 0x3F);
			}
			utf8Length += 2;
		}
		else if (utfCodePoint < 0x10000)
		{
			//  Encode 16 bit code point as 3 bytes
			if (utf8Length + 3 <= utf8BufferLength)
			{
				utf8[utf8Length] = 0xE0 | (uint8_t)(utfCodePoint >> 12);
				utf8[utf8Length + 1] = 0x80 | (uint8_t)((utfCodePoint >> 6) & 0x3F);
				utf8[utf8Length + 2] = 0x80 | (uint8_t)(utfCodePoint & 0x3F);
			}
			utf8Length += 3;
		}
		else
		{
			//  Encode 21 bit code point as 4 bytes. Any code point from utf-16 will fit
			if (utf8Length + 4 <= utf8BufferLength)
			{
				utf8[utf8Length] = 0xF0 | (uint8_t)(utfCodePoint >> 18);
				utf8[utf8Length + 1] = 0x80 | (uint8_t)((utfCodePoint >> 12) & 0x3F);
				utf8[utf8Length + 2] = 0x80 | (uint8_t)((utfCodePoint >> 6) & 0x3F);
				utf8[utf8Length + 3] = 0x80 | (uint8_t)(utfCodePoint & 0x3F);
			}
			utf8Length += 4;
		}
	}
	return utf8Length;
}

#ifdef __cplusplus
}
#endif // __cplusplus
