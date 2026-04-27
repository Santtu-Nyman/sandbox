/*
	Implementation for Win32 function CommandLineToArgvW by Santtu Nyman.
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
#include "FlCommandLineToArgvW.h"

WCHAR** FlCommandLineToArgvW(_In_ _Null_terminated_ const WCHAR* cmdLine, _Out_ int* numArgs)
{
	const size_t maxFileNameLength = UNICODE_STRING_MAX_CHARS;
	const size_t intMax = (int)(((unsigned int)~0) >> 1);

	size_t commandLength = 0;
	if (cmdLine)
	{
		while (cmdLine[commandLength])
		{
			commandLength++;
		}
	}

	if (!commandLength)
	{
		WCHAR** executableNameBuffer = (WCHAR**)LocalAlloc(LMEM_FIXED, (2 * sizeof(WCHAR*)) + ((MAX_PATH + 1) * sizeof(WCHAR)));
		if (!executableNameBuffer)
		{
			return 0;
		}
		size_t executableNameBufferSize = LocalSize(executableNameBuffer);
		if (!executableNameBufferSize)
		{
			DWORD sizeError = GetLastError();
			LocalFree(executableNameBuffer);
			SetLastError(sizeError);
			return 0;
		}

		size_t executableNameLength = (size_t)GetModuleFileNameW(0, (WCHAR*)((uintptr_t)executableNameBuffer + (2 * sizeof(WCHAR*))), (DWORD)((executableNameBufferSize - (2 * sizeof(WCHAR*))) / sizeof(WCHAR)));
		if (!executableNameLength || executableNameLength > (executableNameBufferSize / sizeof(WCHAR)))
		{
			WCHAR** newExecutableNameBuffer = (WCHAR**)LocalReAlloc(executableNameBuffer, (2 * sizeof(WCHAR*)) + ((maxFileNameLength + 1) * sizeof(WCHAR)), 0);
			if (!newExecutableNameBuffer)
			{
				DWORD reallocError = GetLastError();
				LocalFree(executableNameBuffer);
				SetLastError(reallocError);
				return 0;
			}
			executableNameBuffer = newExecutableNameBuffer;
			executableNameBufferSize = LocalSize(executableNameBuffer);
			if (!executableNameBufferSize)
			{
				DWORD sizeError = GetLastError();
				LocalFree(executableNameBuffer);
				SetLastError(sizeError);
				return 0;
			}

			executableNameLength = (size_t)GetModuleFileNameW(0, (WCHAR*)((uintptr_t)executableNameBuffer + (2 * sizeof(WCHAR*))), (DWORD)((executableNameBufferSize - (2 * sizeof(WCHAR*))) / sizeof(WCHAR)));
			if (!executableNameLength || executableNameLength > (executableNameBufferSize / sizeof(WCHAR)))
			{
				DWORD reallocError = GetLastError();
				LocalFree(executableNameBuffer);
				SetLastError(reallocError);
				return 0;
			}
		}

		WCHAR** truncatedExecutableNameBuffer = (WCHAR**)LocalReAlloc(executableNameBuffer, (2 * sizeof(WCHAR*)) + ((executableNameLength + 1) * sizeof(WCHAR)), 0);
		if (!truncatedExecutableNameBuffer)
		{
			DWORD truncateError = GetLastError();
			LocalFree(executableNameBuffer);
			SetLastError(truncateError);
			return 0;
		}
		executableNameBuffer = truncatedExecutableNameBuffer;

		executableNameBuffer[0] = (WCHAR*)((uintptr_t)executableNameBuffer + (2 * sizeof(WCHAR*)));
		executableNameBuffer[1] = 0;
		*numArgs = 1;
		SetLastError(ERROR_SUCCESS);
		return executableNameBuffer;
	}

	size_t argumentCount = 0;
	size_t argumentCharacterCount = 0;
	int inArgument = 1;
	int inQuotes = 0;
	size_t backslashCount = 0;
	for (size_t i = 0; i != commandLength; ++i)
	{
		if (cmdLine[i] == L'\\')
		{
			inArgument = 1;
			++backslashCount;
		}
		else if (cmdLine[i] == L'"')
		{
			inArgument = 1;
			int quotedDoubleQuote = inQuotes && i + 1 != commandLength && cmdLine[i + 1] == L'"';
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
		else if (cmdLine[i] == L' ' || cmdLine[i] == L'\t')
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

	WCHAR** argumentTable = (WCHAR**)LocalAlloc(LMEM_FIXED, (((size_t)argumentCount + 1) * sizeof(WCHAR*)) + (((size_t)argumentCharacterCount + (size_t)argumentCount) * sizeof(WCHAR)));
	if (!argumentTable)
	{
		return 0;
	}

	WCHAR* argumentWrite = (WCHAR*)((uintptr_t)argumentTable + (((size_t)argumentCount + 1) * sizeof(WCHAR*)));
	inArgument = 1;
	inQuotes = 0;
	backslashCount = 0;
	size_t argumentIndex = 0;
	argumentTable[0] = argumentWrite;
	for (size_t i = 0; i != commandLength; ++i)
	{
		if (cmdLine[i] == L'\\')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentTable[argumentIndex] = argumentWrite;
			}
			++backslashCount;
		}
		else if (cmdLine[i] == L'"')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentTable[argumentIndex] = argumentWrite;
			}
			int quotedDoubleQuote = inQuotes && i + 1 != commandLength && cmdLine[i + 1] == L'"';
			if (backslashCount && !(backslashCount & 1))
			{
				for (WCHAR* fillEnd = argumentWrite + (backslashCount / 2); argumentWrite != fillEnd;)
				{
					*argumentWrite++ = L'\\';
				}
				inQuotes = !inQuotes;
			}
			else if (backslashCount && (backslashCount & 1))
			{
				for (WCHAR* fillEnd = argumentWrite + (backslashCount / 2); argumentWrite != fillEnd;)
				{
					*argumentWrite++ = L'\\';
				}
				*argumentWrite++ = L'\"';
			}
			else
			{
				inQuotes = !inQuotes;
			}
			if (quotedDoubleQuote)
			{
				*argumentWrite++ = L'\"';
				++i;
			}
			backslashCount = 0;
		}
		else if (cmdLine[i] == L' ' || cmdLine[i] == L'\t')
		{
			if (inArgument)
			{
				if (backslashCount)
				{
					for (WCHAR* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
					{
						*argumentWrite++ = L'\\';
					}
					backslashCount = 0;
				}
				if (inQuotes)
				{
					*argumentWrite++ = cmdLine[i];
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
				for (WCHAR* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
				{
					*argumentWrite++ = L'\\';
				}
				backslashCount = 0;
			}
			*argumentWrite++ = cmdLine[i];
		}
	}
	if (inArgument)
	{
		if (backslashCount)
		{
			for (WCHAR* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
			{
				*argumentWrite++ = L'\\';
			}
			backslashCount = 0;
		}
		*argumentWrite++ = 0;
		++argumentIndex;
	}
	argumentTable[argumentCount] = 0;

	if (argumentCount > intMax)
	{
		LocalFree(argumentTable);
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	*numArgs = (int)argumentCount;
	SetLastError(ERROR_SUCCESS);
	return argumentTable;
}

#ifdef __cplusplus
}
#endif
