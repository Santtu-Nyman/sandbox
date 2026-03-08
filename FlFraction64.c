/*
	Santtu S. Nyman's 2026 public domain 64 bit fraction encode and decode procedures.

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
*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "FlFraction64.h"
#include <assert.h>

#if defined(__x86_64__) || defined(_M_X64)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <immintrin.h>
#endif
#endif

uint64_t FlDecodeFraction64(size_t length, const char* text)
{
	if (length > 37)
	{
		length = 37;
	}
	uint64_t fractionHigh = 0x1999999999999999ull;
	uint64_t fractionLow = 0x04CCCCCCCCCCCCCCull;
	uint64_t high = 0;
	uint64_t low = 0;
	for (int i = 0; i < (int)length; i++)
	{
		assert(text[i] >= '0' && text[i] <= '9');
		int digit = (int)text[i] ^ 0x30;
		uint64_t digitHighValue = (uint64_t)digit * fractionHigh;
		uint64_t digitLowValue = (uint64_t)digit * fractionLow;
		fractionLow = (((fractionHigh % 10) << 59) + fractionLow) / 10;
		fractionHigh /= 10;
		low += digitLowValue;
		uint64_t carry = low >> 59;
		low &= 0x07FFFFFFFFFFFFFFull;
		high += digitHighValue + carry;
	}
	if (low & 0x0400000000000000ull)
	{
		high++;
	}
	return high;
}

size_t FlEncodeFraction64(uint64_t value, int precision, char* text)
{
	assert(precision >= 0 && precision <= FL_FRACTION_64_FULL_PRECISION);
#ifdef _MSC_VER
	__assume(precision >= 0 && precision <= FL_FRACTION_64_FULL_PRECISION);
#else
	if (precision < 0 || precision > FL_FRACTION_64_FULL_PRECISION)
	{
		__builtin_unreachable();
	}
#endif
	int digitCount = 0;
	if (precision)
	{
		do
		{
			uint64_t valueLow = value << 1;
			uint64_t valueHigh = value << 3;
#if defined(__x86_64__) || defined(_M_X64)
			uint64_t nextValue;
			int nextValueCarry = (int)_addcarry_u64(0, valueLow, valueHigh, &nextValue);
#else
			uint64_t nextValue = valueLow + valueHigh;
			int nextValueCarry = (int)(((valueLow >> 1) + (valueHigh >> 1)) >> 63);
#endif
			int digit = (int)((value >> 63) + (value >> 61)) + nextValueCarry;
			text[digitCount] = (char)(digit | 0x30);
			value = nextValue;
			digitCount++;
		} while (value && digitCount != precision);
	}
	int carry = 0;
	if (digitCount == precision)
	{
		uint64_t valueLow = value << 1;
		uint64_t valueHigh = value << 3;
#if defined(__x86_64__) || defined(_M_X64)
		uint64_t nextValue;
		int nextValueCarry = (int)_addcarry_u64(0, valueLow, valueHigh, &nextValue);
#else
		int nextValueCarry = (int)(((valueLow >> 1) + (valueHigh >> 1)) >> 63);
#endif
		int digit = (int)((value >> 63) + (value >> 61)) + nextValueCarry;
		if (digit > 4)
		{
			carry = 1;
			while (carry && digitCount)
			{
				char textDigit = text[digitCount - 1];
				if (textDigit != 0x39)
				{
					text[digitCount - 1] = textDigit + 1;
					carry = 0;
				}
				else
				{
					digitCount--;
				}
			}
		}
		else
		{
			while (digitCount > 1 && text[digitCount - 1] == 0x30)
			{
				digitCount--;
			}
		}
	}
	return !carry ? (size_t)digitCount : FL_FRACTION_64_ROUND_TO_ONE;
}

#ifdef __cplusplus
}
#endif // __cplusplus
