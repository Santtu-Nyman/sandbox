/*
	Copyright (C) 2025 Santtu S. Nyman

	MultiClipboard is a program that allows it's user to have multiple clipboards.
	The user is able to switch between these clipboards using hot keys.

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

#define WINVER 0x0A00 // _WIN32_WINNT_WIN10
#define _WIN32_WINNT 0x0A00 // _WIN32_WINNT_WIN10
#ifndef CINTERFACE
#define CINTERFACE
#endif // CINTERFACE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <assert.h>
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif // EXIT_SUCCESS
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif // EXIT_SUCCESS

#if defined(_MSC_VER)
#if !defined(_WIN32) || (!defined(_M_IX86) && !defined(_M_X64))
#error Invalid build configuration. Invalid target platform.
#endif
#if defined(_M_IX86_FP) && (_M_IX86_FP < 2)
#error Invalid build configuration. Compile to SSE2 or later x86 architecture. Set /arch compiler option.
#endif
#if defined(_MANAGED) || defined(__cplusplus_cli)
#error Invalid build configuration. Remove managed code common language runtime. Remove /clr compiler option.
#endif
#if defined(_DLL)
#error Invalid build configuration. Remove dynamically linked CRT, link CRT statically. Add /MT or /MTd compiler option.
#endif
#if !defined(_DEBUG)
#if defined(__MSVC_RUNTIME_CHECKS)
#error Invalid build configuration. Remove runtime error checks. Remove /RTC compiler option.
#endif
#if defined(_CONTROL_FLOW_GUARD)
#error Invalid build configuration. Remove runtime control flow checks. Remove /guard:cf compiler option.
#endif
#if defined(_M_FP_EXCEPT)
#error Invalid build configuration. Remove floating-point math exceptions. Add /fp:except- compiler option.
#endif
#if defined(_CPPRTTI)
#error Invalid build configuration. Remove C++ runtime type information. Add /GR- compiler option.
#endif
#if defined(_CPPUNWIND)
#error Invalid build configuration. Remove C++ exceptions. Remove /clr and /EH compiler options.
#endif
#endif
#endif

#define DEFAULT_ALINGMENT (16)
#define ALIGN_SIZE(N, A) (((N) + ((A) - 1)) & ~((A) - 1))

#define MAX_CLIPBOARD_COUNT 256

#define TOTAL_BUFFER_MEMORY_SPACE 0x80000000
#define CLIP_SLOT_BUFFER_SIZE 0x10000
#define CLIP_HEAP_MINIMUM_COMMIT_SIZE 0x100000

typedef struct ClipboardEntryT
{
	DWORD VirtualKeyCode;
	SIZE_T SavedDataSize;
	void* SavedDataBuffer;
	SIZE_T HeapSaveSize;
	void* FastSaveSlot;
} ClipboardEntryT;

typedef struct MultiClipboardStateT
{
	HHOOK LowLevelKeyboardHookHandle;
	SIZE_T CurrentClipboardIndex;
	SIZE_T PageSize;
	HANDLE LockFileHandle;
	HANDLE ProcessHandle;
	HANDLE MainThreadHandle;
	DWORD ProcessId;
	DWORD MainThreadId;
	SIZE_T ClipboardCount;
	ClipboardEntryT* ClipboardTable;
	SIZE_T ClipSlotBufferSize;
	void* ClipSlotBufferBase;
	SIZE_T ClipHeapReservedSize;
	SIZE_T ClipHeapCommitedSize;
	SIZE_T ClipHeapUsedSize;
	void* ClipHeapBufferBase;
} MultiClipboardStateT;

HRESULT SetupProcessEnvironment(void);

HRESULT CreateMultiClipboard(MultiClipboardStateT** MultiClipboardP);

void DeleteMultiClipboard(MultiClipboardStateT* MultiClipboard);

HRESULT ProcessMultiClipboard(MultiClipboardStateT* MultiClipboard);

void* ClipHeapAllocate(MultiClipboardStateT* MultiClipboard, SIZE_T Size);

void ClipHeapFree(MultiClipboardStateT* MultiClipboard, SIZE_T Size, void* Allocation);

LRESULT CALLBACK LowLevelKeyboardHookProcedure(int Code, WPARAM WParameter, LPARAM LParameter);

MultiClipboardStateT* g_MultiClipboard;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	HRESULT Result = SetupProcessEnvironment();
	if (FAILED(Result))
	{
		return EXIT_FAILURE;
	}
	MultiClipboardStateT* MultiClipboard = NULL;
	Result = CreateMultiClipboard(&MultiClipboard);
	if (FAILED(Result))
	{
		return EXIT_FAILURE;
	}
	Result = ProcessMultiClipboard(MultiClipboard);
	DeleteMultiClipboard(MultiClipboard);
	return SUCCEEDED(Result) ? EXIT_SUCCESS : EXIT_FAILURE;
}

HRESULT SetupProcessEnvironment(void)
{
	HANDLE CurrentProcessPseudoHandle = GetCurrentProcess();
	HANDLE CurrentThreadPseudoHandle = GetCurrentThread();

	PROCESS_MEMORY_EXHAUSTION_INFO MemoryExhaustionInformation;
	ZeroMemory(&MemoryExhaustionInformation, sizeof(PROCESS_MEMORY_EXHAUSTION_INFO));
	MemoryExhaustionInformation.Version = 1;
	MemoryExhaustionInformation.Reserved = 0;
	MemoryExhaustionInformation.Type = PMETypeFailFastOnCommitFailure;
	MemoryExhaustionInformation.Value = PME_FAILFAST_ON_COMMIT_FAIL_DISABLE;
	SetProcessInformation(CurrentProcessPseudoHandle, ProcessMemoryExhaustionInfo, &MemoryExhaustionInformation, sizeof(PROCESS_MEMORY_EXHAUSTION_INFO));

	MEMORY_PRIORITY_INFORMATION MemoryPriorityInformation;
	ZeroMemory(&MemoryPriorityInformation, sizeof(MEMORY_PRIORITY_INFORMATION));
	MemoryPriorityInformation.MemoryPriority = MEMORY_PRIORITY_NORMAL;
	SetProcessInformation(CurrentProcessPseudoHandle, ProcessMemoryPriority, &MemoryPriorityInformation, sizeof(MEMORY_PRIORITY_INFORMATION));

	PROCESS_POWER_THROTTLING_STATE PowerThrottlingState;
	ZeroMemory(&PowerThrottlingState, sizeof(PROCESS_POWER_THROTTLING_STATE));
	PowerThrottlingState.Version = 1;
	PowerThrottlingState.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
	PowerThrottlingState.StateMask = 0; // Off
	SetProcessInformation(CurrentProcessPseudoHandle, ProcessPowerThrottling, &PowerThrottlingState, sizeof(PROCESS_POWER_THROTTLING_STATE));

	SetProcessPreferredUILanguages(0, 0, 0);
	SetThreadLocale(LOCALE_INVARIANT);
	SetThreadPreferredUILanguages(0, L"\0", 0);
	SetThreadErrorMode(0, 0);

	SetPriorityClass(CurrentProcessPseudoHandle, NORMAL_PRIORITY_CLASS);
	SetThreadPriority(CurrentThreadPseudoHandle, THREAD_PRIORITY_NORMAL);
	SetThreadPriorityBoost(CurrentThreadPseudoHandle, FALSE);

	SetLastError(ERROR_SUCCESS);
	return S_OK;
}

HRESULT CreateMultiClipboard(MultiClipboardStateT** MultiClipboardP)
{
	HRESULT Result = S_OK;

	SYSTEM_INFO SystemInfo;

	HANDLE CurrentProcessPseudoHandle = GetCurrentProcess();
	HANDLE CurrentThreadPseudoHandle = GetCurrentThread();

	ZeroMemory(&SystemInfo, sizeof(SYSTEM_INFO));
	GetSystemInfo(&SystemInfo);

	assert(CLIP_SLOT_BUFFER_SIZE >= 0x10000 && CLIP_SLOT_BUFFER_SIZE >= ((UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR)));
	SIZE_T ClipSlotSize = ALIGN_SIZE(ALIGN_SIZE((SIZE_T)CLIP_SLOT_BUFFER_SIZE, DEFAULT_ALINGMENT), (SIZE_T)SystemInfo.dwPageSize);
	SIZE_T StructureHeaderSize = ALIGN_SIZE(ALIGN_SIZE(sizeof(MultiClipboardStateT), DEFAULT_ALINGMENT) + ALIGN_SIZE((MAX_CLIPBOARD_COUNT * sizeof(ClipboardEntryT)), DEFAULT_ALINGMENT), (SIZE_T)SystemInfo.dwPageSize);
	SIZE_T ClipSlotTableSize = ClipSlotSize;
	SIZE_T InitAllocationCommitSize = StructureHeaderSize + ClipSlotTableSize;
	SIZE_T AllocationReserveSize = ALIGN_SIZE(ALIGN_SIZE((SIZE_T)TOTAL_BUFFER_MEMORY_SPACE, DEFAULT_ALINGMENT), (SIZE_T)SystemInfo.dwPageSize);
	assert(AllocationReserveSize >= InitAllocationCommitSize);
	MultiClipboardStateT* MultiClipboard = (MultiClipboardStateT*)VirtualAlloc(NULL, AllocationReserveSize, MEM_RESERVE, PAGE_NOACCESS);
	Result = MultiClipboard ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Memory allocation failed.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		*MultiClipboardP = NULL;
		return Result;
	}
	Result = VirtualAlloc(MultiClipboard, InitAllocationCommitSize, MEM_COMMIT, PAGE_READWRITE) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Memory allocation failed.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		*MultiClipboardP = NULL;
		return Result;
	}
	MultiClipboard->LowLevelKeyboardHookHandle = NULL;
	MultiClipboard->CurrentClipboardIndex = 0; // Zero is the default clipboard
	MultiClipboard->PageSize = (SIZE_T)SystemInfo.dwPageSize;
	MultiClipboard->ClipboardTable = (ClipboardEntryT*)((UINT_PTR)MultiClipboard + ALIGN_SIZE(sizeof(MultiClipboardStateT), DEFAULT_ALINGMENT));
	MultiClipboard->ClipSlotBufferBase = (void*)((UINT_PTR)MultiClipboard + StructureHeaderSize);

	Result = DuplicateHandle(CurrentProcessPseudoHandle, CurrentProcessPseudoHandle, CurrentProcessPseudoHandle, &MultiClipboard->ProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Process handle creation failed.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		MultiClipboard->ProcessHandle = NULL;
		goto cleanup;
	}
	MultiClipboard->ProcessId = GetProcessId(MultiClipboard->ProcessHandle);

	Result = DuplicateHandle(CurrentProcessPseudoHandle, CurrentThreadPseudoHandle, CurrentProcessPseudoHandle, &MultiClipboard->MainThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Thread handle creation failed.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		MultiClipboard->MainThreadHandle = NULL;
		goto cleanup;
	}
	MultiClipboard->MainThreadId = GetThreadId(MultiClipboard->MainThreadHandle);

	MultiClipboard->ClipboardCount = 0;
	HKEY ClipboardListKeyHandle = NULL;
	Result = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MultiClipboard\\ClipboardList", 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ClipboardListKeyHandle));
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Failed to open settings registry key \"HKEY_CURRENT_USER\\Software\\MultiClipboard\\ClipboardList\".", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		goto cleanup;
	}
	Result = S_OK;
	for (DWORD i = 0; SUCCEEDED(Result); i++)
	{
		DWORD ClipboardKeyNameLength = UNICODE_STRING_MAX_CHARS + 1;
		WCHAR* ClipboardKeyName = (WCHAR*)MultiClipboard->ClipSlotBufferBase;
		Result = HRESULT_FROM_WIN32(RegEnumKeyExW(ClipboardListKeyHandle, i, ClipboardKeyName, &ClipboardKeyNameLength, NULL, NULL, NULL, NULL));
		if (SUCCEEDED(Result))
		{
			HKEY ClipboardKeyHandle = NULL;
			Result = HRESULT_FROM_WIN32(RegOpenKeyExW(ClipboardListKeyHandle, ClipboardKeyName, 0, KEY_QUERY_VALUE, &ClipboardKeyHandle));
			if (FAILED(Result))
			{
				MessageBoxW(NULL, L"Failed to start MultiClipboard. Failed to open settings registry subkey of \"HKEY_CURRENT_USER\\Software\\MultiClipboard\\ClipboardList\".", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
				RegCloseKey(ClipboardListKeyHandle);
				goto cleanup;
			}

			DWORD DefaultRegType = REG_NONE;
			DWORD DefaultRegDataSize = sizeof(BOOL);
			BOOL Default = FALSE;
			Result = HRESULT_FROM_WIN32(RegQueryValueExW(ClipboardKeyHandle, L"Default", NULL, &DefaultRegType, (BYTE*)&Default, &DefaultRegDataSize));
			if (SUCCEEDED(Result) && (DefaultRegType != REG_DWORD || DefaultRegDataSize != sizeof(DWORD)))
			{
				Result = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
			}
			if (FAILED(Result))
			{
				Default = FALSE;
			}

			DWORD VirtualKeyCodeRegType = REG_NONE;
			DWORD VirtualKeyCodeRegDataSize = sizeof(DWORD);
			DWORD VirtualKeyCode = 0;
			Result = HRESULT_FROM_WIN32(RegQueryValueExW(ClipboardKeyHandle, L"VirtualKeyCode", NULL, &VirtualKeyCodeRegType, (BYTE*)&VirtualKeyCode, &VirtualKeyCodeRegDataSize));
			if (SUCCEEDED(Result) && (VirtualKeyCodeRegType != REG_DWORD || VirtualKeyCodeRegType != sizeof(DWORD)))
			{
				Result = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
			}
			RegCloseKey(ClipboardKeyHandle);
			if (FAILED(Result))
			{
				MessageBoxW(NULL, L"Failed to start MultiClipboard. Failed to read DWORD value \"VirtualKeyCode\" from settings registry subkey of \"HKEY_CURRENT_USER\\Software\\MultiClipboard\\ClipboardList\".", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
				RegCloseKey(ClipboardListKeyHandle);
				goto cleanup;
			}

			if (MultiClipboard->ClipboardCount == MAX_CLIPBOARD_COUNT)
			{
				MessageBoxW(NULL, L"Failed to start MultiClipboard. Too many clipboards defined in settings registry key \"HKEY_CURRENT_USER\\Software\\MultiClipboard\\ClipboardList\".", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
				Result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
				RegCloseKey(ClipboardListKeyHandle);
				goto cleanup;
			}
			if (Default)
			{
				if (MultiClipboard->ClipboardCount)
				{
					CopyMemory(&MultiClipboard->ClipboardTable[MultiClipboard->ClipboardCount], &MultiClipboard->ClipboardTable[0], sizeof(ClipboardEntryT));
				}
				ZeroMemory(&MultiClipboard->ClipboardTable[0], sizeof(ClipboardEntryT));
				MultiClipboard->ClipboardTable[0].VirtualKeyCode = VirtualKeyCode;
			}
			else
			{
				ZeroMemory(&MultiClipboard->ClipboardTable[MultiClipboard->ClipboardCount], sizeof(ClipboardEntryT));
				MultiClipboard->ClipboardTable[MultiClipboard->ClipboardCount].VirtualKeyCode = VirtualKeyCode;
			}
			MultiClipboard->ClipboardCount++;
		}
	}
	if (Result == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
	{
		Result = MultiClipboard->ClipboardCount ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_DATA);
	}
	RegCloseKey(ClipboardListKeyHandle);
	if (FAILED(Result))
	{
		if (Result == HRESULT_FROM_WIN32(ERROR_NO_DATA) && !MultiClipboard->ClipboardCount)
		{
			MessageBoxW(NULL, L"Failed to start MultiClipboard. Clipboard setting subkeys are missing from \"HKEY_CURRENT_USER\\Software\\MultiClipboard\\ClipboardList\". For each clipboard add one subkey and in the subkey add DWORD value \"VirtualKeyCode\" that defines the hotkey to activate that clipboard.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			MessageBoxW(NULL, L"Failed to start MultiClipboard. Failed to read setting from registry key \"HKEY_CURRENT_USER\\Software\\MultiClipboard\\ClipboardList\".", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		}
		goto cleanup;
	}

	ZeroMemory(MultiClipboard->ClipSlotBufferBase, ClipSlotSize);

	StructureHeaderSize = ALIGN_SIZE(ALIGN_SIZE(sizeof(MultiClipboardStateT), DEFAULT_ALINGMENT) + ALIGN_SIZE((MultiClipboard->ClipboardCount * sizeof(ClipboardEntryT)), DEFAULT_ALINGMENT), (SIZE_T)SystemInfo.dwPageSize);
	ClipSlotTableSize = MultiClipboard->ClipboardCount * ClipSlotSize;
	SIZE_T ClipHeapMinimumCommitSize = ALIGN_SIZE((SIZE_T)CLIP_HEAP_MINIMUM_COMMIT_SIZE, (SIZE_T)SystemInfo.dwPageSize);
	SIZE_T AllocationCommitSize = StructureHeaderSize + ClipSlotTableSize + ClipHeapMinimumCommitSize;
	if (AllocationCommitSize > InitAllocationCommitSize)
	{
		SIZE_T AllocationCommitExpansionSize = AllocationCommitSize - InitAllocationCommitSize;
		Result = VirtualAlloc((void*)((UINT_PTR)MultiClipboard + InitAllocationCommitSize), AllocationCommitExpansionSize, MEM_COMMIT, PAGE_READWRITE) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
		if (FAILED(Result))
		{
			MessageBoxW(NULL, L"Failed to start MultiClipboard. Memory allocation failed.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}
	else if (AllocationCommitSize < InitAllocationCommitSize)
	{
		SIZE_T AllocationCommitRemoveSize = InitAllocationCommitSize - AllocationCommitSize;
		Result = VirtualFree((void*)((UINT_PTR)MultiClipboard + InitAllocationCommitSize), AllocationCommitRemoveSize, MEM_DECOMMIT) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
		if (FAILED(Result))
		{
			MessageBoxW(NULL, L"Failed to start MultiClipboard. Memory allocation failed.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}

	MultiClipboard->ClipSlotBufferBase = (void*)((UINT_PTR)MultiClipboard + StructureHeaderSize);

	MultiClipboard->ClipHeapReservedSize = (SIZE_T)TOTAL_BUFFER_MEMORY_SPACE - (StructureHeaderSize + ClipSlotTableSize);
	MultiClipboard->ClipHeapCommitedSize = ClipHeapMinimumCommitSize;
	MultiClipboard->ClipHeapUsedSize = 0;
	MultiClipboard->ClipHeapBufferBase = (void*)((UINT_PTR)MultiClipboard + StructureHeaderSize + ClipSlotTableSize);

	for (SIZE_T n = MultiClipboard->ClipboardCount, i = 0; i < n; i++)
	{
		MultiClipboard->ClipboardTable[i].SavedDataSize = 0;
		MultiClipboard->ClipboardTable[i].SavedDataBuffer = NULL;
		MultiClipboard->ClipboardTable[i].HeapSaveSize = 0;
		MultiClipboard->ClipboardTable[i].FastSaveSlot = (void*)((UINT_PTR)MultiClipboard + StructureHeaderSize + (i * ClipSlotSize));
	}

	WCHAR* Path = (WCHAR*)MultiClipboard->ClipSlotBufferBase;
	SIZE_T PathLength = (SIZE_T)GetEnvironmentVariableW(L"LOCALAPPDATA", Path, UNICODE_STRING_MAX_CHARS + 1);
	Result = (PathLength && PathLength <= UNICODE_STRING_MAX_CHARS) ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_DATA);
	if (FAILED(Result))
	{
		PathLength = (SIZE_T)GetEnvironmentVariableW(L"USERPROFILE", Path, UNICODE_STRING_MAX_CHARS + 1);
		Result = (PathLength && PathLength <= UNICODE_STRING_MAX_CHARS) ? S_OK : HRESULT_FROM_WIN32(ERROR_NO_DATA);
	}
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Failed to read \"LOCALAPPDATA\" environment variable.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		goto cleanup;
	}
	if (PathLength && (Path[PathLength - 1] == L'\\' || Path[PathLength - 1] == L'/'))
	{
		PathLength--;
		Path[PathLength] = 0;
	}
	Result = ((PathLength + 24) <= UNICODE_STRING_MAX_CHARS) ? S_OK : HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"Failed to start MultiClipboard. Directory path of \"LOCALAPPDATA\" is too long.", L"MultiClipboard Error", MB_OK | MB_ICONERROR);
		goto cleanup;
	}
	CreateDirectoryW(Path, NULL);
	CopyMemory(Path + PathLength, L"\\MultiClipboard", (15 + 1) * sizeof(WCHAR));
	CreateDirectoryW(Path, NULL);
	CopyMemory(Path + PathLength + 15, L"\\Lock.dat", (9 + 1) * sizeof(WCHAR));

	MultiClipboard->LockFileHandle = CreateFileW(Path, DELETE | SYNCHRONIZE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
	Result = (MultiClipboard->LockFileHandle != INVALID_HANDLE_VALUE) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(Result))
	{
		MessageBoxW(NULL, L"MultiClipboard is already running.", L"MultiClipboard", MB_OK | MB_ICONINFORMATION);
		goto cleanup;
	}
	ZeroMemory(MultiClipboard->ClipSlotBufferBase, ClipSlotSize);

	*MultiClipboardP = MultiClipboard;
	return Result;
cleanup:

	if (MultiClipboard->LockFileHandle)
	{
		CloseHandle(MultiClipboard->LockFileHandle);
	}
	if (MultiClipboard->MainThreadHandle)
	{
		CloseHandle(MultiClipboard->MainThreadHandle);
	}
	if (MultiClipboard->ProcessHandle)
	{
		CloseHandle(MultiClipboard->ProcessHandle);
	}
	VirtualFree(MultiClipboard, 0, MEM_RELEASE);
	*MultiClipboardP = NULL;
	return Result;
}

void DeleteMultiClipboard(MultiClipboardStateT* MultiClipboard)
{
	CloseHandle(MultiClipboard->LockFileHandle);
	CloseHandle(MultiClipboard->MainThreadHandle);
	CloseHandle(MultiClipboard->ProcessHandle);
	VirtualFree(MultiClipboard, 0, MEM_RELEASE);
}

HRESULT ProcessMultiClipboard(MultiClipboardStateT* MultiClipboard)
{
	if (g_MultiClipboard)
	{
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	}
	g_MultiClipboard = MultiClipboard;

	DWORD OriginalPriorityClass = GetPriorityClass(MultiClipboard->ProcessHandle);
	int OriginalThreadPriority = GetThreadPriority(MultiClipboard->MainThreadHandle);
	if (OriginalPriorityClass != REALTIME_PRIORITY_CLASS)
	{
		if (!SetPriorityClass(MultiClipboard->ProcessHandle, REALTIME_PRIORITY_CLASS))
		{
			SetPriorityClass(MultiClipboard->ProcessHandle, HIGH_PRIORITY_CLASS);
		}
	}
	if (OriginalThreadPriority != THREAD_PRIORITY_TIME_CRITICAL)
	{
		if (!SetThreadPriority(MultiClipboard->MainThreadHandle, THREAD_PRIORITY_TIME_CRITICAL))
		{
			SetThreadPriority(MultiClipboard->MainThreadHandle, THREAD_PRIORITY_HIGHEST);
		}
	}
	
	for (;;)
	{
		MultiClipboard->LowLevelKeyboardHookHandle = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardHookProcedure, NULL, 0);
		MSG Message;
		ZeroMemory(&Message, sizeof(MSG));
		while (GetMessageW(&Message, NULL, 0, 0))
		{
			TranslateMessage(&Message);
			DispatchMessageW(&Message);
		}
		UnhookWindowsHookEx(MultiClipboard->LowLevelKeyboardHookHandle);
		MultiClipboard->LowLevelKeyboardHookHandle = NULL;
	}

	SetThreadPriority(MultiClipboard->MainThreadHandle, OriginalThreadPriority);
	SetPriorityClass(MultiClipboard->ProcessHandle, OriginalPriorityClass);

	g_MultiClipboard = NULL;
	return  HRESULT_FROM_WIN32(ERROR_SUCCESS);
}

void* ClipHeapAllocate(MultiClipboardStateT* MultiClipboard, SIZE_T Size)
{
	Size = ALIGN_SIZE(Size, MultiClipboard->PageSize);
	SIZE_T TotalRemaining = MultiClipboard->ClipHeapReservedSize - MultiClipboard->ClipHeapUsedSize;
	if (Size > TotalRemaining)
	{
		return NULL;
	}
	void* ClipHeapTop = (void*)((UINT_PTR)MultiClipboard->ClipHeapBufferBase + MultiClipboard->ClipHeapUsedSize);
	SIZE_T CommitRemaining = MultiClipboard->ClipHeapCommitedSize - MultiClipboard->ClipHeapUsedSize;
	if (Size <= CommitRemaining)
	{
		MultiClipboard->ClipHeapUsedSize += Size;
		return ClipHeapTop;
	}
	SIZE_T CommitExtensionSize = Size - CommitRemaining;
	if (!VirtualAlloc((void*)((UINT_PTR)MultiClipboard->ClipHeapBufferBase + MultiClipboard->ClipHeapCommitedSize), CommitExtensionSize, MEM_COMMIT, PAGE_READWRITE))
	{
		return NULL;
	}
	MultiClipboard->ClipHeapCommitedSize += CommitExtensionSize;
	MultiClipboard->ClipHeapUsedSize += Size;
	return ClipHeapTop;
}

void ClipHeapFree(MultiClipboardStateT* MultiClipboard, SIZE_T Size, void* Allocation)
{
	Size = ALIGN_SIZE(Size, MultiClipboard->PageSize);
	SIZE_T Offset = (SIZE_T)((UINT_PTR)Allocation - (UINT_PTR)MultiClipboard->ClipHeapBufferBase);
	if (Offset + Size < MultiClipboard->ClipHeapUsedSize)
	{
		memmove(Allocation, (void*)((UINT_PTR)Allocation + Size), MultiClipboard->ClipHeapUsedSize - (Offset + Size));
		for (SIZE_T n = MultiClipboard->ClipboardCount, i = 0; i < n; i++)
		{
			if (MultiClipboard->ClipboardTable[i].HeapSaveSize && (UINT_PTR)MultiClipboard->ClipboardTable[i].SavedDataBuffer > (UINT_PTR)Allocation)
			{
				MultiClipboard->ClipboardTable[i].SavedDataBuffer = (void*)((UINT_PTR)MultiClipboard->ClipboardTable[i].SavedDataBuffer - Size);
			}
		}
	}
	MultiClipboard->ClipHeapUsedSize -= Size;
	SIZE_T CommitRemaining = MultiClipboard->ClipHeapCommitedSize - MultiClipboard->ClipHeapUsedSize;
	SIZE_T ClipHeapMinimumCommitSize = ALIGN_SIZE((SIZE_T)CLIP_HEAP_MINIMUM_COMMIT_SIZE, MultiClipboard->PageSize);
	if (CommitRemaining && MultiClipboard->ClipHeapCommitedSize > ClipHeapMinimumCommitSize)
	{
		SIZE_T CommitRemoveSize = CommitRemaining;
		if (MultiClipboard->ClipHeapCommitedSize - CommitRemoveSize < ClipHeapMinimumCommitSize)
		{
			CommitRemoveSize = MultiClipboard->ClipHeapCommitedSize - ClipHeapMinimumCommitSize;
		}
		VirtualFree((void*)((UINT_PTR)MultiClipboard->ClipHeapBufferBase + MultiClipboard->ClipHeapCommitedSize - CommitRemoveSize), CommitRemoveSize, MEM_DECOMMIT);
		MultiClipboard->ClipHeapCommitedSize -= CommitRemoveSize;
	}
}

LRESULT CALLBACK LowLevelKeyboardHookProcedure(int Code, WPARAM WParameter, LPARAM LParameter)
{
	MultiClipboardStateT* MultiClipboard = g_MultiClipboard;
	if (Code < 0)
	{
		return CallNextHookEx(MultiClipboard->LowLevelKeyboardHookHandle, Code, WParameter, LParameter);
	}
	if (WParameter == WM_KEYDOWN)
	{
		KBDLLHOOKSTRUCT* KeyboardInput = (KBDLLHOOKSTRUCT*)LParameter;
		DWORD VirtualKeyCode = KeyboardInput->vkCode;
		for (SIZE_T n = MultiClipboard->ClipboardCount, i = 0; i < n; i++)
		{
			if (VirtualKeyCode == MultiClipboard->ClipboardTable[i].VirtualKeyCode)
			{
				if (i != MultiClipboard->CurrentClipboardIndex)
				{
					MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataSize = 0;
					if (MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].HeapSaveSize)
					{
						ClipHeapFree(MultiClipboard, MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].HeapSaveSize, MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer);
						MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer = NULL;
						MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].HeapSaveSize = 0;
					}
					if (OpenClipboard(NULL))
					{
						HGLOBAL ClipboardDataHandle = GetClipboardData(CF_UNICODETEXT);
						if (ClipboardDataHandle)
						{
							WCHAR* ClipboardData = (WCHAR*)GlobalLock(ClipboardDataHandle);
							if (ClipboardData)
							{
								SIZE_T ClipboardDataLength = wcslen(ClipboardData);
								if (ClipboardDataLength * sizeof(WCHAR) <= CLIP_SLOT_BUFFER_SIZE)
								{
									MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer = MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].FastSaveSlot;
								}
								else
								{
									MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].HeapSaveSize = ClipboardDataLength * sizeof(WCHAR);
									MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer = ClipHeapAllocate(MultiClipboard, MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].HeapSaveSize);
									if (!MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer)
									{
										ClipboardDataLength = CLIP_SLOT_BUFFER_SIZE / sizeof(WCHAR);
										MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].HeapSaveSize = 0;
										MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer = MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].FastSaveSlot;
									}
								}
								MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataSize = ClipboardDataLength * sizeof(WCHAR);
								memcpy(MultiClipboard->ClipboardTable[MultiClipboard->CurrentClipboardIndex].SavedDataBuffer, ClipboardData, ClipboardDataLength * sizeof(WCHAR));
								GlobalUnlock(ClipboardData);
							}
						}
						CloseClipboard();
					}
					SIZE_T SavedDataLength = MultiClipboard->ClipboardTable[i].SavedDataSize / sizeof(WCHAR);
					HGLOBAL ClipboardDataHandle = GlobalAlloc(GMEM_MOVEABLE, (SavedDataLength + 1) * sizeof(WCHAR));
					if (ClipboardDataHandle)
					{
						WCHAR* ClipboardData = (WCHAR*)GlobalLock(ClipboardDataHandle);
						if (ClipboardData)
						{
							memcpy(ClipboardData, MultiClipboard->ClipboardTable[i].SavedDataBuffer, SavedDataLength * sizeof(WCHAR));
							ClipboardData[SavedDataLength] = 0;
							GlobalUnlock(ClipboardDataHandle);
						}
						BOOL ClipboardSet = FALSE;
						if (OpenClipboard(NULL))
						{
							EmptyClipboard();
							ClipboardSet = !SetClipboardData(CF_UNICODETEXT, ClipboardDataHandle);
							CloseClipboard();
						}
						if (!ClipboardSet)
						{
							GlobalFree(ClipboardDataHandle);
						}
					}
					MultiClipboard->CurrentClipboardIndex = i;
				}
				break;
			}
		}
	}
	return CallNextHookEx(MultiClipboard->LowLevelKeyboardHookHandle, Code, WParameter, LParameter);
}
