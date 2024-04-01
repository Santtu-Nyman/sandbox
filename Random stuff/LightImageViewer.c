#ifdef __cplusplus
extern "C" {
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Sddl.h>
#include <Aclapi.h>

void liwFatalError(DWORD error, const WCHAR* message);

void liwstbiAssert(BOOL assertion)
{
	if (assertion)
		return;
	liwFatalError(ERROR_UNIDENTIFIED_ERROR, L"Image decoder library assetion failed.");
}

#define STBI_NO_STDIO
#define STBI_ASSERT(x) (liwstbiAssert((BOOL)(x)))
#define STBI_MALLOC(sz) ((void*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)(sz)))
#define STBI_REALLOC(p,newsz) ((void*)HeapReAlloc(GetProcessHeap(), 0, (LPVOID)(p), (SIZE_T)(newsz)))
#define STBI_FREE(p) (HeapFree(GetProcessHeap(), 0, (LPVOID)(p)))
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const WCHAR* liwSearchArgument(SIZE_T argumentCount, const WCHAR** argumentValues, WCHAR argumentCharacter);

DWORD liwGetLastError();

void liwFitRectengleInRectangle(const DWORD* rectengleWHAndsubRectengleWH, DWORD* resultRectengleXYWH);

DWORD liwParseCommandLine(SIZE_T* argumentCount, const WCHAR*** arguments);

DWORD liwCreateWindow(HWND* windowHandle, DWORD dwExStyle, const WCHAR* lpClassName, const WCHAR* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

#define LIW_WINDOW_CLASS_NAME_TERMINATED_LENGTH 0x10
DWORD liwRegisterWindowClass(WNDCLASSEXW* windowClass);

DWORD liwGetRectangleSizeOfAllMonitors(SIZE_T* widthAndheight);

#define LIW_STUCTURE_ALIGNMENT 0x40
typedef struct _liwMainWindow
{
	BOOL run;
	DWORD error;
	HDC windowDeviceContext;
	HDC windowMemoryDeviceContext;
	HBITMAP windowBitmap;
	HGDIOBJ originalWindowMemoryDeviceContextObject;
	DWORD displayWHAndImageWH[4];
	DWORD displayImageXYWH[4];
	BITMAPINFOHEADER bitmapHeader;
	HWND windowHandle;
	HINSTANCE Instance;
	SIZE_T pageSize;
	SIZE_T committedBufferSize;
	SIZE_T reservedBufferSize;
	RECT clientRectangle;
	MSG windowMessage;
	WCHAR windowClassName[LIW_WINDOW_CLASS_NAME_TERMINATED_LENGTH];
} liwMainWindow;
#define LIW_WINDOW_STUCTURE_SIZE ((sizeof(liwMainWindow) + (LIW_STUCTURE_ALIGNMENT - 1)) & ~(LIW_STUCTURE_ALIGNMENT - 1))
#define LIW_GET_WINDOW_IMAGE_BUFFER(w) ((DWORD*)((UINT_PTR)(w) + LIW_WINDOW_STUCTURE_SIZE))
#define LIW_GET_IMAGE_BUFFER_SIZE(w, h) (((((SIZE_T)(w) * (SIZE_T)(h)) << 2) + (LIW_STUCTURE_ALIGNMENT - 1)) & ~(LIW_STUCTURE_ALIGNMENT - 1))
#define LIW_GET_WINDOW_DISPLAY_BUFFER(w) ((DWORD*)((UINT_PTR)(w) + LIW_WINDOW_STUCTURE_SIZE + LIW_GET_IMAGE_BUFFER_SIZE((SIZE_T)((liwMainWindow*)(w))->displayWHAndImageWH[2], (SIZE_T)((liwMainWindow*)(w))->displayWHAndImageWH[3])))

DWORD liwAsynchronousPrefetchVirtualMemory(const LPVOID address, SIZE_T size, HANDLE* prefechCompletedEvent);

void liwInitializeSecurity();

void liwDrawDisplayImage(liwMainWindow* data);

DWORD liwCreateMainWindow(liwMainWindow** window, const WCHAR* imageFileName);

void liwCloseMainWindow(liwMainWindow* window);

DWORD liwMainWindowLoop(liwMainWindow* window);

DWORD liwLoadImage(const WCHAR* pipeName, const WCHAR* imageFileName);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	liwInitializeSecurity();
	SIZE_T argumentCount;
	const WCHAR** arguments;
	DWORD error = liwParseCommandLine(&argumentCount, &arguments);
	if (error)
		liwFatalError(error, L"Parsing command line failed.");
	const WCHAR* loadPipeName = liwSearchArgument(argumentCount, arguments, L'l');
	const WCHAR* loadFileName = liwSearchArgument(argumentCount, arguments, L'm');
	if (loadPipeName && loadFileName)
	{
		error = liwLoadImage(loadPipeName, loadFileName);
		LocalFree(arguments);
		ExitProcess((UINT)error);
	}
	if (argumentCount != 2)
	{
		LocalFree(arguments);
		ExitProcess(ERROR_BAD_ARGUMENTS);
	}
	DWORD fileAtributes = GetFileAttributesW(arguments[1]);
	if (fileAtributes == INVALID_FILE_ATTRIBUTES || fileAtributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		LocalFree(arguments);
		MessageBoxW(0, L"Error can't open the specified file.", L"Error", MB_OK | MB_ICONERROR);
		ExitProcess(ERROR_BAD_PATHNAME);
	}
	liwMainWindow* window;
	error = liwCreateMainWindow(&window, arguments[1]);
	LocalFree(arguments);
	if (error)
	{
		if (error == ERROR_UNSUPPORTED_TYPE || error == ERROR_INVALID_DATA)
			MessageBoxW(0, L"Error can't read file format of the specified file.", L"Error", MB_OK | MB_ICONERROR);
		else
			liwFatalError((UINT)error, L"Can't create window.");
	}
	else
	{
		error = liwMainWindowLoop(window);
		liwCloseMainWindow(window);
	}
	ExitProcess((UINT)error);
	return ERROR_UNIDENTIFIED_ERROR;
}

const WCHAR* liwSearchArgument(SIZE_T argumentCount, const WCHAR** argumentValues, WCHAR argumentCharacter)
{
	for (SIZE_T index = 0; index != argumentCount; ++index)
		if (argumentValues[index][0] == L'-' && argumentCharacter == argumentValues[index][1] && !argumentValues[index][2] && index + 1 != argumentCount)
			return argumentValues[index + 1];
	return 0;
}

void liwFatalError(DWORD error, const WCHAR* message)
{
	const SIZE_T errorMessagBufferSize = 0x10000;
	if (!error)
		error = ERROR_UNIDENTIFIED_ERROR;
	WCHAR* errorMessageBuffer = (WCHAR*)VirtualAlloc(0, errorMessagBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!errorMessageBuffer)
		ExitProcess((UINT)error);
	WCHAR* errorMessageBufferEnd = (WCHAR*)((UINT_PTR)errorMessageBuffer + errorMessagBufferSize);
	if (errorMessageBuffer > errorMessageBufferEnd)
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	WCHAR* writeMessage = errorMessageBuffer;
	if (writeMessage + 20 >= errorMessageBufferEnd)
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	for (WCHAR* source = (WCHAR*)L"Process 0x", *sourceEnd = source + 10; source != sourceEnd;)
		*writeMessage++ = *source++;
	for (DWORD process = GetCurrentProcessId(), index = 0; index != 8; ++index)
	{
		DWORD textOutput = (process >> ((7 - index) << 2)) & 0xF;
		textOutput = textOutput < 0xA ? textOutput + 0x30 : textOutput + 0x37;
		*writeMessage++ = (WCHAR)textOutput;
	}
	for (WCHAR* source = (WCHAR*)L" \"", *sourceEnd = source + 2; source != sourceEnd;)
		*writeMessage++ = *source++;
	SIZE_T moduleFileNameLength = (SIZE_T)GetModuleFileNameW(0, writeMessage, (DWORD)(errorMessageBufferEnd - writeMessage) / sizeof(WCHAR));
	if (!moduleFileNameLength || moduleFileNameLength >= (SIZE_T)(errorMessageBufferEnd - writeMessage) / sizeof(WCHAR))
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	writeMessage += moduleFileNameLength;
	if (writeMessage + 49 >= errorMessageBufferEnd)
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	for (WCHAR* source = (WCHAR*)L"\" has encountered fatal error.\nERROR 0x", *sourceEnd = source + 39; source != sourceEnd;)
		*writeMessage++ = *source++;
	for (DWORD index = 0; index != 8; ++index)
	{
		DWORD textOutput = (error >> ((7 - index) << 2)) & 0xF;
		textOutput = textOutput < 0xA ? textOutput + 0x30 : textOutput + 0x37;
		*writeMessage++ = (WCHAR)textOutput;
	}
	for (WCHAR* source = (WCHAR*)L" \"", *sourceEnd = source + 2; source != sourceEnd;)
		*writeMessage++ = *source++;
	SIZE_T formatMessageLength = (SIZE_T)FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), writeMessage, (DWORD)((errorMessageBufferEnd - writeMessage) / sizeof(WCHAR)), 0);
	if (!formatMessageLength)
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	writeMessage += formatMessageLength;
	while (formatMessageLength && writeMessage[-1] < (WCHAR)0x0021)
	{
		--writeMessage;
		--formatMessageLength;
	}
	if (writeMessage + 3 >= errorMessageBufferEnd)
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	for (WCHAR* source = (WCHAR*)L"\".\n", *sourceEnd = source + 3; source != sourceEnd;)
		*writeMessage++ = *source++;
	SIZE_T messageLength = 0;
	if (message)
		while (message[messageLength])
			++messageLength;
	if (writeMessage + messageLength + 1 >= errorMessageBufferEnd)
	{
		VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
		ExitProcess((UINT)error);
	}
	for (WCHAR* source = (WCHAR*)message, *sourceEnd = source + messageLength; source != sourceEnd;)
		*writeMessage++ = *source++;
	*writeMessage = 0;
	MessageBoxW(0, errorMessageBuffer, L"Fatal error!", MB_OK | MB_ICONERROR);
	VirtualFree(errorMessageBuffer, 0, MEM_RELEASE);
	ExitProcess((UINT)error);
}

