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
#include "FlSha256.h"
#include "FlWin32FilePath.h"
#include <stdint.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef struct
{
	void* BufferAddress;
	DWORD CpuId0To1ABCD[8];
	DWORD CpuId80000000To80000004ABCD[20];
	HANDLE ImageFileHandle;
	HANDLE VolumeHandle;
	BY_HANDLE_FILE_INFORMATION ImageFileInformation;
	DWORD ComputerNameLength;
	WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	SYSTEM_INFO SystemInfo;
	SIZE_T LargePageMinimum;
	ULONGLONG PhysicallyInstalledSystemMemory;
	BOOL AutomaticSystemResume;
	WORD MaximumProcessorGroupCount;
	DWORD FirstGroupMaximumProcessorCount;
	ULONG HighestNumaNodeNumber;
	DWORD RegistryQuotaAllowed;
	DWORD RegistryQuotaUsed;
	HMODULE ImageBase;
	HMODULE Kernel32;
	HMODULE Advapi32;
	HANDLE CurrentProcess;
	HANDLE CurrentThread;
	HANDLE CurrentProcessToken;
	WCHAR* CommandLine;
	STARTUPINFOW StartupInfo;
	DWORD ThreadErrorMode;
	DWORD CurrentProcessId;
	DWORD CurrentThreadId;
	DWORD CurrentSessionId;
	DYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZoneInformation;
	LCID ThreadLocale;
	LANGID ThreadUILanguage;
	HWND ConsoleWindow;
	SYSTEM_POWER_STATUS SystemPowerStatus;
	SIZE_T WorkingSetMinimum;
	SIZE_T WorkingSetMaximum;
	DWORD WorkingSetFlags;
	HANDLE StdInput;
	HANDLE StdOutput;
	HANDLE StdError;
	PROCESSOR_NUMBER CurrentProcessorNumber;
	WORD ActiveProcessorGroupCount;
	DWORD FirstGroupActiveProcessorGroupCount;
	MEMORYSTATUSEX MemoryStatus;
	int CurrentThreadPriority;
	DWORD CurrentProcessClass;
	DWORD CurrentProcessHandleCount;
	SYSTEMTIME LocalTime;
	IO_COUNTERS CurrentProcessIoCounters;
	FILETIME CurrentProcessCreationTime;
	FILETIME CurrentProcessExitTime;
	FILETIME CurrentProcessKernelTime;
	FILETIME CurrentProcessUserTime;
	ULONG64 CurrentProcessCycleCount;
	ULONGLONG TickCount;
	DWORD64 SystemTime;
	ULONGLONG UnbiasedInterruptTime;
	LARGE_INTEGER PerformanceFrequency;
	LARGE_INTEGER PerformanceCounter;
	DWORD64 ProcessorClockCycles;
} FlUuidInternalGenerateRandomSeedData;

#define FL_UUID_INTERNAL_EXPECTED_TOKEN_USER_SIZE (sizeof(TOKEN_USER) + (size_t)&((const SID*)0)->SubAuthority[5])
#define FL_UUID_INTERNAL_MAX_USER_NAME_SIZE (((size_t)256 + 1) * sizeof(WCHAR))
#define FL_UUID_INTERNAL_MAX_FILE_PATH_SIZE ((size_t)2 * (((size_t)MAX_PATH + 1) * sizeof(WCHAR)))
#define FL_UUID_INTERNAL_HW_PROFILE_SIZE (sizeof(HW_PROFILE_INFOW))
#define FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_0 (FL_UUID_INTERNAL_EXPECTED_TOKEN_USER_SIZE > FL_UUID_INTERNAL_MAX_USER_NAME_SIZE ? FL_UUID_INTERNAL_EXPECTED_TOKEN_USER_SIZE : FL_UUID_INTERNAL_MAX_USER_NAME_SIZE)
#define FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_1 (FL_UUID_INTERNAL_MAX_FILE_PATH_SIZE > FL_UUID_INTERNAL_HW_PROFILE_SIZE ? FL_UUID_INTERNAL_MAX_FILE_PATH_SIZE : FL_UUID_INTERNAL_HW_PROFILE_SIZE)
#define FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE (FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_0 > FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_1 ? FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_0 : FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_1)

