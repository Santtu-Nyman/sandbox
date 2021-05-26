#ifndef SSN_WIN32_EXTENDED_PATHS_H
#define SSN_WIN32_EXTENDED_PATHS_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>

int ssn_win32_extend_path(const WCHAR* path, size_t* required_buffer_size_address, size_t buffer_size, WCHAR* buffer);

int ssn_win32_unextend_path(const WCHAR* path, size_t* required_buffer_size_address, size_t buffer_size, WCHAR* buffer);

int ssn_win32_extend_path_utf8(const char* path, size_t* required_buffer_size_address, size_t buffer_size, char* buffer);

int ssn_win32_unextend_path_utf8(const char* path, size_t* required_buffer_size_address, size_t buffer_size, char* buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SSN_WIN32_EXTENDED_PATHS_H
