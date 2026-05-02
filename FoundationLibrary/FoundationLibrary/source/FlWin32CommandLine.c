/*
	Win32 command line parsing utility by by Santtu Nyman.
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
#include "FlWin32CommandLine.h"
#include "FlUtf8Utf16Converter.h"

#define FL_LOCAL_IS_POSITIVE_POWER_OF_2(N)  (((N) - 1) < ((N) ^ ((N) - 1)))

HRESULT FlWin32CommandLineToArgumentsUtf16(_In_ _Null_terminated_ const WCHAR* win32CommandLine, _In_ size_t bufferSize, _Out_writes_bytes_to_(bufferSize, *bufferRequiredAddress) WCHAR** argumentBuffer, _Out_ size_t* bufferRequiredAddress, _Out_ size_t* argumentCountAddress)
{
	size_t win32CommandLength = 0;
	if (win32CommandLine)
	{
		while (win32CommandLine[win32CommandLength])
		{
			win32CommandLength++;
		}
	}

	if (!win32CommandLength)
	{
		SYSTEM_INFO systemInfo;
		memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
		GetSystemInfo(&systemInfo);
		size_t pageSize = (size_t)systemInfo.dwPageSize;
		if (!FL_LOCAL_IS_POSITIVE_POWER_OF_2(pageSize))
		{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
			pageSize = 0x1000;
#else
			pageSize = 0x10000;
#endif
		}
		_Analysis_assume_(FL_LOCAL_IS_POSITIVE_POWER_OF_2(pageSize) && ((pageSize != 0) && ((pageSize & (pageSize - 1)) == 0))); // Try to tell the static analyzer that page size is power of 2

		size_t utf16ExecutableNameSize = ((((size_t)MAX_PATH + 1) * sizeof(WCHAR)) + (pageSize - 1)) & ~(pageSize - 1);
		_Analysis_assume_(utf16ExecutableNameSize >= ((size_t)MAX_PATH + 1) * sizeof(WCHAR));
		WCHAR* utf16ExecutableName = (WCHAR*)VirtualAlloc(0, utf16ExecutableNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!utf16ExecutableName)
		{
			*bufferRequiredAddress = 0;
			*argumentCountAddress = 0;
			return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
		}

		DWORD getModuleNameBufferLength = (DWORD)(utf16ExecutableNameSize / sizeof(WCHAR));
		_Analysis_assume_((size_t)getModuleNameBufferLength * sizeof(WCHAR) == utf16ExecutableNameSize);
		size_t utf16ExecutableNameLength = (size_t)GetModuleFileNameW(NULL, utf16ExecutableName, getModuleNameBufferLength);
		if (!utf16ExecutableNameLength || utf16ExecutableNameLength >= (size_t)getModuleNameBufferLength)
		{
			VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
			utf16ExecutableNameSize = ((((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR)) + (pageSize - 1)) & ~(pageSize - 1);
			_Analysis_assume_(utf16ExecutableNameSize >= ((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			utf16ExecutableName = (WCHAR*)VirtualAlloc(0, utf16ExecutableNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (!utf16ExecutableName)
			{
				*bufferRequiredAddress = 0;
				*argumentCountAddress = 0;
				return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
			}

			getModuleNameBufferLength = (DWORD)(utf16ExecutableNameSize / sizeof(WCHAR));
			_Analysis_assume_((size_t)getModuleNameBufferLength * sizeof(WCHAR) == utf16ExecutableNameSize);
			utf16ExecutableNameLength = (size_t)GetModuleFileNameW(NULL, utf16ExecutableName, getModuleNameBufferLength);
			if (!utf16ExecutableNameLength || utf16ExecutableNameLength >= (size_t)getModuleNameBufferLength)
			{
				DWORD getModuleFileNameError = GetLastError();
				if (getModuleFileNameError == ERROR_SUCCESS)
				{
					getModuleFileNameError = ERROR_UNIDENTIFIED_ERROR;
				}
				VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
				*bufferRequiredAddress = 0;
				*argumentCountAddress = 0;
				return HRESULT_FROM_WIN32(getModuleFileNameError);
			}
		}

		size_t requiredBufferSize = ((size_t)2 * sizeof(WCHAR*)) + ((utf16ExecutableNameLength + 1) * sizeof(WCHAR));
		_Analysis_assume_(requiredBufferSize > ((size_t)2 * sizeof(WCHAR*)) && requiredBufferSize > ((utf16ExecutableNameLength + 1) * sizeof(WCHAR)));
		if (requiredBufferSize > bufferSize)
		{
			VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
			*bufferRequiredAddress = requiredBufferSize;
			*argumentCountAddress = 1;
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
		WCHAR* utf16TableExecutableName = (WCHAR*)((uintptr_t)argumentBuffer + (2 * sizeof(WCHAR*)));
		argumentBuffer[0] = utf16TableExecutableName;
		argumentBuffer[1] = 0;
		memcpy(utf16TableExecutableName, utf16ExecutableName, utf16ExecutableNameLength);
		utf16TableExecutableName[utf16ExecutableNameLength] = 0;
		VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
		*bufferRequiredAddress = requiredBufferSize;
		*argumentCountAddress = 1;
		return S_OK;
	}

	size_t argumentCount = 0;
	size_t argumentCharacterCount = 0;
	size_t inArgument = 1;
	size_t inQuotes = 0;
	size_t backslashCount = 0;
	for (size_t i = 0; i < win32CommandLength; i++)
	{
		if (win32CommandLine[i] == L'\\')
		{
			inArgument = 1;
			backslashCount++;
		}
		else if (win32CommandLine[i] == L'"')
		{
			inArgument = 1;
			int quotedDoubleQuote = inQuotes && i + 1 != win32CommandLength && win32CommandLine[i + 1] == L'"';
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
				argumentCharacterCount++;
				i++;
			}
			backslashCount = 0;
		}
		else if (win32CommandLine[i] == ' ' || win32CommandLine[i] == L'\t')
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
					argumentCharacterCount++;
				}
				else
				{
					inArgument = 0;
					argumentCount++;
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
			argumentCharacterCount++;
		}
	}
	if (inArgument)
	{
		if (backslashCount)
		{
			argumentCharacterCount += backslashCount;
			backslashCount = 0;
		}
		argumentCount++;
	}

	_Analysis_assume_(
		(argumentCharacterCount + argumentCount) >= argumentCharacterCount &&
		(argumentCharacterCount + argumentCount) >= argumentCount &&
		(((argumentCharacterCount + argumentCount) * sizeof(WCHAR)) / sizeof(WCHAR)) == (argumentCharacterCount + argumentCount));// we can't have counted more character than can possibly fit in memory
	if ((argumentCount + 1) < argumentCount ||
		(((argumentCount + 1) * sizeof(WCHAR*)) / sizeof(WCHAR*)) != (argumentCount + 1) ||
		(((argumentCount + 1) * sizeof(WCHAR*)) + ((argumentCharacterCount + argumentCount) * sizeof(WCHAR))) < ((argumentCount + 1) * sizeof(WCHAR*)))
	{
		*bufferRequiredAddress = 0;
		*argumentCountAddress = 0;
		return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
	}
	size_t argumentTableSize = ((argumentCount + 1) * sizeof(WCHAR*)) + ((argumentCharacterCount + argumentCount) * sizeof(WCHAR));
	if (bufferSize < argumentTableSize)
	{
		*bufferRequiredAddress = argumentTableSize;
		*argumentCountAddress = argumentCount;
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}

	WCHAR* argumentWrite = (WCHAR*)((uintptr_t)argumentBuffer + (((size_t)argumentCount + 1) * sizeof(WCHAR*)));
	inArgument = 1;
	inQuotes = 0;
	backslashCount = 0;
	size_t argumentIndex = 0;
	argumentBuffer[0] = argumentWrite;
	for (size_t i = 0; i < win32CommandLength; i++)
	{
		if (win32CommandLine[i] == L'\\')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentBuffer[argumentIndex] = argumentWrite;
			}
			backslashCount++;
		}
		else if (win32CommandLine[i] == L'"')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentBuffer[argumentIndex] = argumentWrite;
			}
			int quotedDoubleQuote = inQuotes && i + 1 != win32CommandLength && win32CommandLine[i + 1] == L'"';
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
				i++;
			}
			backslashCount = 0;
		}
		else if (win32CommandLine[i] == L' ' || win32CommandLine[i] == L'\t')
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
					*argumentWrite++ = win32CommandLine[i];
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
				argumentBuffer[argumentIndex] = argumentWrite;
			}
			if (backslashCount)
			{
				for (WCHAR* fillEnd = argumentWrite + backslashCount; argumentWrite != fillEnd;)
				{
					*argumentWrite++ = L'\\';
				}
				backslashCount = 0;
			}
			*argumentWrite++ = win32CommandLine[i];
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
	argumentBuffer[argumentCount] = 0;

	*bufferRequiredAddress = argumentTableSize;
	*argumentCountAddress = argumentCount;
	return S_OK;
}

HRESULT FlWin32CommandLineToArgumentsUtf8(_In_ _Null_terminated_ const WCHAR* win32CommandLine, _In_ size_t bufferSize, _Out_writes_bytes_to_(bufferSize, *bufferRequiredAddress) char** argumentBuffer, _Out_ size_t* bufferRequiredAddress, _Out_ size_t* argumentCountAddress)
{
	SYSTEM_INFO systemInfo;
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&systemInfo);
	size_t pageSize = (size_t)systemInfo.dwPageSize;
	if (!FL_LOCAL_IS_POSITIVE_POWER_OF_2(pageSize))
	{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
		pageSize = 0x1000;
#else
		pageSize = 0x10000;
#endif
	}
	_Analysis_assume_(FL_LOCAL_IS_POSITIVE_POWER_OF_2(pageSize) && ((pageSize != 0) && ((pageSize & (pageSize - 1)) == 0))); // Try to tell the static analyzer that page size is power of 2

	size_t win32CommandLength = 0;
	if (win32CommandLine)
	{
		while (win32CommandLine[win32CommandLength])
		{
			win32CommandLength++;
		}
	}

	if (!win32CommandLength)
	{
		size_t utf16ExecutableNameSize = ((((size_t)MAX_PATH + 1) * sizeof(WCHAR)) + (pageSize - 1)) & ~(pageSize - 1);
		_Analysis_assume_(utf16ExecutableNameSize >= ((size_t)MAX_PATH + 1) * sizeof(WCHAR));
		WCHAR* utf16ExecutableName = (WCHAR*)VirtualAlloc(0, utf16ExecutableNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!utf16ExecutableName)
		{
			*bufferRequiredAddress = 0;
			*argumentCountAddress = 0;
			return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
		}

		DWORD getModuleNameBufferLength = (DWORD)(utf16ExecutableNameSize / sizeof(WCHAR));
		_Analysis_assume_((size_t)getModuleNameBufferLength * sizeof(WCHAR) == utf16ExecutableNameSize);
		size_t utf16ExecutableNameLength = (size_t)GetModuleFileNameW(NULL, utf16ExecutableName, getModuleNameBufferLength);
		if (!utf16ExecutableNameLength || utf16ExecutableNameLength >= (size_t)getModuleNameBufferLength)
		{
			VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
			utf16ExecutableNameSize = ((((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR)) + (pageSize - 1)) & ~(pageSize - 1);
			_Analysis_assume_(utf16ExecutableNameSize >= ((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			utf16ExecutableName = (WCHAR*)VirtualAlloc(0, utf16ExecutableNameSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (!utf16ExecutableName)
			{
				*bufferRequiredAddress = 0;
				*argumentCountAddress = 0;
				return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
			}

			getModuleNameBufferLength = (DWORD)(utf16ExecutableNameSize / sizeof(WCHAR));
			_Analysis_assume_((size_t)getModuleNameBufferLength * sizeof(WCHAR) == utf16ExecutableNameSize);
			utf16ExecutableNameLength = (size_t)GetModuleFileNameW(NULL, utf16ExecutableName, getModuleNameBufferLength);
			if (!utf16ExecutableNameLength || utf16ExecutableNameLength >= (size_t)getModuleNameBufferLength)
			{
				DWORD getModuleFileNameError = GetLastError();
				if (getModuleFileNameError == ERROR_SUCCESS)
				{
					getModuleFileNameError = ERROR_UNIDENTIFIED_ERROR;
				}
				VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
				*bufferRequiredAddress = 0;
				*argumentCountAddress = 0;
				return HRESULT_FROM_WIN32(getModuleFileNameError);
			}
		}

		char* utf8ExecutableName = (char*)((uintptr_t)argumentBuffer + (2 * sizeof(char*)));
		size_t utf8ExecutableNameLength = FlConvertUtf16LeToUtf8(utf16ExecutableNameLength, utf16ExecutableName, (bufferSize > (2 * sizeof(char*))) ? (bufferSize - (2 * sizeof(char*))) : 0, (bufferSize > (2 * sizeof(char*))) ? utf8ExecutableName : NULL);
		_Analysis_assume_(utf8ExecutableNameLength);
		size_t requiredBufferSize = ((size_t)2 * sizeof(char*)) + (utf8ExecutableNameLength + 1);
		_Analysis_assume_(requiredBufferSize > ((size_t)2 * sizeof(char*)) && requiredBufferSize > (utf8ExecutableNameLength + 1));
		VirtualFree(utf16ExecutableName, 0, MEM_RELEASE);
		if (requiredBufferSize > bufferSize)
		{
			*bufferRequiredAddress = requiredBufferSize;
			*argumentCountAddress = 1;
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
		argumentBuffer[0] = utf8ExecutableName;
		argumentBuffer[1] = 0;
		utf8ExecutableName[utf8ExecutableNameLength] = 0;
		*bufferRequiredAddress = requiredBufferSize;
		*argumentCountAddress = 1;
		return S_OK;
	}

	size_t commandLength = FlConvertUtf16LeToUtf8(win32CommandLength, win32CommandLine, 0, 0);
	_Analysis_assume_(commandLength);
	size_t commandSize = ((commandLength + 1) + (pageSize - 1)) & ~(pageSize - 1);
	if (commandSize < commandLength)
	{
		*bufferRequiredAddress = 0;
		*argumentCountAddress = 0;
		return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
	}
	char* command = (char*)VirtualAlloc(0, commandSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!command)
	{
		*bufferRequiredAddress = 0;
		*argumentCountAddress = 0;
		return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
	}
	FlConvertUtf16LeToUtf8(win32CommandLength, win32CommandLine, commandLength, command);
	command[commandLength] = 0;

	size_t argumentCount = 0;
	size_t argumentCharacterCount = 0;
	size_t inArgument = 1;
	size_t inQuotes = 0;
	size_t backslashCount = 0;
	for (size_t i = 0; i < commandLength; i++)
	{
		if (command[i] == '\\')
		{
			inArgument = 1;
			backslashCount++;
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
				argumentCharacterCount++;
				i++;
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
					argumentCharacterCount++;
				}
				else
				{
					inArgument = 0;
					argumentCount++;
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
			argumentCharacterCount++;
		}
	}
	if (inArgument)
	{
		if (backslashCount)
		{
			argumentCharacterCount += backslashCount;
			backslashCount = 0;
		}
		argumentCount++;
	}

	_Analysis_assume_(
		(argumentCharacterCount + argumentCount) >= argumentCharacterCount &&
		(argumentCharacterCount + argumentCount) >= argumentCount);// we can't have counted more character than can possibly fit in memory
	if ((argumentCount + 1) < argumentCount ||
		(((argumentCount + 1) * sizeof(char*)) / sizeof(char*)) != (argumentCount + 1) ||
		(((argumentCount + 1) * sizeof(char*)) + (argumentCharacterCount + argumentCount)) < ((argumentCount + 1) * sizeof(char*)))
	{
		*bufferRequiredAddress = 0;
		*argumentCountAddress = 0;
		return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
	}
	size_t argumentTableSize = ((argumentCount + 1) * sizeof(char*)) + (argumentCharacterCount + argumentCount);
	if (bufferSize < argumentTableSize)
	{
		VirtualFree(command, 0, MEM_RELEASE);
		*bufferRequiredAddress = argumentTableSize;
		*argumentCountAddress = argumentCount;
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}

	char* argumentWrite = (char*)((uintptr_t)argumentBuffer + (((size_t)argumentCount + 1) * sizeof(char*)));
	inArgument = 1;
	inQuotes = 0;
	backslashCount = 0;
	size_t argumentIndex = 0;
	argumentBuffer[0] = argumentWrite;
	for (size_t i = 0; i != commandLength; i++)
	{
		if (command[i] == '\\')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentBuffer[argumentIndex] = argumentWrite;
			}
			backslashCount++;
		}
		else if (command[i] == '"')
		{
			if (!inArgument)
			{
				inArgument = 1;
				argumentBuffer[argumentIndex] = argumentWrite;
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
				i++;
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
				argumentBuffer[argumentIndex] = argumentWrite;
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
	argumentBuffer[argumentCount] = 0;

	VirtualFree(command, 0, MEM_RELEASE);
	*bufferRequiredAddress = argumentTableSize;
	*argumentCountAddress = argumentCount;
	return S_OK;
}

WCHAR** FlCommandLineToArgvW(_In_ _Null_terminated_ const WCHAR* win32CommandLine, _Out_ int* argumentCountAddress)
{
	const size_t intMax = (size_t)(~((unsigned int)0) >> 1);
	size_t requiredBufferSize = 0;
	size_t argumentCount = 0;
	HRESULT error = FlWin32CommandLineToArgumentsUtf16(win32CommandLine, 0, NULL, &requiredBufferSize, &argumentCount);
	if (error != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		if (error == HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY))
		{
			SetLastError(ERROR_OUTOFMEMORY);
		}
		else
		{
			SetLastError(ERROR_UNIDENTIFIED_ERROR);
		}
		*argumentCountAddress = 0;
		return NULL;
	}
	if (argumentCount > intMax)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		*argumentCountAddress = 0;
		return NULL;
	}
	size_t bufferSize = requiredBufferSize;
	_Analysis_assume_(bufferSize);
	WCHAR** argumentTableBuffer = (WCHAR**)LocalAlloc(LMEM_FIXED, bufferSize);
	if (!argumentTableBuffer)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		*argumentCountAddress = 0;
		return NULL;
	}
	error = FlWin32CommandLineToArgumentsUtf16(win32CommandLine, bufferSize, argumentTableBuffer, &requiredBufferSize, &argumentCount);
	if (FAILED(error))
	{
		if (error == HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY))
		{
			SetLastError(ERROR_OUTOFMEMORY);
		}
		else
		{
			SetLastError(ERROR_UNIDENTIFIED_ERROR);
		}
		*argumentCountAddress = 0;
		return NULL;
	}
	*argumentCountAddress = (int)argumentCount;
	return argumentTableBuffer;
}

#ifdef __cplusplus
}
#endif
