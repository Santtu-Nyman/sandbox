/*
	Santtu S. Nyman's random byte generator library

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
#include "FlRandom.h"
#include "FlSha256.h"
#include "FlSha256Hmac.h"
#include "FlWin32FilePath.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

#define FL_RANDOM_INTERNAL_BLOCK_SIZE FL_SHA256_DIGEST_SIZE

typedef struct
{
	void* bufferAddress;
	DWORD cpuId0To1ABCD[8];
	DWORD cpuId80000000To80000004ABCD[20];
	HANDLE imageFileHandle;
	HANDLE volumeHandle;
	BY_HANDLE_FILE_INFORMATION imageFileInformation;
	DWORD computerNameLength;
	WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
	SYSTEM_INFO systemInfo;
	SIZE_T largePageMinimum;
	ULONGLONG physicallyInstalledSystemMemory;
	BOOL automaticSystemResume;
	WORD maximumProcessorGroupCount;
	DWORD firstGroupMaximumProcessorCount;
	ULONG highestNumaNodeNumber;
	DWORD registryQuotaAllowed;
	DWORD registryQuotaUsed;
	HMODULE imageBase;
	HMODULE kernel32;
	HMODULE advapi32;
	HMODULE iphlpapi;
	HMODULE user32;
	HANDLE currentProcess;
	HANDLE currentThread;
	HANDLE currentProcessToken;
	WCHAR* commandLine;
	STARTUPINFOW startupInfo;
	DWORD threadErrorMode;
	DWORD currentProcessId;
	DWORD currentThreadId;
	DWORD currentSessionId;
	DYNAMIC_TIME_ZONE_INFORMATION dynamicTimeZoneInformation;
	LCID threadLocale;
	LANGID threadUILanguage;
	HWND consoleWindow;
	SYSTEM_POWER_STATUS systemPowerStatus;
	SIZE_T workingSetMinimum;
	SIZE_T workingSetMaximum;
	DWORD workingSetFlags;
	HANDLE stdInput;
	HANDLE stdOutput;
	HANDLE stdError;
	PROCESSOR_NUMBER currentProcessorNumber;
	WORD activeProcessorGroupCount;
	DWORD firstGroupActiveProcessorGroupCount;
	MEMORYSTATUSEX memoryStatus;
	int currentThreadPriority;
	DWORD currentProcessClass;
	DWORD currentProcessHandleCount;
	POINT cursorPosition;
	HKL keybordLayout;
	int isRemoteSession;
	SYSTEMTIME localTime;
	IO_COUNTERS currentProcessIoCounters;
	FILETIME currentProcessCreationTime;
	FILETIME currentProcessExitTime;
	FILETIME currentProcessKernelTime;
	FILETIME currentProcessUserTime;
	ULONG64 currentProcessCycleCount;
	ULONGLONG tickCount;
	DWORD64 systemTime;
	ULONGLONG unbiasedInterruptTime;
	LARGE_INTEGER performanceFrequency;
	LARGE_INTEGER performanceCounter;
	DWORD64 processorClockCycles;
} FlInternalGenerateRandomSeedData;

#define FL_INTERNAL_MAX_USER_NAME_SIZE (((size_t)256 + 1) * sizeof(WCHAR))
#define FL_INTERNAL_MAX_FILE_PATH_SIZE ((size_t)2 * (((size_t)MAX_PATH + 1) * sizeof(WCHAR)))
#define FL_INTERNAL_HW_PROFILE_SIZE (sizeof(HW_PROFILE_INFOW))
#define FL_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_0 (TOKEN_USER_MAX_SIZE > FL_INTERNAL_MAX_USER_NAME_SIZE ? TOKEN_USER_MAX_SIZE : FL_INTERNAL_MAX_USER_NAME_SIZE)
#define FL_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_1 (FL_INTERNAL_MAX_FILE_PATH_SIZE > FL_INTERNAL_HW_PROFILE_SIZE ? FL_INTERNAL_MAX_FILE_PATH_SIZE : FL_INTERNAL_HW_PROFILE_SIZE)
#define FL_INTERNAL_TEMPORAL_BUFFER_SIZE (FL_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_0 > FL_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_1 ? FL_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_0 : FL_INTERNAL_TEMPORAL_BUFFER_SIZE_TMP_1)

__declspec(noinline) static void FlRandomInternalGenerateRandomSeed(uint64_t* nonce, void* randomSeed)
{
	HANDLE currentProcess = GetCurrentProcess();
	HANDLE currentProcessTokenHandle = 0;

	HMODULE kernel32Module = GetModuleHandleW(L"Kernel32.dll");
	HMODULE advapi32Module = NULL;
	HMODULE iphlpapiModule = NULL;

	DWORD64 systemTime = 0;
	GetSystemTimeAsFileTime((FILETIME*)&systemTime);

	HANDLE imageFileHandle = INVALID_HANDLE_VALUE;
	HANDLE volumeHandle = INVALID_HANDLE_VALUE;
	BY_HANDLE_FILE_INFORMATION imageFileInformation;
	memset(&imageFileInformation, 0, sizeof(BY_HANDLE_FILE_INFORMATION));

	SYSTEM_INFO systemInfo;
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&systemInfo);
	if (!systemInfo.dwPageSize)
	{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
		systemInfo.dwPageSize = 0x1000;
#else
		systemInfo.dwPageSize = 0x10000;
#endif
	}

	FlSha256Context sha256Context;
	FlSha256CreateHash(&sha256Context);

	size_t bufferSize = (FL_INTERNAL_TEMPORAL_BUFFER_SIZE + ((size_t)systemInfo.dwPageSize - 1)) & ~((size_t)systemInfo.dwPageSize - 1);
	size_t smbiosFirmwareTableBufferSize = (size_t)GetSystemFirmwareTable(0x52534D42, 0, 0, 0);
	smbiosFirmwareTableBufferSize = (smbiosFirmwareTableBufferSize + ((size_t)systemInfo.dwPageSize - 1)) & ~((size_t)systemInfo.dwPageSize - 1);
	if (smbiosFirmwareTableBufferSize > bufferSize)
	{
		bufferSize = smbiosFirmwareTableBufferSize;
	}
	void* buffer = (void*)VirtualAlloc(0, bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (buffer)
	{
		for (;;)
		{
			size_t bufferTextSpaceLength = (bufferSize / 2) / sizeof(WCHAR);
			size_t requiredBufferSize = 0;

			memset(buffer, 0, bufferSize);
			size_t smbiosFirmwareTableSize = (size_t)GetSystemFirmwareTable(0x52534D42, 0, buffer, (DWORD)bufferSize);
			if (smbiosFirmwareTableSize && smbiosFirmwareTableSize <= bufferSize)
			{
				FlSha256HashData(&sha256Context, smbiosFirmwareTableSize, buffer);
			}
			else if (smbiosFirmwareTableSize)
			{
				smbiosFirmwareTableBufferSize = smbiosFirmwareTableSize;
				requiredBufferSize = smbiosFirmwareTableBufferSize;
			}

			WCHAR* systemDirectoryPath = (WCHAR*)buffer;
			memset(systemDirectoryPath, 0, bufferTextSpaceLength * sizeof(WCHAR));
			size_t systemDirectoryPathLength = (size_t)GetSystemDirectoryW(systemDirectoryPath, (UINT)bufferTextSpaceLength);
			if (systemDirectoryPathLength && systemDirectoryPathLength < bufferTextSpaceLength)
			{
				FlSha256HashData(&sha256Context, systemDirectoryPathLength * sizeof(WCHAR), systemDirectoryPath);
			}
			else if (systemDirectoryPathLength)
			{
				requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			}

			WCHAR* windowsDirectoryPath = (WCHAR*)buffer;
			memset(windowsDirectoryPath, 0, bufferTextSpaceLength * sizeof(WCHAR));
			size_t windowsDirectoryPathLength = (size_t)GetSystemWindowsDirectoryW(windowsDirectoryPath, (UINT)bufferTextSpaceLength);
			if (windowsDirectoryPathLength && windowsDirectoryPathLength < bufferTextSpaceLength)
			{
				FlSha256HashData(&sha256Context, windowsDirectoryPathLength * sizeof(WCHAR), windowsDirectoryPath);
			}
			else if (windowsDirectoryPathLength)
			{
				requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			}

			WCHAR* rawImageFilePath = (WCHAR*)buffer;
			WCHAR* imageFilePath = (WCHAR*)((uintptr_t)buffer + (bufferTextSpaceLength * sizeof(WCHAR)));
			memset(rawImageFilePath, 0, bufferTextSpaceLength * sizeof(WCHAR));
			memset(imageFilePath, 0, bufferTextSpaceLength * sizeof(WCHAR));
			size_t rawImageFilePathLength = (size_t)GetModuleFileNameW(0, rawImageFilePath, (DWORD)bufferTextSpaceLength);
			size_t imageFilePathLength = 0;
			if (rawImageFilePathLength && rawImageFilePathLength < bufferTextSpaceLength)
			{
				imageFilePathLength = FlWin32GetFullyQualifiedPath(rawImageFilePathLength, rawImageFilePath, 0, 0, bufferTextSpaceLength - 1, imageFilePath);
				if (imageFilePathLength && imageFilePathLength < bufferTextSpaceLength)
				{
					imageFilePath[imageFilePathLength] = 0;
				}
				else if (imageFilePathLength)
				{
					rawImageFilePathLength = 0;
					imageFilePathLength = 0;
					requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
				}
				else
				{
					imageFilePathLength = rawImageFilePathLength;
					memcpy(imageFilePath, rawImageFilePath, (rawImageFilePathLength + 1) * sizeof(WCHAR));
				}
			}
			else if (rawImageFilePathLength)
			{
				rawImageFilePathLength = 0;
				imageFilePathLength = 0;
				requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
			}

			if (imageFilePathLength)
			{
				imageFileHandle = CreateFileW(imageFilePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
				if (imageFileHandle != INVALID_HANDLE_VALUE)
				{
					memset(&imageFileInformation, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
					GetFileInformationByHandle(imageFileHandle, &imageFileInformation);
					CloseHandle(imageFileHandle);
				}
				FlSha256HashData(&sha256Context, imageFilePathLength * sizeof(WCHAR), imageFilePath);

				WCHAR* volumeMountPath = (WCHAR*)buffer;
				memset(volumeMountPath, 0, bufferTextSpaceLength * sizeof(WCHAR));
				if (GetVolumePathNameW(imageFilePath, volumeMountPath, (DWORD)bufferTextSpaceLength))
				{
					WCHAR* volumeGuidPath = (WCHAR*)((uintptr_t)buffer + (bufferTextSpaceLength * sizeof(WCHAR)));
					memset(volumeGuidPath, 0, bufferTextSpaceLength * sizeof(WCHAR));
					if (GetVolumeNameForVolumeMountPointW(volumeMountPath, volumeGuidPath, (DWORD)bufferTextSpaceLength))
					{
						size_t volumeGuidPathLength = wcslen(volumeGuidPath);
						FlSha256HashData(&sha256Context, (size_t)volumeGuidPathLength * sizeof(WCHAR), volumeGuidPath);

						DWORD diskFreeSpaceData[4] = { 0, 0, 0, 0 };
						if (GetDiskFreeSpaceW(volumeGuidPath, &diskFreeSpaceData[0], &diskFreeSpaceData[1], &diskFreeSpaceData[2], &diskFreeSpaceData[3]))
						{
							FlSha256HashData(&sha256Context, (size_t)4 * sizeof(DWORD), &diskFreeSpaceData[0]);
						}

						volumeHandle = CreateFileW(volumeGuidPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
						if (volumeHandle != INVALID_HANDLE_VALUE)
						{
							WCHAR* volumeName = (WCHAR*)buffer;
							WCHAR* volumeFileSystemName = (WCHAR*)((uintptr_t)buffer + (bufferTextSpaceLength * sizeof(WCHAR)));
							DWORD volumeData[3] = { 0, 0, 0 };
							memset(volumeName, 0, bufferTextSpaceLength * sizeof(WCHAR));
							memset(volumeFileSystemName, 0, bufferTextSpaceLength * sizeof(WCHAR));
							if (GetVolumeInformationByHandleW(volumeHandle, volumeName, (DWORD)bufferTextSpaceLength, &volumeData[0], &volumeData[1], &volumeData[2], volumeFileSystemName, (DWORD)bufferTextSpaceLength))
							{
								size_t volumeNameLength = wcslen(volumeName);
								size_t volumeFileSystemNameLength = wcslen(volumeFileSystemName);
								FlSha256HashData(&sha256Context, volumeNameLength * sizeof(WCHAR), volumeName);
								FlSha256HashData(&sha256Context, volumeFileSystemNameLength * sizeof(WCHAR), volumeFileSystemName);
								FlSha256HashData(&sha256Context, (size_t)3 * sizeof(DWORD), &volumeData[0]);
							}
							else
							{
								DWORD getVolumeInformationByHandleError = GetLastError();
								if (getVolumeInformationByHandleError == ERROR_FILENAME_EXCED_RANGE || getVolumeInformationByHandleError == ERROR_MORE_DATA || getVolumeInformationByHandleError == ERROR_BUFFER_OVERFLOW || getVolumeInformationByHandleError == ERROR_INSUFFICIENT_BUFFER)
								{
									requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
								}
							}
							CloseHandle(volumeHandle);
						}
					}
					else
					{
						DWORD getVolumeNameForVolumeMountPointError = GetLastError();
						if (getVolumeNameForVolumeMountPointError == ERROR_FILENAME_EXCED_RANGE || getVolumeNameForVolumeMountPointError == ERROR_MORE_DATA || getVolumeNameForVolumeMountPointError == ERROR_BUFFER_OVERFLOW || getVolumeNameForVolumeMountPointError == ERROR_INSUFFICIENT_BUFFER)
						{
							requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
						}
					}
				}
				else
				{
					DWORD getVolumePathNameError = GetLastError();
					if (getVolumePathNameError == ERROR_FILENAME_EXCED_RANGE || getVolumePathNameError == ERROR_MORE_DATA || getVolumePathNameError == ERROR_BUFFER_OVERFLOW || getVolumePathNameError == ERROR_INSUFFICIENT_BUFFER)
					{
						requiredBufferSize = (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR));
					}
				}
			}

			advapi32Module = LoadLibraryW(L"Advapi32.dll");
			BOOL(WINAPI * allocateLocallyUniqueIdProcedure)(LUID* Luid);
			BOOL(WINAPI * openProcessTokenProcedure)(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle) = 0;
			BOOL(WINAPI * getTokenInformationProcedure)(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength) = 0;
			DWORD(WINAPI * getLengthSidProcedure)(PSID pSid) = 0;
			BOOL(WINAPI * getUserNameWProcedure)(LPWSTR lpBuffer, LPDWORD pcbBuffer) = 0;
			BOOL(WINAPI * getCurrentHwProfileWProcedure)(LPHW_PROFILE_INFOW lpHwProfileInfo) = 0;
			if (advapi32Module)
			{
				allocateLocallyUniqueIdProcedure = (BOOL(WINAPI*)(LUID*))GetProcAddress(advapi32Module, "AllocateLocallyUniqueId");
				openProcessTokenProcedure = (BOOL(WINAPI*)(HANDLE, DWORD, PHANDLE))GetProcAddress(advapi32Module, "OpenProcessToken");
				getTokenInformationProcedure = (BOOL(WINAPI*)(HANDLE, TOKEN_INFORMATION_CLASS, LPVOID, DWORD, PDWORD))GetProcAddress(advapi32Module, "GetTokenInformation");
				getLengthSidProcedure = (DWORD(WINAPI*)(PSID))GetProcAddress(advapi32Module, "GetLengthSid");
				getUserNameWProcedure = (BOOL(WINAPI*)(LPWSTR, LPDWORD))GetProcAddress(advapi32Module, "GetUserNameW");
				getCurrentHwProfileWProcedure = (BOOL(WINAPI*)(LPHW_PROFILE_INFOW))GetProcAddress(advapi32Module, "GetCurrentHwProfileW");

				if (allocateLocallyUniqueIdProcedure)
				{
					LUID locallyUniqueId = { 0, 0 };
					allocateLocallyUniqueIdProcedure(&locallyUniqueId);
					FlSha256HashData(&sha256Context, sizeof(LUID), &locallyUniqueId);
				}

				if (openProcessTokenProcedure && getTokenInformationProcedure && getLengthSidProcedure)
				{
					currentProcessTokenHandle = 0;
					if (openProcessTokenProcedure(currentProcess, TOKEN_QUERY, &currentProcessTokenHandle))
					{
						DWORD tokenUserSize = 0;
						TOKEN_USER* tokenUserData = (TOKEN_USER*)buffer;
						memset(tokenUserData, 0, TOKEN_USER_MAX_SIZE);
						if (getTokenInformationProcedure(currentProcessTokenHandle, TokenUser, tokenUserData, (DWORD)TOKEN_USER_MAX_SIZE, &tokenUserSize))
						{
							DWORD tokenUserSIDSize = getLengthSidProcedure(tokenUserData->User.Sid);
							FlSha256HashData(&sha256Context, (size_t)tokenUserSIDSize, tokenUserData->User.Sid);
						}
						CloseHandle(currentProcessTokenHandle);
					}
				}

				if (getUserNameWProcedure)
				{
					WCHAR* userName = (WCHAR*)buffer;
					DWORD userNameLength = (DWORD)(bufferSize / sizeof(WCHAR));
					memset(userName, 0, (size_t)userNameLength * sizeof(WCHAR));
					if (getUserNameWProcedure(userName, &userNameLength))
					{
						FlSha256HashData(&sha256Context, (size_t)userNameLength * sizeof(WCHAR), userName);
					}
					else
					{
						DWORD getUserNameError = GetLastError();
						if (getUserNameError == ERROR_INSUFFICIENT_BUFFER)
						{
							size_t requiredUserNameLength = (size_t)userNameLength * sizeof(WCHAR);
							requiredBufferSize = requiredUserNameLength;
						}
					}
				}

				if (getCurrentHwProfileWProcedure)
				{
					HW_PROFILE_INFOW* hwProfileInfo = (HW_PROFILE_INFOW*)buffer;
					memset(hwProfileInfo, 0, sizeof(HW_PROFILE_INFOW));
					if (getCurrentHwProfileWProcedure(hwProfileInfo))
					{
						FlSha256HashData(&sha256Context, sizeof(HW_PROFILE_INFOW), hwProfileInfo);
					}
				}

				FreeLibrary(advapi32Module);
			}
			
			iphlpapiModule = LoadLibraryW(L"Iphlpapi.dll");
			if (iphlpapiModule)
			{
				ULONG (WINAPI* getAdaptersAddressesProcedure)(ULONG Family, ULONG Flags, PVOID Reserved, IP_ADAPTER_ADDRESSES* AdapterAddresses, PULONG SizePointer) = (ULONG (WINAPI*)(ULONG, ULONG, PVOID, IP_ADAPTER_ADDRESSES*, PULONG))GetProcAddress(iphlpapiModule, "GetAdaptersAddresses");
				if (getAdaptersAddressesProcedure)
				{
					memset(buffer, 0, bufferSize);
					ULONG adapterAddressesBufferSize = bufferSize <= (ULONG)ULONG_MAX ? (ULONG)bufferSize : (ULONG)ULONG_MAX;
					IP_ADAPTER_ADDRESSES* adapterAddresses = (IP_ADAPTER_ADDRESSES*)buffer;
					ULONG getAdaptersAddressesResult = getAdaptersAddressesProcedure(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, adapterAddresses, &adapterAddressesBufferSize);
					if (getAdaptersAddressesResult == ERROR_SUCCESS)
					{
						for (IP_ADAPTER_ADDRESSES* i = adapterAddresses; i; i = i->Next)
						{
							size_t adapterStructureLength = (size_t)i->Length;
							if (adapterStructureLength < (((size_t)(&((const IP_ADAPTER_ADDRESSES*)0)->FirstDnsSuffix)) + sizeof(((const IP_ADAPTER_ADDRESSES*)0)->FirstDnsSuffix)))
							{
								adapterStructureLength = (((size_t)(&((const IP_ADAPTER_ADDRESSES*)0)->FirstDnsSuffix)) + sizeof(((const IP_ADAPTER_ADDRESSES*)0)->FirstDnsSuffix));
							}
							CHAR* adapterName = i->AdapterName;
							WCHAR* dnsSuffix = i->DnsSuffix;
							WCHAR* description = i->Description;
							WCHAR* friendlyName = i->FriendlyName;
							IP_ADAPTER_UNICAST_ADDRESS* unicastAddress = i->FirstUnicastAddress;
							IP_ADAPTER_ANYCAST_ADDRESS* anycastAddress = i->FirstAnycastAddress;
							IP_ADAPTER_MULTICAST_ADDRESS* multicastAddress = i->FirstMulticastAddress;
							IP_ADAPTER_DNS_SERVER_ADDRESS* dnsServerAddress = i->FirstDnsServerAddress;
							IP_ADAPTER_PREFIX* prefix = i->FirstPrefix;
							IP_ADAPTER_GATEWAY_ADDRESS* gatewayAddress = i->FirstGatewayAddress;
							FlSha256HashData(&sha256Context, adapterStructureLength, i);
							FlSha256HashData(&sha256Context, strlen(adapterName) * sizeof(char), adapterName);
							FlSha256HashData(&sha256Context, wcslen(dnsSuffix) * sizeof(WCHAR), dnsSuffix);
							FlSha256HashData(&sha256Context, wcslen(description) * sizeof(WCHAR), description);
							FlSha256HashData(&sha256Context, wcslen(friendlyName) * sizeof(WCHAR), friendlyName);
							for (IP_ADAPTER_UNICAST_ADDRESS* j = unicastAddress; j; j = j->Next)
							{
								FlSha256HashData(&sha256Context, (size_t)j->Length, j);
								FlSha256HashData(&sha256Context, (size_t)j->Address.iSockaddrLength, j->Address.lpSockaddr);
							}
							for (IP_ADAPTER_ANYCAST_ADDRESS* j = anycastAddress; j; j = j->Next)
							{
								FlSha256HashData(&sha256Context, (size_t)j->Length, j);
								FlSha256HashData(&sha256Context, (size_t)j->Address.iSockaddrLength, j->Address.lpSockaddr);
							}
							for (IP_ADAPTER_MULTICAST_ADDRESS* j = multicastAddress; j; j = j->Next)
							{
								FlSha256HashData(&sha256Context, (size_t)j->Length, j);
								FlSha256HashData(&sha256Context, (size_t)j->Address.iSockaddrLength, j->Address.lpSockaddr);
							}
							for (IP_ADAPTER_DNS_SERVER_ADDRESS* j = dnsServerAddress; j; j = j->Next)
							{
								FlSha256HashData(&sha256Context, (size_t)j->Length, j);
								FlSha256HashData(&sha256Context, (size_t)j->Address.iSockaddrLength, j->Address.lpSockaddr);
							}
							for (IP_ADAPTER_PREFIX* j = prefix; j; j = j->Next)
							{
								FlSha256HashData(&sha256Context, (size_t)j->Length, j);
								FlSha256HashData(&sha256Context, (size_t)j->Address.iSockaddrLength, j->Address.lpSockaddr);
							}
							for (IP_ADAPTER_GATEWAY_ADDRESS* j = gatewayAddress; j; j = j->Next)
							{
								FlSha256HashData(&sha256Context, (size_t)j->Length, j);
								FlSha256HashData(&sha256Context, (size_t)j->Address.iSockaddrLength, j->Address.lpSockaddr);
							}
						}
					}
					else if (getAdaptersAddressesResult == ERROR_BUFFER_OVERFLOW && bufferSize < (size_t)adapterAddressesBufferSize)
					{
						requiredBufferSize = (size_t)adapterAddressesBufferSize;
					}
				}

				FreeLibrary(iphlpapiModule);
			}

			VirtualFree(buffer, 0, MEM_RELEASE);

			if (requiredBufferSize && bufferSize < (size_t)2 * (((size_t)UNICODE_STRING_MAX_CHARS + 1) * sizeof(WCHAR)))
			{
				bufferSize = (requiredBufferSize + ((size_t)systemInfo.dwPageSize - 1)) & ~((size_t)systemInfo.dwPageSize - 1);
				buffer = (void*)VirtualAlloc(0, bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
				if (!buffer)
				{
					break;
				}
				else
				{
					FlSha256CreateHash(&sha256Context);
				}
			}
			else
			{
				break;
			}
		}
	}

	int (WINAPI* getUserDefaultGeoNameProcedure)(LPWSTR geoName, int geoNameCount) = (int (WINAPI*)(LPWSTR, int))GetProcAddress(kernel32Module, "GetUserDefaultGeoName");
	if (getUserDefaultGeoNameProcedure)
	{
		WCHAR userGeoName[4];
		int userGeoNameLength = getUserDefaultGeoNameProcedure(&userGeoName[0], (int)(sizeof(userGeoName) / sizeof(userGeoName[0])));
		if (userGeoNameLength > 0 && userGeoNameLength <= (int)(sizeof(userGeoName) / sizeof(userGeoName[0])))
		{
			FlSha256HashData(&sha256Context, (size_t)userGeoNameLength * sizeof(WCHAR), &userGeoName[0]);
		}
	}

	POINT cursorPosition = { 0, 0 };
	HKL keybordLayout = NULL;
	int isRemoteSession = 0;
	HMODULE user32Module = LoadLibraryW(L"User32.dll");
	if (user32Module)
	{
		BOOL (WINAPI* getCursorPosProcedure)(HANDLE lpPoint) = (BOOL(WINAPI*)(POINT*))GetProcAddress(user32Module, "GetCursorPos");
		HKL (WINAPI* getKeyboardLayoutProcedure)(DWORD idThread) = (HKL(WINAPI*)(DWORD))GetProcAddress(user32Module, "GetKeyboardLayout");
		int (WINAPI* getSystemMetricsProcedure)(int nIndex) = (int(WINAPI*)(int))GetProcAddress(user32Module, "GetSystemMetrics");
		if (getCursorPosProcedure)
		{
			getCursorPosProcedure(&cursorPosition);
		}
		if (getKeyboardLayoutProcedure)
		{
			keybordLayout = getKeyboardLayoutProcedure(0);
		}
		if (getSystemMetricsProcedure)
		{
			isRemoteSession = getSystemMetricsProcedure(SM_REMOTESESSION);
		}
		FreeLibrary(user32Module);
	}

	WCHAR* environmentBlock = GetEnvironmentStringsW();
	if (environmentBlock)
	{
		size_t environmentBlockSize = 0;
		while (environmentBlock[environmentBlockSize] || environmentBlock[environmentBlockSize + 1])
		{
			environmentBlockSize++;
		}
		environmentBlockSize += 2;
		environmentBlockSize *= sizeof(WCHAR);
		FlSha256HashData(&sha256Context, environmentBlockSize, environmentBlock);
		FreeEnvironmentStringsW(environmentBlock);
	}

	HANDLE currentThread = GetCurrentThread();
	FlInternalGenerateRandomSeedData data;
	memset(&data, 0, sizeof(FlInternalGenerateRandomSeedData));
	data.bufferAddress = buffer;
	__cpuidex((int*)&data.cpuId0To1ABCD[0], 0, 0);
	if (data.cpuId0To1ABCD[0] >= 1)
	{
		__cpuidex((int*)&data.cpuId0To1ABCD[4], 1, 0);
	}
	__cpuidex((int*)&data.cpuId80000000To80000004ABCD[0], (int)0x80000000, 0);
	for (DWORD n = (data.cpuId80000000To80000004ABCD[0] < (DWORD)0x80000004) ? data.cpuId80000000To80000004ABCD[0] : (DWORD)0x80000004, i = (DWORD)0x80000001; i <= n; i++)
	{
		__cpuidex((int*)&data.cpuId80000000To80000004ABCD[i * 4], (DWORD)i, 0);
	}
	data.imageFileHandle = imageFileHandle;
	data.volumeHandle = volumeHandle;
	memcpy(&data.imageFileInformation, &imageFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION));
	data.computerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW(&data.computerName[0], &data.computerNameLength);
	memcpy(&data.systemInfo, &systemInfo, sizeof(SYSTEM_INFO));
	data.largePageMinimum = GetLargePageMinimum();
	GetPhysicallyInstalledSystemMemory(&data.physicallyInstalledSystemMemory);
	data.automaticSystemResume = IsSystemResumeAutomatic();
	data.maximumProcessorGroupCount = GetMaximumProcessorGroupCount();
	data.firstGroupMaximumProcessorCount = GetMaximumProcessorCount(0);
	GetNumaHighestNodeNumber(&data.highestNumaNodeNumber);
	GetSystemRegistryQuota(&data.registryQuotaAllowed, &data.registryQuotaUsed);
	data.imageBase = (HMODULE)&__ImageBase;
	data.kernel32 = kernel32Module;
	data.advapi32 = advapi32Module;
	data.iphlpapi = iphlpapiModule;
	data.user32 = user32Module;
	data.currentProcess = currentProcess;
	data.currentThread = currentThread;
	data.currentProcessToken = currentProcessTokenHandle;
	data.commandLine = GetCommandLineW();
	GetStartupInfoW(&data.startupInfo);
	data.threadErrorMode = GetThreadErrorMode();
	data.currentProcessId = GetCurrentProcessId();
	data.currentThreadId = GetCurrentThreadId();
	ProcessIdToSessionId(data.currentProcessId, &data.currentSessionId);
	GetDynamicTimeZoneInformation(&data.dynamicTimeZoneInformation);
	data.threadLocale = GetThreadLocale();
	data.threadUILanguage = GetThreadUILanguage();
	data.consoleWindow = GetConsoleWindow();
	GetSystemPowerStatus(&data.systemPowerStatus);
	GetProcessWorkingSetSizeEx(currentProcess, &data.workingSetMinimum, &data.workingSetMaximum, &data.workingSetFlags);
	data.stdInput = GetStdHandle(STD_INPUT_HANDLE);
	data.stdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	data.stdError = GetStdHandle(STD_ERROR_HANDLE);
	GetCurrentProcessorNumberEx(&data.currentProcessorNumber);
	data.activeProcessorGroupCount = GetActiveProcessorGroupCount();
	data.firstGroupActiveProcessorGroupCount = GetActiveProcessorCount(0);
	data.memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&data.memoryStatus);
	data.currentThreadPriority = GetThreadPriority(currentThread);
	data.currentProcessClass = GetPriorityClass(currentProcess);
	GetProcessHandleCount(currentProcess, &data.currentProcessHandleCount);
	memcpy(&data.cursorPosition, &cursorPosition, sizeof(POINT));
	data.keybordLayout = keybordLayout;
	data.isRemoteSession = isRemoteSession;
	GetLocalTime(&data.localTime);
	GetProcessIoCounters(currentProcess, &data.currentProcessIoCounters);
	GetProcessTimes(currentProcess, &data.currentProcessCreationTime, &data.currentProcessExitTime, &data.currentProcessKernelTime, &data.currentProcessUserTime);
	QueryProcessCycleTime(currentProcess, &data.currentProcessCycleCount);
	data.tickCount = GetTickCount64();
	data.systemTime = systemTime;
	QueryUnbiasedInterruptTime(&data.unbiasedInterruptTime);
	QueryPerformanceFrequency(&data.performanceFrequency);
	QueryPerformanceCounter(&data.performanceCounter);
	data.processorClockCycles = __rdtsc();

	*nonce = (uint64_t)systemTime;

	FlSha256HashData(&sha256Context, sizeof(FlInternalGenerateRandomSeedData), &data);
	FlSha256FinishHash(&sha256Context, randomSeed);
}

volatile uint64_t randomInternalRngNonce;
#ifdef _WIN64
volatile uint64_t randomInternalRngAtomicCounter;
#else
volatile uint32_t randomInternalRngAtomicCounter;
#endif // _WIN64
volatile uint64_t randomInternalRngState[4];

static void FlRandomInternalGenerateRandomBlock(void* buffer)
{
	// Note that there are races in this function, but all the results of these races are valid.

	uint64_t nonce = randomInternalRngNonce;
	if (!nonce)
	{
		uint64_t randomSeed[4];
		FlRandomInternalGenerateRandomSeed(&nonce, &randomSeed);
		randomInternalRngState[0] = randomSeed[0];
		randomInternalRngState[1] = randomSeed[1];
		randomInternalRngState[2] = randomSeed[2];
		randomInternalRngState[3] = randomSeed[3];
		randomInternalRngNonce = nonce;
	}
#ifdef _WIN64
	uint64_t counter = (uint64_t)InterlockedIncrement64((LONG64 volatile*)&randomInternalRngAtomicCounter);
#else
	uint64_t counter = (uint64_t)InterlockedIncrement((LONG volatile*)&randomInternalRngAtomicCounter);
#endif // _WIN64

	uint64_t nonceAndCounter[2] = { nonce, counter };
	FlSha256HmacContext hmacContext;
	FlSha256HmacCreateHmac(&hmacContext, sizeof(randomInternalRngState), (const void*)&randomInternalRngState[0]);
	FlSha256HmacHashData(&hmacContext, sizeof(nonceAndCounter), &nonceAndCounter[0]);
	FlSha256Hmac256FinishHmac(&hmacContext, buffer);
}

void FlGenerateRandomData(_In_ size_t size, _Out_writes_bytes_all_(size) void* buffer)
{
	size_t sizeRemainder = size & ((size_t)FL_RANDOM_INTERNAL_BLOCK_SIZE - 1);
	size &= ~((size_t)FL_RANDOM_INTERNAL_BLOCK_SIZE - 1);
	size_t i = 0;
	while (i < size)
	{
		FlRandomInternalGenerateRandomBlock((void*)((uintptr_t)buffer + i));
		i += FL_RANDOM_INTERNAL_BLOCK_SIZE;
	}
	if (sizeRemainder)
	{
		uint8_t remainderBlock[FL_RANDOM_INTERNAL_BLOCK_SIZE];
		FlRandomInternalGenerateRandomBlock(&remainderBlock);
		memcpy((void*)((uintptr_t)buffer + i), &remainderBlock, sizeRemainder);
	}
}

#ifdef __cplusplus
}
#endif // __cplusplus
