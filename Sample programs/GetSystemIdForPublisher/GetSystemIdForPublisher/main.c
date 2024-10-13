/*
	GetSystemIdForPublisher procedure from Windows ABI Windows.System.Profile.SystemIdentification class example by Santtu S. Nyman.

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

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "GetSystemIdForPublisher.h"

int main(int argc, char** argv)
{
	size_t SystemIdLength = 0;
	HRESULT Result = GetSystemIdForPublisher(0, &SystemIdLength, 0);
	if (Result == S_OK || Result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		uint8_t* SystemId = (uint8_t*)HeapAlloc(GetProcessHeap(), 0, SystemIdLength ? SystemIdLength : 1);
		Result = GetSystemIdForPublisher(SystemIdLength, &SystemIdLength, SystemId);
		if (Result == S_OK)
		{
			printf("GetSystemIdForPublisher -> ");
			for (size_t i = 0; i < SystemIdLength; i++)
			{
				printf("%02X", SystemId[i]);
			}
			printf("\n");
		}
		HeapFree(GetProcessHeap(), 0, SystemId);
	}
	if (Result != S_OK)
	{
		printf("GetSystemIdForPublisher failed with error 0x%08X\n", Result);
	}
	return EXIT_SUCCESS;
}
