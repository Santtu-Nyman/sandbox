/*
	VarastoRobo common code 2019-11-10 by Santtu Nyman.
*/

#ifndef ROBO_WIN32_FILE_H
#define ROBO_WIN32_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <windows.h>
#include "jsonpl.h"

void robo_win32_free_file_data(LPVOID file_data);

DWORD robo_win32_load_file(const WCHAR* file_name, SIZE_T* file_size, LPVOID* file_data);

DWORD robo_win32_load_program_directory_file(const WCHAR* file_name, SIZE_T* file_size, LPVOID* file_data);

DWORD robo_win32_store_file(const WCHAR* file_name, SIZE_T file_size, LPVOID file_data);

DWORD robo_win32_load_json_from_file(const WCHAR* file_name, jsonpl_value_t** json_content);

DWORD robo_win32_load_json_from_program_directory_file(const WCHAR* file_name, jsonpl_value_t** json_content);

#ifdef __cplusplus
}
#endif

#endif