__declspec(noinline) static void FlUuidInternalGenerateRandomSeed(uint64_t* Nonce, void* RandomSeed)
{
	HANDLE CurrentProcess = GetCurrentProcess();
	HANDLE CurrentProcessTokenHandle = 0;

	HMODULE Kernel32Module = GetModuleHandleW(L"Kernel32.dll");
	HMODULE Advapi32Module = 0;

	DWORD64 SystemTime = 0;
	GetSystemTimeAsFileTime((FILETIME*)&SystemTime);

	HANDLE ImageFileHandle = INVALID_HANDLE_VALUE;
	HANDLE VolumeHandle = INVALID_HANDLE_VALUE;
	BY_HANDLE_FILE_INFORMATION ImageFileInformation;
	memset(&ImageFileInformation, 0, sizeof(BY_HANDLE_FILE_INFORMATION));

	SYSTEM_INFO SystemInfo;
	memset(&SystemInfo, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&SystemInfo);
	if (!SystemInfo.dwPageSize)
	{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
		SystemInfo.dwPageSize = 0x1000;
#else
		SystemInfo.dwPageSize = 0x10000;
#endif
	}

	FlSha256Context Sha256Context;
	FlSha256CreateHash(&Sha256Context);

	size_t BufferSize = (FL_UUID_INTERNAL_TEMPORAL_BUFFER_SIZE + ((size_t)SystemInfo.dwPageSize - 1)) & ~((size_t)SystemInfo.dwPageSize - 1);
	size_t SMBIOSFirmwareTableBufferSize = (size_t)GetSystemFirmwareTable(0x52534D42, 0, 0, 0);
	SMBIOSFirmwareTableBufferSize = (SMBIOSFirmwareTableBufferSize + ((size_t)SystemInfo.dwPageSize - 1)) & ~((size_t)SystemInfo.dwPageSize - 1);
	if (SMBIOSFirmwareTableBufferSize > BufferSize)
	{
		BufferSize = SMBIOSFirmwareTableBufferSize;
	}
	void* Buffer = (void*)VirtualAlloc(0, BufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (Buffer)
	{
		for (;;)
		{
			size_t BufferTextSpaceLength = (BufferSize / 2) / sizeof(WCHAR);
			size_t RequiredBufferSize = 0;

			memset(Buffer, 0, BufferSize);
			size_t SMBIOSFirmwareTableSize = (size_t)GetSystemFirmwareTable(0x52534D42, 0, Buffer, (DWORD)BufferSize);
			if (SMBIOSFirmwareTableSize && SMBIOSFirmwareTableSize <= BufferSize)
			{
				FlSha256HashData(&Sha256Context, SMBIOSFirmwareTableSize, Buffer);
			}
			else if (SMBIOSFirmwareTableSize)
			{
				SMBIOSFirmwareTableBufferSize = SMBIOSFirmwareTableSize;
				RequiredBufferSize = SMBIOSFirmwareTableBufferSize;
			}

			WCHAR* SystemDirectoryPath = (WCHAR*)Buffer;
			memset(SystemDirectoryPath, 0, BufferTextSpaceLength * sizeof(WCHAR));
			size_t SystemDirectoryPathLength = (size_t)GetSystemDirectoryW(SystemDirectoryPath, (UINT)BufferTextSpaceLength);
			if (SystemDirectoryPathLength && SystemDirectoryPathLength < BufferTextSpaceLength)
			{
				FlSha256HashData(&Sha256Context, SystemDirectoryPathLength * sizeof(WCHAR), SystemDirectoryPath);
			}
			else if (SystemDirectoryPathLength)
			{
				RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			}

			WCHAR* WindowsDirectoryPath = (WCHAR*)Buffer;
			memset(WindowsDirectoryPath, 0, BufferTextSpaceLength * sizeof(WCHAR));
			size_t WindowsDirectoryPathLength = (size_t)GetSystemWindowsDirectoryW(WindowsDirectoryPath, (UINT)BufferTextSpaceLength);
			if (WindowsDirectoryPathLength && WindowsDirectoryPathLength < BufferTextSpaceLength)
			{
				FlSha256HashData(&Sha256Context, WindowsDirectoryPathLength * sizeof(WCHAR), WindowsDirectoryPath);
			}
			else if (WindowsDirectoryPathLength)
			{
				RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			}

			WCHAR* RawImageFilePath = (WCHAR*)Buffer;
			WCHAR* ImageFilePath = (WCHAR*)((uintptr_t)Buffer + (BufferTextSpaceLength * sizeof(WCHAR)));
			memset(RawImageFilePath, 0, BufferTextSpaceLength * sizeof(WCHAR));
			memset(ImageFilePath, 0, BufferTextSpaceLength * sizeof(WCHAR));
			size_t RawImageFilePathLength = (size_t)GetModuleFileNameW(0, RawImageFilePath, (DWORD)BufferTextSpaceLength);
			size_t ImageFilePathLength = 0;
			if (RawImageFilePathLength && RawImageFilePathLength < BufferTextSpaceLength)
			{
				ImageFilePathLength = FlWin32GetFullyQualifiedPath(RawImageFilePathLength, RawImageFilePath, 0, 0, BufferTextSpaceLength - 1, ImageFilePath);
				if (ImageFilePathLength && ImageFilePathLength < BufferTextSpaceLength)
				{
					ImageFilePath[ImageFilePathLength] = 0;
				}
				else if (ImageFilePathLength)
				{
					RawImageFilePathLength = 0;
					ImageFilePathLength = 0;
					RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
				}
				else
				{
					ImageFilePathLength = RawImageFilePathLength;
					memcpy(ImageFilePath, RawImageFilePath, (RawImageFilePathLength + 1) * sizeof(WCHAR));
				}
			}
			else if (RawImageFilePathLength)
			{
				RawImageFilePathLength = 0;
				ImageFilePathLength = 0;
				RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			}

			if (ImageFilePathLength)
			{
				ImageFileHandle = CreateFileW(ImageFilePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
				if (ImageFileHandle != INVALID_HANDLE_VALUE)
				{
					memset(&ImageFileInformation, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
					GetFileInformationByHandle(ImageFileHandle, &ImageFileInformation);
					CloseHandle(ImageFileHandle);
				}
				FlSha256HashData(&Sha256Context, ImageFilePathLength * sizeof(WCHAR), ImageFilePath);
				
				WCHAR* VolumeMountPath = (WCHAR*)Buffer;
				memset(VolumeMountPath, 0, BufferTextSpaceLength * sizeof(WCHAR));
				if (GetVolumePathNameW(ImageFilePath, VolumeMountPath, (DWORD)BufferTextSpaceLength))
				{
					WCHAR* VolumeGuidPath = (WCHAR*)((uintptr_t)Buffer + (BufferTextSpaceLength * sizeof(WCHAR)));
					memset(VolumeGuidPath, 0, BufferTextSpaceLength * sizeof(WCHAR));
					if (GetVolumeNameForVolumeMountPointW(VolumeMountPath, VolumeGuidPath, (DWORD)BufferTextSpaceLength))
					{
						size_t VolumeGuidPathLength = wcslen(VolumeGuidPath);
						FlSha256HashData(&Sha256Context, (size_t)VolumeGuidPathLength * sizeof(WCHAR), VolumeGuidPath);

						DWORD DiskFreeSpaceData[4] = { 0, 0, 0, 0 };
						if (GetDiskFreeSpaceW(VolumeGuidPath, &DiskFreeSpaceData[0], &DiskFreeSpaceData[1], &DiskFreeSpaceData[2], &DiskFreeSpaceData[3]))
						{
							FlSha256HashData(&Sha256Context, (size_t)4 * sizeof(DWORD), &DiskFreeSpaceData[0]);
						}

						VolumeHandle = CreateFileW(VolumeGuidPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
						if (VolumeHandle != INVALID_HANDLE_VALUE)
						{
							WCHAR* VolumeName = (WCHAR*)Buffer;
							WCHAR* VolumeFileSystemName = (WCHAR*)((uintptr_t)Buffer + (BufferTextSpaceLength * sizeof(WCHAR)));
							DWORD VolumeData[3] = { 0, 0, 0 };
							memset(VolumeName, 0, BufferTextSpaceLength * sizeof(WCHAR));
							memset(VolumeFileSystemName, 0, BufferTextSpaceLength * sizeof(WCHAR));
							if (GetVolumeInformationByHandleW(VolumeHandle, VolumeName, (DWORD)BufferTextSpaceLength, &VolumeData[0], &VolumeData[1], &VolumeData[2], VolumeFileSystemName, (DWORD)BufferTextSpaceLength))
							{
								size_t VolumeNameLength = wcslen(VolumeName);
								size_t VolumeFileSystemNameLength = wcslen(VolumeFileSystemName);
								FlSha256HashData(&Sha256Context, VolumeNameLength * sizeof(WCHAR), VolumeName);
								FlSha256HashData(&Sha256Context, VolumeFileSystemNameLength * sizeof(WCHAR), VolumeFileSystemName);
								FlSha256HashData(&Sha256Context, (size_t)3 * sizeof(DWORD), &VolumeData[0]);
							}
							else
							{
								DWORD GetVolumeInformationByHandleError = GetLastError();
								if (GetVolumeInformationByHandleError == ERROR_FILENAME_EXCED_RANGE || GetVolumeInformationByHandleError == ERROR_MORE_DATA || GetVolumeInformationByHandleError == ERROR_BUFFER_OVERFLOW || GetVolumeInformationByHandleError == ERROR_INSUFFICIENT_BUFFER)
								{
									RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
								}
							}
							CloseHandle(VolumeHandle);
						}
					}
					else
					{
						DWORD GetVolumeNameForVolumeMountPointError = GetLastError();
						if (GetVolumeNameForVolumeMountPointError == ERROR_FILENAME_EXCED_RANGE || GetVolumeNameForVolumeMountPointError == ERROR_MORE_DATA || GetVolumeNameForVolumeMountPointError == ERROR_BUFFER_OVERFLOW || GetVolumeNameForVolumeMountPointError == ERROR_INSUFFICIENT_BUFFER)
						{
							RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
						}
					}
				}
				else
				{
					DWORD GetVolumePathNameError = GetLastError();
					if (GetVolumePathNameError == ERROR_FILENAME_EXCED_RANGE || GetVolumePathNameError == ERROR_MORE_DATA || GetVolumePathNameError == ERROR_BUFFER_OVERFLOW || GetVolumePathNameError == ERROR_INSUFFICIENT_BUFFER)
					{
						RequiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
					}
				}
			}

			Advapi32Module = LoadLibraryW(L"Advapi32.dll");
			BOOL (WINAPI* OpenProcessTokenProcedure)(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle) = 0;
			BOOL (WINAPI* GetTokenInformationProcedure)(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength) = 0;
			DWORD (WINAPI* GetLengthSidProcedure)(PSID pSid) = 0;
			BOOL (WINAPI* GetUserNameWProcedure)(LPWSTR lpBuffer, LPDWORD pcbBuffer) = 0;
			BOOL (WINAPI* GetCurrentHwProfileWProcedure)(LPHW_PROFILE_INFOW lpHwProfileInfo) = 0;
			if (Advapi32Module)
			{
				OpenProcessTokenProcedure = (BOOL(WINAPI*)(HANDLE, DWORD, PHANDLE))GetProcAddress(Advapi32Module, "OpenProcessToken");
				GetTokenInformationProcedure = (BOOL(WINAPI*)(HANDLE, TOKEN_INFORMATION_CLASS, LPVOID, DWORD, PDWORD))GetProcAddress(Advapi32Module, "GetTokenInformation");
				GetLengthSidProcedure = (DWORD(WINAPI*)(PSID))GetProcAddress(Advapi32Module, "GetLengthSid");
				GetUserNameWProcedure = (BOOL(WINAPI*)(LPWSTR, LPDWORD))GetProcAddress(Advapi32Module, "GetUserNameW");
				GetCurrentHwProfileWProcedure = (BOOL(WINAPI*)(LPHW_PROFILE_INFOW))GetProcAddress(Advapi32Module, "GetCurrentHwProfileW");

				if (OpenProcessTokenProcedure && GetTokenInformationProcedure && GetLengthSidProcedure)
				{
					CurrentProcessTokenHandle = 0;
					if (OpenProcessTokenProcedure(CurrentProcess, TOKEN_QUERY, &CurrentProcessTokenHandle))
					{
						DWORD TokenUserSize = 0;
						TOKEN_USER* TokenUserData = (TOKEN_USER*)Buffer;
						memset(TokenUserData, 0, BufferSize);
						if (GetTokenInformationProcedure(CurrentProcessTokenHandle, TokenUser, TokenUserData, (DWORD)BufferSize, &TokenUserSize))
						{
							DWORD TokenUserSIDSize = GetLengthSidProcedure(TokenUserData->User.Sid);
							FlSha256HashData(&Sha256Context, (size_t)TokenUserSIDSize, TokenUserData->User.Sid);
						}
						else
						{
							DWORD GetTokenInformationError = GetLastError();
							if (GetTokenInformationError == ERROR_INSUFFICIENT_BUFFER || GetTokenInformationError == ERROR_MORE_DATA || GetTokenInformationError == ERROR_BUFFER_OVERFLOW)
							{
								size_t RequiredTokenUserSize = (size_t)TokenUserSize;
								RequiredBufferSize = RequiredTokenUserSize;
							}
						}
						CloseHandle(CurrentProcessTokenHandle);
					}
				}

				if (GetUserNameWProcedure)
				{
					WCHAR* UserName = (WCHAR*)Buffer;
					DWORD UserNameLength = (DWORD)(BufferSize / sizeof(WCHAR));
					memset(UserName, 0, (size_t)UserNameLength * sizeof(WCHAR));
					if (GetUserNameWProcedure(UserName, &UserNameLength))
					{
						FlSha256HashData(&Sha256Context, (size_t)UserNameLength * sizeof(WCHAR), UserName);
					}
					else
					{
						DWORD GetUserNameError = GetLastError();
						if (GetUserNameError == ERROR_INSUFFICIENT_BUFFER)
						{
							size_t RequiredUserNameLength = (size_t)UserNameLength * sizeof(WCHAR);
							RequiredBufferSize = RequiredUserNameLength;
						}
					}
				}

				if (GetCurrentHwProfileWProcedure)
				{
					HW_PROFILE_INFOW* HwProfileInfo = (HW_PROFILE_INFOW*)Buffer;
					memset(HwProfileInfo, 0, sizeof(HW_PROFILE_INFOW));
					if (GetCurrentHwProfileWProcedure(HwProfileInfo))
					{
						FlSha256HashData(&Sha256Context, sizeof(HW_PROFILE_INFOW), HwProfileInfo);
					}
				}

				FreeLibrary(Advapi32Module);
			}

			VirtualFree(Buffer, 0, MEM_RELEASE);

			if (RequiredBufferSize && BufferSize < (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR)))
			{
				BufferSize = (RequiredBufferSize + ((size_t)SystemInfo.dwPageSize - 1)) & ~((size_t)SystemInfo.dwPageSize - 1);
				Buffer = (void*)VirtualAlloc(0, BufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
				if (!Buffer)
				{
					break;
				}
				else
				{
					FlSha256CreateHash(&Sha256Context);
				}
			}
			else
			{
				break;
			}
		}
	}

	WCHAR* EnvironmentBlock = GetEnvironmentStringsW();
	if (EnvironmentBlock)
	{
		size_t EnvironmentBlockSize = 0;
		while (EnvironmentBlock[EnvironmentBlockSize] || EnvironmentBlock[EnvironmentBlockSize + 1])
		{
			EnvironmentBlockSize++;
		}
		EnvironmentBlockSize += 2;
		EnvironmentBlockSize *= sizeof(WCHAR);
		FlSha256HashData(&Sha256Context, EnvironmentBlockSize, EnvironmentBlock);
		FreeEnvironmentStringsW(EnvironmentBlock);
	}

	HANDLE CurrentThread = GetCurrentThread();
	FlUuidInternalGenerateRandomSeedData Data;
	memset(&Data, 0, sizeof(FlUuidInternalGenerateRandomSeedData));
	Data.BufferAddress = Buffer;
	__cpuidex((int*)&Data.CpuId0To1ABCD[0], 0, 0);
	if (Data.CpuId0To1ABCD[0] >= 1)
	{
		__cpuidex((int*)&Data.CpuId0To1ABCD[4], 1, 0);
	}
	__cpuidex((int*)&Data.CpuId80000000To80000004ABCD[0], (int)0x80000000, 0);
	for (DWORD n = (Data.CpuId80000000To80000004ABCD[0] < (DWORD)0x80000004) ? Data.CpuId80000000To80000004ABCD[0] : (DWORD)0x80000004, i = (DWORD)0x80000001; i <= n; i++)
	{
		__cpuidex((int*)&Data.CpuId80000000To80000004ABCD[i * 4], (DWORD)i, 0);
	}
	Data.ImageFileHandle = ImageFileHandle;
	Data.VolumeHandle = VolumeHandle;
	memcpy(&Data.ImageFileInformation, &ImageFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION));
	Data.ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW(&Data.ComputerName[0], &Data.ComputerNameLength);
	memcpy(&Data.SystemInfo, &SystemInfo, sizeof(SYSTEM_INFO));
	Data.LargePageMinimum = GetLargePageMinimum();
	GetPhysicallyInstalledSystemMemory(&Data.PhysicallyInstalledSystemMemory);
	Data.AutomaticSystemResume = IsSystemResumeAutomatic();
	Data.MaximumProcessorGroupCount = GetMaximumProcessorGroupCount();
	Data.FirstGroupMaximumProcessorCount = GetMaximumProcessorCount(0);
	GetNumaHighestNodeNumber(&Data.HighestNumaNodeNumber);
	GetSystemRegistryQuota(&Data.RegistryQuotaAllowed, &Data.RegistryQuotaUsed);
	Data.ImageBase = (HMODULE)&__ImageBase;
	Data.Kernel32 = Kernel32Module;
	Data.Advapi32 = Advapi32Module;
	Data.CurrentProcess = CurrentProcess;
	Data.CurrentThread = CurrentThread;
	Data.CurrentProcessToken = CurrentProcessTokenHandle;
	Data.CommandLine = GetCommandLineW();
	GetStartupInfoW(&Data.StartupInfo);
	Data.ThreadErrorMode = GetThreadErrorMode();
	Data.CurrentProcessId = GetCurrentProcessId();
	Data.CurrentThreadId = GetCurrentThreadId();
	ProcessIdToSessionId(Data.CurrentProcessId, &Data.CurrentSessionId);
	GetDynamicTimeZoneInformation(&Data.DynamicTimeZoneInformation);
	Data.ThreadLocale = GetThreadLocale();
	Data.ThreadUILanguage = GetThreadUILanguage();
	Data.ConsoleWindow = GetConsoleWindow();
	GetSystemPowerStatus(&Data.SystemPowerStatus);
	GetProcessWorkingSetSizeEx(CurrentProcess, &Data.WorkingSetMinimum, &Data.WorkingSetMaximum, &Data.WorkingSetFlags);
	Data.StdInput = GetStdHandle(STD_INPUT_HANDLE);
	Data.StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	Data.StdError = GetStdHandle(STD_ERROR_HANDLE);
	GetCurrentProcessorNumberEx(&Data.CurrentProcessorNumber);
	Data.ActiveProcessorGroupCount = GetActiveProcessorGroupCount();
	Data.FirstGroupActiveProcessorGroupCount = GetActiveProcessorCount(0);
	Data.MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&Data.MemoryStatus);
	Data.CurrentThreadPriority = GetThreadPriority(CurrentThread);
	Data.CurrentProcessClass = GetPriorityClass(CurrentProcess);
	GetProcessHandleCount(CurrentProcess, &Data.CurrentProcessHandleCount);
	GetLocalTime(&Data.LocalTime);
	GetProcessIoCounters(CurrentProcess, &Data.CurrentProcessIoCounters);
	GetProcessTimes(CurrentProcess, &Data.CurrentProcessCreationTime, &Data.CurrentProcessExitTime, &Data.CurrentProcessKernelTime, &Data.CurrentProcessUserTime);
	QueryProcessCycleTime(CurrentProcess, &Data.CurrentProcessCycleCount);
	Data.TickCount = GetTickCount64();
	Data.SystemTime = SystemTime;
	QueryUnbiasedInterruptTime(&Data.UnbiasedInterruptTime);
	QueryPerformanceFrequency(&Data.PerformanceFrequency);
	QueryPerformanceCounter(&Data.PerformanceCounter);
	Data.ProcessorClockCycles = __rdtsc();

	*Nonce = (uint64_t)SystemTime;

	FlSha256HashData(&Sha256Context, sizeof(FlUuidInternalGenerateRandomSeedData), &Data);
	FlSha256FinishHash(&Sha256Context, RandomSeed);
}

volatile uint64_t UuidInternalRngNonce;
#ifdef _WIN64
volatile uint64_t UuidInternalRngAtomicCounter;
#else
volatile uint32_t UuidInternalRngAtomicCounter;
#endif // _WIN64
volatile uint64_t UuidInternalRngState[4];

static void FlUuidInternalGenerateRandomBits(void* Buffer)
{
	uint64_t Nonce = UuidInternalRngNonce;
	if (!Nonce)
	{
		uint64_t RandomSeed[4];
		FlUuidInternalGenerateRandomSeed(&Nonce, &RandomSeed);
		UuidInternalRngNonce = Nonce;
		UuidInternalRngState[0] = RandomSeed[0];
		UuidInternalRngState[1] = RandomSeed[1];
		UuidInternalRngState[2] = RandomSeed[2];
		UuidInternalRngState[3] = RandomSeed[3];
	}
#ifdef _WIN64
	uint64_t Counter = (uint64_t)InterlockedIncrement64((LONG64 volatile*)&UuidInternalRngAtomicCounter);
#else
	uint64_t Counter = (uint64_t)InterlockedIncrement((LONG volatile*)&UuidInternalRngAtomicCounter);
#endif // _WIN64

	// The copying of nonce or RNG state does not need to be atomic, only the counter most be used atomically.
	uint64_t RngBuffer[6] = {
		Nonce,
		Counter,
		UuidInternalRngState[0],
		UuidInternalRngState[1],
		UuidInternalRngState[2],
		UuidInternalRngState[3] };

	uint64_t NextRngState[4];
	FlSha256Context Sha256Context;
	FlSha256CreateHash(&Sha256Context);
	FlSha256HashData(&Sha256Context, sizeof(RngBuffer), &RngBuffer);
	FlSha256FinishHash(&Sha256Context, &NextRngState);
	UuidInternalRngState[0] = NextRngState[0];
	UuidInternalRngState[1] = NextRngState[1];
	UuidInternalRngState[2] = NextRngState[2];
	UuidInternalRngState[3] = NextRngState[3];

	memcpy(Buffer, &NextRngState, 16);
}

void FlUuidCreateRandomId(void* Uuid)
{
	uint8_t* RandomIdBytes = (uint8_t*)Uuid;
	FlUuidInternalGenerateRandomBits(RandomIdBytes);
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
