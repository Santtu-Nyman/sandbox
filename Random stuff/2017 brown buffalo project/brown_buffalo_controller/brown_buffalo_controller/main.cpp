#ifdef __cplusplus
extern "C" {
#endif

// t‰t‰ ei k‰ytet‰ mihink‰‰n visual studio tarvitsee sen kun c standard libi‰ ei k‰ytet‰
int _fltused = 0;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <immintrin.h>
#include "brown_buffalo_controller.h"

#define CONTROLLER_STATUS_NO_IP 0
#define CONTROLLER_STATUS_NOT_CONNECTED 1
#define CONTROLLER_STATUS_CONNECTING 2
#define CONTROLLER_STATUS_CONNECTED 3
#define CONTROLLER_TIMIR_ID 0x5457
#define CONTROLLER_BUTTON_IP_ID 0x5049
#define CONTROLLER_BUTTON_FAN_SPEED_ID 0x5046
#define CONTROLLER_BUTTON_LOW_LIMIT_ID 0x4C4C
#define CONTROLLER_BUTTON_HIGH_LIMIT_ID 0x4C48
#define CONTROLLER_CONNECTION_THREAD_STATUS_CLOSED 0
#define CONTROLLER_CONNECTION_THREAD_STATUS_RUN 1
#define CONTROLLER_CONNECTION_THREAD_STATUS_SHUTDOW 2
#define CONTROLLER_CONNECTION_THREAD_STATUS_EXITED 3

typedef struct controllerMainData
{
	BOOL closeWindow;
	DWORD status;
	DWORD waitTimer;
	DWORD timedMessageTimer;
	HWND windowHandle;
	HWND statusTextWindow;
	HWND setIPButton;
	HWND setFanSpeedButton;
	HWND setLowLimitButton;
	HWND setHighLimitButton;
	HWND setIPEdit;
	WNDPROC setIPEditProcedure;
	HWND setFanSpeedEdit;
	WNDPROC setFanSpeedEditProcedure;
	HWND setLowLimitEdit;
	WNDPROC setLowLimitEditProcedure;
	HWND setHighLimitEdit;
	WNDPROC setHighLimitEditProcedure;
	HWND fanStatusWindow;
	MSG windowMessage;
	WCHAR buffer[0x100];
	WCHAR serverAddressString[0x100];
	SOCKADDR_STORAGE stringAddressDecodingBuffer;
	WNDCLASSEXW windowClass;
	volatile DWORD lock;
	volatile DWORD connectionThreadStatus;// 0 closed 1 run 2 exit
	volatile BYTE setFanSpeed;
	volatile float setLowLimit;
	volatile float setHighLimit;
	volatile ULONGLONG lastResponseTime;
	volatile fanStatusData fanStatus;
	volatile BOOL newServerAddress;
	volatile SOCKADDR_STORAGE newServerAddressBuffer;
	SOCKET clientSocket;
	SOCKADDR_STORAGE serverAddress;
	SOCKADDR_STORAGE temporalAddressBuffer;
	WSADATA WindowsSocketsInfo;

} controllerMainData;

void acquireControllerLock(controllerMainData* controller)
{
	while (controller->lock || InterlockedCompareExchange(&controller->lock, 1, 0))
		SwitchToThread();
}

void releaseControllerLock(controllerMainData* controller)
{
	InterlockedXor((volatile LONG*)&controller->lock, controller->lock);
}

DWORD CALLBACK connectionThread(controllerMainData* controller)
{
	const DWORD IPV6Only = 0;
	const DWORD receiveTimeout = 0x1000;
	u_short clientPort = getClientRandomPort();
	DWORD error = 0;
	controller->clientSocket = INVALID_SOCKET;
	while (InterlockedCompareExchange(&controller->connectionThreadStatus, CONTROLLER_CONNECTION_THREAD_STATUS_CLOSED, CONTROLLER_CONNECTION_THREAD_STATUS_SHUTDOW) == CONTROLLER_CONNECTION_THREAD_STATUS_RUN)
	{
		acquireControllerLock(controller);
		BOOL newServerAddress = controller->newServerAddress;
		if (newServerAddress)
		{
			copy((void*)&controller->newServerAddressBuffer, &controller->serverAddress, sizeof(SOCKADDR_STORAGE));
			controller->newServerAddress = FALSE;
		}
		releaseControllerLock(controller);
		if (newServerAddress)
		{
			if (controller->clientSocket != INVALID_SOCKET)
			{
				shutdown(controller->clientSocket, SD_BOTH);
				closesocket(controller->clientSocket);
				controller->clientSocket = INVALID_SOCKET;
			}
			if (controller->serverAddress.ss_family == AF_INET6)
			{
				clear(&controller->temporalAddressBuffer, sizeof(SOCKADDR_STORAGE));
				((SOCKADDR_IN6*)&controller->temporalAddressBuffer)->sin6_family = AF_INET6;
				((SOCKADDR_IN6*)&controller->temporalAddressBuffer)->sin6_addr = IN6ADDR_ANY_INIT;
				((SOCKADDR_IN6*)&controller->temporalAddressBuffer)->sin6_port = htons(clientPort);
				controller->clientSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
				if (controller->clientSocket != SOCKET_ERROR)
				{
					if (setsockopt(controller->clientSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&IPV6Only, sizeof(DWORD)) != SOCKET_ERROR && setsockopt(controller->clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) != SOCKET_ERROR && bind(controller->clientSocket, (SOCKADDR*)&controller->temporalAddressBuffer, sizeof(SOCKADDR_IN6)) != SOCKET_ERROR)
						error = 0;
					else
					{
						error = WSAGetLastError();
						shutdown(controller->clientSocket, SD_BOTH);
						closesocket(controller->clientSocket);
						controller->clientSocket = INVALID_SOCKET;
					}
				}
				else
				{
					error = WSAGetLastError();
					controller->clientSocket = INVALID_SOCKET;
				}
			}
			else if (controller->serverAddress.ss_family == AF_INET)
			{
				clear(&controller->temporalAddressBuffer, sizeof(SOCKADDR_STORAGE));
				((SOCKADDR_IN*)&controller->temporalAddressBuffer)->sin_family = AF_INET;
				((SOCKADDR_IN*)&controller->temporalAddressBuffer)->sin_addr.s_addr = INADDR_ANY;
				((SOCKADDR_IN*)&controller->temporalAddressBuffer)->sin_port = htons(clientPort);
				controller->clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (controller->clientSocket != SOCKET_ERROR)
				{
					if (setsockopt(controller->clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&receiveTimeout, sizeof(DWORD)) != SOCKET_ERROR && bind(controller->clientSocket, (SOCKADDR*)&controller->temporalAddressBuffer, sizeof(SOCKADDR_IN)) != SOCKET_ERROR)
						error = 0;
					else
					{
						error = WSAGetLastError();
						shutdown(controller->clientSocket, SD_BOTH);
						closesocket(controller->clientSocket);
						controller->clientSocket = INVALID_SOCKET;
					}
				}
				else
				{
					error = WSAGetLastError();
					controller->clientSocket = INVALID_SOCKET;
				}
			}
		}
		if (controller->clientSocket != INVALID_SOCKET)
		{
			int otherAddressSize;
			acquireControllerLock(controller);
			fanStatusData fanStatus = { 0.0f, controller->setLowLimit, controller->setHighLimit, controller->setFanSpeed };
			if (fanStatus.fanSpeed != 0xFF)
				controller->setFanSpeed = 0xFF;
			if (!(fanStatus.lowLimit < 0.0f))
				controller->setLowLimit = -1.0f;
			if (!(fanStatus.highLimit < 0.0f))
				controller->setHighLimit = -1.0f;
			releaseControllerLock(controller);
			error = 0;
			if (!error && fanStatus.fanSpeed != 0xFF)
				error = srcpWrite(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 2, 1, (BYTE*)&fanStatus.fanSpeed);
			if (!error && !(fanStatus.lowLimit < 0.0f))
				error = srcpWrite(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 3, 4, (BYTE*)&fanStatus.lowLimit);
			if (!error && !(fanStatus.highLimit < 0.0f))
				error = srcpWrite(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 4, 4, (BYTE*)&fanStatus.highLimit);
			if (!error)
				error = srcpRead(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 5, 4, (BYTE*)&fanStatus.temperature);
			if (!error)
				error = srcpRead(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 2, 1, (BYTE*)&fanStatus.fanSpeed);
			if (!error)
				error = srcpRead(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 3, 4, (BYTE*)&fanStatus.lowLimit);
			if (!error)
				error = srcpRead(controller->clientSocket, (SOCKADDR_STORAGE*)&controller->serverAddress, &otherAddressSize, &controller->temporalAddressBuffer, 4, 4, 4, (BYTE*)&fanStatus.highLimit);
			if (!error)
			{
				ULONGLONG lastResponseTime = GetTickCount64();
				acquireControllerLock(controller);
				if (!controller->newServerAddress)
				{
					controller->lastResponseTime = lastResponseTime;
					copy(&fanStatus, (void*)&controller->fanStatus, sizeof(fanStatusData));
				}
				releaseControllerLock(controller);
			}
		}
		Sleep(1000);
	}
	if (controller->clientSocket != INVALID_SOCKET)
	{
		shutdown(controller->clientSocket, SD_BOTH);
		closesocket(controller->clientSocket);
	}
	ExitThread(0);
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParameter, LPARAM lParameter)
{
	controllerMainData* controller = (controllerMainData*)GetWindowLongPtrW(window, GWLP_USERDATA);
	if (controller)
	{
		if (controller->windowHandle == window)
		{
			switch (message)
			{
				case WM_TIMER:
					if (controller->timedMessageTimer)
						--controller->timedMessageTimer;
					else if (controller->status == CONTROLLER_STATUS_CONNECTED)
					{
						SIZE_T serverAddressLength = (SIZE_T)lstrlenW(controller->serverAddressString);
						if (serverAddressLength < 0x80)
						{
							copy(L"connected to ", controller->buffer, 13 * sizeof(WCHAR));
							copy(controller->serverAddressString, controller->buffer + 13, (serverAddressLength + 1) * sizeof(WCHAR));
						}
						else
							copy(L"connected to server", controller->buffer, 20 * sizeof(WCHAR));
						SetWindowTextW(controller->statusTextWindow, controller->buffer);
						ULONGLONG lastResponseTime;
						fanStatusData fanStatus;
						acquireControllerLock(controller);
						lastResponseTime = controller->lastResponseTime;
						fanStatus.fanSpeed = controller->fanStatus.fanSpeed;
						fanStatus.lowLimit = controller->fanStatus.lowLimit;
						fanStatus.highLimit = controller->fanStatus.highLimit;
						fanStatus.temperature = controller->fanStatus.temperature;
						releaseControllerLock(controller);
						if (lastResponseTime + 0x4000 > GetTickCount64())
						{
							printFanStatus(&fanStatus, controller->buffer);
							SetWindowTextW(controller->fanStatusWindow, controller->buffer);
						}
						else
						{
							SetWindowTextW(controller->fanStatusWindow, L"");
							controller->status = CONTROLLER_STATUS_NOT_CONNECTED;
						}
					}
					else if (controller->status == CONTROLLER_STATUS_NOT_CONNECTED)
					{
						acquireControllerLock(controller);
						ULONGLONG lastResponseTime = controller->lastResponseTime;
						releaseControllerLock(controller);
						if (lastResponseTime + 0x4000 > GetTickCount64())
						{
							controller->status = CONTROLLER_STATUS_CONNECTED;
							return 0;
						}
						SIZE_T serverAddressLength = (SIZE_T)lstrlenW(controller->serverAddressString);
						if (serverAddressLength < 0x80)
						{
							copy(L"connecting to ", controller->buffer, 14 * sizeof(WCHAR));
							copy(controller->serverAddressString, controller->buffer + 14, serverAddressLength * sizeof(WCHAR));
							copy(L"...", controller->buffer + 14 + serverAddressLength, controller->waitTimer * sizeof(WCHAR));
							controller->buffer[14 + serverAddressLength + controller->waitTimer] = 0;
						}
						else
						{
							copy(L"connecting to the server...", controller->buffer, (24 + controller->waitTimer) * sizeof(WCHAR));
							controller->buffer[24 + controller->waitTimer] = 0;
						}
						SetWindowTextW(controller->statusTextWindow, controller->buffer);
						controller->waitTimer = (controller->waitTimer + 1) & 3;
					}
					else if (controller->status == CONTROLLER_STATUS_NO_IP)
						SetWindowTextW(controller->statusTextWindow, L"not connected");
					else
						SetWindowTextW(controller->statusTextWindow, L"");
					return 0;
				case WM_COMMAND:
					if (wParameter == CONTROLLER_BUTTON_IP_ID)
					{
						GetWindowTextW(controller->setIPEdit, controller->buffer, 0x100);
						if (*controller->buffer)
						{
							DWORD error = decodeAddressStringAndAddPort(controller->buffer, (SOCKADDR_STORAGE*)&controller->stringAddressDecodingBuffer, 0x29A);
							if (error)
							{
								SetWindowTextW(controller->statusTextWindow, L"error connecting to specified IP");
								controller->timedMessageTimer = 3;
							}
							else
							{
								copy(controller->buffer, controller->serverAddressString, 0x100 * sizeof(WCHAR));
								setLastUsedIPAddress(controller->serverAddressString);
								acquireControllerLock(controller);
								copy(&controller->stringAddressDecodingBuffer, (void*)&controller->newServerAddressBuffer, sizeof(SOCKADDR_STORAGE));
								controller->newServerAddress = TRUE;
								controller->lastResponseTime = 0;
								releaseControllerLock(controller);
							}
							SetWindowTextW(controller->fanStatusWindow, L"");
							controller->status = CONTROLLER_STATUS_NOT_CONNECTED;
						}
						else
							controller->status = CONTROLLER_STATUS_NO_IP;
					}
					else if (wParameter == CONTROLLER_BUTTON_FAN_SPEED_ID)
					{
						if (controller->status == CONTROLLER_STATUS_CONNECTED)
						{
							GetWindowTextW(controller->setFanSpeedEdit, controller->buffer, 0x100);
							BYTE newFanSpeed = 0xFF;
							if (!lstrcmpW(controller->buffer, L"0") || !lstrcmpW(controller->buffer, L"0/3"))
								newFanSpeed = 0;
							else if (!lstrcmpW(controller->buffer, L"1") || !lstrcmpW(controller->buffer, L"1/3"))
								newFanSpeed = 1;
							else if (!lstrcmpW(controller->buffer, L"2") || !lstrcmpW(controller->buffer, L"2/3"))
								newFanSpeed = 2;
							else if (!lstrcmpW(controller->buffer, L"3") || !lstrcmpW(controller->buffer, L"3/3"))
								newFanSpeed = 3;
							if (newFanSpeed != 0xFF)
							{
								acquireControllerLock(controller);
								controller->setFanSpeed = newFanSpeed;
								releaseControllerLock(controller);
							}
							else
							{
								SetWindowTextW(controller->statusTextWindow, L"error specified fan speed is invalid");
								controller->timedMessageTimer = 3;
							}
						}
						else
						{
							SetWindowTextW(controller->statusTextWindow, L"error not connected");
							controller->timedMessageTimer = 3;
						}
					}
					else if (wParameter == CONTROLLER_BUTTON_LOW_LIMIT_ID)
					{
						if (controller->status == CONTROLLER_STATUS_CONNECTED)
						{
							GetWindowTextW(controller->setLowLimitEdit, controller->buffer, 0x100);
							float newlowLimit;
							if (readFloat(controller->buffer, &newlowLimit) && !(newlowLimit < 0.0f))
							{
								acquireControllerLock(controller);
								float highLimit = controller->fanStatus.highLimit;
								releaseControllerLock(controller);
								if (newlowLimit < highLimit)
								{
									acquireControllerLock(controller);
									controller->setLowLimit = newlowLimit;
									releaseControllerLock(controller);
								}
								else
								{
									SetWindowTextW(controller->statusTextWindow, L"error specified low temperature limit is invalid");
									controller->timedMessageTimer = 3;
								}
							}
							else
							{
								SetWindowTextW(controller->statusTextWindow, L"error specified low temperature limit is invalid");
								controller->timedMessageTimer = 3;
							}
						}
						else
						{
							SetWindowTextW(controller->statusTextWindow, L"error not connected");
							controller->timedMessageTimer = 3;
						}
					}
					else if (wParameter == CONTROLLER_BUTTON_HIGH_LIMIT_ID)
					{
						if (controller->status == CONTROLLER_STATUS_CONNECTED)
						{
							GetWindowTextW(controller->setHighLimitEdit, controller->buffer, 0x100);
							float newHighLimit;
							if (readFloat(controller->buffer, &newHighLimit) && !(newHighLimit < 0.0f))
							{
								acquireControllerLock(controller);
								float lowLimit = controller->fanStatus.lowLimit;
								releaseControllerLock(controller);
								if (newHighLimit > lowLimit)
								{
									acquireControllerLock(controller);
									controller->setHighLimit = newHighLimit;
									releaseControllerLock(controller);
								}
								else
								{
									SetWindowTextW(controller->statusTextWindow, L"error specified high temperature limit is invalid");
									controller->timedMessageTimer = 3;
								}
							}
							else
							{
								SetWindowTextW(controller->statusTextWindow, L"error specified high temperature limit is invalid");
								controller->timedMessageTimer = 3;
							}
						}
						else
						{
							SetWindowTextW(controller->statusTextWindow, L"error not connected");
							controller->timedMessageTimer = 3;
						}
					}
					return 0;
				case WM_CLOSE:
				case WM_DESTROY:
				case WM_QUIT:
					controller->closeWindow = TRUE;
					return 0;
				default:
					return DefWindowProcW(window, message, wParameter, lParameter);
			}
		}
		else if (controller->setIPEdit == window)
		{
			if (message == WM_KEYDOWN && wParameter == VK_RETURN)
			{
				GetWindowTextW(controller->setIPEdit, controller->buffer, 0x100);
				if (*controller->buffer)
				{
					DWORD error = decodeAddressStringAndAddPort(controller->buffer, (SOCKADDR_STORAGE*)&controller->stringAddressDecodingBuffer, 0x29A);
					if (error)
					{
						SetWindowTextW(controller->statusTextWindow, L"error connecting to specified IP");
						controller->timedMessageTimer = 3;
					}
					else
					{
						copy(controller->buffer, controller->serverAddressString, 0x100 * sizeof(WCHAR));
						setLastUsedIPAddress(controller->serverAddressString);
						acquireControllerLock(controller);
						copy(&controller->stringAddressDecodingBuffer, (void*)&controller->newServerAddressBuffer, sizeof(SOCKADDR_STORAGE));
						controller->newServerAddress = TRUE;
						controller->lastResponseTime = 0;
						releaseControllerLock(controller);
					}
					SetWindowTextW(controller->fanStatusWindow, L"");
					controller->status = CONTROLLER_STATUS_NOT_CONNECTED;
				}
				else
					controller->status = CONTROLLER_STATUS_NO_IP;
			}
			return controller->setIPEditProcedure(window, message, wParameter, lParameter);
		}
		else if (controller->setFanSpeedEdit == window)
		{
			if (message == WM_KEYDOWN && wParameter == VK_RETURN)
			{
				if (controller->status == CONTROLLER_STATUS_CONNECTED)
				{
					GetWindowTextW(controller->setFanSpeedEdit, controller->buffer, 0x100);
					BYTE newFanSpeed = 0xFF;
					if (!lstrcmpW(controller->buffer, L"0") || !lstrcmpW(controller->buffer, L"0/3"))
						newFanSpeed = 0;
					else if (!lstrcmpW(controller->buffer, L"1") || !lstrcmpW(controller->buffer, L"1/3"))
						newFanSpeed = 1;
					else if (!lstrcmpW(controller->buffer, L"2") || !lstrcmpW(controller->buffer, L"2/3"))
						newFanSpeed = 2;
					else if (!lstrcmpW(controller->buffer, L"3") || !lstrcmpW(controller->buffer, L"3/3"))
						newFanSpeed = 3;
					if (newFanSpeed != 0xFF)
					{
						acquireControllerLock(controller);
						controller->setFanSpeed = newFanSpeed;
						releaseControllerLock(controller);
					}
					else
					{
						SetWindowTextW(controller->statusTextWindow, L"error specified fan speed is invalid");
						controller->timedMessageTimer = 3;
					}
				}
				else
				{
					SetWindowTextW(controller->statusTextWindow, L"error not connected");
					controller->timedMessageTimer = 3;
				}
			}
			return controller->setFanSpeedEditProcedure(window, message, wParameter, lParameter);
		}
		else if (controller->setLowLimitEdit == window)
		{
			if (message == WM_KEYDOWN && wParameter == VK_RETURN)
			{
				if (controller->status == CONTROLLER_STATUS_CONNECTED)
				{
					GetWindowTextW(controller->setLowLimitEdit, controller->buffer, 0x100);
					float newlowLimit;
					if (readFloat(controller->buffer, &newlowLimit) && !(newlowLimit < 0.0f))
					{
						acquireControllerLock(controller);
						float highLimit = controller->fanStatus.highLimit;
						releaseControllerLock(controller);
						if (newlowLimit < highLimit)
						{
							acquireControllerLock(controller);
							controller->setLowLimit = newlowLimit;
							releaseControllerLock(controller);
						}
						else
						{
							SetWindowTextW(controller->statusTextWindow, L"error specified low temperature limit is invalid");
							controller->timedMessageTimer = 3;
						}
					}
					else
					{
						SetWindowTextW(controller->statusTextWindow, L"error specified low temperature limit is invalid");
						controller->timedMessageTimer = 3;
					}
				}
				else
				{
					SetWindowTextW(controller->statusTextWindow, L"error not connected");
					controller->timedMessageTimer = 3;
				}
			}
			return controller->setLowLimitEditProcedure(window, message, wParameter, lParameter);
		}
		else if (controller->setHighLimitEdit == window)
		{
			if (message == WM_KEYDOWN && wParameter == VK_RETURN)
			{
				if (controller->status == CONTROLLER_STATUS_CONNECTED)
				{
					GetWindowTextW(controller->setHighLimitEdit, controller->buffer, 0x100);
					float newHighLimit;
					if (readFloat(controller->buffer, &newHighLimit) && !(newHighLimit < 0.0f))
					{
						acquireControllerLock(controller);
						float lowLimit = controller->fanStatus.lowLimit;
						releaseControllerLock(controller);
						if (newHighLimit > lowLimit)
						{
							acquireControllerLock(controller);
							controller->setHighLimit = newHighLimit;
							releaseControllerLock(controller);
						}
						else
						{
							SetWindowTextW(controller->statusTextWindow, L"error specified high temperature limit is invalid");
							controller->timedMessageTimer = 3;
						}
					}
					else
					{
						SetWindowTextW(controller->statusTextWindow, L"error specified high temperature limit is invalid");
						controller->timedMessageTimer = 3;
					}
				}
				else
				{
					SetWindowTextW(controller->statusTextWindow, L"error not connected");
					controller->timedMessageTimer = 3;
				}
			}
			return controller->setHighLimitEditProcedure(window, message, wParameter, lParameter);
		}
		else
			return DefWindowProcW(window, message, wParameter, lParameter);
	}
	else if (message == WM_CREATE)
	{
		controllerMainData* controller = (controllerMainData*)((CREATESTRUCT*)lParameter)->lpCreateParams;
		SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)controller);
		SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
		return (controllerMainData*)GetWindowLongPtrW(window, GWLP_USERDATA) == controller ? 0 : (LRESULT)-1;
	}
	else
		return DefWindowProcW(window, message, wParameter, lParameter);
}

