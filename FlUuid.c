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

void FlUuidCreateRandomId(void* Uuid)
{
	uint8_t* RandomIdBytes = (uint8_t*)Uuid;
	FlGenerateRandomData(16, RandomIdBytes);
	RandomIdBytes[7] = (RandomIdBytes[7] & 0x0F) | 0x40;
	RandomIdBytes[8] = (RandomIdBytes[8] & 0x3F) | 0x80;
}

size_t FlUuidEncodeStringUtf8(const void* Uuid, char* Buffer)
{
	// The length is always 38, so having a length parameter is pointless
	*Buffer++ = '{';
	for (int i = 0; i < 16; i++)
	{
		if (i > 3 && i < 11 && !(i & 1))
		{
			*Buffer++ = '-';
		}
		int ByteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		uint8_t Byte = *(((const uint8_t*)Uuid) + ByteIndex);
		uint8_t HighNibble = Byte >> 4;
		uint8_t LowNibble = Byte & 0xF;
		*Buffer++ = (char)((HighNibble < 0xA) ? (HighNibble | 0x30) : (HighNibble + 0x37));
		*Buffer++ = (char)((LowNibble < 0xA) ? (LowNibble | 0x30) : (LowNibble + 0x37));
	}
	*Buffer++ = '}';
	return 38;
}

size_t FlUuidDecodeStringUtf8(void* Uuid, size_t Length, const char* String)
{
	// The actual UUID length is 38 or 36 depending on whether it has brackets or not
	size_t DecodedLength;
	if (Length >= 38 && String[0] == '{' && String[37] == '}')
	{
		DecodedLength = 38;
		Length--;
		String++;
	}
	else if (Length >= 36)
	{
		DecodedLength = 36;
	}
	else
	{
		return 0;
	}
	for (int i = 0; i < 16; i++)
	{
		char Character = *String++;
		if (i > 3 && i < 11 && !(i & 1))
		{
			if (Character != '-')
			{
				return 0;
			}
			Character = *String++;
		}
		uint8_t HighNibble;
		if (Character >= '0' && Character <= '9')
		{
			HighNibble = (uint8_t)Character & 0xF;
		}
		else if (Character >= 'A' && Character <= 'F')
		{
			HighNibble = (uint8_t)Character - 0x37;
		}
		else if (Character >= 'a' && Character <= 'f')
		{
			HighNibble = (uint8_t)Character - 0x57;
		}
		else
		{
			return 0;
		}
		Character = *String++;
		uint8_t LowNibble;
		if (Character >= '0' && Character <= '9')
		{
			LowNibble = (uint8_t)Character & 0xF;
		}
		else if (Character >= 'A' && Character <= 'F')
		{
			LowNibble = (uint8_t)Character - 0x37;
		}
		else if (Character >= 'a' && Character <= 'f')
		{
			LowNibble = (uint8_t)Character - 0x57;
		}
		else
		{
			return 0;
		}
		uint8_t Byte = (HighNibble << 4) | LowNibble;
		int ByteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		*(((uint8_t*)Uuid) + ByteIndex) = Byte;
	}
	return DecodedLength;
}

size_t FlUuidEncodeStringUtf16(const void* Uuid, WCHAR* Buffer)
{
	// The length is always 38, so having a length parameter is pointless
	*Buffer++ = L'{';
	for (int i = 0; i < 16; i++)
	{
		if (i > 3 && i < 11 && !(i & 1))
		{
			*Buffer++ = L'-';
		}
		int ByteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		uint8_t Byte = *(((const uint8_t*)Uuid) + ByteIndex);
		uint8_t HighNibble = Byte >> 4;
		uint8_t LowNibble = Byte & 0xF;
		*Buffer++ = (WCHAR)((HighNibble < 0xA) ? (HighNibble | 0x30) : (HighNibble + 0x37));
		*Buffer++ = (WCHAR)((LowNibble < 0xA) ? (LowNibble | 0x30) : (LowNibble + 0x37));
	}
	*Buffer++ = L'}';
	return 38;
}

size_t FlUuidDecodeStringUtf16(void* Uuid, size_t Length, const WCHAR* String)
{
	// The actual UUID length is 38 or 36 depending on whether it has brackets or not
	size_t DecodedLength;
	if (Length >= 38 && String[0] == L'{' && String[37] == L'}')
	{
		DecodedLength = 38;
		Length--;
		String++;
	}
	else if (Length >= 36)
	{
		DecodedLength = 36;
	}
	else
	{
		return 0;
	}
	for (int i = 0; i < 16; i++)
	{
		WCHAR Character = *String++;
		if (i > 3 && i < 11 && !(i & 1))
		{
			if (Character != L'-')
			{
				return 0;
			}
			Character = *String++;
		}
		uint8_t HighNibble;
		if (Character >= L'0' && Character <= L'9')
		{
			HighNibble = (uint8_t)Character & 0xF;
		}
		else if (Character >= L'A' && Character <= L'F')
		{
			HighNibble = (uint8_t)Character - 0x37;
		}
		else if (Character >= L'a' && Character <= L'f')
		{
			HighNibble = (uint8_t)Character - 0x57;
		}
		else
		{
			return 0;
		}
		Character = *String++;
		uint8_t LowNibble;
		if (Character >= L'0' && Character <= L'9')
		{
			LowNibble = (uint8_t)Character & 0xF;
		}
		else if (Character >= L'A' && Character <= L'F')
		{
			LowNibble = (uint8_t)Character - 0x37;
		}
		else if (Character >= L'a' && Character <= L'f')
		{
			LowNibble = (uint8_t)Character - 0x57;
		}
		else
		{
			return 0;
		}
		uint8_t Byte = (HighNibble << 4) | LowNibble;
		int ByteIndex = (i < 8) ? (int)((0x67450123lu >> (i << 2)) & 0x7) : i;
		*(((uint8_t*)Uuid) + ByteIndex) = Byte;
	}
	return DecodedLength;
}

#ifdef __cplusplus
}
#endif // __cplusplus
