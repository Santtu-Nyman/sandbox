/*
	Graph Drawing Tool 0.1.0 2019-07-18 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#include "gdt_runtime_parameters.h"
#include <errno.h>

#ifdef _WIN32

int gdt_get_process_arguments_win32(size_t* argument_count, WCHAR*** arguments)
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return ENOMEM;
	HMODULE shell32 = LoadLibraryW(L"Shell32.dll");
	if (!shell32)
		return ENOSYS;
	SIZE_T local_argument_count = 0;
	WCHAR** local_argument_values = ((WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(shell32, "CommandLineToArgvW"))(GetCommandLineW(), (int*)&local_argument_count);
	if (!local_argument_values)
	{
		FreeLibrary(shell32);
		return ENOSYS;
	}
	SIZE_T argument_value_data_size = 0;
	for (SIZE_T i = 0; i != local_argument_count; ++i)
	{
		SIZE_T overflow_check = argument_value_data_size + (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
		if (overflow_check < argument_value_data_size)
		{
			LocalFree((HLOCAL)local_argument_values);
			FreeLibrary(shell32);
			return ENOMEM;
		}
		argument_value_data_size = overflow_check;
	}
	if ((local_argument_count * sizeof(WCHAR*)) / sizeof(WCHAR*) != local_argument_count || local_argument_count * sizeof(WCHAR*) + argument_value_data_size < local_argument_count * sizeof(WCHAR*))
	{
		LocalFree((HLOCAL)local_argument_values);
		FreeLibrary(shell32);
		return ENOMEM;
	}
	WCHAR** argument_buffer = (WCHAR**)HeapAlloc(heap, 0, local_argument_count * sizeof(WCHAR*) + argument_value_data_size);
	if (!argument_buffer)
	{
		LocalFree((HLOCAL)local_argument_values);
		FreeLibrary(shell32);
		return ENOMEM;
	}
	for (SIZE_T w = local_argument_count * sizeof(WCHAR*), i = 0; i != local_argument_count; ++i)
	{
		WCHAR* p = (WCHAR*)((UINT_PTR)argument_buffer + w);
		SIZE_T s = (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
		argument_buffer[i] = p;
		for (WCHAR* copy_source = (WCHAR*)local_argument_values[i], *copy_source_end = (WCHAR*)((UINT_PTR)copy_source + s), *copy_destination = argument_buffer[i]; copy_source != copy_source_end; ++copy_source, ++copy_destination)
			*copy_destination = *copy_source;
		w += s;
	}
	LocalFree((HLOCAL)local_argument_values);
	FreeLibrary(shell32);
	*argument_count = local_argument_count;
	*arguments = argument_buffer;
	return 0;
}

int gdt_get_flag_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name)
{
	if (argument_count > 1)
		for (size_t i = 1; i != argument_count; ++i)
			if (!lstrcmpW(name, arguments[i]))
				return 0;
	return ENOENT;
}

int gdt_get_string_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, WCHAR** value)
{
	if (argument_count > 2)
		for (size_t i = 1; i + 1 != argument_count; ++i)
			if (!lstrcmpW(name, arguments[i]))
			{
				*value = arguments[i + 1];
				return 0;
			}
	return ENOENT;
}

int gdt_get_integer_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, int* value_is_negative, uint64_t* value)
{
	WCHAR* string;
	int error = gdt_get_string_argument_win32(argument_count, arguments, name, &string);
	if (error)
		return error;
	uint64_t local_value = 0;
	int local_value_is_negative = 0;
	if (*string == L'-')
	{
		local_value_is_negative = 1;
		++string;
	}
	else if (*string == L'+')
		++string;
	if (!*string)
		return EBADMSG;
	while (*string)
		if (*string >= L'0' && *string <= L'9')
		{
			uint64_t overflow_check = local_value;
			local_value = (uint64_t)10 * local_value;
			if (local_value / 10 != overflow_check)
				return ERANGE;
			overflow_check = local_value;
			local_value += (uint64_t)(*string++ - L'0');
			if (local_value < overflow_check)
				return ERANGE;
		}
		else
			return EBADMSG;
	*value_is_negative = local_value_is_negative;
	*value = local_value;
	return 0;
}


int gdt_get_little_integer_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, int* value)
{
	int is_negative;
	uint64_t large_integer;
	int error = gdt_get_integer_argument_win32(argument_count, arguments, name, &is_negative, &large_integer);
	if (error)
		return error;
	if (sizeof(int) <= sizeof(uint64_t) && ((is_negative && large_integer > ((uint64_t)1 << (((uint64_t)8 * (uint64_t)sizeof(int)) - (uint64_t)1))) || (!is_negative && large_integer > (((uint64_t)1 << (((uint64_t)8 * (uint64_t)sizeof(int)) - (uint64_t)1)) - (uint64_t)1))))
		return ERANGE;
	*value = is_negative ? -(int)large_integer : (int)large_integer;
	return 0;
}

int gdt_get_enumeration_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, int enumeration_value_count, const WCHAR** enumeration_values, int* value)
{
	WCHAR* string;
	int error = gdt_get_string_argument_win32(argument_count, arguments, name, &string);
	if (error)
		return error;
	for (int i = 0; i != enumeration_value_count; ++i)
		if (!lstrcmpW(string, enumeration_values[i]))
		{
			*value = i;
			return 0;
		}
	return EBADMSG;
}

int gdt_get_float_argument_win32(size_t argument_count, WCHAR** arguments, const WCHAR* name, float* value)
{
	WCHAR* string;
	int error = gdt_get_string_argument_win32(argument_count, arguments, name, &string);
	if (error)
		return error;
	float local_value = 0.0f;
	int value_is_negative = 0;
	if (*string == L'-')
	{
		value_is_negative = 1;
		++string;
	}
	else if (*string == L'+')
		++string;
	if (!*string)
		return EBADMSG;
	while (*string && *string != L'.')
		if (*string >= L'0' && *string <= L'9')
			local_value = 10.0f * local_value + (float)(*string++ - L'0');
		else
			return EBADMSG;
	if (*string == L'.')
	{
		++string;
		float value_low = 0.0f;
		float multiplier = 0.1f;
		while (*string && *string != L'.')
			if (*string >= L'0' && *string <= L'9')
			{
				value_low += multiplier * (float)(*string++ - L'0');
				multiplier *= 0.1f;
			}
			else
				return EBADMSG;
		local_value += value_low;
	}
	*value = value_is_negative ? -local_value : local_value;
	return 0;
}

int gdt_get_process_arguments(size_t* argument_count, char*** arguments)
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return ENOMEM;
	HMODULE shell32 = LoadLibraryW(L"Shell32.dll");
	if (!shell32)
		return ENOSYS;
	SIZE_T local_argument_count = 0;
	WCHAR** local_argument_values = ((WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(shell32, "CommandLineToArgvW"))(GetCommandLineW(), (int*)&local_argument_count);
	if (!local_argument_values)
	{
		FreeLibrary(shell32);
		return ENOSYS;
	}
	SIZE_T valid_argument_count = 0;
	SIZE_T argument_value_data_size = 0;
	for (SIZE_T i = 0; i != local_argument_count; ++i)
	{
		int wide_string_length = lstrlenW(local_argument_values[i]);
		if (wide_string_length >= 0)
		{
			int string_length = WideCharToMultiByte(CP_UTF8, 0, local_argument_values[i], wide_string_length, 0, 0, 0, 0);
			if (string_length >= 0)
			{
				SIZE_T overflow_check = argument_value_data_size + ((SIZE_T)string_length + 1) * sizeof(char);
				if (overflow_check < argument_value_data_size)
				{
					LocalFree((HLOCAL)local_argument_values);
					FreeLibrary(shell32);
					return ENOMEM;
				}
				argument_value_data_size = overflow_check;
				++valid_argument_count;
			}
		}
	}
	if ((valid_argument_count * sizeof(char*)) / sizeof(char*) != valid_argument_count || valid_argument_count * sizeof(char*) + argument_value_data_size < valid_argument_count * sizeof(char*))
	{
		LocalFree((HLOCAL)local_argument_values);
		FreeLibrary(shell32);
		return ENOMEM;
	}
	char** argument_buffer = (char**)HeapAlloc(heap, 0, valid_argument_count * sizeof(char*) + argument_value_data_size);
	if (!argument_buffer)
	{
		LocalFree((HLOCAL)local_argument_values);
		FreeLibrary(shell32);
		return ENOMEM;
	}
	for (SIZE_T w = valid_argument_count * sizeof(char*), c = 0, i = 0; c != valid_argument_count; ++i)
	{
		int wide_string_length = lstrlenW(local_argument_values[i]);
		if (wide_string_length >= 0)
		{
			int string_length = WideCharToMultiByte(CP_UTF8, 0, local_argument_values[i], wide_string_length, 0, 0, 0, 0);
			if (string_length >= 0)
			{
				char* string = (char*)((UINT_PTR)argument_buffer + w);
				argument_buffer[c] = string;
				if (WideCharToMultiByte(CP_UTF8, 0, local_argument_values[i], wide_string_length, string, string_length, 0, 0) != string_length)
				{
					HeapFree(heap, 0, argument_buffer);
					LocalFree((HLOCAL)local_argument_values);
					FreeLibrary(shell32);
					return EBADMSG;
				}
				string[string_length] = 0;
				w += (SIZE_T)(string_length + 1) * sizeof(char);
				++c;
			}
		}
	}
	LocalFree((HLOCAL)local_argument_values);
	FreeLibrary(shell32);
	*argument_count = valid_argument_count;
	*arguments = argument_buffer;
	return 0;
}

int gdt_get_flag_argument(size_t argument_count, char** arguments, const char* name)
{
	if (argument_count > 1)
		for (size_t i = 1; i != argument_count; ++i)
			if (!lstrcmpA(name, arguments[i]))
				return 0;
	return ENOENT;
}

int gdt_get_string_argument(size_t argument_count, char** arguments, const char* name, char** value)
{
	if (argument_count > 2)
		for (size_t i = 1; i + 1 != argument_count; ++i)
			if (!lstrcmpA(name, arguments[i]))
			{
				*value = arguments[i + 1];
				return 0;
			}
	return ENOENT;
}

void gdt_free(void* memory)
{
	HeapFree(GetProcessHeap(), 0, memory);
}

#else

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

int gdt_get_process_arguments(size_t* argument_count, char*** arguments)
{
	const size_t allocation_granularity = 256 * sizeof(char);
	int error = 0;
	size_t buffer_size = allocation_granularity;
	char* buffer = malloc(buffer_size);
	if (!buffer)
		return ENOMEM;
	size_t file_size = 0;
	int file_descriptor = -1;
	while (file_descriptor == -1)
	{
		file_descriptor = open("/proc/self/cmdline", O_RDONLY);
		if (file_descriptor == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				free(buffer);
				return error;
			}
		}
	}
	for (;;)
	{
		if (file_size == buffer_size)
		{
			buffer_size += allocation_granularity;
			char* buffer_tmp = (char*)realloc(buffer, buffer_size);
			if (!buffer_tmp)
			{
				close(file_descriptor);
				free(buffer);
				return ENOMEM;
			}
		}
		ssize_t read_size = read(file_descriptor, (void*)((uintptr_t)buffer + file_size), ((buffer_size - file_size) < (size_t)0x40000000) ? (buffer_size - file_size) : (size_t)0x40000000);
		if (read_size == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				close(file_descriptor);
				free(buffer);
				return error;
			}
			else
				read_size = 0;
		}
		else if (read_size)
			file_size += (size_t)read_size;
		else
			break;
	}
	close(file_descriptor);
	size_t count = 0;
	for (const char* command_read = (const char*)buffer, *command_end = (const char*)((uintptr_t)buffer + file_size); command_read != command_end; ++command_read)
		if (!*command_read)
			++count;
	if (!count)
	{
		free(buffer);
		return EBADMSG;
	}
	if (buffer_size < (count * sizeof(char*)) + file_size)
	{
		buffer_size = (count * sizeof(char*)) + file_size;
		char* buffer_tmp = (char*)realloc(buffer, buffer_size);
		if (!buffer_tmp)
		{
			free(buffer);
			return ENOMEM;
		}
	}
	char** argument_table = (char**)buffer;
	char* argument_read = (char*)((uintptr_t)buffer + (count * sizeof(char*)));
	memmove(argument_read, buffer, count * sizeof(char*));
	for (size_t i = 0; i != count; ++i)
	{
		argument_table[i] = argument_read;
		while (*argument_read)
			++argument_read;
		++argument_read;
	}
	*argument_count = count;
	*arguments = argument_table;
	return 0;
}

int gdt_get_flag_argument(size_t argument_count, char** arguments, const char* name)
{
	if (argument_count > 1)
		for (size_t i = 1; i != argument_count; ++i)
			if (!strcmp(name, arguments[i]))
				return 0;
	return ENOENT;
}

int gdt_get_string_argument(size_t argument_count, char** arguments, const char* name, char** value)
{
	if (argument_count > 2)
		for (size_t i = 1; i + 1 != argument_count; ++i)
			if (!strcmp(name, arguments[i]))
			{
				*value = arguments[i + 1];
				return 0;
			}
	return ENOENT;
}

void gdt_free(void* memory)
{
	free(memory);
};

#endif

int gdt_get_integer_argument(size_t argument_count, char** arguments, const char* name, int* value_is_negative, uint64_t* value)
{
	char* string;
	int error = gdt_get_string_argument(argument_count, arguments, name, &string);
	if (error)
		return error;
	uint64_t local_value = 0;
	int local_value_is_negative = 0;
	if (*string == '-')
	{
		local_value_is_negative = 1;
		++string;
	}
	else if (*string == '+')
		++string;
	if (!*string)
		return EBADMSG;
	while (*string)
		if (*string >= '0' && *string <= '9')
		{
			uint64_t overflow_check = local_value;
			local_value = (uint64_t)10 * local_value;
			if (local_value / 10 != overflow_check)
				return ERANGE;
			overflow_check = local_value;
			local_value += (uint64_t)(*string++ - '0');
			if (local_value < overflow_check)
				return ERANGE;
		}
		else
			return EBADMSG;
	*value_is_negative = local_value_is_negative;
	*value = local_value;
	return 0;
}

int gdt_get_little_integer_argument(size_t argument_count, char** arguments, const char* name, int* value)
{
	int is_negative;
	uint64_t large_integer;
	int error = gdt_get_integer_argument(argument_count, arguments, name, &is_negative, &large_integer);
	if (error)
		return error;
	if (sizeof(int) <= sizeof(uint64_t) && ((is_negative && large_integer > ((uint64_t)1 << (((uint64_t)8 * (uint64_t)sizeof(int)) - (uint64_t)1))) || (!is_negative && large_integer > (((uint64_t)1 << (((uint64_t)8 * (uint64_t)sizeof(int)) - (uint64_t)1)) - (uint64_t)1))))
		return ERANGE;
	*value = is_negative ? -(int)large_integer : (int)large_integer;
	return 0;
}

int gdt_get_enumeration_argument(size_t argument_count, char** arguments, const char* name, int enumeration_value_count, const char** enumeration_values, int* value)
{
	char* string;
	int error = gdt_get_string_argument(argument_count, arguments, name, &string);
	if (error)
		return error;
	for (int i = 0; i != enumeration_value_count; ++i)
		if (!lstrcmpA(string, enumeration_values[i]))
		{
			*value = i;
			return 0;
		}
	return EBADMSG;
}

int gdt_get_float_argument(size_t argument_count, char** arguments, const char* name, float* value)
{
	char* string;
	int error = gdt_get_string_argument(argument_count, arguments, name, &string);
	if (error)
		return error;
	float local_value = 0.0f;
	int value_is_negative = 0;
	if (*string == '-')
	{
		value_is_negative = 1;
		++string;
	}
	else if (*string == '+')
		++string;
	if (!*string)
		return EBADMSG;
	while (*string && *string != '.')
		if (*string >= '0' && *string <= '9')
			local_value = 10.0f * local_value + (float)(*string++ - '0');
		else
			return EBADMSG;
	if (*string == '.')
	{
		++string;
		float value_low = 0.0f;
		float multiplier = 0.1f;
		while (*string && *string != '.')
			if (*string >= '0' && *string <= '9')
			{
				value_low += multiplier * (float)(*string++ - '0');
				multiplier *= 0.1f;
			}
			else
				return EBADMSG;
		local_value += value_low;
	}
	*value = value_is_negative ? -local_value : local_value;
	return 0;
}
