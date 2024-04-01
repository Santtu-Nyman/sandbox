#ifndef LAB6TEHTAVAK_H
#define LAB6TEHTAVAK_H

#include <windows.h>

#define L6TK_INPUT_MAXIMUM_LEGTH 256

typedef struct l6tkData
{
	HANDLE heap;
	HANDLE consoleOut;
	HANDLE consoleIn;
	WCHAR* currentDirectoryAnd2CharaterBuffer;
	SIZE_T fileListLength;
	SIZE_T numberOfDirectories;
	WIN32_FIND_DATAW* fileList;
	CHAR consoleInput[L6TK_INPUT_MAXIMUM_LEGTH];
} l6tkData;

SIZE_T l6ktGetStringLength(const CHAR* string);

typedef struct l6tkPrint
{
	BYTE printDataType;
	VOID* dataAddress;
} l6tkPrint;

#define L6TK_PRINT_TYPE_STRING           0x00
#define L6TK_PRINT_TYPE_UNSIGNED_INTEGER 0x01
#define L6TK_PRINT_TYPE_SIGNED_INTEGER   0x02
#define L6TK_PRINT_TYPE_NATIVE_STRING    0x03

DWORD l6tkPrintOut(HANDLE consoleOutput, SIZE_T printDataCount, l6tkPrint* printData);

DWORD l6tkListCurrentDirectory(l6tkData* data);

DWORD l6tkInitialize(l6tkData* data);

void l6tkClose(l6tkData* data);

DWORD l6tkConsoleInput(l6tkData* data);

DWORD l6tkDecodeInteger(const CHAR* integerString, DWORD* result);

DWORD l6tkPrintAllDirectories(l6tkData* data);

DWORD l6tkSetCurrentDirectoryByIndex(l6tkData* data, SIZE_T index);

DWORD l6tkGetCurrentDirectory(HANDLE heap, WCHAR** currentDirectoryAnd2CharaterBufferAddress);

DWORD l6tkPrintLine(HANDLE consoleOutput, WCHAR* String);

DWORD l6tkGetSavedCurrentDirectory(HANDLE heap, WCHAR** currentDirectoryAnd2CharaterBufferAddress);

DWORD l6tkSetSavedCurrentDirectory(const WCHAR* currentDirectory);

void l6tkMain()
{
	l6tkData mainData;
	DWORD error = l6tkInitialize(&mainData);
	if (error)
		ExitProcess((UINT)error);
	l6tkPrint printError = { L6TK_PRINT_TYPE_STRING, "error can't move to that directory\n" };
	l6tkPrint printInvalidInput = { L6TK_PRINT_TYPE_STRING, "invalid input\n" };
	l6tkPrint printCurrentDirectory[3] = { { L6TK_PRINT_TYPE_NATIVE_STRING, L"move to one of listed directories by entering it's number\n" },{ L6TK_PRINT_TYPE_NATIVE_STRING, 0 },{ L6TK_PRINT_TYPE_NATIVE_STRING, L"\n" } };
	for (DWORD index;;)
	{
		printCurrentDirectory[1].dataAddress = mainData.currentDirectoryAnd2CharaterBuffer;
		l6tkPrintOut(mainData.consoleOut, 3, printCurrentDirectory);
		l6tkPrintAllDirectories(&mainData);
		l6tkConsoleInput(&mainData);
		if (!l6tkDecodeInteger(mainData.consoleInput, &index) && index < mainData.numberOfDirectories)
		{
			error = l6tkSetCurrentDirectoryByIndex(&mainData, (SIZE_T)index);
			if (error)
				l6tkPrintOut(mainData.consoleOut, 1, &printError);
		}
		else
			l6tkPrintOut(mainData.consoleOut, 1, &printInvalidInput);
	}
	l6tkClose(&mainData);
	ExitProcess(0);
}

SIZE_T l6ktGetStringLength(const CHAR* string)
{
	const CHAR* read = string;
	while (*read)
		++read;
	return (SIZE_T)((UINT_PTR)read - (UINT_PTR)string);
}