DWORD liwGetLastError()
{
	DWORD error = GetLastError();
	return error ? error : ERROR_UNIDENTIFIED_ERROR;
}

void liwFitRectengleInRectangle(const DWORD* rectengleWHAndsubRectengleWH, DWORD* resultRectengleXYWH)
{
	ULONGLONG frameWidth = (ULONGLONG)rectengleWHAndsubRectengleWH[0] << 16;
	ULONGLONG frameHeight = (ULONGLONG)rectengleWHAndsubRectengleWH[1] << 16;
	ULONGLONG rectengleWidth = (ULONGLONG)rectengleWHAndsubRectengleWH[2] << 16;
	ULONGLONG rectengleHeight = (ULONGLONG)rectengleWHAndsubRectengleWH[3] << 16;
	ULONGLONG scale = ((frameWidth << 16) / rectengleWidth);
	ULONGLONG scaledDimension = (scale * rectengleHeight) >> 16;
	ULONGLONG width;
	ULONGLONG height;
	if (scaledDimension <= frameHeight)
	{
		width = frameWidth;
		height = scaledDimension;
	}
	else
	{
		scale = ((frameHeight << 16) / rectengleHeight);
		scaledDimension = (scale * rectengleWidth) >> 16;
		width = scaledDimension;
		height = frameHeight;
	}
	resultRectengleXYWH[0] = (DWORD)((frameWidth - width) >> 17);
	resultRectengleXYWH[1] = (DWORD)((frameHeight - height) >> 17);
	resultRectengleXYWH[2] = (DWORD)(width >> 16);
	resultRectengleXYWH[3] = (DWORD)(height >> 16);
}

DWORD liwParseCommandLine(SIZE_T* argumentCount, const WCHAR*** arguments)
{
	int commandLineArgumentCount;
	HMODULE Shell32 = LoadLibraryW(L"Shell32.dll");
	const WCHAR** commandLineArguments = ((const WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(Shell32, "CommandLineToArgvW"))(GetCommandLineW(), &commandLineArgumentCount);
	if (!commandLineArguments)
	{
		DWORD error = GetLastError();
		FreeLibrary(Shell32);
		return error;
	}
	FreeLibrary(Shell32);
	*argumentCount = (SIZE_T)commandLineArgumentCount;
	*arguments = commandLineArguments;
	return 0;
}

DWORD liwCreateWindow(HWND* windowHandle, DWORD dwExStyle, const WCHAR* lpClassName, const WCHAR* lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	RECT windowRectangle = {
		(x != CW_USEDEFAULT) ? x : 0,
		(y != CW_USEDEFAULT) ? y : 0,
		(nWidth != CW_USEDEFAULT) ? nWidth : (LONG)GetSystemMetrics(SM_CXSCREEN),
		(nHeight != CW_USEDEFAULT) ? nHeight : (LONG)GetSystemMetrics(SM_CYSCREEN) };
	if (!AdjustWindowRectEx(&windowRectangle, dwStyle, hMenu ? TRUE : 0, dwExStyle))
		return GetLastError();
	HWND handle = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle,
		(x != CW_USEDEFAULT) ? (int)windowRectangle.left : CW_USEDEFAULT,
		(y != CW_USEDEFAULT) ? (int)windowRectangle.top : CW_USEDEFAULT,
		(nWidth != CW_USEDEFAULT) ? (int)(windowRectangle.right - windowRectangle.left) : CW_USEDEFAULT,
		(nHeight != CW_USEDEFAULT) ? (int)(windowRectangle.bottom - windowRectangle.top) : CW_USEDEFAULT,
		hWndParent, hMenu, hInstance, lpParam);
	if (!handle)
		return liwGetLastError();
	if (windowHandle)
		*windowHandle = handle;
	return 0;
}

DWORD liwRegisterWindowClass(WNDCLASSEXW* windowClass)
{
	WCHAR* windowClassName = (WCHAR*)windowClass->lpszClassName;
	*windowClassName++ = L'L';
	*windowClassName++ = L'I';
	*windowClassName++ = L'W';
	*windowClassName++ = L'_';
	for (DWORD process = GetCurrentProcessId(), index = 0; index != 8; ++index)
	{
		DWORD textOutput = (process >> ((7 - index) << 2)) & 0xF;
		textOutput = textOutput < 0xA ? textOutput + 0x30 : textOutput + 0x37;
		*windowClassName++ = (WCHAR)textOutput;
	}
	*windowClassName++ = L'_';
	*(windowClassName + 2) = 0;
	for (DWORD error, digit, windowNumber = 0;;)
	{
		digit = windowNumber / 36;
		*windowClassName++ = (WCHAR)(digit < 0xA ? digit + 0x30 : digit + 0x37);
		digit = windowNumber % 36;
		*windowClassName++ = (WCHAR)(digit < 0xA ? digit + 0x30 : digit + 0x37);
		if (RegisterClassExW(windowClass))
			return 0;
		error = GetLastError();
		if (error != ERROR_CLASS_ALREADY_EXISTS || windowNumber != 1295)
			return error;
		else
		{
			windowClassName -= 2;
			++windowNumber;
		}
	}
}

BOOL CALLBACK liwGetRectangleSizeOfAllMonitorsMonitorEnumerationPrucedure(HMONITOR monitor, HDC monitorDeviceContext, RECT* monitorRectangle, RECT* rectangle)
{
	if (monitorRectangle->left < rectangle->left)
		rectangle->left = monitorRectangle->left;
	if (monitorRectangle->top < rectangle->top)
		rectangle->top = monitorRectangle->top;
	if (monitorRectangle->right > rectangle->right)
		rectangle->right = monitorRectangle->right;
	if (monitorRectangle->bottom > rectangle->bottom)
		rectangle->bottom = monitorRectangle->bottom;
	return TRUE;
}

