#include "RemoteGetProcAddress.h"

int main()
{
	HMODULE Advapi32 = LoadLibraryW(L"Advapi32.dll");
	HMODULE User32 = LoadLibraryW(L"User32.dll");
	HMODULE Gdiplus = LoadLibraryW(L"Gdiplus.dll");

	HANDLE Process = GetCurrentProcess();

	HANDLE Heap = GetProcessHeap();
	if (!Heap)
		return GetLastError();
	
	HMODULE test = 0;
	DWORD Error = RemoteGetModuleHandle(Process, "User32.dll", &test);



	const IMAGE_DOS_HEADER* DOSHeader = (const IMAGE_DOS_HEADER*)GetModuleHandleA("Kernel32.dll");
	const IMAGE_NT_HEADERS* NTHeader = (const IMAGE_NT_HEADERS*)((UINT_PTR)DOSHeader + (UINT_PTR)DOSHeader->e_lfanew);
	const IMAGE_EXPORT_DIRECTORY* ExportDirectory = (const IMAGE_EXPORT_DIRECTORY*)((UINT_PTR)DOSHeader + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);





	return -1;
}