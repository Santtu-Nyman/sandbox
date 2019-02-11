#ifndef REMOTE_GET_PROC_ADDRESS_H
#define REMOTE_GET_PROC_ADDRESS_H

#include <Windows.h>

DWORD WINAPI RemoteGetModuleHandle(HANDLE hProcess, const CHAR* pModuleName, HMODULE* phModule);
DWORD WINAPI RemoteGetProcAddress(HANDLE hProcess, HMODULE hModule, LPCSTR lpProcName, FARPROC* pProc);

#endif