DWORD liwGetRectangleSizeOfAllMonitors(SIZE_T* widthAndheight)
{
	RECT rectangle = { 0, 0, 0, 0 };
	EnumDisplayMonitors(0, 0, (MONITORENUMPROC)liwGetRectangleSizeOfAllMonitorsMonitorEnumerationPrucedure, (LPARAM)&rectangle);
	if (!(SIZE_T)(rectangle.right - rectangle.left) || !(SIZE_T)(rectangle.bottom - rectangle.top))
	{
		rectangle.left = 0;
		rectangle.top = 0;
		rectangle.right = GetSystemMetrics(SM_CXSCREEN);
		rectangle.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	widthAndheight[0] = (SIZE_T)(rectangle.right - rectangle.left);
	widthAndheight[1] = (SIZE_T)(rectangle.bottom - rectangle.top);
	return 0;
}

BOOL WINAPI liwPrefetchVirtualMemory(HANDLE hProcess, ULONG_PTR NumberOfEntries, const UINT_PTR* VirtualAddresses, ULONG Flags)
{
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	SIZE_T pageSize = (SIZE_T)systemInfo.dwPageSize;
	volatile BYTE ignored;
	for (const UINT_PTR* endVirtualAddresses = VirtualAddresses + (NumberOfEntries << 1); VirtualAddresses != endVirtualAddresses;)
	{
		UINT_PTR prefetch = *VirtualAddresses++;
		UINT_PTR endPrefetch = prefetch + *VirtualAddresses++;
		prefetch &= ~(pageSize - 1);
		endPrefetch = (endPrefetch + (pageSize - 1)) & ~(pageSize - 1);
		while (prefetch != endPrefetch)
		{
			ignored = *(volatile BYTE*)prefetch;
			prefetch += pageSize;
		}
	}
	return TRUE;
}

DWORD CALLBACK liwAsynchronousPrefetchVirtualMemoryProcedure(LPVOID prefechInfo)
{
	HANDLE currentProcess = GetCurrentProcess();
	BOOL (WINAPI* PrefetchVirtualMemory)(HANDLE hProcess, ULONG_PTR NumberOfEntries, const UINT_PTR* VirtualAddresses, ULONG Flags) = (BOOL(WINAPI*)(HANDLE, ULONG_PTR, const UINT_PTR*, ULONG))GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "PrefetchVirtualMemory");
	if (!PrefetchVirtualMemory)
		PrefetchVirtualMemory = liwPrefetchVirtualMemory;
	UINT_PTR fileMapping[2] = { *(UINT_PTR*)prefechInfo, *(UINT_PTR*)((UINT_PTR)prefechInfo + sizeof(UINT_PTR)) };
	HANDLE prefechCompletedEvent = *(HANDLE*)((UINT_PTR)prefechInfo + (2 * sizeof(UINT_PTR)));
	HeapFree(*(HANDLE*)((UINT_PTR)prefechInfo + (2 * sizeof(UINT_PTR)) + sizeof(HANDLE)), 0, prefechInfo);
	DWORD error = PrefetchVirtualMemory(currentProcess, 1, fileMapping, 0) ? 0 : GetLastError();
	SetEvent(prefechCompletedEvent);
	ExitThread(error);
}

DWORD liwAsynchronousPrefetchVirtualMemory(const LPVOID address, SIZE_T size, HANDLE* prefechCompletedEvent)
{
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	SIZE_T allocationGranularity = (SIZE_T)systemInfo.dwAllocationGranularity;
	HANDLE event = CreateEventW(0, FALSE, FALSE, 0);
	if (!event)
		return liwGetLastError();
	HANDLE heap = GetProcessHeap();
	if (!heap)
	{
		CloseHandle(event);
		return liwGetLastError();
	}
	LPVOID prefechInfo = HeapAlloc(heap, 0, (2 * sizeof(UINT_PTR) + (2 * sizeof(HANDLE))));
	if (!prefechInfo)
		return liwGetLastError();
	*(UINT_PTR*)prefechInfo = (UINT_PTR)address;
	*(UINT_PTR*)((UINT_PTR)prefechInfo + sizeof(UINT_PTR)) = (UINT_PTR)size;
	*(HANDLE*)((UINT_PTR)prefechInfo + (2 * sizeof(UINT_PTR))) = event;
	*(HANDLE*)((UINT_PTR)prefechInfo + (2 * sizeof(UINT_PTR)) + sizeof(HANDLE)) = heap;
	HANDLE prefechThread = CreateThread(0, (0x10000 + (allocationGranularity - 1)) & ~(allocationGranularity - 1), liwAsynchronousPrefetchVirtualMemoryProcedure, prefechInfo, 0, 0);
	if (!prefechThread)
	{
		DWORD error = liwGetLastError();
		CloseHandle(event);
		HeapFree(heap, 0, prefechInfo);
		return error;
	}
	CloseHandle(prefechThread);
	*prefechCompletedEvent = event;
	return 0;
}

#define PROCESS_MITIGATION_POLICY_DEP 0
#define PROCESS_MITIGATION_POLICY_ASLR 1
#define PROCESS_MITIGATION_POLICY_DYNAMIC_CODE 2
#define PROCESS_MITIGATION_POLICY_STRICT_HANDLE_CHECK 3
#define PROCESS_MITIGATION_POLICY_SYSTEM_CALL_DISABLE 4
#define PROCESS_MITIGATION_POLICY_OPTION_MASK 5
#define PROCESS_MITIGATION_POLICY_EXTENSION_POINT_DISABLE 6
#define PROCESS_MITIGATION_POLICY_CONTROL_FLOW_GUARD 7
#define PROCESS_MITIGATION_POLICY_SIGNATURE 8
#define PROCESS_MITIGATION_POLICY_FONT_DISABLE 9
#define PROCESS_MITIGATION_POLICY_IMAGE_LOAD 10
void liwInitializeSecurity()
{
	HMODULE Kernel32 = GetModuleHandleW(L"Kernel32.dll");
	if (!Kernel32)
		liwFatalError(liwGetLastError(), L"Program startup failed.");
	BOOL (WINAPI* SetProcessMitigationPolicy)(int MitigationPolicy, const PVOID lpBuffer, SIZE_T dwLength) = (BOOL (WINAPI*)(int, const PVOID, SIZE_T))GetProcAddress(Kernel32, "SetProcessMitigationPolicy");
	if (SetProcessMitigationPolicy)
	{
		const DWORD signaturePolicy = 0x00000005;
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_SIGNATURE, (const PVOID)&signaturePolicy, sizeof(signaturePolicy));
		const DWORD dynamicCodePolicy = 0x00000001;
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_DYNAMIC_CODE, (const PVOID)&dynamicCodePolicy, sizeof(dynamicCodePolicy));
		const DWORD imageLoadPolicy = 0x00000007;
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_IMAGE_LOAD, (const PVOID)&imageLoadPolicy, sizeof(imageLoadPolicy));
		const DWORD aslrPolicy = 0x00000007;
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_ASLR, (const PVOID)&aslrPolicy, sizeof(aslrPolicy));
		const DWORD fontPolicy = 0x00000001;
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_FONT_DISABLE, (const PVOID)&fontPolicy, sizeof(fontPolicy));
		const DWORD strictHandleCheckPolicy = 0x00000003;
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_FONT_DISABLE, (const PVOID)&strictHandleCheckPolicy, sizeof(strictHandleCheckPolicy));
		struct { DWORD flasg; BOOLEAN permanent; } const DEPPolicy = { 0x00000003, TRUE };
		SetProcessMitigationPolicy(PROCESS_MITIGATION_POLICY_DEP, (const PVOID)&DEPPolicy, sizeof(DEPPolicy));
	}
	BOOL (WINAPI* HeapSetInformation)(HANDLE HeapHandle, int HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength) = (BOOL (WINAPI*)(HANDLE, int, PVOID, SIZE_T))GetProcAddress(Kernel32, "HeapSetInformation");
	HANDLE heap = GetProcessHeap();
	if (HeapSetInformation && heap)
		HeapSetInformation(heap, 1, 0, 0);
}