DWORD createWindowWithClientRectangle(HWND* handle, DWORD dwExStyle, const WCHAR* lpClassName, const WCHAR* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	RECT clientRectangle = { 0, 0, nWidth, nHeight };
	if (!AdjustWindowRectEx(&clientRectangle, dwStyle, hMenu ? TRUE : 0, dwExStyle))
		return GetLastError();
	nWidth = (int)(clientRectangle.right - clientRectangle.left);
	nHeight = (int)(clientRectangle.bottom - clientRectangle.top);
	HWND windowHandle = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (!windowHandle)
		return GetLastError();
	*handle = windowHandle;
	return 0;
}

void main()
{
	DWORD error = 0;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		ExitProcess((UINT)GetLastError());
	controllerMainData* controller = (controllerMainData*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(controllerMainData));
	if (WSAStartup(MAKEWORD(2, 2), &controller->WindowsSocketsInfo))
		ExitProcess((UINT)WSAGetLastError());
	if (!controller)
	{
		error = GetLastError();
		WSACleanup();
		ExitProcess((UINT)error);
	}
	controller->setFanSpeed = 0xFF;
	controller->setLowLimit = -1.0f;
	controller->setHighLimit = -1.0f;
	controller->timedMessageTimer = 0;
	controller->closeWindow = FALSE;
	BOOL BrownBuffaloRegidteryOpen = FALSE;
	HKEY BrownBuffalo;
	WCHAR* lastUsedIPAddress;
	if (!getLastUsedIPAddress(heap, &lastUsedIPAddress))
	{
		SIZE_T lastUsedIPAddressLength = (SIZE_T)lstrlenW(lastUsedIPAddress);
		if (lastUsedIPAddressLength && lastUsedIPAddressLength < 0x80)
		{
			controller->status = CONTROLLER_STATUS_NOT_CONNECTED;
			controller->waitTimer = 0;
			copy(lastUsedIPAddress, controller->serverAddressString, (lastUsedIPAddressLength + 1) * sizeof(WCHAR));
			error = decodeAddressStringAndAddPort(controller->serverAddressString, (SOCKADDR_STORAGE*)&controller->newServerAddressBuffer, 0x29A);
			if (!error)
				controller->newServerAddress = TRUE;
			else
			{
				controller->newServerAddress = FALSE;
				if (error == ERROR_INVALID_PARAMETER)
				{
					LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Brown Buffalo", 0, 0, REG_OPTION_VOLATILE, KEY_SET_VALUE, 0, &BrownBuffalo, 0);
					if (!result)
					{
						RegDeleteValueW(BrownBuffalo, L"last used IP");
						BrownBuffaloRegidteryOpen = TRUE;
					}
				}
				controller->status = CONTROLLER_STATUS_NO_IP;
				controller->waitTimer = 0;
			}
		}
		else
		{
			HKEY BrownBuffalo;
			LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Brown Buffalo", 0, 0, REG_OPTION_VOLATILE, KEY_SET_VALUE, 0, &BrownBuffalo, 0);
			if (!result)
			{
				RegDeleteValueW(BrownBuffalo, L"last used IP");
				BrownBuffaloRegidteryOpen = TRUE;
			}
			controller->status = CONTROLLER_STATUS_NO_IP;
			controller->waitTimer = 0;
		}
		HeapFree(heap, 0, lastUsedIPAddress);
	}
	else
	{
		controller->status = CONTROLLER_STATUS_NO_IP;
		controller->waitTimer = 0;
	}
	if (!BrownBuffaloRegidteryOpen && !RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Brown Buffalo", 0, KEY_QUERY_VALUE, &BrownBuffalo))
		BrownBuffaloRegidteryOpen = TRUE;
	controller->windowClass.cbSize = sizeof(WNDCLASSEXW);
	controller->windowClass.style = CS_HREDRAW | CS_VREDRAW;
	controller->windowClass.lpfnWndProc = windowProcedure;
	controller->windowClass.cbClsExtra = 0;
	controller->windowClass.cbWndExtra = 0;
	controller->windowClass.hInstance = (HINSTANCE)GetModuleHandleW(0);
	controller->windowClass.hIcon = LoadIconW(0, (const WCHAR*)IDI_APPLICATION);
	controller->windowClass.hCursor = LoadCursorW(0, (const WCHAR*)IDC_ARROW);
	controller->windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	controller->windowClass.lpszMenuName = 0;
	controller->windowClass.lpszClassName = L"BROWN_BUFFALO_CONTROLLER_MAIN_WINDOW";
	controller->windowClass.hIconSm = (HICON)LoadImageW(controller->windowClass.hInstance, (const WCHAR*)MAKEINTRESOURCE(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	error = RegisterClassExW(&controller->windowClass) ? 0 : GetLastError();
	if (error)
	{
		error = GetLastError();
		if (BrownBuffaloRegidteryOpen)
			RegCloseKey(BrownBuffalo);
		WSACleanup();
		ExitProcess((UINT)error);
	}
	error = createWindowWithClientRectangle(&controller->windowHandle, WS_EX_APPWINDOW, controller->windowClass.lpszClassName,
		L"BROWN BUFFALO fan controller", WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
		8 + 256 + 128 + 8 + 8, 40 * 5 + 8 + 128 + 8, 0, 0, controller->windowClass.hInstance, controller);
	if (error)
	{
		UnregisterClassW(controller->windowClass.lpszClassName, controller->windowClass.hInstance);
		if (BrownBuffaloRegidteryOpen)
			RegCloseKey(BrownBuffalo);
		WSACleanup();
		ExitProcess((UINT)error);
	}
	controller->statusTextWindow = CreateWindowExW(0, L"STATIC", L"starting controller", WS_CHILD | WS_VISIBLE | WS_BORDER, 8, 8, 256 + 128 + 8, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
	controller->setIPButton = CreateWindowExW(0, L"BUTTON", L"connect to IP", WS_CHILD | WS_VISIBLE | WS_BORDER, 8, 48, 128, 32, controller->windowHandle, (HMENU)CONTROLLER_BUTTON_IP_ID, controller->windowClass.hInstance, 0);
	controller->setFanSpeedButton = CreateWindowExW(0, L"BUTTON", L"set fan speed", WS_CHILD | WS_VISIBLE | WS_BORDER, 8, 40 * 2 + 8, 128, 32, controller->windowHandle, (HMENU)CONTROLLER_BUTTON_FAN_SPEED_ID, controller->windowClass.hInstance, 0);
	controller->setLowLimitButton = CreateWindowExW(0, L"BUTTON", L"set low limit", WS_CHILD | WS_VISIBLE | WS_BORDER, 8, 40 * 3 + 8, 128, 32, controller->windowHandle, (HMENU)CONTROLLER_BUTTON_LOW_LIMIT_ID, controller->windowClass.hInstance, 0);
	controller->setHighLimitButton = CreateWindowExW(0, L"BUTTON", L"set high limit", WS_CHILD | WS_VISIBLE | WS_BORDER, 8, 40 * 4 + 8, 128, 32, controller->windowHandle, (HMENU)CONTROLLER_BUTTON_HIGH_LIMIT_ID, controller->windowClass.hInstance, 0);
	controller->fanStatusWindow = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 8, 40 * 5 + 8, 256 + 128 + 8, 128, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
	if (BrownBuffaloRegidteryOpen)
	{
		DWORD valueType;
		DWORD valueSize = 0xFF * sizeof(WCHAR);
		error = (DWORD)RegQueryValueExW(BrownBuffalo, L"auto input IP", 0, &valueType, (BYTE*)controller->buffer, &valueSize);
		if (error || valueType != REG_SZ || valueSize % sizeof(WCHAR))
			valueSize = 0;
		*(WCHAR*)((UINT_PTR)controller->buffer + (UINT_PTR)valueSize) = 0;
		controller->setIPEdit = CreateWindowExW(0, L"EDIT", controller->buffer, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		valueSize = 0xFF * sizeof(WCHAR);
		error = (DWORD)RegQueryValueExW(BrownBuffalo, L"auto input fan speed", 0, &valueType, (BYTE*)controller->buffer, &valueSize);
		if (error || valueType != REG_SZ || valueSize % sizeof(WCHAR))
			valueSize = 0;
		*(WCHAR*)((UINT_PTR)controller->buffer + (UINT_PTR)valueSize) = 0;
		controller->setFanSpeedEdit = CreateWindowExW(0, L"EDIT", controller->buffer, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 * 2 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		valueSize = 0xFF * sizeof(WCHAR);
		error = (DWORD)RegQueryValueExW(BrownBuffalo, L"auto input low limit", 0, &valueType, (BYTE*)controller->buffer, &valueSize);
		if (error || valueType != REG_SZ || valueSize % sizeof(WCHAR))
			valueSize = 0;
		*(WCHAR*)((UINT_PTR)controller->buffer + (UINT_PTR)valueSize) = 0;
		controller->setLowLimitEdit = CreateWindowExW(0, L"EDIT", controller->buffer, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 * 3 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		valueSize = 0xFF * sizeof(WCHAR);
		error = (DWORD)RegQueryValueExW(BrownBuffalo, L"auto input high limit", 0, &valueType, (BYTE*)controller->buffer, &valueSize);
		if (error || valueType != REG_SZ || valueSize % sizeof(WCHAR))
			valueSize = 0;
		*(WCHAR*)((UINT_PTR)controller->buffer + (UINT_PTR)valueSize) = 0;
		controller->setHighLimitEdit = CreateWindowExW(0, L"EDIT", controller->buffer, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 * 4 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		RegCloseKey(BrownBuffalo);
	}
	else
	{
		controller->setIPEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		controller->setFanSpeedEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 * 2 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		controller->setLowLimitEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 * 3 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
		controller->setHighLimitEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER, 128 + 16, 40 * 4 + 8, 256, 32, controller->windowHandle, 0, controller->windowClass.hInstance, 0);
	}
	if (!controller->statusTextWindow ||
		!controller->setIPButton ||
		!controller->setFanSpeedButton ||
		!controller->setLowLimitButton ||
		!controller->setHighLimitButton ||
		!controller->setIPEdit ||
		!controller->setFanSpeedEdit ||
		!controller->setLowLimitEdit ||
		!controller->setHighLimitEdit ||
		!controller->fanStatusWindow ||
		!UpdateWindow(controller->windowHandle) ||
		!SetTimer(controller->windowHandle, CONTROLLER_TIMIR_ID, 1000, 0))
	{
		error = GetLastError();
		if (controller->statusTextWindow)
			DestroyWindow(controller->statusTextWindow);
		if (controller->setIPButton)
			DestroyWindow(controller->setIPButton);
		if (controller->setFanSpeedButton)
			DestroyWindow(controller->setFanSpeedButton);
		if (controller->setLowLimitButton)
			DestroyWindow(controller->setLowLimitButton);
		if (controller->setHighLimitButton)
			DestroyWindow(controller->setHighLimitButton);
		if (controller->setIPEdit)
			DestroyWindow(controller->setIPEdit);
		if (controller->setFanSpeedEdit)
			DestroyWindow(controller->setFanSpeedEdit);
		if (controller->setLowLimitEdit)
			DestroyWindow(controller->setLowLimitEdit);
		if (controller->setHighLimitEdit)
			DestroyWindow(controller->setHighLimitEdit);
		if (controller->fanStatusWindow)
			DestroyWindow(controller->fanStatusWindow);
		DestroyWindow(controller->windowHandle);
		UnregisterClassW(controller->windowClass.lpszClassName, controller->windowClass.hInstance);
		HeapFree(heap, 0, controller);
		WSACleanup();
		ExitProcess(error);
	}
	controller->setIPEditProcedure = (WNDPROC)GetWindowLongPtrW(controller->setIPEdit, GWLP_WNDPROC);
	SetWindowLongPtrW(controller->setIPEdit, GWLP_WNDPROC, (LONG_PTR)windowProcedure);
	SetWindowLongPtrW(controller->setIPEdit, GWLP_USERDATA, (LONG_PTR)controller);
	SetWindowPos(controller->setIPEdit, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	controller->setFanSpeedEditProcedure = (WNDPROC)GetWindowLongPtrW(controller->setFanSpeedEdit, GWLP_WNDPROC);
	SetWindowLongPtrW(controller->setFanSpeedEdit, GWLP_WNDPROC, (LONG_PTR)windowProcedure);
	SetWindowLongPtrW(controller->setFanSpeedEdit, GWLP_USERDATA, (LONG_PTR)controller);
	SetWindowPos(controller->setFanSpeedEdit, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	controller->setLowLimitEditProcedure = (WNDPROC)GetWindowLongPtrW(controller->setLowLimitEdit, GWLP_WNDPROC);
	SetWindowLongPtrW(controller->setLowLimitEdit, GWLP_WNDPROC, (LONG_PTR)windowProcedure);
	SetWindowLongPtrW(controller->setLowLimitEdit, GWLP_USERDATA, (LONG_PTR)controller);
	SetWindowPos(controller->setLowLimitEdit, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	controller->setHighLimitEditProcedure = (WNDPROC)GetWindowLongPtrW(controller->setHighLimitEdit, GWLP_WNDPROC);
	SetWindowLongPtrW(controller->setHighLimitEdit, GWLP_WNDPROC, (LONG_PTR)windowProcedure);
	SetWindowLongPtrW(controller->setHighLimitEdit, GWLP_USERDATA, (LONG_PTR)controller);
	SetWindowPos(controller->setHighLimitEdit, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	if (!controller->setIPEditProcedure || !controller->setFanSpeedEditProcedure || !controller->setLowLimitEditProcedure || !controller->setHighLimitEditProcedure ||
		(WNDPROC)GetWindowLongPtrW(controller->setIPEdit, GWLP_WNDPROC) != windowProcedure ||
		(controllerMainData*)GetWindowLongPtrW(controller->setIPEdit, GWLP_USERDATA) != controller ||
		(WNDPROC)GetWindowLongPtrW(controller->setFanSpeedEdit, GWLP_WNDPROC) != windowProcedure ||
		(controllerMainData*)GetWindowLongPtrW(controller->setFanSpeedEdit, GWLP_USERDATA) != controller ||
		(WNDPROC)GetWindowLongPtrW(controller->setLowLimitEdit, GWLP_WNDPROC) != windowProcedure ||
		(controllerMainData*)GetWindowLongPtrW(controller->setLowLimitEdit, GWLP_USERDATA) != controller ||
		(WNDPROC)GetWindowLongPtrW(controller->setHighLimitEdit, GWLP_WNDPROC) != windowProcedure ||
		(controllerMainData*)GetWindowLongPtrW(controller->setHighLimitEdit, GWLP_USERDATA) != controller)
	{
		DestroyWindow(controller->statusTextWindow);
		DestroyWindow(controller->setIPButton);
		DestroyWindow(controller->setFanSpeedButton);
		DestroyWindow(controller->setLowLimitButton);
		DestroyWindow(controller->setHighLimitButton);
		DestroyWindow(controller->setIPEdit);
		DestroyWindow(controller->setFanSpeedEdit);
		DestroyWindow(controller->setLowLimitEdit);
		DestroyWindow(controller->setHighLimitEdit);
		DestroyWindow(controller->fanStatusWindow);
		DestroyWindow(controller->windowHandle);
		UnregisterClassW(controller->windowClass.lpszClassName, controller->windowClass.hInstance);
		HeapFree(heap, 0, controller);
		WSACleanup();
		ExitProcess(0);
	}
	controller->connectionThreadStatus = CONTROLLER_CONNECTION_THREAD_STATUS_RUN;
	MemoryBarrier();
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)connectionThread, controller, 0, 0);
	for (BOOL loop = GetMessageW(&controller->windowMessage, controller->windowHandle, 0, 0); !controller->closeWindow && loop && loop != (BOOL)-1; loop = GetMessageW(&controller->windowMessage, controller->windowHandle, 0, 0))
	{
		TranslateMessage(&controller->windowMessage);
		DispatchMessageW(&controller->windowMessage);
	}
	KillTimer(controller->windowHandle, CONTROLLER_TIMIR_ID);
	DestroyWindow(controller->statusTextWindow);
	DestroyWindow(controller->setIPButton);
	DestroyWindow(controller->setFanSpeedButton);
	DestroyWindow(controller->setLowLimitButton);
	DestroyWindow(controller->setHighLimitButton);
	DestroyWindow(controller->setIPEdit);
	DestroyWindow(controller->setFanSpeedEdit);
	DestroyWindow(controller->setLowLimitEdit);
	DestroyWindow(controller->setHighLimitEdit);
	DestroyWindow(controller->fanStatusWindow);
	DestroyWindow(controller->windowHandle);
	UnregisterClassW(controller->windowClass.lpszClassName, controller->windowClass.hInstance);
	while (InterlockedCompareExchange(&controller->connectionThreadStatus, CONTROLLER_CONNECTION_THREAD_STATUS_SHUTDOW, CONTROLLER_CONNECTION_THREAD_STATUS_RUN) != CONTROLLER_CONNECTION_THREAD_STATUS_SHUTDOW)
		continue;
	while (InterlockedCompareExchange(&controller->connectionThreadStatus, CONTROLLER_CONNECTION_THREAD_STATUS_EXITED, CONTROLLER_CONNECTION_THREAD_STATUS_CLOSED))
		continue;
	HeapFree(heap, 0, controller);
	WSACleanup();
	ExitProcess(0);
}

#ifdef __cplusplus
}
#endif