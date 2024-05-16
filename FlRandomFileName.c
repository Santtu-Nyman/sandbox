/*
	Santtu S. Nyman's random file name generation library

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

#define WIN32_LEAN_AND_MEAN
#include "FlRandomFileName.h"
#include "FlRandom.h"

#define FL_INTERNAL_RANDOM_FILE_NAME_CHARACTER_COUNT 52

static const char FlInternalRandomFileNameCharacterTable[FL_INTERNAL_RANDOM_FILE_NAME_CHARACTER_COUNT] = {
	'0', '1',  '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B',  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L',  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V',  'W', 'X', 'Y', 'Z', '!', '#', '$', '%',
	'&', '\'', '(', ')', '-', '@', '^', '_', '`', '{',
	'}', '~' };

/*
uint32_t FlInternalBoundedRandom(uint32_t upper_bound, uint64_t random_bits)
{
	uint32_t r0 = (uint32_t)(random_bits & 0xFFFFFFFF);
	uint32_t r1 = (uint32_t)(random_bits >> 32);
	uint64_t r0_bits = (uint64_t)r0 * (uint64_t)upper_bound;
	uint64_t r1_bits = (uint64_t)r1 * (uint64_t)upper_bound;
	uint64_t sum_bits = (r0_bits & 0xFFFFFFFF) + (r1_bits >> 32);
	return (r0_bits >> 32) + (sum_bits >> 32);
}
*/

static char FlInternalGetRandomFileNameCharacter(WORD RandomBits)
{
	BYTE R0 = (BYTE)(RandomBits & 0xFF);
	BYTE R1 = (BYTE)(RandomBits >> 8);
	WORD R0Bits = (WORD)R0 * (WORD)FL_INTERNAL_RANDOM_FILE_NAME_CHARACTER_COUNT;
	WORD R1Bits = (WORD)R1 * (WORD)FL_INTERNAL_RANDOM_FILE_NAME_CHARACTER_COUNT;
	WORD SumBits = (R0Bits & 0xFF) + (R1Bits >> 8);
	BYTE RandomNumber = (BYTE)(R0Bits >> 8) + (BYTE)(SumBits >> 8);
	char RandomCharacter = FlInternalRandomFileNameCharacterTable[RandomNumber];
	return RandomCharacter;
}

size_t FlCreateRandomFileName(WCHAR* NameBuffer)
{
	WORD RandomBits[FL_RANDOM_FILE_NAME_LENGTH];
	FlGenerateRandomData(FL_RANDOM_FILE_NAME_LENGTH * sizeof(WORD), &RandomBits);
	for (size_t i = 0; i < FL_RANDOM_FILE_NAME_LENGTH; i++)
	{
		NameBuffer[i] = (WCHAR)FlInternalGetRandomFileNameCharacter(RandomBits[i]);
	}
	return FL_RANDOM_FILE_NAME_LENGTH;
}

size_t FlCreateRandomFileNameUtf8(char* NameBuffer)
{
	WORD RandomBits[FL_RANDOM_FILE_NAME_LENGTH];
	FlGenerateRandomData(FL_RANDOM_FILE_NAME_LENGTH * sizeof(WORD), &RandomBits);
	for (size_t i = 0; i < FL_RANDOM_FILE_NAME_LENGTH; i++)
	{
		NameBuffer[i] = (char)FlInternalGetRandomFileNameCharacter(RandomBits[i]);
	}
	return FL_RANDOM_FILE_NAME_LENGTH;
}

#ifdef __cplusplus
}
#endif // __cplusplus
