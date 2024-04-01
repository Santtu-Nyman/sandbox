/*
	VarastoRobo common code 2019-11-10 by Santtu Nyman.
*/

#ifndef ROBO_WIN32_CONSOLE_H
#define ROBO_WIN32_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <windows.h>

DWORD robo_win32_print(const WCHAR* text);

DWORD robo_win32_print_utf8(const CHAR* text);

#ifdef __cplusplus
}
#endif

#endif