void liwDrawDisplayImage(liwMainWindow* data)
{
	SIZE_T outputWidth = (SIZE_T)data->displayWHAndImageWH[0];
	SIZE_T outputHeight = (SIZE_T)data->displayWHAndImageWH[1];
	SIZE_T sourceWidth = (SIZE_T)data->displayWHAndImageWH[2];
	SIZE_T sourceHeight = (SIZE_T)data->displayWHAndImageWH[3];
	const DWORD* source = (const DWORD*)LIW_GET_WINDOW_IMAGE_BUFFER(data);
	SIZE_T destinationX = (SIZE_T)data->displayImageXYWH[0];
	SIZE_T destinationY = (SIZE_T)data->displayImageXYWH[1];
	SIZE_T destinationWidth = (SIZE_T)data->displayImageXYWH[2];
	SIZE_T destinationHeight = (SIZE_T)data->displayImageXYWH[3];
	DWORD * destination = (DWORD*)LIW_GET_WINDOW_DISPLAY_BUFFER(data);
	float w = (float)sourceWidth / (float)destinationWidth;
	float h = (float)sourceHeight / (float)destinationHeight;
	float colors[4][3];
	DWORD* write = destination;
	for (DWORD* fillEnd = write + (destinationY * outputWidth); write != fillEnd;)
		*write++ = 0;
	for (float y = 0.0f, x = 0.0f; y < (float)destinationHeight; ++y, x = 0.0f)
	{
		for (DWORD* fillEnd = write + destinationX; write != fillEnd;)
			*write++ = 0;
		float blendY = (float)((y * h) - (float)(int)(y * h));
		for (const DWORD* rows[2] = { source + ((SIZE_T)(y * h) * sourceWidth), source + ((SIZE_T)((y + 1.0f) * h) * sourceWidth) }; x < (float)destinationWidth; ++x)
		{
			float blendX = (float)((x * w) - (float)(int)(x * w));
			DWORD SourcePixel = rows[0][(SIZE_T)(x * w)];
			colors[0][0] = (float)(SourcePixel & 0xFF);
			colors[0][1] = (float)((SourcePixel & 0xFF00) >> 8);
			colors[0][2] = (float)((SourcePixel & 0xFF0000) >> 16);
			SourcePixel = rows[0][(SIZE_T)((x + 1.0f) * w)];
			colors[1][0] = (float)(SourcePixel & 0xFF);
			colors[1][1] = (float)((SourcePixel & 0xFF00) >> 8);
			colors[1][2] = (float)((SourcePixel & 0xFF0000) >> 16);
			SourcePixel = rows[1][(SIZE_T)(x * w)];
			colors[2][0] = (float)(SourcePixel & 0xFF);
			colors[2][1] = (float)((SourcePixel & 0xFF00) >> 8);
			colors[2][2] = (float)((SourcePixel & 0xFF0000) >> 16);
			SourcePixel = rows[1][(SIZE_T)((x + 1.0f) * w)];
			colors[3][0] = (float)(SourcePixel & 0xFF);
			colors[3][1] = (float)((SourcePixel & 0xFF00) >> 8);
			colors[3][2] = (float)((SourcePixel & 0xFF0000) >> 16);
			colors[2][0] = ((1.0f - blendX) * colors[2][0]) + (blendX * colors[3][0]);
			colors[2][1] = ((1.0f - blendX) * colors[2][1]) + (blendX * colors[3][1]);
			colors[2][2] = ((1.0f - blendX) * colors[2][2]) + (blendX * colors[3][2]);
			colors[0][0] = ((1.0f - blendX) * colors[0][0]) + (blendX * colors[1][0]);
			colors[0][1] = ((1.0f - blendX) * colors[0][1]) + (blendX * colors[1][1]);
			colors[0][2] = ((1.0f - blendX) * colors[0][2]) + (blendX * colors[1][2]);
			colors[0][0] = ((1.0f - blendY) * colors[0][0]) + (blendY * colors[2][0]);
			colors[0][1] = ((1.0f - blendY) * colors[0][1]) + (blendY * colors[2][1]);
			colors[0][2] = ((1.0f - blendY) * colors[0][2]) + (blendY * colors[2][2]);
			*write++ = (DWORD)colors[0][0] | ((DWORD)colors[0][1] << 8) | ((DWORD)colors[0][2] << 16);
		}
		for (DWORD* fillEnd = write + (outputWidth - (destinationX + destinationWidth)); write != fillEnd;)
			*write++ = 0;
	}
	for (DWORD* fillEnd = write + ((outputHeight - (destinationY + destinationHeight)) * outputWidth); write != fillEnd;)
		*write++ = 0;
}

LRESULT CALLBACK liwWindowProcedure(HWND window, UINT message, WPARAM wParameter, LPARAM lParameter)
{
	liwMainWindow* data = (liwMainWindow*)GetWindowLongPtrW(window, GWLP_USERDATA);
	if (data)
	{
		switch (message)
		{
			case WM_PAINT :
			{
				RECT update;
				GetUpdateRect(window, &update, FALSE);
				BitBlt(data->windowDeviceContext, (int)update.left, (int)update.top, (int)(update.right - update.left), (int)(update.bottom - update.top), data->windowMemoryDeviceContext, (int)update.left, (int)update.top, SRCCOPY);
				ValidateRect(window, &update);
				return 0;
			}
			case WM_ERASEBKGND :
			{
				return 1;
			}
			case WM_SIZE :
			{
				if (wParameter != SIZE_MINIMIZED)
				{
					if (GetClientRect(window, &data->clientRectangle))
					{
						data->displayWHAndImageWH[0] = (DWORD)data->clientRectangle.right;
						data->displayWHAndImageWH[1] = (DWORD)data->clientRectangle.bottom;
						data->bitmapHeader.biWidth = (LONG)data->displayWHAndImageWH[0];
						data->bitmapHeader.biHeight = (LONG)data->displayWHAndImageWH[1];
						liwFitRectengleInRectangle(data->displayWHAndImageWH, data->displayImageXYWH);
						liwDrawDisplayImage(data);
						SelectObject(data->windowMemoryDeviceContext, data->originalWindowMemoryDeviceContextObject);
						data->originalWindowMemoryDeviceContextObject = 0;
						DeleteObject((HGDIOBJ)data->windowBitmap);
						data->windowBitmap = CreateCompatibleBitmap(data->windowDeviceContext, (int)data->displayWHAndImageWH[0], (int)data->displayWHAndImageWH[1]);
						if (data->windowBitmap)
						{
							if (SetDIBits(data->windowDeviceContext, data->windowBitmap, 0, (UINT)data->displayWHAndImageWH[1], (const VOID*)LIW_GET_WINDOW_DISPLAY_BUFFER(data), (const BITMAPINFO*)&data->bitmapHeader, DIB_RGB_COLORS) == (int)data->displayWHAndImageWH[1])
							{
								data->originalWindowMemoryDeviceContextObject = SelectObject(data->windowMemoryDeviceContext, (HGDIOBJ)data->windowBitmap);
								if (data->originalWindowMemoryDeviceContextObject)
								{
									if (BitBlt(data->windowDeviceContext, 0, 0, (int)data->displayWHAndImageWH[0], (int)data->displayWHAndImageWH[1], data->windowMemoryDeviceContext, 0, 0, SRCCOPY) && ValidateRect(window, 0))
										return 0;
									else
										data->error = liwGetLastError();
									SelectObject(data->windowMemoryDeviceContext, data->originalWindowMemoryDeviceContextObject);
									data->originalWindowMemoryDeviceContextObject = 0;
								}
								else
									data->error = liwGetLastError();
							}
							else
								data->error = liwGetLastError();
							DeleteObject((HGDIOBJ)data->windowBitmap);
							data->windowBitmap = 0;
						}
						else
							data->error = liwGetLastError();
					}
					else
						data->error = liwGetLastError();
					data->run = FALSE;
				}
				return 0;
			}
			case WM_TIMER :
			{
				return 0;
			}
			case WM_KEYDOWN :
			{
				return 0;
			}
			case WM_CLOSE :
			case WM_DESTROY :
			case WM_QUIT :
			{
				data->run = FALSE;
				for (DWORD* clear = (DWORD*)LIW_GET_WINDOW_DISPLAY_BUFFER(data), *endClear = clear + ((SIZE_T)data->displayWHAndImageWH[0] * (SIZE_T)data->displayWHAndImageWH[1]); clear != endClear;)
					*clear++ = 0;
				SelectObject(data->windowMemoryDeviceContext, data->originalWindowMemoryDeviceContextObject);
				data->originalWindowMemoryDeviceContextObject = 0;
				if (SetDIBits(data->windowDeviceContext, data->windowBitmap, 0, (UINT)data->displayWHAndImageWH[1], (const VOID*)LIW_GET_WINDOW_DISPLAY_BUFFER(data), (const BITMAPINFO*)&data->bitmapHeader, DIB_RGB_COLORS) == (int)data->displayWHAndImageWH[1])
				{
					data->originalWindowMemoryDeviceContextObject = SelectObject(data->windowMemoryDeviceContext, (HGDIOBJ)data->windowBitmap);
					if (data->originalWindowMemoryDeviceContextObject)
					{
						if (BitBlt(data->windowDeviceContext, 0, 0, (int)data->displayWHAndImageWH[0], (int)data->displayWHAndImageWH[1], data->windowMemoryDeviceContext, 0, 0, SRCCOPY))
							ValidateRect(window, 0);
					}
				}
				return 0;
			}
			default :
				return DefWindowProcW(window, message, wParameter, lParameter);
		}
	}
	else if (message == WM_CREATE)
	{
		data = (liwMainWindow*)((CREATESTRUCT*)lParameter)->lpCreateParams;
		data->windowHandle = window;
		SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)data);
		SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
		if ((liwMainWindow*)GetWindowLongPtrW(window, GWLP_USERDATA) != data || !GetClientRect(window, &data->clientRectangle))
			return (LRESULT)-1;
		data->displayWHAndImageWH[0] = (DWORD)data->clientRectangle.right;
		data->displayWHAndImageWH[1] = (DWORD)data->clientRectangle.bottom;
		data->bitmapHeader.biWidth = (LONG)data->displayWHAndImageWH[0];
		data->bitmapHeader.biHeight = (LONG)data->displayWHAndImageWH[1];
		liwFitRectengleInRectangle(data->displayWHAndImageWH, data->displayImageXYWH);
		data->windowDeviceContext = GetDC(window);
		if (!data->windowDeviceContext)
			return (LRESULT)-1;
		liwDrawDisplayImage(data);
		data->windowMemoryDeviceContext = CreateCompatibleDC(data->windowDeviceContext);
		if (data->windowMemoryDeviceContext)
		{
			data->windowBitmap = CreateCompatibleBitmap(data->windowDeviceContext, (int)data->displayWHAndImageWH[0], (int)data->displayWHAndImageWH[1]);
			if (data->windowBitmap)
			{
				if (SetDIBits(data->windowDeviceContext, data->windowBitmap, 0, (UINT)data->displayWHAndImageWH[1], (const VOID*)LIW_GET_WINDOW_DISPLAY_BUFFER(data), (const BITMAPINFO*)&data->bitmapHeader, DIB_RGB_COLORS) == (int)data->displayWHAndImageWH[1])
				{
					data->originalWindowMemoryDeviceContextObject = SelectObject(data->windowMemoryDeviceContext, (HGDIOBJ)data->windowBitmap);
					if (data->originalWindowMemoryDeviceContextObject)
					{
						if (BitBlt(data->windowDeviceContext, 0, 0, (int)data->displayWHAndImageWH[0], (int)data->displayWHAndImageWH[1], data->windowMemoryDeviceContext, 0, 0, SRCCOPY) && ValidateRect(window, 0))
							return 0;
						SelectObject(data->windowMemoryDeviceContext, data->originalWindowMemoryDeviceContextObject);
					}
				}
				DeleteObject((HGDIOBJ)data->windowBitmap);
			}
			DeleteObject((HGDIOBJ)data->windowMemoryDeviceContext);
		}
		ReleaseDC(window, data->windowDeviceContext);
		return (LRESULT)-1;
	}
	else
		return DefWindowProcW(window, message, wParameter, lParameter);
}

