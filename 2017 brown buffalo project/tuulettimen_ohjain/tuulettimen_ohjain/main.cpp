#ifdef __cplusplus
extern "C" {
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "brown_buffalo_controller.h"

// tätä ei käytetä mihinkään visual studio tarvitsee sen kun crt:tä ei käytetä
extern "C" int _fltused = 0;

BOOL searchArgument(SIZE_T argumentCount, const WCHAR** argumentValues, const WCHAR* shortArgument, const WCHAR* longArgument, const WCHAR** value)
{
	for (SIZE_T index = 0; index != argumentCount; ++index)
		if ((shortArgument && !lstrcmpW(shortArgument, argumentValues[index])) || (longArgument && !lstrcmpW(longArgument, argumentValues[index])))
		{
			if (value)
				*value = index + 1 != argumentCount ? argumentValues[index + 1] : 0;
			return TRUE;
		}
	if (value)
		*value = 0;
	return FALSE;
}

void main()
{
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HANDLE consoleOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	HANDLE heap = GetProcessHeap();
	if (!heap)
	{
		error = GetLastError();
		if (consoleOut != INVALID_HANDLE_VALUE)
		{
			print(consoleOut, L"Unable to start program. ");
			printError(error);
			CloseHandle(consoleOut);
		}
		ExitProcess((UINT)error);
	}
	HMODULE shell32 = LoadLibraryExW(L"Shell32.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!shell32)
	{
		error = GetLastError();
		if (consoleOut != INVALID_HANDLE_VALUE)
		{
			print(consoleOut, L"Unable to start program. ");
			printError(error);
			CloseHandle(consoleOut);
		}
		ExitProcess((UINT)error);
	}
	SIZE_T argumentCount = 0;
	const WCHAR** argumentValues = ((const WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(shell32, "CommandLineToArgvW"))(GetCommandLineW(), (int*)&argumentCount);
	if (!argumentValues)
	{
		error = GetLastError();
		FreeLibrary(shell32);
		if (consoleOut != INVALID_HANDLE_VALUE)
		{
			print(consoleOut, L"Unable to start program. ");
			printError(error);
			CloseHandle(consoleOut);
		}
		ExitProcess((UINT)error);
	}
	WCHAR* IPServerAddressString;
	if (getLastUsedIPAddress(heap, &IPServerAddressString))
		IPServerAddressString = 0;
	const WCHAR* addressArgument;
	searchArgument(argumentCount, argumentValues, L"-a", L"--address", &addressArgument);
	BOOL statusArgumentIsSet = searchArgument(argumentCount, argumentValues, L"-g", L"--get_fan_status", 0);
	const WCHAR* speedArgument;
	BOOL speedArgumentIsSet = searchArgument(argumentCount, argumentValues, L"-f", L"--fan_speed", &speedArgument);
	BOOL wakeArgumentIsSet = searchArgument(argumentCount, argumentValues, L"-w", L"--wake", 0);
	BOOL sleepArgumentIsSet = searchArgument(argumentCount, argumentValues, L"-s", L"--sleep", 0);
	const WCHAR* minArgument;
	searchArgument(argumentCount, argumentValues, L"-n", L"--min", &minArgument);
	const WCHAR* maxArgument;
	searchArgument(argumentCount, argumentValues, L"-x", L"--max", &maxArgument);
	if (!statusArgumentIsSet && !speedArgumentIsSet && !wakeArgumentIsSet && !sleepArgumentIsSet && !minArgument && !maxArgument)
		statusArgumentIsSet = TRUE;
	if ((!addressArgument && !IPServerAddressString) || (statusArgumentIsSet && (speedArgumentIsSet || wakeArgumentIsSet || sleepArgumentIsSet || minArgument || maxArgument)) || (((speedArgumentIsSet ? 1 : 0) + (wakeArgumentIsSet ? 1 : 0) + (sleepArgumentIsSet ? 1 : 0)) > 1))
	{
		LocalFree(argumentValues);
		FreeLibrary(shell32);
		if (consoleOut != INVALID_HANDLE_VALUE)
		{
			print(consoleOut, L"Arguments passed to program are invalid. ");
			printError(ERROR_BAD_ARGUMENTS);
			CloseHandle(consoleOut);
		}
		ExitProcess(ERROR_BAD_ARGUMENTS);
	}
	if (addressArgument)
	{
		if (IPServerAddressString)
			HeapFree(heap, 0, IPServerAddressString);
		SIZE_T IPServerAddressStringSize = (lstrlenW(addressArgument) + 1) * sizeof(WCHAR);
		IPServerAddressString = (WCHAR*)HeapAlloc(heap, 0, IPServerAddressStringSize);
		if (!IPServerAddressString)
		{
			LocalFree(argumentValues);
			FreeLibrary(shell32);
			error = GetLastError();
			if (consoleOut != INVALID_HANDLE_VALUE)
			{
				print(consoleOut, L"Memory allocation failed. ");
				printError(error);
				CloseHandle(consoleOut);
			}
			ExitProcess((UINT)error);
		}
		copy(addressArgument, IPServerAddressString, IPServerAddressStringSize);
	}
	BOOL getStatus = statusArgumentIsSet ? TRUE : FALSE;
	BYTE setFanSpeed = 0xFF;
	float lowLimit = -1.0f;
	float highLimit = -1.0f;
	if (!getStatus)
	{
		if (speedArgumentIsSet)
		{
			DWORD tempralSetFanSpeed;
			if (!readIntegerU32(speedArgument, &tempralSetFanSpeed) || tempralSetFanSpeed > 3)
			{
				HeapFree(heap, 0, IPServerAddressString);
				LocalFree(argumentValues);
				FreeLibrary(shell32);
				if (consoleOut != INVALID_HANDLE_VALUE)
				{
					print(consoleOut, L"Arguments passed to program are invalid. ");
					printError(ERROR_BAD_ARGUMENTS);
					CloseHandle(consoleOut);
				}
				ExitProcess(ERROR_BAD_ARGUMENTS);
			}
			setFanSpeed = (BYTE)tempralSetFanSpeed;
		}
		else if (wakeArgumentIsSet)
			setFanSpeed = 1;
		else if (sleepArgumentIsSet)
			setFanSpeed = 0;
		if (minArgument)
		{
			if (!readFloat(minArgument, &lowLimit))
			{
				HeapFree(heap, 0, IPServerAddressString);
				LocalFree(argumentValues);
				FreeLibrary(shell32);
				if (consoleOut != INVALID_HANDLE_VALUE)
				{
					print(consoleOut, L"Arguments passed to program are invalid. ");
					printError(ERROR_BAD_ARGUMENTS);
					CloseHandle(consoleOut);
				}
				ExitProcess(ERROR_BAD_ARGUMENTS);
			}
		}
		if (maxArgument)
		{
			if (!readFloat(maxArgument, &highLimit))
			{
				HeapFree(heap, 0, IPServerAddressString);
				LocalFree(argumentValues);
				FreeLibrary(shell32);
				if (consoleOut != INVALID_HANDLE_VALUE)
				{
					print(consoleOut, L"Arguments passed to program are invalid. ");
					printError(ERROR_BAD_ARGUMENTS);
					CloseHandle(consoleOut);
				}
				ExitProcess(ERROR_BAD_ARGUMENTS);
			}
		}
	}
	LocalFree(argumentValues);
	FreeLibrary(shell32);
	fanStatusData status;
	if (getStatus)
	{
		error = getFanStatus(IPServerAddressString, 0x10, FAN_STATUS_MASK_TEMPERATURE | FAN_STATUS_MASK_SPEED | FAN_STATUS_MASK_LOW | FAN_STATUS_MASK_HIGH, &status);
		if (error)
		{
			HeapFree(heap, 0, IPServerAddressString);
			if (consoleOut != INVALID_HANDLE_VALUE)
			{
				print(consoleOut, L"reading fan status from device failed. ");
				printError(error);
				CloseHandle(consoleOut);
			}
			ExitProcess((UINT)error);
		}
		WCHAR* statusMessageBuffer = (WCHAR*)HeapAlloc(heap, 0, 0x100 * sizeof(WCHAR));
		if (!statusMessageBuffer)
		{
			error = GetLastError();
			HeapFree(heap, 0, IPServerAddressString);
			if (consoleOut != INVALID_HANDLE_VALUE)
			{
				print(consoleOut, L"Memory allocation failed. ");
				printError(error);
				CloseHandle(consoleOut);
			}
			ExitProcess((UINT)error);
		}
		printFanStatus(&status, statusMessageBuffer);
		print(consoleOut, statusMessageBuffer);
		HeapFree(heap, 0, statusMessageBuffer);
	}
	else
	{
		WORD statusSetMask = 0;
		if (setFanSpeed != 0xFF)
		{
			status.fanSpeed = setFanSpeed;
			statusSetMask |= FAN_STATUS_MASK_SPEED;
		}
		if (!(lowLimit < 0.0f))
		{
			if (!(highLimit < 0.0f) && lowLimit > highLimit)
			{
				HeapFree(heap, 0, IPServerAddressString);
				if (consoleOut != INVALID_HANDLE_VALUE)
				{
					print(consoleOut, L"Max limit is lower than min limit. ");
					printError(ERROR_BAD_ARGUMENTS);
					CloseHandle(consoleOut);
				}
				ExitProcess(ERROR_BAD_ARGUMENTS);
			}
			status.lowLimit = lowLimit;
			statusSetMask |= FAN_STATUS_MASK_LOW;
		}
		if (!(highLimit < 0.0f))
		{
			if (!(lowLimit < 0.0f) && highLimit < lowLimit)
			{
				HeapFree(heap, 0, IPServerAddressString);
				if (consoleOut != INVALID_HANDLE_VALUE)
				{
					print(consoleOut, L"Max limit is lower than min limit. ");
					printError(ERROR_BAD_ARGUMENTS);
					CloseHandle(consoleOut);
				}
				ExitProcess(ERROR_BAD_ARGUMENTS);
			}
			status.highLimit = highLimit;
			statusSetMask |= FAN_STATUS_MASK_HIGH;
		}
		if (statusSetMask)
		{
			error = setFanStatus(IPServerAddressString, 0x10, statusSetMask, &status);
			if (error)
			{
				HeapFree(heap, 0, IPServerAddressString);
				if (consoleOut != INVALID_HANDLE_VALUE)
				{
					print(consoleOut, L"Setting fan status failed. ");
					printError(error);
					CloseHandle(consoleOut);
				}
				ExitProcess((UINT)error);
			}
		}
	}
	HeapFree(heap, 0, IPServerAddressString);
	if (consoleOut != INVALID_HANDLE_VALUE)
		CloseHandle(consoleOut);
	ExitProcess(0);
}

#ifdef __cplusplus
}
#endif