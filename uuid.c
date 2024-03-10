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
#include "uuid.h"
#include <stdint.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef struct
{
	uint64_t Size;
	uint32_t Buffer[8];
	uint8_t Input[64];
} UuidInternalSha256Context;

#define UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(X, N) ((X >> N) | (X << (32 - N)))

static const uint32_t UuidInternalSha256InternalConstantTableK[64] = {
	0x428a2f98lu, 0x71374491lu, 0xb5c0fbcflu, 0xe9b5dba5lu, 0x3956c25blu, 0x59f111f1lu, 0x923f82a4lu, 0xab1c5ed5lu,
	0xd807aa98lu, 0x12835b01lu, 0x243185belu, 0x550c7dc3lu, 0x72be5d74lu, 0x80deb1felu, 0x9bdc06a7lu, 0xc19bf174lu,
	0xe49b69c1lu, 0xefbe4786lu, 0x0fc19dc6lu, 0x240ca1cclu, 0x2de92c6flu, 0x4a7484aalu, 0x5cb0a9dclu, 0x76f988dalu,
	0x983e5152lu, 0xa831c66dlu, 0xb00327c8lu, 0xbf597fc7lu, 0xc6e00bf3lu, 0xd5a79147lu, 0x06ca6351lu, 0x14292967lu,
	0x27b70a85lu, 0x2e1b2138lu, 0x4d2c6dfclu, 0x53380d13lu, 0x650a7354lu, 0x766a0abblu, 0x81c2c92elu, 0x92722c85lu,
	0xa2bfe8a1lu, 0xa81a664blu, 0xc24b8b70lu, 0xc76c51a3lu, 0xd192e819lu, 0xd6990624lu, 0xf40e3585lu, 0x106aa070lu,
	0x19a4c116lu, 0x1e376c08lu, 0x2748774clu, 0x34b0bcb5lu, 0x391c0cb3lu, 0x4ed8aa4alu, 0x5b9cca4flu, 0x682e6ff3lu,
	0x748f82eelu, 0x78a5636flu, 0x84c87814lu, 0x8cc70208lu, 0x90befffalu, 0xa4506ceblu, 0xbef9a3f7lu, 0xc67178f2lu };

static void UuidInternalSha256InternalConsumeChunk(uint32_t* h, const uint32_t* p)
{
	uint32_t ah[8];
	uint32_t w[16];
	for (int i = 0; i < 8; i++)
	{
		ah[i] = h[i];
	}
	for (int i = 0; i < 16; i++)
	{
		uint32_t wx = p[i];
		wx = (wx >> 24) | ((wx >> 8) & 0xff00) | ((wx << 8) & 0xff0000) | (wx << 24);
		uint32_t s1 = UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 6) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 11) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 25);
		uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
		uint32_t temp1 = ah[7] + s1 + ch + UuidInternalSha256InternalConstantTableK[i] + wx;
		uint32_t s0 = UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 2) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 13) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 22);
		uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
		uint32_t temp2 = s0 + maj;
		w[i] = wx;
		ah[7] = ah[6];
		ah[6] = ah[5];
		ah[5] = ah[4];
		ah[4] = ah[3] + temp1;
		ah[3] = ah[2];
		ah[2] = ah[1];
		ah[1] = ah[0];
		ah[0] = temp1 + temp2;
	}
	for (int i = 0; i < 48; i++)
	{
		int j = i & 0xf;
		uint32_t s0 = UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 1) & 0xf], 7) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 1) & 0xf], 18) ^ (w[(j + 1) & 0xf] >> 3);
		uint32_t s1 = UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 14) & 0xf], 17) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(w[(j + 14) & 0xf], 19) ^ (w[(j + 14) & 0xf] >> 10);
		uint32_t wx = w[j] + s0 + w[(j + 9) & 0xf] + s1;
		uint32_t s3 = UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 6) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 11) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[4], 25);
		uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
		uint32_t temp1 = ah[7] + s3 + ch + UuidInternalSha256InternalConstantTableK[((i & 0x30) + 16) | j] + wx;
		uint32_t s2 = UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 2) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 13) ^ UUID_INTERNAL_SHA256_INTERNAL_ROTATE_RIGHT_32(ah[0], 22);
		uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
		uint32_t temp2 = s2 + maj;
		w[j] = wx;
		ah[7] = ah[6];
		ah[6] = ah[5];
		ah[5] = ah[4];
		ah[4] = ah[3] + temp1;
		ah[3] = ah[2];
		ah[2] = ah[1];
		ah[1] = ah[0];
		ah[0] = temp1 + temp2;
	}
	for (int i = 0; i < 8; i++)
	{
		h[i] += ah[i];
	}
}

