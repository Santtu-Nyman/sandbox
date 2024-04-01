#ifndef BROWN_BUFFALO_CONTROLLER_H
#define BROWN_BUFFALO_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <immintrin.h>

void clear(VOID* Memory, SIZE_T Size)
{
	if (Size < sizeof(__m128i))
	{
		for (VOID* MemoryEnd = (VOID*)((UINT_PTR)Memory + Size); Memory != MemoryEnd; Memory = (VOID*)((UINT_PTR)Memory + 1))
			*(BYTE*)Memory = 0;
		return;
	}
	__m128i DataBlock = _mm_setzero_si128();
	if ((UINT_PTR)Memory & (sizeof(__m128i) - 1))
	{
		UINT_PTR AligmentFix = sizeof(__m128i) - ((UINT_PTR)Memory & (sizeof(__m128i) - 1));
		_mm_storeu_si128((__m128i*)Memory, DataBlock);
		Memory = (VOID*)((UINT_PTR)Memory + AligmentFix);
		Size -= AligmentFix;
	}
	for (VOID* MemoryEnd = (VOID*)((UINT_PTR)Memory + (Size & ~(sizeof(__m128i) - 1))); Memory != MemoryEnd; Memory = (VOID*)((UINT_PTR)Memory + sizeof(__m128i)))
		_mm_store_si128((__m128i*)Memory, DataBlock);
	for (VOID* MemoryEnd = (VOID*)((UINT_PTR)Memory + (Size & (sizeof(__m128i) - 1))); Memory != MemoryEnd; Memory = (VOID*)((UINT_PTR)Memory + 1))
		*(BYTE*)Memory = 0;
}

void copy(const VOID* Source, VOID* Destination, SIZE_T Size)
{
	if (Size < sizeof(__m128i))
	{
		for (const VOID* SourceEnd = (const VOID*)((UINT_PTR)Source + Size); Source != SourceEnd; Source = (const VOID*)((UINT_PTR)Source + 1), Destination = (VOID*)((UINT_PTR)Destination + 1))
			*(BYTE*)Destination = *(const BYTE*)Source;
		return;
	}
	__m128i DataBlock;
	if (((UINT_PTR)Destination & (sizeof(__m128i) - 1)) == ((UINT_PTR)Source & (sizeof(__m128i) - 1)))
	{
		if ((UINT_PTR)Source & (sizeof(__m128i) - 1))
		{
			UINT_PTR AligmentFix = sizeof(__m128i) - ((UINT_PTR)Source & (sizeof(__m128i) - 1));
			DataBlock = _mm_loadu_si128((const __m128i*)Source);
			_mm_storeu_si128((__m128i*)Destination, DataBlock);
			Source = (const VOID*)((UINT_PTR)Source + AligmentFix);
			Destination = (VOID*)((UINT_PTR)Destination + AligmentFix);
			Size -= AligmentFix;
		}
		for (const VOID* SourceEnd = (const VOID*)((UINT_PTR)Source + ((UINT_PTR)Size & ~(sizeof(__m128i) - 1))); Source != SourceEnd; Source = (const VOID*)((UINT_PTR)Source + sizeof(__m128i)), Destination = (VOID*)((UINT_PTR)Destination + sizeof(__m128i)))
		{
			DataBlock = _mm_load_si128((const __m128i*)Source);
			_mm_store_si128((__m128i*)Destination, DataBlock);
		}
	}
	else
		for (const VOID* SourceEnd = (const VOID*)((UINT_PTR)Source + ((UINT_PTR)Size & ~(sizeof(__m128i) - 1))); Source != SourceEnd; Source = (const VOID*)((UINT_PTR)Source + sizeof(__m128i)), Destination = (VOID*)((UINT_PTR)Destination + sizeof(__m128i)))
		{
			DataBlock = _mm_loadu_si128((const __m128i*)Source);
			_mm_storeu_si128((__m128i*)Destination, DataBlock);
		}
	for (const VOID* SourceEnd = (VOID*)((UINT_PTR)Source + ((UINT_PTR)Size & (sizeof(__m128i) - 1))); Source != SourceEnd; Source = (const VOID*)((UINT_PTR)Source + 1), Destination = (VOID*)((UINT_PTR)Destination + 1))
		*(BYTE*)Destination = *(const BYTE*)Source;
}