DWORD l6tkPrintOut(HANDLE consoleOutput, SIZE_T printDataCount, l6tkPrint* printData)
{
	CHAR buffer[16];
	DWORD consoleWritten;
	for (l6tkPrint* printDataEnd = printData + printDataCount; printData != printDataEnd; ++printData)
	{
		if (printData->printDataType == L6TK_PRINT_TYPE_STRING)
		{
			if (!WriteConsoleA(consoleOutput, printData->dataAddress, (DWORD)l6ktGetStringLength((CHAR*)printData->dataAddress), &consoleWritten, 0))
				return GetLastError();
		}
		else if (printData->printDataType == L6TK_PRINT_TYPE_UNSIGNED_INTEGER || printData->printDataType == L6TK_PRINT_TYPE_SIGNED_INTEGER)
		{
			DWORD integer = printData->printDataType == L6TK_PRINT_TYPE_UNSIGNED_INTEGER ? (DWORD)*(unsigned int*)printData->dataAddress : (DWORD)*(int*)printData->dataAddress;
			DWORD isNegative = 0;
			if (printData->printDataType == L6TK_PRINT_TYPE_SIGNED_INTEGER && integer >> 31)
			{
				isNegative = 1;
				buffer[0] = '-';
				integer = 0 - integer;
			}
			DWORD digitsWritten = 0;
			while (integer)
			{
				for (CHAR* move = buffer + isNegative + digitsWritten++; move != buffer + isNegative; --move)
					move[0] = move[-1];
				buffer[isNegative] = "0123456789"[integer % 10];
				integer /= 10;
			}
			if (!digitsWritten)
			{
				digitsWritten = 1;
				buffer[isNegative] = '0';
			}
			if (!WriteConsoleA(consoleOutput, buffer, isNegative + digitsWritten, &consoleWritten, 0))
				return GetLastError();
		}
		else if (printData->printDataType == L6TK_PRINT_TYPE_NATIVE_STRING)
		{
			if (!WriteConsoleW(consoleOutput, printData->dataAddress, (DWORD)lstrlenW((WCHAR*)printData->dataAddress), &consoleWritten, 0))
				return GetLastError();
		}
		else
			return ERROR_INVALID_PARAMETER;
	}
	return 0;
}

DWORD l6tkListCurrentDirectory(l6tkData* data)
{
	SIZE_T capasity = 16;
	SIZE_T directories = 0;
	SIZE_T length = 0;
	WIN32_FIND_DATAW* list = (WIN32_FIND_DATAW*)HeapAlloc(data->heap, 0, capasity * sizeof(WIN32_FIND_DATAW));
	if (!list)
		return GetLastError();
	SIZE_T currentDirectoryLength = (SIZE_T)lstrlenW(data->currentDirectoryAnd2CharaterBuffer);
	if (currentDirectoryLength && (data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength - 1] == L'\\' || data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength - 1] == L'/'))
	{
		data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength] = L'*';
		data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength + 1] = 0;
	}
	else
	{
		data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength] = L'\\';
		data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength + 1] = L'*';
		data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength + 2] = 0;
	}
	HANDLE search = FindFirstFileExW(data->currentDirectoryAnd2CharaterBuffer, FindExInfoBasic, list, FindExSearchNameMatch, 0, 0);
	DWORD error;
	if (search != INVALID_HANDLE_VALUE)
	{
		error = 0;
		while (!error)
		{
			if (list[length].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				++directories;
			++length;
			if (length == capasity)
			{
				capasity += 16;
				WIN32_FIND_DATAW* temporal = (WIN32_FIND_DATAW*)HeapReAlloc(data->heap, 0, list, capasity * sizeof(WIN32_FIND_DATAW));
				if (!temporal)
				{
					error = GetLastError();
					FindClose(search);
					HeapFree(data->heap, 0, list);
					data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength] = 0;
					return error;
				}
				list = temporal;
			}
			if (!FindNextFileW(search, list + length))
				error = GetLastError();
		}
		FindClose(search);
	}
	else
		error = GetLastError();
	if (error == ERROR_NO_MORE_FILES)
		error = 0;
	if (!error)
	{
		if (data->fileList)
			HeapFree(data->heap, 0, data->fileList);
		data->fileList = list;
		data->fileListLength = length;
		data->numberOfDirectories = directories;
	}
	else
		HeapFree(data->heap, 0, list);
	data->currentDirectoryAnd2CharaterBuffer[currentDirectoryLength] = 0;
	return error;
}

