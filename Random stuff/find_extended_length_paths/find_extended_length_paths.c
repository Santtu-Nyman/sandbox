#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>
#include <errno.h>
#include "ssn_error.h"
#include "ssn_native_command_to_arguments.h"
#include "ssn_file_api.h"
#include "ssn_win32_extended_paths.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

static size_t get_page_size()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (size_t)system_info.dwPageSize;
}

static void* allocator_callback(void* context, size_t size)
{
	return (void*)VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

static void deallocator_callback(void* context, size_t size, void* allocation)
{
	VirtualFree(allocation, 0, MEM_RELEASE);
}

static int get_current_directory(char** current_directory_address)
{
	size_t native_current_directory_length = (size_t)GetCurrentDirectoryW(0, 0);
	if (!native_current_directory_length)
		return EIO;

	size_t page_size = get_page_size();
	WCHAR* native_current_directory = (WCHAR*)VirtualAlloc(0, (((size_t)native_current_directory_length * sizeof(WCHAR)) + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!native_current_directory)
		return ENOMEM;

	if (GetCurrentDirectoryW((DWORD)native_current_directory_length, native_current_directory) != (native_current_directory_length - 1))
	{
		VirtualFree(native_current_directory, 0, MEM_RELEASE);
		return EIO;
	}

	int current_directory_length = WideCharToMultiByte(CP_UTF8, 0, native_current_directory, -1, 0, 0, 0, 0);
	if (!current_directory_length)
	{
		VirtualFree(native_current_directory, 0, MEM_RELEASE);
		return EIO;
	}

	char* current_directory = (char*)VirtualAlloc(0, (((size_t)current_directory_length * sizeof(char)) + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!current_directory)
	{
		VirtualFree(native_current_directory, 0, MEM_RELEASE);
		return ENOMEM;
	}

	if (WideCharToMultiByte(CP_UTF8, 0, native_current_directory, -1, current_directory, current_directory_length, 0, 0) != current_directory_length)
	{
		VirtualFree(current_directory, 0, MEM_RELEASE);
		VirtualFree(native_current_directory, 0, MEM_RELEASE);
		return EINVAL;
	}
	VirtualFree(native_current_directory, 0, MEM_RELEASE);

	*current_directory_address = current_directory;
	return 0;
}

static int get_absolute_path(const char* path, char** absolute_path_address)
{
	// slow but, what ever
	// do not use this in any kind of loop or you are bad person

	int path_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, 0, 0);

	if (path_length < 1)
		return EINVAL;

	if (path_length > 0x8000)
		return ENAMETOOLONG;

	size_t page_size = get_page_size();
	WCHAR* native_path = (WCHAR*)VirtualAlloc(0, (((size_t)path_length * sizeof(WCHAR)) + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!native_path)
		return ENOMEM;

	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, native_path, path_length) != path_length)
	{
		VirtualFree(native_path, 0, MEM_RELEASE);
		return EINVAL;
	}

	int native_absolute_path_length = (int)GetFullPathNameW(native_path, 0, 0, 0);
	if (!native_absolute_path_length)
	{
		VirtualFree(native_path, 0, MEM_RELEASE);
		return EINVAL;
	}

	WCHAR* native_absolute_path = (WCHAR*)VirtualAlloc(0, (((size_t)native_absolute_path_length * sizeof(WCHAR)) + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!native_absolute_path)
	{
		VirtualFree(native_path, 0, MEM_RELEASE);
		return ENOMEM;
	}

	if ((int)GetFullPathNameW(native_path, (DWORD)native_absolute_path_length, native_absolute_path, 0) != (native_absolute_path_length - 1))
	{
		VirtualFree(native_absolute_path, 0, MEM_RELEASE);
		VirtualFree(native_path, 0, MEM_RELEASE);
		return EIO;
	}
	VirtualFree(native_path, 0, MEM_RELEASE);

	int absolute_path_length = WideCharToMultiByte(CP_UTF8, 0, native_absolute_path, -1, 0, 0, 0, 0);
	if (!absolute_path_length)
	{
		VirtualFree(native_absolute_path, 0, MEM_RELEASE);
		return EIO;
	}

	char* absolute_path = (char*)VirtualAlloc(0, (((size_t)absolute_path_length * sizeof(char)) + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!absolute_path)
	{
		VirtualFree(native_absolute_path, 0, MEM_RELEASE);
		return ENOMEM;
	}

	if (WideCharToMultiByte(CP_UTF8, 0, native_absolute_path, -1, absolute_path, absolute_path_length, 0, 0) != absolute_path_length)
	{
		VirtualFree(absolute_path, 0, MEM_RELEASE);
		VirtualFree(native_absolute_path, 0, MEM_RELEASE);
		return EINVAL;
	}
	VirtualFree(native_absolute_path, 0, MEM_RELEASE);

	*absolute_path_address = absolute_path;
	return 0;
}

static int list_extended_length_paths(size_t path_count, const ssn_file_entry_t* path_table, size_t* extanded_path_count_address, char*** extanded_path_table_address)
{
	const size_t maximum_name_length = 0x7FFF;
	size_t page_size = get_page_size();

	WCHAR* wide_name_buffer = (WCHAR*)VirtualAlloc(0, (((maximum_name_length + 1) * sizeof(WCHAR)) + ((maximum_name_length + 1) * sizeof(WCHAR)) + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!wide_name_buffer)
		return ENOMEM;
	WCHAR* unextend_wide_name_buffer = (WCHAR*)((uintptr_t)wide_name_buffer + ((maximum_name_length + 1) * sizeof(WCHAR)));

	size_t extanded_path_count = 0;
	size_t string_data_size = 0;
	for (size_t i = 0; i != path_count; ++i) 
	{
		int wide_length = MultiByteToWideChar(CP_UTF8, 0, path_table[i].name, (int)path_table[i].name_length, wide_name_buffer, (int)maximum_name_length);
		if (wide_length < 1 || wide_length > (int)maximum_name_length)
		{
			VirtualFree(wide_name_buffer, 0, MEM_RELEASE);
			return EINVAL;
		}
		*(wide_name_buffer + wide_length) = 0;

		size_t unextend_wide_size;
		int unextend_error = ssn_win32_unextend_path(wide_name_buffer, &unextend_wide_size, (maximum_name_length + 1) * sizeof(WCHAR), unextend_wide_name_buffer);
		int unextend_wide_length = (int)(unextend_wide_size / sizeof(WCHAR));
		if (unextend_error)
		{
			VirtualFree(wide_name_buffer, 0, MEM_RELEASE);
			return unextend_error;
		}

		if (unextend_wide_length > (MAX_PATH - 1))
		{
			++extanded_path_count;
			string_data_size += path_table[i].name_length + 1;
		}
	}

	size_t buffer_size = (extanded_path_count * sizeof(char*)) + string_data_size;
	if (!buffer_size)
		buffer_size = 1;
	char** extanded_path_table = (char**)VirtualAlloc(0, (buffer_size + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!extanded_path_table)
	{
		VirtualFree(wide_name_buffer, 0, MEM_RELEASE);
		return ENOMEM;
	}

	for (size_t c = 0, o = (extanded_path_count * sizeof(char*)), i = 0; c != extanded_path_count; ++i)
	{
		int wide_length = MultiByteToWideChar(CP_UTF8, 0, path_table[i].name, (int)path_table[i].name_length, wide_name_buffer, (int)maximum_name_length);
		if (wide_length < 1 || wide_length > (int)maximum_name_length)
		{
			VirtualFree(wide_name_buffer, 0, MEM_RELEASE);
			return EINVAL;
		}
		*(wide_name_buffer + wide_length) = 0;

		size_t unextend_wide_size;
		int unextend_error = ssn_win32_unextend_path(wide_name_buffer, &unextend_wide_size, (maximum_name_length + 1) * sizeof(WCHAR), unextend_wide_name_buffer);
		int unextend_wide_length = (int)(unextend_wide_size / sizeof(WCHAR));
		if (unextend_error)
		{
			VirtualFree(wide_name_buffer, 0, MEM_RELEASE);
			return unextend_error;
		}

		if (unextend_wide_length > (MAX_PATH - 1))
		{
			char* buffer_location = (char*)((uintptr_t)extanded_path_table + o);
			o += path_table[i].name_length + 1;
			extanded_path_table[c] = buffer_location;
			memcpy(buffer_location, path_table[i].name, path_table[i].name_length + 1);
			++c;
		}
	}

	VirtualFree(wide_name_buffer, 0, MEM_RELEASE);
	*extanded_path_count_address = extanded_path_count;
	*extanded_path_table_address = extanded_path_table;
	return 0;
}

static int list_extended_length_paths_from_directory(const char* directory_path, size_t* extanded_path_count_address, char*** extanded_path_table_address, size_t* directories_searched_address, size_t* files_searched_address)
{
	char* absolute_directory_path;
	int error = get_absolute_path(directory_path, &absolute_directory_path);
	if (error)
		return error;
	size_t absolute_directory_path_length = strlen(absolute_directory_path);

	size_t path_table_size;
	size_t directory_path_count;
	size_t file_path_count;
	ssn_file_entry_t* path_table;
	error = ssn_allocate_and_list_directory(absolute_directory_path_length, absolute_directory_path, 0, allocator_callback, deallocator_callback, &path_table_size, &path_table, &directory_path_count, &file_path_count);
	VirtualFree(absolute_directory_path, 0, MEM_RELEASE);
	if (error)
		return error;
	size_t path_count = directory_path_count + file_path_count;

	*directories_searched_address = directory_path_count;
	*files_searched_address = file_path_count;

	error = list_extended_length_paths(path_count, path_table, extanded_path_count_address, extanded_path_table_address);
	deallocator_callback(0, path_table_size, path_table);
	return error;
}

static int show_extended_length_paths_from_directory(const char* directory_path)
{
	printf("Searching directory \"%s\" for extended paths.\n", directory_path);

	size_t page_size = get_page_size();
	size_t extend_directory_path_size;
	char* extend_directory_path;
	int error = ssn_win32_extend_path_utf8(directory_path, &extend_directory_path_size, 0, 0);
	if (error == ENOBUFS)
	{
		extend_directory_path = (char*)VirtualAlloc(0, (extend_directory_path_size + (page_size - 1)) & ~(page_size - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!extend_directory_path)
		{
			printf("Error. Memory allocation failed.\n");
			return error;
		}
		error = ssn_win32_extend_path_utf8(directory_path, &extend_directory_path_size, extend_directory_path_size, extend_directory_path);
		if (error)
		{
			VirtualFree(extend_directory_path, 0, MEM_RELEASE);
			printf("Error %i unable to extand directory path.\n", error);
			return error;
		}
		printf("Using path \"%s\" for search.\n", extend_directory_path);
	}
	else
	{
		printf("Error %i unable to extend directory path.\n", error);
		return error;
	}

	printf("Searching...\n");
	size_t extanded_path_count;
	char** extanded_path_table;
	size_t directories_searched;
	size_t files_searched;
	error = list_extended_length_paths_from_directory(extend_directory_path, &extanded_path_count, &extanded_path_table, &directories_searched, &files_searched);
	if (error)
	{
		VirtualFree(extend_directory_path, 0, MEM_RELEASE);
		printf("Error %i unable to search directory.\n", error);
		return error;
	}
	else
		printf("Search successful. %zu directories and %zu files searched.\n", directories_searched, files_searched);

	size_t output_buffer_size = (((0x7FFF + 1) * sizeof(char)) + (page_size - 1)) & ~(page_size - 1);
	char* output_buffer = (char*)VirtualAlloc(0, output_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!output_buffer)
	{
		VirtualFree(extanded_path_table, 0, MEM_RELEASE);
		VirtualFree(extend_directory_path, 0, MEM_RELEASE);
		printf("Error. Memory allocation failed.\n");
		return error;
	}

	if (extanded_path_count)
	{
		printf("%zu extended paths found. Extended paths\n", extanded_path_count);
		for (size_t i = 0; i != extanded_path_count; ++i)
		{
			size_t extanded_path_size;
			error = ssn_win32_unextend_path_utf8(extanded_path_table[i], &extanded_path_size, output_buffer_size, output_buffer);
			if (!error)
				printf("\"%s\"\n", output_buffer);
			else
				printf("\"%s\"\n", extanded_path_table[i]);
		}
	}
	else
		printf("No extended paths found.\n");

	VirtualFree(output_buffer, 0, MEM_RELEASE);
	VirtualFree(extanded_path_table, 0, MEM_RELEASE);
	VirtualFree(extend_directory_path, 0, MEM_RELEASE);
	return 0;
}

void main()
{
	size_t argument_count;
	char** argument_table;
	int error = ssn_native_command_to_arguments(GetCommandLineW(), &argument_count, &argument_table);
	if (error)
		ExitProcess(EXIT_FAILURE);

	if (argument_count < 2)
	{
		printf("No search directory specified.\n");
		ExitProcess(EXIT_FAILURE);
	}

	char* search_directory = argument_table[1];
	error = show_extended_length_paths_from_directory(search_directory);

	VirtualFree(argument_table, 0, MEM_RELEASE);
	ExitProcess(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

#ifdef __cplusplus
}
#endif // __cplusplus
