/*
	Santtu S. Nyman's 2024 public domain UUID utilities.

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
#include "FlUuid.h"
#include "FlRandom.h"
#include <stdint.h>

void FlUuidCreateRandomId(_Out_writes_bytes_all_(FL_UUID_SIZE) void* uuid)
{
	uint8_t* randomIdBytes = (uint8_t*)uuid;
	FlGenerateRandomData(16, randomIdBytes);
	randomIdBytes[7] = (randomIdBytes[7] & 0x0F) | 0x40;
	randomIdBytes[8] = (randomIdBytes[8] & 0x3F) | 0x80;
}

size_t FlUuidEncodeStringUtf8(_In_reads_bytes_(FL_UUID_SIZE) const void* uuid, _Out_writes_to_(38,return) char* buffer)
{
	// The length is always 38, so having a length parameter is pointless
	*buffer++ = '{';
	for (int i = 0; i < 16; i++)
	{
		if (i > 3 && i < 11 && !(i & 1))
		{
			*buffer++ = '-';
		}
		int byteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		uint8_t byte = *(((const uint8_t*)uuid) + byteIndex);
		uint8_t highNibble = byte >> 4;
		uint8_t lowNibble = byte & 0xF;
		*buffer++ = (char)((highNibble < 0xA) ? (highNibble | 0x30) : (highNibble + 0x37));
		*buffer++ = (char)((lowNibble < 0xA) ? (lowNibble | 0x30) : (lowNibble + 0x37));
	}
	*buffer++ = '}';
	return 38;
}

size_t FlUuidDecodeStringUtf8(_Out_writes_bytes_all_(FL_UUID_SIZE) void* uuid, _In_ size_t length, _In_reads_(length) const char* string)
{
	// The actual UUID length is 38 or 36 depending on whether it has brackets or not
	size_t decodedLength;
	if (length >= 38 && string[0] == '{' && string[37] == '}')
	{
		decodedLength = 38;
		length--;
		string++;
	}
	else if (length >= 36)
	{
		decodedLength = 36;
	}
	else
	{
		return 0;
	}
	for (int i = 0; i < 16; i++)
	{
		char character = *string++;
		if (i > 3 && i < 11 && !(i & 1))
		{
			if (character != '-')
			{
				return 0;
			}
			character = *string++;
		}
		uint8_t highNibble;
		if (character >= '0' && character <= '9')
		{
			highNibble = (uint8_t)character & 0xF;
		}
		else if (character >= 'A' && character <= 'F')
		{
			highNibble = (uint8_t)character - 0x37;
		}
		else if (character >= 'a' && character <= 'f')
		{
			highNibble = (uint8_t)character - 0x57;
		}
		else
		{
			return 0;
		}
		character = *string++;
		uint8_t lowNibble;
		if (character >= '0' && character <= '9')
		{
			lowNibble = (uint8_t)character & 0xF;
		}
		else if (character >= 'A' && character <= 'F')
		{
			lowNibble = (uint8_t)character - 0x37;
		}
		else if (character >= 'a' && character <= 'f')
		{
			lowNibble = (uint8_t)character - 0x57;
		}
		else
		{
			return 0;
		}
		uint8_t byte = (highNibble << 4) | lowNibble;
		int byteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		*(((uint8_t*)uuid) + byteIndex) = byte;
	}
	return decodedLength;
}

size_t FlUuidEncodeStringUtf16(_In_reads_bytes_(FL_UUID_SIZE) const void* uuid, _Out_writes_to_(38,return) WCHAR* buffer)
{
	// The length is always 38, so having a length parameter is pointless
	*buffer++ = L'{';
	for (int i = 0; i < 16; i++)
	{
		if (i > 3 && i < 11 && !(i & 1))
		{
			*buffer++ = L'-';
		}
		int byteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		uint8_t byte = *(((const uint8_t*)uuid) + byteIndex);
		uint8_t highNibble = byte >> 4;
		uint8_t lowNibble = byte & 0xF;
		*buffer++ = (WCHAR)((highNibble < 0xA) ? (highNibble | 0x30) : (highNibble + 0x37));
		*buffer++ = (WCHAR)((lowNibble < 0xA) ? (lowNibble | 0x30) : (lowNibble + 0x37));
	}
	*buffer++ = L'}';
	return 38;
}

size_t FlUuidDecodeStringUtf16(_Out_writes_bytes_all_(FL_UUID_SIZE) void* uuid, _In_ size_t length, _In_reads_(length) const WCHAR* string)
{
	// The actual UUID length is 38 or 36 depending on whether it has brackets or not
	size_t decodedLength;
	if (length >= 38 && string[0] == L'{' && string[37] == L'}')
	{
		decodedLength = 38;
		length--;
		string++;
	}
	else if (length >= 36)
	{
		decodedLength = 36;
	}
	else
	{
		return 0;
	}
	for (int i = 0; i < 16; i++)
	{
		WCHAR character = *string++;
		if (i > 3 && i < 11 && !(i & 1))
		{
			if (character != L'-')
			{
				return 0;
			}
			character = *string++;
		}
		uint8_t highNibble;
		if (character >= L'0' && character <= L'9')
		{
			highNibble = (uint8_t)character & 0xF;
		}
		else if (character >= L'A' && character <= L'F')
		{
			highNibble = (uint8_t)character - 0x37;
		}
		else if (character >= L'a' && character <= L'f')
		{
			highNibble = (uint8_t)character - 0x57;
		}
		else
		{
			return 0;
		}
		character = *string++;
		uint8_t lowNibble;
		if (character >= L'0' && character <= L'9')
		{
			lowNibble = (uint8_t)character & 0xF;
		}
		else if (character >= L'A' && character <= L'F')
		{
			lowNibble = (uint8_t)character - 0x37;
		}
		else if (character >= L'a' && character <= L'f')
		{
			lowNibble = (uint8_t)character - 0x57;
		}
		else
		{
			return 0;
		}
		uint8_t byte = (highNibble << 4) | lowNibble;
		int byteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		*(((uint8_t*)uuid) + byteIndex) = byte;
	}
	return decodedLength;
}

#ifdef __cplusplus
}
#endif // __cplusplus