DWORD l6tkInitialize(l6tkData* data)
{
	data->heap = GetProcessHeap();
	if (!data->heap)
		return GetLastError();
	data->consoleOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (data->consoleOut == INVALID_HANDLE_VALUE)
		return GetLastError();
	data->consoleIn = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (data->consoleIn == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		CloseHandle(data->consoleOut);
		return error;
	}
	data->currentDirectoryAnd2CharaterBuffer = 0;
	data->fileList = 0;
	data->fileListLength = 0;
	data->numberOfDirectories = 0;
	data->consoleInput[0] = 0;
	DWORD error = l6tkGetSavedCurrentDirectory(data->heap, &data->currentDirectoryAnd2CharaterBuffer);
	if (!error)
	{
		SetCurrentDirectoryW(data->currentDirectoryAnd2CharaterBuffer);
		HeapFree(data->heap, 0, data->currentDirectoryAnd2CharaterBuffer);
		data->currentDirectoryAnd2CharaterBuffer = 0;
	}
	error = l6tkGetCurrentDirectory(data->heap, &data->currentDirectoryAnd2CharaterBuffer);
	if (error)
	{
		l6tkClose(data);
		return error;
	}
	error = l6tkListCurrentDirectory(data);
	if (error)
	{
		l6tkClose(data);
		return error;
	}
	l6tkSetSavedCurrentDirectory(data->currentDirectoryAnd2CharaterBuffer);
	return 0;
}

void l6tkClose(l6tkData* data)
{
	CloseHandle(data->consoleOut);
	CloseHandle(data->consoleIn);
	if (data->currentDirectoryAnd2CharaterBuffer)
		HeapFree(data->heap, 0, data->currentDirectoryAnd2CharaterBuffer);
	if (data->fileList)
		HeapFree(data->heap, 0, data->fileList);
}

DWORD l6tkConsoleInput(l6tkData* data)
{
	DWORD readLength;
	if (!ReadConsoleA(data->consoleIn, data->consoleInput, L6TK_INPUT_MAXIMUM_LEGTH - 1, &readLength, 0))
		return GetLastError();
	data->consoleInput[readLength] = 0;
	return 0;
}

DWORD l6tkDecodeInteger(const CHAR* integerString, DWORD* result)
{
	if (*integerString < '0' || *integerString > '9')
		return ERROR_INVALID_PARAMETER;
	DWORD integer = 0;
	for (CHAR c = *integerString++; c > '0' - 1 && c < '9' + 1; c = *integerString++)
		integer = integer * 10 + (DWORD)c - (DWORD)'0';
	*result = integer;
	return 0;
}