void UuidInternalSha256Initialize(UuidInternalSha256Context* Context)
{
	Context->Size = 0;
	Context->Buffer[0] = 0x6a09e667lu;
	Context->Buffer[1] = 0xbb67ae85lu;
	Context->Buffer[2] = 0x3c6ef372lu;
	Context->Buffer[3] = 0xa54ff53alu;
	Context->Buffer[4] = 0x510e527flu;
	Context->Buffer[5] = 0x9b05688clu;
	Context->Buffer[6] = 0x1f83d9ablu;
	Context->Buffer[7] = 0x5be0cd19lu;
	memset(&Context->Input, 0, 64);
}

void UuidInternalSha256Update(UuidInternalSha256Context* Context, size_t InputSize, const void* InputData)
{
	const uint8_t* Input = (const uint8_t*)InputData;
	int InitialStepOffset = (int)(Context->Size & 0x3F);
	Context->Size += InputSize;
	int InitialStepInputSize = 64 - InitialStepOffset;
	if ((size_t)InitialStepInputSize > InputSize)
	{
		InitialStepInputSize = (int)InputSize;
	}
	memcpy(Context->Input + InitialStepOffset, InputData, InitialStepInputSize);
	if (InitialStepOffset + InitialStepInputSize < 64)
	{
		return;
	}
	UuidInternalSha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
	Input += (size_t)InitialStepInputSize;
	InputSize -= (size_t)InitialStepInputSize;
	while (InputSize >= 64)
	{
		memcpy(Context->Input, Input, 64);
		UuidInternalSha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
		Input += 64;
		InputSize -= 64;
	}
	memcpy(Context->Input, Input, InputSize);
}

void UuidInternalSha256Finalize(UuidInternalSha256Context* Context, void* Digest)
{
	size_t ChunkIndex = Context->Size & 0x3F;
	Context->Input[ChunkIndex] = 0x80;
	ChunkIndex++;
	if (ChunkIndex > 56)
	{
		memset(Context->Input + ChunkIndex, 0, 64 - ChunkIndex);
		UuidInternalSha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
		ChunkIndex = 0;
	}
	memset(Context->Input + ChunkIndex, 0, 56 - ChunkIndex);
	uint64_t BitSize = Context->Size << 3;
	for (int i = 0; i < 8; i++)
	{
		Context->Input[56 + i] = (uint8_t)(BitSize >> ((7 - i) << 3));
	}
	UuidInternalSha256InternalConsumeChunk(Context->Buffer, (const uint32_t*)&Context->Input);
	for (int i = 0; i < 8; i++)
	{
		uint32_t swap = Context->Buffer[i];
		swap = (swap >> 24) | ((swap >> 8) & 0xff00) | ((swap << 8) & 0xff0000) | (swap << 24);
		Context->Buffer[i] = swap;
	}
	memcpy(Digest, Context->Buffer, 32);
}

typedef struct
{
	void* BufferAddress;
	DWORD ComputerNameLength;
	WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	SYSTEM_INFO SystemInfo;
	SIZE_T LargePageMinimum;
	ULONGLONG PhysicallyInstalledSystemMemory;
	WORD MaximumProcessorGroupCount;
	DWORD FirstGroupMaximumProcessorCount;
	ULONG HighestNumaNodeNumber;
	HMODULE ImageBase;
	HMODULE Kernel32;
	HANDLE CurrentProcess;
	HANDLE CurrentThread;
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
} UuidInternalGenerateRandomSeedData;

