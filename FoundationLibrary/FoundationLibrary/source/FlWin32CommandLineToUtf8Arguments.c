/*
	UTF-8 version for Win32 function CommandLineToArgvW by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/sandbox

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
#endif

#define WIN32_LEAN_AND_MEAN
#include "FlWin32CommandLineToUtf8Arguments.h"
#include "FlUtf8Utf16Converter.h"

DWORD FlWin32CommandLineToUtf8Arguments(_In_ _Null_terminated_ const WCHAR* nativeCommand, _Out_ size_t* argumentCountAddress, _Out_ char*** argumentTableAddress)
{
	const size_t maxFileNameLength = UNICODE_STRING_MAX_CHARS;

	SYSTEM_INFO systemInfo;
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&systemInfo);
	size_t pageSize = (size_t)systemInfo.dwPageSize;
	if (!pageSize)
	{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
		pageSize = 0x1000;
#else
		pageSize = 0x10000;
#endif
	}

	size_t nativeCommandLength = 0;
	if (nativeCommand)
	{
		while (nativeCommand[nativeCommandLength])
		{
			nativeCommandLength++;
		}
	}

	if (!nativeCommandLength)
	{
		size_t utf16ExecutableNameSize = (((MAX_PATH + 1) * sizeof(WCHAR)) + (pageSize - 1)) & ~(pageSize - 1);
		WCHAR* utf16ExecutableName = (WCHAR*)VirtualAlloc(0, utf16ExecutableNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!utf16ExecutableName)
		{
			return ERROR_OUTOFMEMORY;
		}

		size_t utf16ExecutableNameLength = (size_t)GetModuleFileNameW(0, utf16ExecutableName, (DWORD)(utf16ExecutableNameSize / sizeof(WCHAR)));
		if (!utf16ExecutableNameLength || utf16ExecutableNameLength > (utf16ExecutableNameSize / sizeof(WCHAR)))
		{
			VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
			utf16ExecutableNameSize = (((maxFileNameLength + 1) * sizeof(WCHAR)) + (pageSize - 1)) & ~(pageSize - 1);
			utf16ExecutableName = (WCHAR*)VirtualAlloc(0, utf16ExecutableNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (!utf16ExecutableName)
			{
				return ERROR_OUTOFMEMORY;
			}

			utf16ExecutableNameLength = (size_t)GetModuleFileNameW(0, utf16ExecutableName, (DWORD)(utf16ExecutableNameSize / sizeof(WCHAR)));
			if (!utf16ExecutableNameLength || utf16ExecutableNameLength > (utf16ExecutableNameSize / sizeof(WCHAR)))
			{
				DWORD getModuleFileNameError = GetLastError();
				if (getModuleFileNameError == ERROR_SUCCESS)
				{
					getModuleFileNameError = ERROR_UNIDENTIFIED_ERROR;
				}
				VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
				return getModuleFileNameError;
			}
		}

		size_t utf8ExecutableNameLength = FlConvertUtf16LeToUtf8(utf16ExecutableNameLength, utf16ExecutableName, 0, 0);
		size_t utf8TableSize = (((2 * sizeof(char*)) + (utf8ExecutableNameLength + 1)) + (pageSize - 1)) & ~(pageSize - 1);
		char** utf8Table = (char**)VirtualAlloc(0, utf8TableSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!utf8Table)
		{
			VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
			return ERROR_OUTOFMEMORY;
		}
		char* utf8ExecutableName = (char*)((uintptr_t)utf8Table + (2 * sizeof(char*)));
		utf8Table[0] = utf8ExecutableName;
		utf8Table[1] = 0;
		FlConvertUtf16LeToUtf8(utf16ExecutableNameLength, utf16ExecutableName, utf8ExecutableNameLength, utf8ExecutableName);
		utf8ExecutableName[utf8ExecutableNameLength] = 0;

		VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);

		*argumentCountAddress = 1;
		*argumentTableAddress = utf8Table;
		return 0;
	}

	size_t commandLength = FlConvertUtf16LeToUtf8(nativeCommandLength, nativeCommand, 0, 0);
	size_t commandSize = (((size_t)commandLength + 1) + (pageSize - 1)) & ~(pageSize - 1);
	char* command = (char*)VirtualAlloc(0, commandSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!command)
	{
		return ERROR_OUTOFMEMORY;
	}
	FlConvertUtf16LeToUtf8(nativeCommandLength, nativeCommand, commandLength, command);
	command[commandLength] = 0;

	size_t argumentCount = 0;
	size_t argumentCharacterCount = 0;
	size_t inArgument = 1;
	size_t inQuotes = 0;
	size_t backslashCount = 0;
	for (size_t i = 0; i != commandLength; ++i)
	{
		if (command[i] == '\\')
		{
			inArgument = 1;
			++backslashCount;
		}
		else if (command[i] == '"')
		{
			inArgument = 1;
			int quotedDoubleQuote = inQuotes && i + 1 != commandLength && command[i + 1] == '"';
			if (backslashCount && !(backslashCount & 1))
			{
				argumentCharacterCount += (backslashCount / 2);
				inQuotes = !inQuotes;
			}
			else if (backslashCount && (backslashCount & 1))
			{
				argumentCharacterCount += (backslashCount / 2) + 1;
			}
			else
			{
				inQuotes = !inQuotes;
			}
			if (quotedDoubleQuote)
			{
				++argumentCharacterCount;
				++i;
			}
			backslashCount = 0;
		}
		else if (command[i] == ' ' || command[i] == '\t')
		{
			if (inArgument)
			{
				if (backslashCount)
				{
					argumentCharacterCount += backslashCount;
					backslashCount = 0;
				}
				if (inQuotes)
				{
					++argumentCharacterCount;
				}
				else
				{
					inArgument = 0;
					++argumentCount;
				}
			}
		}
		else
		{
			inArgument = 1;
			if (backslashCount)
			{
				argumentCharacterCount += backslashCount;
				backslashCount = 0;
			}
			++argumentCharacterCount;
		}
	}
	if (inArgument)
	{
		if (backslashCount)
		{
			argumentCharacterCount += backslashCount;
			backslashCount = 0;
		}
		++argumentCount;
	}

	size_t argumentTableSize = (((((size_t)argumentCount + 1) * sizeof(char*)) + (((size_t)argumentCharacterCount + (size_t)argumentCount))) + (pageSize - 1)) & ~(pageSize - 1);
	char** argumentTable = (char**)VirtualAlloc(0, argumentTableSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!argumentTable)
	{
		VirtualFree(command, 0, MEM_RELEASE);
		return ERROR_OUTOFMEMORY;
	}

	char* argumentWrite = (char*)((uintptr_t)argumentTable + (((size_t)argumentCount + 1) * sizeof(char*)));
	inArgument = 1;
	inQuotes = 0;
	backslashCount = 0;
	size_t argumentIndex = 0;
	argumentTable[0] = argumentWrite;
	for (int i = 0; i != commandLength; ++i)
	{
		if (command[i] == '\\')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentTable[argumentIndex] = argumentWrite;
			}
			++backslashCount;
		}
		else if (command[i] == '"')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentTable[argumentIndex] = argumentWrite;
			}
			int quotedDoubleQuote = inQuotes && i + 1 != commandLength && command[i + 1] == '"';
			if (backslashCount && !(backslashCount & 1))
			{
				for (char* fillEnd = argumentWrite + (backslashCount / 2); argumentWrite != fillEnd;)
				{
					*argumentWrite++ = '\\';
				}
				inQuotes = !inQuotes;
			}
			else if (backslashCount && (backslashCount & 1))
			{
				for (char* fillEnd = argumentWrite + (backslashCount / 2); argumentWrite != fillEnd;)
				{
					*argumentWrite++ = '\\';
				}
				*argumentWrite++ = '\"';
			}
			else
			{
				inQuotes = !inQuotes;
			}
			if (quotedDoubleQuote)
			{
				*argumentWrite++ = '\"';
				++i;
			}
			backslashCount = 0;
		}
		else if (command[i] == ' ' || command[i] == '\t')
		{
			if (inArgument)
			{
				if (backslashCount)
				{
					for (char* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
					{
						*argumentWrite++ = '\\';
					}
					backslashCount = 0;
				}
				if (inQuotes)
				{
					*argumentWrite++ = command[i];
				}
				else
				{
					inArgument = 0;
					*argumentWrite++ = 0;
					++argumentIndex;
				}
			}
		}
		else
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentTable[argumentIndex] = argumentWrite;
			}
			if (backslashCount)
			{
				for (char* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
				{
					*argumentWrite++ = '\\';
				}
				backslashCount = 0;
			}
			*argumentWrite++ = command[i];
		}
	}
	if (inArgument)
	{
		if (backslashCount)
		{
			for (char* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
			{
				*argumentWrite++ = '\\';
			}
			backslashCount = 0;
		}
		*argumentWrite++ = 0;
		++argumentIndex;
	}
	argumentTable[argumentCount] = 0;

	VirtualFree(command, 0, MEM_RELEASE);

	*argumentCountAddress = argumentCount;
	*argumentTableAddress = argumentTable;
	return 0;
}

#ifdef __cplusplus
}
#endif