DWORD getLastUsedIPAddress(HANDLE heap, WCHAR** IPstring)
{
	DWORD valueBufferSize = (MAX_PATH + 1) * sizeof(WCHAR);
	WCHAR* valueBuffer = (WCHAR*)HeapAlloc(heap, 0, (SIZE_T)valueBufferSize);
	HKEY BrownBuffalo;
	LONG status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Brown Buffalo", 0, KEY_QUERY_VALUE, &BrownBuffalo);
	if (status)
	{
		HeapFree(heap, 0, valueBuffer);
		return (DWORD)status;
	}
	for (DWORD valueType, valueSize;;)
	{
		valueSize = valueBufferSize - sizeof(WCHAR);
		status = RegQueryValueExW(BrownBuffalo, L"last used IP", 0, &valueType, (BYTE*)valueBuffer, &valueSize);
		if (status == ERROR_FILE_NOT_FOUND)
		{
			valueSize = valueBufferSize - sizeof(WCHAR);
			status = RegQueryValueExW(BrownBuffalo, L"default IP", 0, &valueType, (BYTE*)valueBuffer, &valueSize);
		}
		if (status == ERROR_MORE_DATA)
		{
			valueBufferSize = valueSize + sizeof(WCHAR);
			WCHAR* Temporal = (WCHAR*)HeapReAlloc(heap, 0, valueBuffer, (SIZE_T)valueBufferSize);
			if (!Temporal)
			{
				DWORD Error = GetLastError();
				RegCloseKey(BrownBuffalo);
				HeapFree(heap, 0, valueBuffer);
				return Error;
			}
			valueBuffer = Temporal;
		}
		else if (status)
		{
			RegCloseKey(BrownBuffalo);
			HeapFree(heap, 0, valueBuffer);
			return (DWORD)status;
		}
		else if (valueType != REG_SZ || valueSize % sizeof(WCHAR))
		{
			RegCloseKey(BrownBuffalo);
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
	RegCloseKey(BrownBuffalo);
	*IPstring = valueBuffer;
	return 0;
}

DWORD setLastUsedIPAddress(const WCHAR* IPstring)
{
	DWORD IPstringSize = ((DWORD)lstrlenW(IPstring) + 1) * sizeof(WCHAR);
	HKEY BrownBuffalo;
	LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Brown Buffalo", 0, 0, REG_OPTION_VOLATILE, KEY_SET_VALUE, 0, &BrownBuffalo, 0);
	if (!result)
	{
		result = RegSetValueExW(BrownBuffalo, L"last used IP", 0, REG_SZ, (const BYTE*)IPstring, IPstringSize);
		RegCloseKey(BrownBuffalo);
	}
	return (DWORD)result;
}

DWORD print(HANDLE consoleOut, const WCHAR* string)
{
	DWORD consoleWriteLenght = 0;
	DWORD stringLenght = lstrlenW(string);
	return WriteConsoleW(consoleOut, string, stringLenght, &consoleWriteLenght, 0) && consoleWriteLenght == stringLenght ? 0 : GetLastError();
}

DWORD printError(DWORD error)
{
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	SIZE_T formatMessageMaxBufferSize = 0x10000;
	WCHAR* errorMessage = (WCHAR*)VirtualAlloc(0, systemInfo.dwPageSize ? (((((21 * sizeof(WCHAR)) + formatMessageMaxBufferSize) + (systemInfo.dwPageSize - 1)) / (systemInfo.dwPageSize)) * systemInfo.dwPageSize) : ((21 * sizeof(WCHAR)) + formatMessageMaxBufferSize), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!errorMessage)
	{
		formatMessageMaxBufferSize = (SIZE_T)systemInfo.dwPageSize;
		errorMessage = (WCHAR*)VirtualAlloc(0, systemInfo.dwPageSize ? (((((21 * sizeof(WCHAR)) + formatMessageMaxBufferSize) + (systemInfo.dwPageSize - 1)) / (systemInfo.dwPageSize)) * systemInfo.dwPageSize) : ((21 * sizeof(WCHAR)) + formatMessageMaxBufferSize), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!errorMessage)
			return GetLastError();
	}
	errorMessage[0] = L'E';
	errorMessage[1] = L'R';
	errorMessage[2] = L'R';
	errorMessage[3] = L'O';
	errorMessage[4] = L'R';
	errorMessage[5] = L' ';
	errorMessage[6] = L'0';
	errorMessage[7] = L'x';
	for (DWORD index = 0; index != 8; ++index)
		errorMessage[8 + index] = L"0123456789ABCDEF"[(error >> ((7 - index) << 2)) & 0xF];
	errorMessage[16] = L' ';
	errorMessage[17] = L'\"';
	SIZE_T formatMessageLength = (SIZE_T)FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &errorMessage[18], (DWORD)(formatMessageMaxBufferSize / sizeof(WCHAR)), 0);
	if (!formatMessageLength)
	{
		DWORD error = GetLastError();
		VirtualFree(errorMessage, 0, MEM_RELEASE);
		return error;
	}
	while (formatMessageLength && (errorMessage[17 + formatMessageLength] < (WCHAR)0x21))
		--formatMessageLength;
	errorMessage[18 + formatMessageLength] = L'\"';
	errorMessage[19 + formatMessageLength] = L'\n';
	errorMessage[20 + formatMessageLength] = 0;
	HANDLE consoleOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (consoleOut == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		VirtualFree(errorMessage, 0, MEM_RELEASE);
		return error;
	}
	DWORD printError = print(consoleOut, errorMessage);
	CloseHandle(consoleOut);
	VirtualFree(errorMessage, 0, MEM_RELEASE);
	return printError;
}

BYTE crc8(SIZE_T size, const BYTE* data)
{
	BYTE h = 0;
	for (const BYTE* e = data + size; data != e; ++data)
	{
		h ^= *data;
		for (BYTE c = 8; c--;)
			h = (h & 0x80) ? ((h << 1) ^ 7) : (h << 1);
	}
	return h;
}

int decodeAddressStringAndAddPort(const WCHAR* addressString, SOCKADDR_STORAGE* address, u_short port)
{
	clear(address, sizeof(SOCKADDR_STORAGE));
	((SOCKADDR_IN6*)address)->sin6_family = AF_INET6;
	if (InetPtonW(AF_INET6, addressString, &((SOCKADDR_IN6*)address)->sin6_addr) == 1)
	{
		((SOCKADDR_IN6*)address)->sin6_port = htons(port);
		return 0;
	}
	clear(address, sizeof(SOCKADDR_STORAGE));
	((SOCKADDR_IN*)address)->sin_family = AF_INET;
	if (InetPtonW(AF_INET, addressString, &((SOCKADDR_IN*)address)->sin_addr) == 1)
	{
		((SOCKADDR_IN*)address)->sin_port = htons(port);
		return 0;
	}
	else
		return WSAGetLastError();
}

// write packet 0x03, (index[0:4] and size[4:8]), data(0)...data(size-1), crc8(msg)
// write result packet 0x02, (index[0:4] and size[4:8]), crc8(data(0)...data(size-1)), crc8(msg)
// read packet 0x01, (index[0:4] and size[4:8]), crc8(msg)
// read result packet 0x00, (index[0:4] and size[4:8]), data(0)...data(size-1), crc8(msg)

#define SRCP_MAX_PACKET_SIZE 19
#define SRCP_CLIENT_BIT 0x01
#define SRCP_WRITE_BIT 0x02

BYTE srcpCreateWritePacket(BYTE variableIndex, BYTE size, const BYTE* data, BYTE* buffer)
{
	BYTE* write = buffer;
	*write++ = SRCP_CLIENT_BIT | SRCP_WRITE_BIT;
	*write++ = (variableIndex & 0x0F) | ((size - 1) << 4);
	for (const BYTE* end = data + size; data != end;)
		*write++ = *data++;
	*write = crc8(2 + size, buffer);
	return 3 + size;
}

bool srcpIsWriteResultPacketValid(const BYTE* writePacket, const BYTE* writeResultPacket)
{
	return writeResultPacket[0] == SRCP_WRITE_BIT &&
		writePacket[1] == writeResultPacket[1] &&
		writeResultPacket[2] == crc8((writePacket[1] >> 4) + 1, writePacket + 2) &&
		writeResultPacket[3] == crc8(3, writeResultPacket);
}

BYTE srcpCreateReadPacket(BYTE variableIndex, BYTE size, BYTE* buffer)
{
	BYTE* write = buffer;
	*write++ = SRCP_CLIENT_BIT;
	*write++ = (variableIndex & 0x0F) | ((size - 1) << 4);
	*write = crc8(2, buffer);
	return 3;
}

const BYTE* srcpReadReadResultPacket(const BYTE* readPacket, const BYTE* readResultPacket)
{
	return (!readResultPacket[0] &&
		readPacket[1] == readResultPacket[1] &&
		readResultPacket[2 + ((readResultPacket[1] >> 4) + 1)] == crc8(2 + ((readResultPacket[1] >> 4) + 1), readResultPacket))
		? readResultPacket + 2 : 0;
}

DWORD srcpWrite(SOCKET client, const SOCKADDR_STORAGE* serverAddress, int* responseAddressSize, SOCKADDR_STORAGE* responseAddress, ULONGLONG tryTimeSeconds, BYTE variableIndex, BYTE size, const BYTE* data)
{
	BYTE writePacketData[SRCP_MAX_PACKET_SIZE];
	BYTE writeResultPacketData[SRCP_MAX_PACKET_SIZE];
	DWORD error = 0;
	ULONGLONG tryTime = GetTickCount64() + (tryTimeSeconds * 1000);
	int writePacketDataSize = srcpCreateWritePacket(variableIndex, size, data, writePacketData);
	do
	{
		*responseAddressSize = sizeof(SOCKADDR_STORAGE);
		clear(responseAddress, sizeof(SOCKADDR_STORAGE));
		if (sendto(client, (const char*)writePacketData, writePacketDataSize, 0, (const SOCKADDR*)serverAddress, serverAddress->ss_family == AF_INET6 ? sizeof(SOCKADDR_IN6) : sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
		{
			int writeResultPacketDataSize = recvfrom(client, (char*)writeResultPacketData, SRCP_MAX_PACKET_SIZE, 0, (SOCKADDR*)responseAddress, responseAddressSize);
			if (writeResultPacketDataSize != SOCKET_ERROR)
				return srcpIsWriteResultPacketValid(writePacketData, writeResultPacketData) ? 0 : ERROR_INVALID_DATA;
		}
		error = WSAGetLastError();
	} while ((error == WSAETIMEDOUT || error == WSAECONNRESET) && GetTickCount64() < tryTime);
	return error;
}

DWORD srcpRead(SOCKET client, const SOCKADDR_STORAGE* serverAddress, int* responseAddressSize, SOCKADDR_STORAGE* responseAddress, ULONGLONG tryTimeSeconds, BYTE variableIndex, BYTE size, BYTE* data)
{
	BYTE readPacketData[SRCP_MAX_PACKET_SIZE];
	BYTE readResultPacketData[SRCP_MAX_PACKET_SIZE];
	DWORD error = 0;
	ULONGLONG tryTime = GetTickCount64() + (tryTimeSeconds * 1000);
	int readPacketDataSize = srcpCreateReadPacket(variableIndex, size, readPacketData);
	do
	{
		*responseAddressSize = sizeof(SOCKADDR_STORAGE);
		clear(responseAddress, sizeof(SOCKADDR_STORAGE));
		if (sendto(client, (const char*)readPacketData, readPacketDataSize, 0, (const SOCKADDR*)serverAddress, serverAddress->ss_family == AF_INET6 ? sizeof(SOCKADDR_IN6) : sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
		{
			int readResultPacketDataSize = recvfrom(client, (char*)readResultPacketData, SRCP_MAX_PACKET_SIZE, 0, (SOCKADDR*)responseAddress, responseAddressSize);
			if (readResultPacketDataSize != SOCKET_ERROR)
			{
				const BYTE* resultData = srcpReadReadResultPacket(readPacketData, readResultPacketData);
				if (resultData)
				{
					copy(readResultPacketData + 2, data, size);
					return 0;
				}
				else
					return ERROR_INVALID_DATA;
			}
		}
		error = WSAGetLastError();
	} while ((error == WSAETIMEDOUT || error == WSAECONNRESET) && GetTickCount64() < tryTime);
	return error;
}

SIZE_T print13CharacterFloat(float degrees, SIZE_T fractionPartMaximumLength, WCHAR* buffer)
{
	WCHAR* write = buffer;
	if (degrees < 0.0f)
	{
		if (degrees < -999999999.0f)
		{
			copy(L"<-999999999.0", buffer, 13 * sizeof(WCHAR));
			return 13;
		}
		degrees = -degrees;
		*write++ = L'-';
	}
	else
	{
		if (degrees > 999999999.0f)
		{
			copy(L">999999999.0", buffer, 12 * sizeof(WCHAR));
			return 12;
		}
	}
	DWORD wholePart = 1;
	for (DWORD powerOf10 = 10; wholePart != 9 && (DWORD)(degrees / (float)powerOf10); powerOf10 *= 10)
		++wholePart;
	DWORD fractionPart = 9 - wholePart;
	DWORD decimalDigitMovingMultiplier = 1;
	for (DWORD i = 0; i != fractionPart; ++i)
		decimalDigitMovingMultiplier *= 10;
	DWORD integer = (DWORD)(degrees * (float)decimalDigitMovingMultiplier);
	for (DWORD i = wholePart - 1; i--;)
		decimalDigitMovingMultiplier *= 10;
	for (DWORD i = wholePart; i--; decimalDigitMovingMultiplier /= 10)
		*write++ = L'0' + (WCHAR)((integer / decimalDigitMovingMultiplier) % 10);
	*write++ = L'.';
	if (fractionPart)
	{
		DWORD irrelevantZeroes = 0;
		for (DWORD n = fractionPartMaximumLength && fractionPartMaximumLength < (SIZE_T)fractionPart ? (DWORD)fractionPartMaximumLength : fractionPart, i = n; i--; decimalDigitMovingMultiplier /= 10)
		{
			WCHAR newDigit = L'0' + (WCHAR)((integer / decimalDigitMovingMultiplier) % 10);
			if (newDigit != L'0' || i + 1 == n)
				irrelevantZeroes = 0;
			else
				++irrelevantZeroes;
			*write++ = newDigit;
		}
		return ((SIZE_T)((UINT_PTR)write - (UINT_PTR)buffer) / sizeof(WCHAR)) - (SIZE_T)irrelevantZeroes;
	}
	else
	{
		*write++ = L'0';
		return (SIZE_T)((UINT_PTR)write - (UINT_PTR)buffer) / sizeof(WCHAR);
	}
}

BOOL readFloat(const WCHAR* string, float* result)
{
	BOOL negative = *string == L'-';
	if (negative)
		++string;
	if (*string < L'0' || L'9' < *string)
	{
		*result = 0.0f;
		return FALSE;
	}
	float value = 0.0f;
	while (*string >= L'0' && L'9' >= *string)
		value = value * 10.0f + (float)(*string++ - L'0');
	if (*string == L'.')
	{
		++string;
		for (float d = 10.0f; *string >= L'0' && L'9' >= *string; d *= 10.0f)
			value += (float)(*string++ - L'0') / d;
	}
	*result = negative ? -value : value;
	return TRUE;
}

BOOL readIntegerU32(const WCHAR* string, DWORD* result)
{
	if (*string < L'0' || L'9' < *string)
	{
		*result = 0;
		return FALSE;
	}
	DWORD value = 0;
	for (DWORD temporal; *string >= L'0' && L'9' >= *string;)
	{
		temporal = value * 10;
		if (temporal / 10 != value)
		{
			*result = 0xFFFFFFFF;
			return TRUE;
		}
		value = temporal;
		temporal = value + (DWORD)(*string++ - L'0');
		if (temporal < value)
		{
			*result = 0xFFFFFFFF;
			return TRUE;
		}
		value = temporal;
	}
	*result = value;
	return TRUE;
}

#define FAN_STATUS_MASK_TEMPERATURE 0x0004
#define FAN_STATUS_MASK_LOW 0x0008
#define FAN_STATUS_MASK_HIGH 0x0010
#define FAN_STATUS_MASK_SPEED 0x0020
typedef struct fanStatusData
{
	float temperature;
	float lowLimit;
	float highLimit;
	BYTE fanSpeed;
} fanStatusData;

void printFanStatus(const fanStatusData* fanStatus, WCHAR* buffer256characters)
{
	//"temperature <-999999999.0\nfan speed invalid speed value\ntemperature low limit <-999999999.0\ntemperature high limit <-999999999.0.\n";
	WCHAR* write = buffer256characters;
	copy(L"temperature ", write, 12 * sizeof(WCHAR));
	write += 12;
	write += print13CharacterFloat(fanStatus->temperature, 1, write);
	copy(L" c\nfan speed ", write, 13 * sizeof(WCHAR));
	write += 13;
	if (!fanStatus->fanSpeed)
	{
		copy(L"0/3 turned off", write, 14 * sizeof(WCHAR));
		write += 14;
	}
	else if (fanStatus->fanSpeed < 4)
	{
		*write++ = (WCHAR)(0x30 + (fanStatus->fanSpeed % 10));
		copy(L"/3", write, 2 * sizeof(WCHAR));
		write += 2;
	}
	else
	{
		copy(L"invalid speed value", write, 19 * sizeof(WCHAR));
		write += 19;
	}
	copy(L"\ntemperature low limit ", write, 23 * sizeof(WCHAR));
	write += 23;
	write += print13CharacterFloat(fanStatus->lowLimit, 1, write);
	copy(L"\ntemperature high limit ", write, 24 * sizeof(WCHAR));
	write += 24;
	write += print13CharacterFloat(fanStatus->highLimit, 1, write);
	copy(L"\n", write, 2 * sizeof(WCHAR));
}

u_short getClientRandomPort()
{
	u_short clientPort = 0x29A;
	while (clientPort == 0x29A)
		clientPort = (u_short)(0x80 + ((GetCurrentThreadId() ^ (GetTickCount() >> 4)) & 0xFFF));
	return clientPort;
}

DWORD setFanStatus(const WCHAR* serverAddressString, ULONGLONG tryTimeSeconds, WORD fanStatusMask, const fanStatusData* fanStatus)
{
	if (fanStatusMask & ~(FAN_STATUS_MASK_SPEED | FAN_STATUS_MASK_LOW | FAN_STATUS_MASK_HIGH))
		return ERROR_INVALID_PARAMETER;
	const DWORD IPV6Only = 0;
	DWORD error = 0;
	u_short clientPort = getClientRandomPort();
	WSADATA wsaStartupData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaStartupData))
		return (DWORD)WSAGetLastError();
	DWORD receiveTimeout = tryTimeSeconds * 1000 < 0x1000 ? 0x400 : 0x1000;
	int otherAddressSize;
	SOCKADDR_STORAGE otherAddress;
	SOCKADDR_STORAGE serverAddress;
	SOCKET client = SOCKET_ERROR;
	error = decodeAddressStringAndAddPort(serverAddressString, &serverAddress, 0x29A);
	if (error)
	{
		WSACleanup();
		return error;
	}
	setLastUsedIPAddress(serverAddressString);
	if (serverAddress.ss_family == AF_INET6)
	{
		clear(&otherAddress, sizeof(SOCKADDR_STORAGE));
		((SOCKADDR_IN6*)&otherAddress)->sin6_family = AF_INET6;
		((SOCKADDR_IN6*)&otherAddress)->sin6_addr = IN6ADDR_ANY_INIT;
		((SOCKADDR_IN6*)&otherAddress)->sin6_port = htons(clientPort);
		client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (client != SOCKET_ERROR)
		{
			if (setsockopt(client, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&IPV6Only, sizeof(DWORD)) != SOCKET_ERROR && setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) != SOCKET_ERROR && bind(client, (SOCKADDR*)&otherAddress, sizeof(SOCKADDR_IN6)) != SOCKET_ERROR)
				error = 0;
			else
			{
				error = WSAGetLastError();
				shutdown(client, SD_BOTH);
				closesocket(client);
			}
		}
		else
			error = WSAGetLastError();
	}
	else if (serverAddress.ss_family == AF_INET)
	{
		clear(&otherAddress, sizeof(SOCKADDR_STORAGE));
		((SOCKADDR_IN*)&otherAddress)->sin_family = AF_INET;
		((SOCKADDR_IN*)&otherAddress)->sin_addr.s_addr = INADDR_ANY;
		((SOCKADDR_IN*)&otherAddress)->sin_port = htons(clientPort);
		client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (client != SOCKET_ERROR)
		{
			if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) != SOCKET_ERROR && bind(client, (SOCKADDR*)&otherAddress, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
				error = 0;
			else
			{
				error = WSAGetLastError();
				shutdown(client, SD_BOTH);
				closesocket(client);
			}
		}
		else
			error = WSAGetLastError();
	}
	else
	{
		WSACleanup();
		return ERROR_INVALID_PARAMETER;
	}
	if (error)
	{
		WSACleanup();
		return error;
	}
	if (fanStatusMask & FAN_STATUS_MASK_SPEED)
	{
		error = srcpWrite(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 2, 1, (const BYTE*)&fanStatus->fanSpeed);
		if (error)
		{
			shutdown(client, SD_BOTH);
			closesocket(client);
			WSACleanup();
			return error;
		}
	}
	if (fanStatusMask & FAN_STATUS_MASK_LOW)
	{
		error = srcpWrite(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 3, 4, (const BYTE*)&fanStatus->lowLimit);
		if (error)
		{
			shutdown(client, SD_BOTH);
			closesocket(client);
			WSACleanup();
			return error;
		}
	}
	if (fanStatusMask & FAN_STATUS_MASK_HIGH)
		error = srcpWrite(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 4, 4, (const BYTE*)&fanStatus->highLimit);
	shutdown(client, SD_BOTH);
	closesocket(client);
	WSACleanup();
	return error;
}

DWORD createConnection(const WCHAR* serverAddressString, WSADATA* WindowsSocketsInfo, SOCKADDR_STORAGE* serverAddress, SOCKET* clientSocket)
{
	const DWORD receiveTimeout = 0x1000;
	const DWORD IPV6Only = 0;
	DWORD error = 0;
	u_short clientPort = getClientRandomPort();
	SOCKADDR_STORAGE clientAddress;
	SOCKET client = SOCKET_ERROR;
	error = decodeAddressStringAndAddPort(serverAddressString, serverAddress, 0x29A);
	if (error)
		return error;
	if (serverAddress->ss_family == AF_INET6)
	{
		clear(&clientAddress, sizeof(SOCKADDR_STORAGE));
		((SOCKADDR_IN6*)&clientAddress)->sin6_family = AF_INET6;
		((SOCKADDR_IN6*)&clientAddress)->sin6_addr = IN6ADDR_ANY_INIT;
		((SOCKADDR_IN6*)&clientAddress)->sin6_port = htons(clientPort);
		client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (client != SOCKET_ERROR)
		{
			if (setsockopt(client, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&IPV6Only, sizeof(DWORD)) == SOCKET_ERROR || setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) == SOCKET_ERROR || bind(client, (SOCKADDR*)&clientAddress, sizeof(SOCKADDR_IN6)) == SOCKET_ERROR)
			{
				error = WSAGetLastError();
				shutdown(client, SD_BOTH);
				closesocket(client);
			}
		}
		else
			error = WSAGetLastError();
	}
	else if (serverAddress->ss_family == AF_INET)
	{
		clear(&clientAddress, sizeof(SOCKADDR_STORAGE));
		((SOCKADDR_IN*)&clientAddress)->sin_family = AF_INET;
		((SOCKADDR_IN*)&clientAddress)->sin_addr.s_addr = INADDR_ANY;
		((SOCKADDR_IN*)&clientAddress)->sin_port = htons(clientPort);
		client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (client != SOCKET_ERROR)
		{
			if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) == SOCKET_ERROR || bind(client, (SOCKADDR*)&clientAddress, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
			{
				error = WSAGetLastError();
				shutdown(client, SD_BOTH);
				closesocket(client);
			}
		}
		else
			error = WSAGetLastError();
	}
	else
		return ERROR_INVALID_PARAMETER;
	if (error)
		return error;
	*clientSocket = client;
	return 0;
}

DWORD getFanStatus(const WCHAR* serverAddressString, ULONGLONG tryTimeSeconds, WORD fanStatusMask, fanStatusData* fanStatus)
{
	if (fanStatusMask & ~(FAN_STATUS_MASK_TEMPERATURE | FAN_STATUS_MASK_SPEED | FAN_STATUS_MASK_LOW | FAN_STATUS_MASK_HIGH))
		return ERROR_INVALID_PARAMETER;
	const DWORD IPV6Only = 0;
	DWORD error = 0;
	u_short clientPort = getClientRandomPort();
	WSADATA wsaStartupData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaStartupData))
		return (DWORD)WSAGetLastError();
	DWORD receiveTimeout = tryTimeSeconds * 1000 < 0x1000 ? 0x400 : 0x1000;
	int otherAddressSize;
	SOCKADDR_STORAGE otherAddress;
	SOCKADDR_STORAGE serverAddress;
	SOCKET client = SOCKET_ERROR;
	error = decodeAddressStringAndAddPort(serverAddressString, &serverAddress, 0x29A);
	if (error)
	{
		WSACleanup();
		return error;
	}
	setLastUsedIPAddress(serverAddressString);
	if (serverAddress.ss_family == AF_INET6)
	{
		clear(&otherAddress, sizeof(SOCKADDR_STORAGE));
		((SOCKADDR_IN6*)&otherAddress)->sin6_family = AF_INET6;
		((SOCKADDR_IN6*)&otherAddress)->sin6_addr = IN6ADDR_ANY_INIT;
		((SOCKADDR_IN6*)&otherAddress)->sin6_port = htons(clientPort);
		client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (client != SOCKET_ERROR)
		{
			if (setsockopt(client, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&IPV6Only, sizeof(DWORD)) != SOCKET_ERROR && setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) != SOCKET_ERROR && bind(client, (SOCKADDR*)&otherAddress, sizeof(SOCKADDR_IN6)) != SOCKET_ERROR)
				error = 0;
			else
			{
				error = WSAGetLastError();
				shutdown(client, SD_BOTH);
				closesocket(client);
			}
		}
		else
			error = WSAGetLastError();
	}
	else if (serverAddress.ss_family == AF_INET)
	{
		clear(&otherAddress, sizeof(SOCKADDR_STORAGE));
		((SOCKADDR_IN*)&otherAddress)->sin_family = AF_INET;
		((SOCKADDR_IN*)&otherAddress)->sin_addr.s_addr = INADDR_ANY;
		((SOCKADDR_IN*)&otherAddress)->sin_port = htons(clientPort);
		client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (client != SOCKET_ERROR)
		{
			if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) != SOCKET_ERROR && bind(client, (SOCKADDR*)&otherAddress, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
				error = 0;
			else
			{
				error = WSAGetLastError();
				shutdown(client, SD_BOTH);
				closesocket(client);
			}
		}
		else
			error = WSAGetLastError();
	}
	else
	{
		WSACleanup();
		return ERROR_INVALID_PARAMETER;
	}
	if (error)
	{
		WSACleanup();
		return error;
	}
	if (fanStatusMask & FAN_STATUS_MASK_TEMPERATURE)
	{
		error = srcpRead(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 5, 4, (BYTE*)&fanStatus->temperature);
		if (error)
		{
			shutdown(client, SD_BOTH);
			closesocket(client);
			WSACleanup();
			return error;
		}
	}
	if (fanStatusMask & FAN_STATUS_MASK_SPEED)
	{
		error = srcpRead(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 2, 1, (BYTE*)&fanStatus->fanSpeed);
		if (error)
		{
			shutdown(client, SD_BOTH);
			closesocket(client);
			WSACleanup();
			return error;
		}
	}
	if (fanStatusMask & FAN_STATUS_MASK_LOW)
	{
		error = srcpRead(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 3, 4, (BYTE*)&fanStatus->lowLimit);
		if (error)
		{
			shutdown(client, SD_BOTH);
			closesocket(client);
			WSACleanup();
			return error;
		}
	}
	if (fanStatusMask & FAN_STATUS_MASK_HIGH)
		error = srcpRead(client, &serverAddress, &otherAddressSize, &otherAddress, tryTimeSeconds, 4, 4, (BYTE*)&fanStatus->highLimit);
	shutdown(client, SD_BOTH);
	closesocket(client);
	WSACleanup();
	return error;
}