static void UuidInternalGenerateRandomSeed(uint64_t* Nonce, void* RandomSeed)
{
	HANDLE CurrentProcess = GetCurrentProcess();
	HANDLE CurrentThread = GetCurrentThread();
	DWORD64 SystemTime = 0;
	GetSystemTimeAsFileTime((FILETIME*)&SystemTime);
	UuidInternalGenerateRandomSeedData Data;

	memset(&Data, 0, sizeof(UuidInternalGenerateRandomSeedData));
	Data.BufferAddress = &Data;
	Data.ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW(&Data.ComputerName[0], &Data.ComputerNameLength);
	GetSystemInfo(&Data.SystemInfo);
	Data.LargePageMinimum = GetLargePageMinimum();
	GetPhysicallyInstalledSystemMemory(&Data.PhysicallyInstalledSystemMemory);
	Data.MaximumProcessorGroupCount = GetMaximumProcessorGroupCount();
	Data.FirstGroupMaximumProcessorCount = GetMaximumProcessorCount(0);
	GetNumaHighestNodeNumber(&Data.HighestNumaNodeNumber);
	Data.ImageBase = (HMODULE)&__ImageBase;
	Data.Kernel32 = GetModuleHandleW(L"Kernel32.dll");
	Data.CurrentProcess = CurrentProcess;
	Data.CurrentThread = CurrentThread;
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

	UuidInternalSha256Context Sha256Context;
	UuidInternalSha256Initialize(&Sha256Context);
	UuidInternalSha256Update(&Sha256Context, sizeof(UuidInternalGenerateRandomSeedData), &Data);
	UuidInternalSha256Finalize(&Sha256Context, RandomSeed);
}

volatile uint64_t UuidInternalRngNonce;
#ifdef _WIN64
volatile uint64_t UuidInternalRngAtomicCounter;
#else
volatile uint32_t UuidInternalRngAtomicCounter;
#endif // _WIN64
volatile uint64_t UuidInternalRngState[4];

static void UuidInternalGenerateRandomBits(void* Buffer)
{
	uint64_t Nonce = UuidInternalRngNonce;
	if (!Nonce)
	{
		uint64_t RandomSeed[4];
		UuidInternalGenerateRandomSeed(&Nonce, &RandomSeed);
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
	UuidInternalSha256Context Sha256Context;
	UuidInternalSha256Initialize(&Sha256Context);
	UuidInternalSha256Update(&Sha256Context, sizeof(RngBuffer), &RngBuffer);
	UuidInternalSha256Finalize(&Sha256Context, &NextRngState);
	UuidInternalRngState[0] = NextRngState[0];
	UuidInternalRngState[1] = NextRngState[1];
	UuidInternalRngState[2] = NextRngState[2];
	UuidInternalRngState[3] = NextRngState[3];

	memcpy(Buffer, &NextRngState, 16);
}

void UuidCreateRandomId(void* Uuid)
{
	uint8_t* RandomIdBytes = (uint8_t*)Uuid;
	UuidInternalGenerateRandomBits(RandomIdBytes);
	RandomIdBytes[7] = (RandomIdBytes[7] & 0x0F) | 0x40;
	RandomIdBytes[8] = (RandomIdBytes[8] & 0x3F) | 0x80;
}

size_t UuidEncodeStringUtf8(const void* Uuid, char* Buffer)
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

size_t UuidDecodeStringUtf8(void* Uuid, size_t Length, const char* String)
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

size_t UuidEncodeStringUtf16(const void* Uuid, WCHAR* Buffer)
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

size_t UuidDecodeStringUtf16(void* Uuid, size_t Length, const WCHAR* String)
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