DWORD l6tkPrintAllDirectories(l6tkData* data)
{
	l6tkPrint Print[4] = { { L6TK_PRINT_TYPE_UNSIGNED_INTEGER, 0 },{ L6TK_PRINT_TYPE_STRING, " \"" },{ L6TK_PRINT_TYPE_NATIVE_STRING, 0 },{ L6TK_PRINT_TYPE_STRING, "\"\n" } };
	for (int c = 0, i = 0, e = (DWORD)data->fileListLength; i != e; ++i)
		if (data->fileList[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			Print[0].dataAddress = &c;
			Print[2].dataAddress = data->fileList[i].cFileName;
			DWORD error = l6tkPrintOut(data->consoleOut, 4, Print);
			if (error)
				return error;
			c++;
		}
	return 0;
}

DWORD l6tkSetCurrentDirectoryByIndex(l6tkData* data, SIZE_T index)
{
	for (int c = 0, i = 0, e = (DWORD)data->fileListLength; i != e; ++i)
		if (data->fileList[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ((SIZE_T)c == index)
			{
				SIZE_T oldCurrentDirectoryLength = (SIZE_T)lstrlenW(data->currentDirectoryAnd2CharaterBuffer);
				BOOL oldCurrentDirectoryLastComponentEndWithSlash = oldCurrentDirectoryLength && (data->currentDirectoryAnd2CharaterBuffer[oldCurrentDirectoryLength - 1] == L'\\' || data->currentDirectoryAnd2CharaterBuffer[oldCurrentDirectoryLength - 1] == L'/') ? TRUE : FALSE;
				SIZE_T newCurrentDirectoryNewComponentLength = (SIZE_T)lstrlenW(data->fileList[i].cFileName);
				WCHAR* newCurrentDirectory = (WCHAR*)HeapAlloc(data->heap, 0, (oldCurrentDirectoryLength + (oldCurrentDirectoryLastComponentEndWithSlash ? 0 : 1) + newCurrentDirectoryNewComponentLength + 1) * sizeof(WCHAR));
				if (!newCurrentDirectory)
					return GetLastError();
				CopyMemory(newCurrentDirectory, data->currentDirectoryAnd2CharaterBuffer, oldCurrentDirectoryLength * sizeof(WCHAR));
				if (oldCurrentDirectoryLastComponentEndWithSlash)
					CopyMemory(newCurrentDirectory + oldCurrentDirectoryLength, data->fileList[i].cFileName, (newCurrentDirectoryNewComponentLength + 1) * sizeof(WCHAR));
				else
				{
					newCurrentDirectory[oldCurrentDirectoryLength] = L'\\';
					CopyMemory(newCurrentDirectory + oldCurrentDirectoryLength + 1, data->fileList[i].cFileName, (newCurrentDirectoryNewComponentLength + 1) * sizeof(WCHAR));
				}
				if (!SetCurrentDirectoryW(newCurrentDirectory))
				{
					DWORD error = GetLastError();
					HeapFree(data->heap, 0, newCurrentDirectory);
					return error;
				}
				HeapFree(data->heap, 0, newCurrentDirectory);
				WCHAR* currentDirectory;
				DWORD error = l6tkGetCurrentDirectory(data->heap, &currentDirectory);
				if (error)
				{
					if (data->currentDirectoryAnd2CharaterBuffer)
						SetCurrentDirectoryW(data->currentDirectoryAnd2CharaterBuffer);
					return error;
				}
				WCHAR* temporal = data->currentDirectoryAnd2CharaterBuffer;
				data->currentDirectoryAnd2CharaterBuffer = currentDirectory;
				error = l6tkListCurrentDirectory(data);
				if (error)
				{
					data->currentDirectoryAnd2CharaterBuffer = temporal;
					if (data->currentDirectoryAnd2CharaterBuffer)
						SetCurrentDirectoryW(data->currentDirectoryAnd2CharaterBuffer);
					HeapFree(data->heap, 0, currentDirectory);
					return error;
				}
				if (temporal)
					HeapFree(data->heap, 0, temporal);
				l6tkSetSavedCurrentDirectory(currentDirectory);
				return 0;
			}
			c++;
		}
	return ERROR_FILE_NOT_FOUND;
}

DWORD l6tkGetCurrentDirectory(HANDLE heap, WCHAR** currentDirectoryAnd2CharaterBufferAddress)
{
	DWORD currentDirectoryCapasity = MAX_PATH + 3;
	WCHAR* currentDirectory = (WCHAR*)HeapAlloc(heap, 0, currentDirectoryCapasity * sizeof(WCHAR));
	for (;;)
	{
		DWORD currentDirectoryLength = GetCurrentDirectoryW(currentDirectoryCapasity - 2, currentDirectory);
		if (!currentDirectoryLength)
		{
			DWORD error = GetLastError();
			HeapFree(heap, 0, currentDirectory);
			return error;
		}
		else if (currentDirectoryLength > currentDirectoryCapasity - 2)
		{
			currentDirectoryCapasity = currentDirectoryLength + 2;
			WCHAR* temporal = (WCHAR*)HeapReAlloc(heap, 0, currentDirectory, currentDirectoryCapasity);
			if (!temporal)
			{
				DWORD error = GetLastError();
				HeapFree(heap, 0, currentDirectory);
				return error;
			}
			currentDirectory = temporal;
		}
		else
		{
			*currentDirectoryAnd2CharaterBufferAddress = currentDirectory;
			return 0;
		}
	}
}

DWORD l6tkPrintLine(HANDLE consoleOutput, WCHAR* String)
{
	DWORD written;
	return WriteConsoleW(consoleOutput, String, (DWORD)lstrlenW(String), &written, 0) && WriteConsoleW(consoleOutput, L"\n", 1, &written, 0) ? 0 : GetLastError();
}

DWORD l6tkGetSavedCurrentDirectory(HANDLE heap, WCHAR** currentDirectoryAnd2CharaterBufferAddress)
{
	DWORD valueBufferSize = (MAX_PATH + 3) * sizeof(WCHAR);
	WCHAR* valueBuffer = (WCHAR*)HeapAlloc(heap, 0, (SIZE_T)valueBufferSize);
	HKEY key;
	LONG status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Santtu Nyman", 0, KEY_QUERY_VALUE, &key);
	if (status)
	{
		HeapFree(heap, 0, valueBuffer);
		return (DWORD)status;
	}
	for (DWORD valueType, valueSize;;)
	{
		valueSize = valueBufferSize - (3 * sizeof(WCHAR));
		status = RegQueryValueExW(key, L"OAMK TVT17SPL Ohjelmoinnin Perusteet Lab 6 Tehtava K", 0, &valueType, (BYTE*)valueBuffer, &valueSize);
		if (status == ERROR_MORE_DATA)
		{
			valueBufferSize = valueSize + (3 * sizeof(WCHAR));
			WCHAR* Temporal = (WCHAR*)HeapReAlloc(heap, 0, valueBuffer, (SIZE_T)valueBufferSize);
			if (!Temporal)
			{
				DWORD Error = GetLastError();
				RegCloseKey(key);
				HeapFree(heap, 0, valueBuffer);
				return Error;
			}
			valueBuffer = Temporal;
		}
		else if (status)
		{
			RegCloseKey(key);
			HeapFree(heap, 0, valueBuffer);
			return (DWORD)status;
		}
		else if (valueType != REG_SZ || valueSize % sizeof(WCHAR))
		{
			RegCloseKey(key);
			HeapFree(heap, 0, valueBuffer);
			return ERROR_INVALID_DATA;
		}
		else
		{
			if (!valueSize)
				*valueBuffer = 0;
			else if (*(WCHAR*)((UINT_PTR)valueBuffer + (UINT_PTR)valueSize - sizeof(WCHAR)))
				*(WCHAR*)((UINT_PTR)valueBuffer + (UINT_PTR)valueSize) = 0;
			break;
		}
	}
	RegCloseKey(key);
	*currentDirectoryAnd2CharaterBufferAddress = valueBuffer;
	return 0;
}

DWORD l6tkSetSavedCurrentDirectory(const WCHAR* currentDirectory)
{
	DWORD currentDirectoryNameSize = ((DWORD)lstrlenW(currentDirectory) + 1) * sizeof(WCHAR);
	HKEY softwareSanttuNyman;
	LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Santtu Nyman", 0, 0, REG_OPTION_VOLATILE, KEY_SET_VALUE, 0, &softwareSanttuNyman, 0);
	if (!result)
	{
		result = RegSetValueExW(softwareSanttuNyman, L"OAMK TVT17SPL Ohjelmoinnin Perusteet Lab 6 Tehtava K", 0, REG_SZ, (const BYTE*)currentDirectory, currentDirectoryNameSize);
		RegCloseKey(softwareSanttuNyman);
	}
	return (DWORD)result;
}

#endif