/*

DWORD CALLBACK fakeServer(LPVOID Parameter)
{
	HANDLE consoleOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	SOCKET s;
	struct sockaddr_in6 server;
	struct sockaddr_in6 other;
	int otherAddressSize = sizeof(struct sockaddr_in6);
	int packetSize;
	BYTE buf[256];
	WSADATA wsa;
	clear(&server, sizeof(struct sockaddr_in6));
	server.sin6_family = AF_INET;
	server.sin6_addr = IN6ADDR_ANY_INIT;
	server.sin6_port = htons(0x29A);
	clear(&other, sizeof(struct sockaddr_in6));
	WSAStartup(MAKEWORD(2, 2), &wsa);


	print(consoleOut, L"Initialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		print(consoleOut, L"Failed WSAStartup.\n");
		ExitThread(-1);
	}
	print(consoleOut, L"Initialised.\n");

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == INVALID_SOCKET)
		print(consoleOut, L"Could not create socket.\n");
	print(consoleOut, L"Socket created.\n");

	server.sin6_family = AF_INET6;
	server.sin6_addr = IN6ADDR_ANY_INIT;
	server.sin6_port = htons(0x29A);

	if (bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		print(consoleOut, L"Bind failed.\n");
		ExitThread(-1);
	}
	print(consoleOut, L"Bind done.\n");

	SetEvent(*(HANDLE*)Parameter);

	CHAR networkConfiguration[0x80];
	BYTE fanSpeed = 0;
	float min = 28.0f;
	float max = 32.0f;
	float temperature = 420.0f;
	BYTE* testTable[0x10];
	testTable[0] = (BYTE*)testTable;
	testTable[1] = (BYTE*)networkConfiguration;
	testTable[2] = (BYTE*)&fanSpeed;
	testTable[3] = (BYTE*)&min;
	testTable[4] = (BYTE*)&max;
	testTable[5] = (BYTE*)&temperature;

	for (;;)
	{
		print(consoleOut, L"Server waiting for data...\n");

		clear(buf, 256);


		if ((packetSize = recvfrom(s, (char*)buf, 256, 0, (struct sockaddr *)&other, &otherAddressSize)) == SOCKET_ERROR)
		{
			print(consoleOut, L"recvfrom() failed.\n");
			ExitThread(-1);
		}


		print(consoleOut, L"\"");
		WCHAR packetHex[3];
		packetHex[2] = 0;
		for (const BYTE* i = (const BYTE*)buf, *e = i + packetSize; i != e; ++i)
		{
			packetHex[0] = L"0123456789ABCDEF"[*i & 0xF];
			packetHex[1] = L"0123456789ABCDEF"[*i >> 4];
			print(consoleOut, packetHex);
		}
		print(consoleOut, L"\".\n");

		if (packetSize > 3 && buf[0] == 0x03 && packetSize == (int)(3 + ((buf[1] >> 4) + 1)) && buf[2 + ((buf[1] >> 4) + 1)] == crc8(2 + ((buf[1] >> 4) + 1), (const BYTE*)buf))
		{
			copy(buf + 2, testTable[buf[1] & 0xF], ((buf[1] >> 4) + 1));
			buf[0] = 0x02;
			buf[2] = crc8(((buf[1] >> 4) + 1), testTable[buf[1] & 0xF]);
			buf[3] = crc8(3, (const BYTE*)buf);
			if (sendto(s, (const char*)buf, 4, 0, (struct sockaddr*)&other, otherAddressSize) == SOCKET_ERROR)
			{
				print(consoleOut, L"sendto() failed.\n");
				ExitThread(-1);
			}
		}
		else if (packetSize == 3 && buf[0] == 0x01 && buf[2] == crc8(2, (const BYTE*)buf))
		{
			buf[0] = 0x00;
			copy(testTable[buf[1] & 0xF], buf + 2, ((buf[1] >> 4) + 1));
			buf[2 + ((buf[1] >> 4) + 1)] = crc8(2 + ((buf[1] >> 4) + 1), (const BYTE*)buf);
			if (sendto(s, (const char*)buf, 3 + ((buf[1] >> 4) + 1), 0, (struct sockaddr*)&other, otherAddressSize) == SOCKET_ERROR)
			{
				print(consoleOut, L"sendto() failed.\n");
				ExitThread(-1);
			}
		}
		else
		{
			print(consoleOut, L" recieved bad packet.\n");
		}
	}
	closesocket(s);
	WSACleanup();
	CloseHandle(consoleOut);
	ExitThread(0);
}

void startFakeServer()
{
	HANDLE event = CreateEventW(0, FALSE, FALSE, 0);
	CloseHandle(CreateThread(0, 0, fakeServer, &event, 0, 0));
	WaitForSingleObject(event, INFINITE);
	CloseHandle(event);
}

*/

#ifdef __cplusplus
}
#endif

#endif