DWORD liwCreateMainWindow(liwMainWindow** window, const WCHAR* imageFileName)
{
	DWORD error = 0;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return liwGetLastError();
	SIZE_T imageFileNameLength = (SIZE_T)lstrlenW(imageFileName);
	DWORD imageFileNameHash = 0xFFFFFFFF;
	for (const BYTE* hashData = (const BYTE*)imageFileName, * hashDataEnd = (const BYTE*)(hashData + (imageFileNameLength * sizeof(WCHAR))); hashData != hashDataEnd; ++hashData)
	{
		imageFileNameHash = imageFileNameHash ^ (DWORD)*hashData;
		for (SIZE_T i = 8; i--;)
			imageFileNameHash = (imageFileNameHash >> 1) ^ (0xEDB88320 & (0 - (imageFileNameHash & 1)));
	}
	HANDLE imageFileHandle = CreateFileW(imageFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (imageFileHandle == INVALID_HANDLE_VALUE)
		return liwGetLastError();
	ULONGLONG nativeImageFileSize;
	if (!GetFileSizeEx(imageFileHandle, (LARGE_INTEGER*)&nativeImageFileSize))
	{
		error = liwGetLastError();
		CloseHandle(imageFileHandle);
		return error;
	}
	if (nativeImageFileSize > (ULONGLONG)((SIZE_T)~0))
	{
		CloseHandle(imageFileHandle);
		return ERROR_FILE_TOO_LARGE;
	}
	WCHAR fileMappingName[28];
	for (WCHAR* source = (WCHAR*)L"Local\\LIW_", *sourceEnd = source + 10, *destination = fileMappingName; source != sourceEnd;)
		*destination++ = *source++;
	DWORD currentProcessIdentity = GetCurrentProcessId();
	for (DWORD index = 0; index != 8; ++index)
	{
		DWORD textOutput = (currentProcessIdentity >> ((7 - index) << 2)) & 0xF;
		textOutput = textOutput < 0xA ? textOutput + 0x30 : textOutput + 0x37;
		fileMappingName[10 + index] = (WCHAR)textOutput;
	}
	fileMappingName[18] = L'_';
	for (DWORD index = 0; index != 8; ++index)
	{
		DWORD textOutput = (imageFileNameHash >> ((7 - index) << 2)) & 0xF;
		textOutput = textOutput < 0xA ? textOutput + 0x30 : textOutput + 0x37;
		fileMappingName[19 + index] = (WCHAR)textOutput;
	}
	fileMappingName[27] = 0;
	SECURITY_ATTRIBUTES lowIntegritySecurityAttributes = { sizeof(SECURITY_ATTRIBUTES), 0, FALSE };
	if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"S:(ML;;NW;;;LW)", SDDL_REVISION_1, &lowIntegritySecurityAttributes.lpSecurityDescriptor, NULL))
	{
		error = liwGetLastError();
		CloseHandle(imageFileHandle);
		return error;
	}
	HANDLE imageFileMappingHandle = CreateFileMappingW(imageFileHandle, &lowIntegritySecurityAttributes, PAGE_READONLY, (DWORD)(nativeImageFileSize >> 32), (DWORD)nativeImageFileSize, fileMappingName);
	if (!imageFileMappingHandle)
	{
		error = liwGetLastError();
		LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
		CloseHandle(imageFileHandle);
		return error;
	}
	CloseHandle(imageFileHandle);
	LPVOID mappedImageFileData = MapViewOfFile(imageFileMappingHandle, FILE_MAP_READ, 0, 0, (SIZE_T)nativeImageFileSize);
	if (!mappedImageFileData)
	{
		error = liwGetLastError();
		CloseHandle(imageFileMappingHandle);
		LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
		return error;
	}
	HANDLE prefetchCompletedEvent;
	if (liwAsynchronousPrefetchVirtualMemory(mappedImageFileData, (SIZE_T)nativeImageFileSize, &prefetchCompletedEvent))
		prefetchCompletedEvent = 0;
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	SIZE_T pageSize = (SIZE_T)systemInfo.dwPageSize;
	SIZE_T allocationGranularity = (SIZE_T)systemInfo.dwAllocationGranularity;
	SIZE_T screenWidthAndheight[2];
	error = liwGetRectangleSizeOfAllMonitors(screenWidthAndheight);
	if (error)
	{
		error = 0;
		screenWidthAndheight[0] = 0;
		screenWidthAndheight[1] = 0;
	}
	SIZE_T currentExecutableFileNameBufferLength = MAX_PATH + 1;
	WCHAR* currentExecutableFileName = (WCHAR*)HeapAlloc(heap, 0, currentExecutableFileNameBufferLength * sizeof(WCHAR));
	if (!currentExecutableFileName)
	{
		error = liwGetLastError();
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
		return error;
	}
	SIZE_T CurrentExecutableFileNameLength = currentExecutableFileNameBufferLength;
	while (CurrentExecutableFileNameLength >= currentExecutableFileNameBufferLength)
	{
		CurrentExecutableFileNameLength = (SIZE_T)GetModuleFileNameW(0, currentExecutableFileName, (DWORD)currentExecutableFileNameBufferLength);
		if (!CurrentExecutableFileNameLength)
		{
			error = liwGetLastError();
			HeapFree(heap, 0, currentExecutableFileName);
			if (prefetchCompletedEvent)
			{
				WaitForSingleObject(prefetchCompletedEvent, INFINITE);
				CloseHandle(prefetchCompletedEvent);
			}
			UnmapViewOfFile(mappedImageFileData);
			CloseHandle(imageFileMappingHandle);
			LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
			return error;
		}
		else if (CurrentExecutableFileNameLength >= currentExecutableFileNameBufferLength)
		{
			currentExecutableFileNameBufferLength = currentExecutableFileNameBufferLength < CurrentExecutableFileNameLength ? CurrentExecutableFileNameLength : CurrentExecutableFileNameLength + 1;
			WCHAR* newCurrentExecutableFileName = (WCHAR*)HeapReAlloc(heap, 0, currentExecutableFileName, currentExecutableFileNameBufferLength * sizeof(WCHAR));
			if (!newCurrentExecutableFileName)
			{
				error = liwGetLastError();
				HeapFree(heap, 0, currentExecutableFileName);
				if (prefetchCompletedEvent)
				{
					WaitForSingleObject(prefetchCompletedEvent, INFINITE);
					CloseHandle(prefetchCompletedEvent);
				}
				UnmapViewOfFile(mappedImageFileData);
				CloseHandle(imageFileMappingHandle);
				LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
				return error;
			}
			currentExecutableFileName = newCurrentExecutableFileName;
		}
	}
	WCHAR* loadProcessCommandLine = (WCHAR*)HeapAlloc(heap, 0, (39 + CurrentExecutableFileNameLength + 27) * sizeof(WCHAR));
	if (!loadProcessCommandLine)
	{
		error = liwGetLastError();
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
		return error;
	}
	loadProcessCommandLine[0] = L'\"';
	for (WCHAR* source = currentExecutableFileName, *sourceEnd = source + CurrentExecutableFileNameLength, * destination = loadProcessCommandLine + 1; source != sourceEnd;)
		*destination++ = *source++;
	for (WCHAR* source = (WCHAR*)L"\" -l \"", *sourceEnd = source + 6, * destination = loadProcessCommandLine + 1 + CurrentExecutableFileNameLength; source != sourceEnd;)
		*destination++ = *source++;
	WCHAR loadPipeName[25];
	for (WCHAR* source = (WCHAR*)L"\\\\.\\PIPE\\LIW_", * sourceEnd = source + 13, * destination = loadPipeName; source != sourceEnd;)
		*destination++ = *source++;
	for (DWORD process = GetCurrentProcessId(), index = 0; index != 8; ++index)
	{
		DWORD textOutput = (process >> ((7 - index) << 2)) & 0xF;
		textOutput = textOutput < 0xA ? textOutput + 0x30 : textOutput + 0x37;
		loadPipeName[13 + index] = (WCHAR)textOutput;
	}
	loadPipeName[21] = L'_';
	DWORD loadPipeNameRandom = (GetCurrentThreadId() ^ (GetTickCount() >> 4)) % 1296;
	loadPipeName[22] = (WCHAR)(loadPipeNameRandom / 36);
	loadPipeName[22] = (loadPipeName[22] < 0xA ? loadPipeName[22] + 0x30 : loadPipeName[22] + 0x37);
	loadPipeName[23] = (WCHAR)(loadPipeNameRandom % 36);
	loadPipeName[23] = (loadPipeName[23] < 0xA ? loadPipeName[23] + 0x30 : loadPipeName[23] + 0x37);
	loadPipeName[24] = 0;
	for (WCHAR* source = loadPipeName, *sourceEnd = source + 24, *destination = loadProcessCommandLine + 1 + CurrentExecutableFileNameLength + 6; source != sourceEnd;)
		*destination++ = *source++;
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 24] = L'\"';
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 25] = L' ';
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 26] = L'-';
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 27] = L'm';
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 28] = L' ';
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 29] = L'\"';
	for (WCHAR* source = (WCHAR*)fileMappingName, *sourceEnd = source + 27, *destination = loadProcessCommandLine + 1 + CurrentExecutableFileNameLength + 6 + 30; source != sourceEnd;)
		*destination++ = *source++;
	loadProcessCommandLine[1 + CurrentExecutableFileNameLength + 6 + 30 + 27] = L'\"';
	loadProcessCommandLine[38 + CurrentExecutableFileNameLength + 27] = 0;
	SIZE_T systemDirectoryBufferLenght = MAX_PATH + 1;
	WCHAR* systemDirectory = (WCHAR*)HeapAlloc(heap, 0, systemDirectoryBufferLenght * sizeof(WCHAR));
	if (!systemDirectory)
	{
		error = GetLastError();
		HeapFree(heap, 0, loadProcessCommandLine);
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
		return error;
	}
	for (SIZE_T systemDirectoryLenght = systemDirectoryBufferLenght; systemDirectoryLenght >= systemDirectoryBufferLenght;)
	{
		systemDirectoryLenght = GetSystemDirectoryW(systemDirectory, (UINT)systemDirectoryBufferLenght);
		if (!systemDirectoryLenght)
		{
			error = GetLastError();
			HeapFree(heap, 0, systemDirectory);
			HeapFree(heap, 0, loadProcessCommandLine);
			HeapFree(heap, 0, currentExecutableFileName);
			if (prefetchCompletedEvent)
			{
				WaitForSingleObject(prefetchCompletedEvent, INFINITE);
				CloseHandle(prefetchCompletedEvent);
			}
			UnmapViewOfFile(mappedImageFileData);
			CloseHandle(imageFileMappingHandle);
			LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
			return error;
		}
		else if (systemDirectoryLenght >= systemDirectoryBufferLenght)
		{
			systemDirectoryBufferLenght = (systemDirectoryLenght > systemDirectoryBufferLenght) ? systemDirectoryLenght : (systemDirectoryLenght + 1);
			WCHAR* newSystemDirectory = (WCHAR*)HeapReAlloc(heap, 0, systemDirectory, systemDirectoryBufferLenght * sizeof(WCHAR));
			if (!newSystemDirectory)
			{
				error = GetLastError();
				HeapFree(heap, 0, systemDirectory);
				HeapFree(heap, 0, loadProcessCommandLine);
				HeapFree(heap, 0, currentExecutableFileName);
				if (prefetchCompletedEvent)
				{
					WaitForSingleObject(prefetchCompletedEvent, INFINITE);
					CloseHandle(prefetchCompletedEvent);
				}
				UnmapViewOfFile(mappedImageFileData);
				CloseHandle(imageFileMappingHandle);
				LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
				return error;
			}
			systemDirectory = newSystemDirectory;
		}
	}
	HANDLE loadPipe = CreateNamedPipeW(loadPipeName, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 0, 0x100000, 0, &lowIntegritySecurityAttributes);
	if (!loadPipe)
		loadPipe = CreateNamedPipeW(loadPipeName, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 0, 0x10000, 0, &lowIntegritySecurityAttributes);
	LocalFree(lowIntegritySecurityAttributes.lpSecurityDescriptor);
	if (!loadPipe)
	{
		error = liwGetLastError();
		HeapFree(heap, 0, systemDirectory);
		HeapFree(heap, 0, loadProcessCommandLine);
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	HANDLE currentProcessToken;
	error = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &currentProcessToken) ? 0 : liwGetLastError();
	if (error)
	{
		CloseHandle(loadPipe);
		HeapFree(heap, 0, systemDirectory);
		HeapFree(heap, 0, loadProcessCommandLine);
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	HANDLE lowIntegrityToken;
	error = DuplicateTokenEx(currentProcessToken, 0, 0, SecurityImpersonation, TokenPrimary, &lowIntegrityToken) ? 0 : liwGetLastError();
	CloseHandle(currentProcessToken);
	if (error)
	{
		CloseHandle(loadPipe);
		HeapFree(heap, 0, systemDirectory);
		HeapFree(heap, 0, loadProcessCommandLine);
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	PSID lowIntegritySID;
	error = ConvertStringSidToSidW(L"S-1-16-4096", &lowIntegritySID) ? 0 : liwGetLastError();
	if (error)
	{
		CloseHandle(lowIntegrityToken);
		CloseHandle(loadPipe);
		HeapFree(heap, 0, systemDirectory);
		HeapFree(heap, 0, loadProcessCommandLine);
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	TOKEN_MANDATORY_LABEL tokenMandatoryLabel = { { lowIntegritySID, SE_GROUP_INTEGRITY } };
	error = SetTokenInformation(lowIntegrityToken, TokenIntegrityLevel, &tokenMandatoryLabel, sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(lowIntegritySID)) ? 0 : liwGetLastError();
	LocalFree(lowIntegritySID);
	if (error)
	{
		CloseHandle(lowIntegrityToken);
		CloseHandle(loadPipe);
		HeapFree(heap, 0, systemDirectory);
		HeapFree(heap, 0, loadProcessCommandLine);
		HeapFree(heap, 0, currentExecutableFileName);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	STARTUPINFO loadProcessStartupInfo = { sizeof(STARTUPINFO), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	PROCESS_INFORMATION loadProcessInfo = { 0, 0, 0, 0 };
	error = CreateProcessAsUserW(lowIntegrityToken, currentExecutableFileName, loadProcessCommandLine, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT, 0, systemDirectory, &loadProcessStartupInfo, &loadProcessInfo) ? 0 : liwGetLastError();
	CloseHandle(lowIntegrityToken);
	HeapFree(heap, 0, systemDirectory);
	HeapFree(heap, 0, loadProcessCommandLine);
	HeapFree(heap, 0, currentExecutableFileName);
	if (error)
	{
		CloseHandle(loadPipe);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	CloseHandle(loadProcessInfo.hThread);
	HANDLE loadProcessHandle = loadProcessInfo.hProcess;
	if (!ConnectNamedPipe(loadPipe, 0) && GetLastError() != ERROR_PIPE_CONNECTED)
	{
		error = liwGetLastError();
		CloseHandle(loadProcessHandle);
		CloseHandle(loadPipe);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	SIZE_T imageWidthAndheight[2];
	DWORD pipeReadResult;
	DWORD pipeReadFailCount = 0;
	for (SIZE_T read = 0; read != 2 * sizeof(SIZE_T);)
	{
		if (ReadFile(loadPipe, (LPVOID)((UINT_PTR)imageWidthAndheight + read), (DWORD)((2 * sizeof(SIZE_T)) - read), &pipeReadResult, 0))
			read += (UINT_PTR)pipeReadResult;
		else
		{
			error = liwGetLastError();
			DWORD waitForLoadProcessResult = WaitForSingleObject(loadProcessHandle, 0);
			if (waitForLoadProcessResult != WAIT_TIMEOUT || pipeReadFailCount++ == 3)
			{
				DisconnectNamedPipe(loadPipe);
				if (waitForLoadProcessResult == WAIT_OBJECT_0 || (waitForLoadProcessResult == WAIT_TIMEOUT && WaitForSingleObject(loadProcessHandle, 0x400) == WAIT_OBJECT_0))
				{
					if (!GetExitCodeProcess(loadProcessHandle, &error))
						error = liwGetLastError();
				}
				else
					TerminateProcess(loadProcessHandle, ERROR_TIMEOUT);
				CloseHandle(loadProcessHandle);
				CloseHandle(loadPipe);
				if (prefetchCompletedEvent)
				{
					WaitForSingleObject(prefetchCompletedEvent, INFINITE);
					CloseHandle(prefetchCompletedEvent);
				}
				UnmapViewOfFile(mappedImageFileData);
				CloseHandle(imageFileMappingHandle);
				return error;
			}
			error = 0;
		}
	}
	SIZE_T estimatedMaximunSize = ((((((imageWidthAndheight[0] + 0xFF) & ~0xFF) * ((imageWidthAndheight[1] + 0xFF) & ~0xFF)) << 2) > ((((screenWidthAndheight[0] + 0xFF) & ~0xFF) * ((screenWidthAndheight[1] + 0xFF) & ~0xFF)) << 2) ? ((((imageWidthAndheight[0] + 0xFF) & ~0xFF) * ((imageWidthAndheight[1] + 0xFF) & ~0xFF)) << 2) : ((((screenWidthAndheight[0] + 0xFF) & ~0xFF) * ((screenWidthAndheight[1] + 0xFF) & ~0xFF)) << 2)) + (allocationGranularity - 1)) & ~(allocationGranularity - 1);
	SIZE_T imageSize = ((SIZE_T)imageWidthAndheight[0] * (SIZE_T)imageWidthAndheight[1]) << 2;
	liwMainWindow* windowData = (liwMainWindow*)VirtualAlloc(0, (LIW_WINDOW_STUCTURE_SIZE + LIW_GET_IMAGE_BUFFER_SIZE(imageWidthAndheight[0], imageWidthAndheight[1]) + estimatedMaximunSize + (allocationGranularity - 1)) & ~(allocationGranularity - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!windowData)
	{
		error = liwGetLastError();
		DisconnectNamedPipe(loadPipe);
		CloseHandle(loadProcessHandle);
		CloseHandle(loadPipe);
		if (prefetchCompletedEvent)
		{
			WaitForSingleObject(prefetchCompletedEvent, INFINITE);
			CloseHandle(prefetchCompletedEvent);
		}
		UnmapViewOfFile(mappedImageFileData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	pipeReadFailCount = 0;
	for (UINT_PTR writeImage = (UINT_PTR)LIW_GET_WINDOW_IMAGE_BUFFER(windowData), writeImageEnd = writeImage + imageSize; writeImage != writeImageEnd;)
	{
		if (ReadFile(loadPipe, (LPVOID)writeImage, (DWORD)(((writeImageEnd - imageSize) > 0x10000) ? 0x10000 : (writeImageEnd - imageSize)), &pipeReadResult, 0))
			writeImage += (UINT_PTR)pipeReadResult;
		else
		{
			error = liwGetLastError();
			DWORD waitForLoadProcessResult = WaitForSingleObject(loadProcessHandle, 0);
			if (waitForLoadProcessResult != WAIT_TIMEOUT || pipeReadFailCount++ == 3)
			{
				error = liwGetLastError();
				VirtualFree(windowData, 0, MEM_RELEASE);
				DisconnectNamedPipe(loadPipe);
				if (waitForLoadProcessResult == WAIT_OBJECT_0 || (waitForLoadProcessResult == WAIT_TIMEOUT && WaitForSingleObject(loadProcessHandle, 0x400) == WAIT_OBJECT_0))
				{
					if (!GetExitCodeProcess(loadProcessHandle, &error))
						error = liwGetLastError();
				}
				else
					TerminateProcess(loadProcessHandle, ERROR_TIMEOUT);
				CloseHandle(loadProcessHandle);
				CloseHandle(loadPipe);
				if (prefetchCompletedEvent)
				{
					WaitForSingleObject(prefetchCompletedEvent, INFINITE);
					CloseHandle(prefetchCompletedEvent);
				}
				UnmapViewOfFile(mappedImageFileData);
				CloseHandle(imageFileMappingHandle);
				return error;
			}
			error = 0;
		}
	}
	DisconnectNamedPipe(loadPipe);
	CloseHandle(loadProcessHandle);
	CloseHandle(loadPipe);
	if (prefetchCompletedEvent)
	{
		WaitForSingleObject(prefetchCompletedEvent, INFINITE);
		CloseHandle(prefetchCompletedEvent);
	}
	UnmapViewOfFile(mappedImageFileData);
	CloseHandle(imageFileMappingHandle);
	windowData->run = TRUE;
	windowData->error = 0;
	windowData->windowDeviceContext = 0;
	windowData->displayWHAndImageWH[0] = 0;
	windowData->displayWHAndImageWH[1] = 0;
	windowData->displayWHAndImageWH[2] = (DWORD)imageWidthAndheight[0];
	windowData->displayWHAndImageWH[3] = (DWORD)imageWidthAndheight[1];
	windowData->displayImageXYWH[0] = 0;
	windowData->displayImageXYWH[1] = 0;
	windowData->displayImageXYWH[2] = 0;
	windowData->displayImageXYWH[3] = 0;
	windowData->bitmapHeader.biSize = sizeof(BITMAPINFOHEADER);
	windowData->bitmapHeader.biWidth = 0;
	windowData->bitmapHeader.biHeight = 0;
	windowData->bitmapHeader.biPlanes = 1;
	windowData->bitmapHeader.biBitCount = 32;
	windowData->bitmapHeader.biCompression = BI_RGB;
	windowData->bitmapHeader.biSizeImage = 0;
	windowData->bitmapHeader.biXPelsPerMeter = 0;
	windowData->bitmapHeader.biYPelsPerMeter = 0;
	windowData->bitmapHeader.biClrUsed = 0;
	windowData->bitmapHeader.biClrImportant = 0;
	windowData->windowHandle = 0;
	windowData->Instance = (HINSTANCE)GetModuleHandleW(0);
	windowData->pageSize = pageSize;
	windowData->committedBufferSize = estimatedMaximunSize;
	windowData->reservedBufferSize = estimatedMaximunSize;
	WNDCLASSEXW windowClass = {
		sizeof(WNDCLASSEXW),
		CS_HREDRAW | CS_VREDRAW,
		liwWindowProcedure,
		0,
		0,
		windowData->Instance,
		LoadIconW(0, (const WCHAR*)IDI_APPLICATION),
		LoadCursorW(0, (const WCHAR*)IDC_ARROW),
		0,
		0,
		windowData->windowClassName,
		(HICON)LoadImageW(windowData->Instance, (const WCHAR*)MAKEINTRESOURCE(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR) };
	error = liwRegisterWindowClass(&windowClass);
	if (error)
	{
		VirtualFree(windowData, 0, MEM_RELEASE);
		return error;
	}
	error = liwCreateWindow(0, WS_EX_APPWINDOW, windowData->windowClassName, imageFileName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, windowData->Instance, (LPVOID)windowData);
	if (error)
	{
		UnregisterClassW(windowData->windowClassName, windowData->Instance);
		VirtualFree(windowData, 0, MEM_RELEASE);
		return error;
	}
	*window = windowData;
	return 0;
}

void liwCloseMainWindow(liwMainWindow* window)
{
	if (window->originalWindowMemoryDeviceContextObject)
		SelectObject(window->windowMemoryDeviceContext, window->originalWindowMemoryDeviceContextObject);
	if (window->windowBitmap)
		DeleteObject((HGDIOBJ)window->windowBitmap);
	DeleteObject((HGDIOBJ)window->windowMemoryDeviceContext);
	ReleaseDC(window->windowHandle, window->windowDeviceContext);
	DestroyWindow(window->windowHandle);
	UnregisterClassW(window->windowClassName, window->Instance);
	VirtualFree(window, 0, MEM_RELEASE);
}

DWORD liwMainWindowLoop(liwMainWindow* window)
{
	HWND windowHandle = window->windowHandle;
	for (BOOL loop = GetMessageW(&window->windowMessage, windowHandle, 0, 0); window->run && loop && loop != (BOOL)-1; loop = GetMessageW(&window->windowMessage, windowHandle, 0, 0))
	{
		TranslateMessage(&window->windowMessage);
		DispatchMessageW(&window->windowMessage);
	}
	return window->error;
}

DWORD liwLoadImage(const WCHAR* pipeName, const WCHAR* imageFileName)
{
	DWORD error;
	HANDLE imageFileMappingHandle = OpenFileMappingW(FILE_MAP_READ, FALSE, imageFileName);
	if (!imageFileMappingHandle)
		return liwGetLastError();
	LPVOID filaData = MapViewOfFile(imageFileMappingHandle, FILE_MAP_READ, 0, 0, 0);
	if (!filaData)
	{
		error = liwGetLastError();
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	MEMORY_BASIC_INFORMATION mappedImageFileDataInfo;
	if (!VirtualQuery(filaData, &mappedImageFileDataInfo, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		error = liwGetLastError();
		UnmapViewOfFile(filaData);
		CloseHandle(imageFileMappingHandle);
		return error;
	}
	CloseHandle(imageFileMappingHandle);
	SIZE_T fileSize = mappedImageFileDataInfo.RegionSize;
	if (fileSize > (SIZE_T)((int)((((unsigned int)~0) >> 1))))
	{
		UnmapViewOfFile(filaData);
		return ERROR_FILE_TOO_LARGE;
	}
	int x = 0;
	int y = 0;
	int c = 0;
	if (!stbi_info_from_memory((const stbi_uc*)filaData, (int)fileSize, &x, &y, &c) && x && y)
	{
		UnmapViewOfFile(filaData);
		return ERROR_UNSUPPORTED_TYPE;
	}
	SIZE_T widthAndheight[2] = { (SIZE_T)x, (SIZE_T)y };
	if (!WaitNamedPipeW(pipeName, NMPWAIT_WAIT_FOREVER))
	{
		error = liwGetLastError();
		UnmapViewOfFile(filaData);
		return error;
	}
	HANDLE loadPipe = CreateFileW(pipeName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (loadPipe == INVALID_HANDLE_VALUE)
	{
		error = liwGetLastError();
		UnmapViewOfFile(filaData);
		return error;
	}
	DWORD writeResult;
	DWORD writeFailCount = 0;
	for (UINT_PTR write = (UINT_PTR)widthAndheight, writeEnd = write + (2 * sizeof(SIZE_T)); write != writeEnd;)
	{
		if (WriteFile(loadPipe, (LPVOID)write, (DWORD)(writeEnd - write), &writeResult, 0))
			write += (UINT_PTR)writeResult;
		else
		{
			if (writeFailCount++ == 3)
			{
				error = liwGetLastError();
				CloseHandle(loadPipe);
				UnmapViewOfFile(filaData);
				return error;
			}
		}
	}
	c = 4;
	stbi_uc* buffer = stbi_load_from_memory((const stbi_uc*)filaData, (int)fileSize, &x, &y, &c, c);
	UnmapViewOfFile(filaData);
	if (!buffer)
	{
		CloseHandle(loadPipe);
		return ERROR_INVALID_DATA;
	}
	if ((SIZE_T)x != widthAndheight[0] || (SIZE_T)y != widthAndheight[1])
	{
		stbi_image_free(buffer);
		CloseHandle(loadPipe);
		return ERROR_INVALID_DATA;
	}
	SIZE_T stride = widthAndheight[0] << 2;
	SIZE_T bufferSize = widthAndheight[1] * stride;
	for (stbi_uc* transform = buffer, * flip = transform + bufferSize - stride, * transformEnd = transform + ((widthAndheight[1] >> 1) * stride), swap; transform != transformEnd; flip -= (stride << 1))
		for (stbi_uc* transformLineEnd = transform + stride; transform != transformLineEnd; transform += 4, flip += 4)
		{
			swap = transform[2];
			transform[2] = flip[0];
			flip[0] = swap;

			swap = transform[1];
			transform[1] = flip[1];
			flip[1] = swap;

			swap = transform[0];
			transform[0] = flip[2];
			flip[2] = swap;

			swap = transform[3];
			transform[3] = flip[3];
			flip[3] = swap;
		}
	writeFailCount = 0;
	for (UINT_PTR write = (UINT_PTR)buffer, writeEnd = write + bufferSize; write != writeEnd;)
	{
		if (WriteFile(loadPipe, (LPVOID)write, (DWORD)((writeEnd - write > 0x10000) ? 0x10000 : (writeEnd - write)), &writeResult, 0))
			write += (UINT_PTR)writeResult;
		else
		{
			if (writeFailCount++ == 3)
			{
				error = liwGetLastError();
				stbi_image_free(buffer);
				CloseHandle(loadPipe);
				return error;
			}
		}
	}
	stbi_image_free(buffer);
	FlushFileBuffers(loadPipe);
	CloseHandle(loadPipe);
	return 0;
}

#ifdef __cplusplus
}
#endif