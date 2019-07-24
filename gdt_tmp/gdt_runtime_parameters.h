/*
	Graph Drawing Tool version 1.2.0 2019-07-24 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#ifndef GDT_RUMTIME_PARAMETERS_H
#define GDT_RUMTIME_PARAMETERS_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

int gdt_get_process_arguments(size_t* argument_count, char*** arguments);

int gdt_get_flag_argument(size_t argument_count, char** arguments, const char* name);

int gdt_get_string_argument(size_t argument_count, char** arguments, const char* name, char** value);

int gdt_get_integer_argument(size_t argument_count, char** arguments, const char* name, int* value_is_negative, uint64_t* value);

int gdt_get_little_integer_argument(size_t argument_count, char** arguments, const char* name, int* value);

int gdt_get_enumeration_argument(size_t argument_count, char** arguments, const char* name, int enumeration_value_count, const char** enumeration_values, int* value);

int gdt_get_float_argument(size_t argument_count, char** arguments, const char* name, float* value);

void gdt_free(void* memory);

#ifdef _WIN32

#include <Windows.h>

int gdt_get_process_arguments_win32(size_t* argument_count, WCHAR*** arguments);

int gdt_get_flag_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name);

int gdt_get_string_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, WCHAR** value);

int gdt_get_integer_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, int* value_is_negative, uint64_t* value);

int gdt_get_little_integer_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, int* value);

int gdt_get_enumeration_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, int enumeration_value_count, const WCHAR** enumeration_values, int* value);

int gdt_get_float_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, float* value);

#endif

#endif
