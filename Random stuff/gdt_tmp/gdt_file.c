/*
	Graph Drawing Tool version 1.4.0 2019-09-16 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#include "gdt_file.h"
#include <errno.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static WCHAR* gdt_utf8_to_wide_string(HANDLE heap, const char* utf8_string)
{
	int string_length = lstrlenA(utf8_string);
	if (string_length < 0)
		return 0;
	if (!string_length)
	{
		WCHAR* wide_terminator = (WCHAR*)HeapAlloc(heap, 0, sizeof(WCHAR));
		if (wide_terminator)
			*wide_terminator = 0;
		return wide_terminator;
	}
	int wide_string_length = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8_string, string_length, 0, 0);
	if (wide_string_length < 1 ||
		(sizeof(int) > sizeof(SIZE_T) && wide_string_length > SIZE_MAX) ||
		(SIZE_T)wide_string_length > ((SIZE_T)wide_string_length + (SIZE_T)1) ||
		((SIZE_T)wide_string_length + (SIZE_T)1) != ((((SIZE_T)wide_string_length + (SIZE_T)1) * sizeof(WCHAR)) / sizeof(WCHAR)))
		return 0;
	WCHAR* wide_string = (WCHAR*)HeapAlloc(heap, 0, ((SIZE_T)wide_string_length + (SIZE_T)1) * sizeof(WCHAR));
	if (!wide_string)
		return 0;
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8_string, string_length, wide_string, wide_string_length) != wide_string_length)
	{
		HeapFree(heap, 0, wide_string);
		return 0;
	}
	wide_string[wide_string_length] = 0;
	return wide_string;
}

static uint64_t gdt_get_unix_time_stamp()
{
	uint64_t file_time;
	GetSystemTimeAsFileTime((FILETIME*)&file_time);
	return (file_time - (uint64_t)116444736000000000) / (uint64_t)10000000;
}

int gdt_load_file(const char* name, size_t* size, void** data)
{
	WCHAR max_path_wide_name[MAX_PATH + 1];
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)name, -1, max_path_wide_name, MAX_PATH + 1) > 0)
		return gdt_load_file_win32(max_path_wide_name, size, data);
	else
	{
		HANDLE heap = GetProcessHeap();
		if (!heap)
			return ENOMEM;
		WCHAR* allocated_wide_name = gdt_utf8_to_wide_string(heap, name);
		if (!allocated_wide_name)
			return ENOMEM;
		int error = gdt_load_file_win32(allocated_wide_name, size, data);
		HeapFree(heap, 0, allocated_wide_name);
		return error;
	}
}

void gdt_free_file_data(void* data, size_t size)
{
	if (size)
		HeapFree(GetProcessHeap(), 0, data);
}

int gdt_store_file(const char* name, size_t size, const void* data)
{
	WCHAR max_path_wide_name[MAX_PATH + 1];
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)name, -1, max_path_wide_name, MAX_PATH + 1) > 0)
		return gdt_store_file_win32(max_path_wide_name, size, data);
	else
	{
		HANDLE heap = GetProcessHeap();
		if (!heap)
			return ENOMEM;
		WCHAR* allocated_wide_name = gdt_utf8_to_wide_string(heap, name);
		if (!allocated_wide_name)
			return ENOMEM;
		int error = gdt_store_file_win32(allocated_wide_name, size, data);
		HeapFree(heap, 0, allocated_wide_name);
		return error;
	}
}

int gdt_read_map_file(const char* name, size_t* size, void** mapping)
{
	WCHAR max_path_wide_name[MAX_PATH + 1];
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)name, -1, max_path_wide_name, MAX_PATH + 1) > 0)
		return gdt_read_map_file_win32(max_path_wide_name, size, mapping);
	else
	{
		HANDLE heap = GetProcessHeap();
		if (!heap)
			return ENOMEM;
		WCHAR* allocated_wide_name = gdt_utf8_to_wide_string(heap, name);
		if (!allocated_wide_name)
			return ENOMEM;
		int error = gdt_read_map_file_win32(allocated_wide_name, size, mapping);
		HeapFree(heap, 0, allocated_wide_name);
		return error;
	}
}

void gdt_unmap_file(void* mapping, size_t size)
{
	if (size)
		UnmapViewOfFile(mapping);
}

int gdt_delete_file(const char* name)
{
	WCHAR max_path_wide_name[MAX_PATH + 1];
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)name, -1, max_path_wide_name, MAX_PATH + 1) > 0)
		return gdt_delete_file_win32(max_path_wide_name);
	else
	{
		HANDLE heap = GetProcessHeap();
		if (!heap)
			return ENOMEM;
		WCHAR* allocated_wide_name = gdt_utf8_to_wide_string(heap, name);
		if (!allocated_wide_name)
			return ENOMEM;
		int error = gdt_delete_file_win32(allocated_wide_name);
		HeapFree(heap, 0, allocated_wide_name);
		return error;
	}
}

int gdt_move_file(const char* current_name, const char* new_name)
{
	WCHAR max_path_wide_current_name[MAX_PATH + 1];
	WCHAR max_path_wide_new_name[MAX_PATH + 1];
	if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)current_name, -1, max_path_wide_current_name, MAX_PATH + 1) > 0 &&
		MultiByteToWideChar(CP_UTF8, 0, (LPCCH)new_name, -1, max_path_wide_new_name, MAX_PATH + 1) > 0)
		return gdt_move_file_win32(max_path_wide_current_name, max_path_wide_new_name);
	else
	{
		HANDLE heap = GetProcessHeap();
		if (!heap)
			return ENOMEM;
		WCHAR* allocated_wide_current_name = gdt_utf8_to_wide_string(heap, current_name);
		if (!allocated_wide_current_name)
			return ENOMEM;
		WCHAR* allocated_wide_new_name = gdt_utf8_to_wide_string(heap, new_name);
		if (!allocated_wide_new_name)
		{
			HeapFree(heap, 0, allocated_wide_current_name);
			return ENOMEM;
		}
		int error = gdt_move_file_win32(allocated_wide_current_name, allocated_wide_new_name);
		HeapFree(heap, 0, allocated_wide_new_name);
		HeapFree(heap, 0, allocated_wide_current_name);
		return error;
	}
}

int gdt_load_file_win32(const WCHAR* name, size_t* size, void** data)
{
	DWORD native_error;
	HANDLE file_handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		native_error = GetLastError();
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_FILE_NOT_FOUND :
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_INVALID_NAME :
				return EINVAL;
			default:
				return EIO;
		}
	}
	SIZE_T file_size;
#ifdef _WIN64
	if (!GetFileSizeEx(file_handle, (LARGE_INTEGER*)&file_size))
	{
		native_error = GetLastError();
		CloseHandle(file_handle);
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED :
				return EACCES;
			case ERROR_INVALID_FUNCTION :
				return EINVAL;
			default:
				return EIO;
		}
	}
#else
	ULARGE_INTEGER raw_file_size;
	if (!GetFileSizeEx(file_handle, (LARGE_INTEGER*)&raw_file_size))
	{
		native_error = GetLastError();
		CloseHandle(file_handle);
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_FUNCTION:
				return EINVAL;
			default:
				return EIO;
		}
	}
	if (raw_file_size.HighPart)
	{
		CloseHandle(file_handle);
		return EFBIG;
	}
	file_size = (SIZE_T)raw_file_size.LowPart;
#endif
	if (!file_size)
	{
		CloseHandle(file_handle);
		*size = 0;
		*data = (void*)~0;
		return 0;
	}
	HANDLE heap = GetProcessHeap();
	if (!heap)
	{
		CloseHandle(file_handle);
		return ENOMEM;
	}
	UINT_PTR file_data = (UINT_PTR)HeapAlloc(heap, 0, file_size);
	if (!file_data)
	{
		CloseHandle(file_handle);
		return ENOMEM;
	}
	for (SIZE_T file_read = 0; file_read != file_size;)
	{
		DWORD file_read_result;
		if (ReadFile(file_handle, (LPVOID)(file_data + file_read), (((file_size - file_read) < (SIZE_T)0x80000000) ? (DWORD)(file_size - file_read) : (DWORD)0x80000000), (DWORD*)&file_read_result, 0))
			file_read += (SIZE_T)file_read_result;
		else
		{
			HeapFree(heap, 0, (LPVOID)file_data);
			CloseHandle(file_handle);
			return EIO;
		}
	}
	CloseHandle(file_handle);
	*size = file_size;
	*data = (void*)file_data;
	return 0;
}

int gdt_store_file_win32(const WCHAR* name, size_t size, const void* data)
{
	static const WCHAR digit_table[36] = {
		L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F',
		L'G', L'H', L'I', L'J', L'K', L'L', L'M', L'N', L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V',
		L'W', L'X', L'Y', L'Z' };
	const DWORD base_to_6 = 2176782336;
	DWORD native_error;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return ENOMEM;
	SIZE_T directory_name_size = (SIZE_T)lstrlenW(name) * sizeof(WCHAR);
	while (directory_name_size && *(WCHAR*)((UINT_PTR)name + directory_name_size - sizeof(WCHAR)) != L'\\' && *(WCHAR*)((UINT_PTR)name + directory_name_size - sizeof(WCHAR)) != L'/')
		directory_name_size -= sizeof(WCHAR);
	SIZE_T temporal_file_name_sizes = directory_name_size + (22 * sizeof(WCHAR));
	if (temporal_file_name_sizes < directory_name_size)
		return ENOMEM;
	if ((2 * temporal_file_name_sizes) / 2 != temporal_file_name_sizes)
		return ENOMEM;
	WCHAR* temporal_new_file_name = (WCHAR*)HeapAlloc(heap, 0, 2 * temporal_file_name_sizes);
	WCHAR* temporal_old_file_name = (WCHAR*)((UINT_PTR)temporal_new_file_name + temporal_file_name_sizes);
	if (!temporal_new_file_name)
		return ENOMEM;
	DWORD temporal_file_index = 0;
	WCHAR* write_name = temporal_new_file_name;
	for (WCHAR* copy_source = (WCHAR*)name, * copy_source_end = (WCHAR*)((UINT_PTR)copy_source + directory_name_size); copy_source != copy_source_end; ++copy_source, ++write_name)
		*write_name = *copy_source;
	*write_name++ = L'~';
	for (DWORD time = (DWORD)gdt_get_unix_time_stamp(), digit_shift = (ULONGLONG)base_to_6; digit_shift; digit_shift /= 36)
		*write_name++ = digit_table[(time / digit_shift) % 36];
	for (DWORD thread_identity = GetCurrentThreadId(), digit_shift = base_to_6; digit_shift; digit_shift /= 36)
		*write_name++ = digit_table[(thread_identity / digit_shift) % 36];
	write_name += 2;
	for (WCHAR* copy_source = (WCHAR*)L".TMP", *copy_source_end = copy_source + 5; copy_source != copy_source_end; ++copy_source, ++write_name)
		*write_name = *copy_source;
	write_name = (WCHAR*)((UINT_PTR)temporal_new_file_name + directory_name_size + (15 * sizeof(WCHAR)));
	HANDLE file_handle = INVALID_HANDLE_VALUE;
	while (file_handle == INVALID_HANDLE_VALUE)
	{
		write_name[0] = digit_table[temporal_file_index / 36];
		write_name[1] = digit_table[temporal_file_index % 36];
		file_handle = CreateFileW(temporal_new_file_name, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
		if (file_handle == INVALID_HANDLE_VALUE)
		{
			native_error = GetLastError();
			if (native_error != ERROR_FILE_EXISTS)
			{
				HeapFree(heap, 0, temporal_new_file_name);
				switch (native_error)
				{
					case ERROR_ACCESS_DENIED:
						return EACCES;
					case ERROR_PATH_NOT_FOUND:
						return ENOENT;
					case ERROR_INVALID_NAME:
						return EINVAL;
					default:
						return EIO;
				}
			}
			if (temporal_file_index != 1295)
				++temporal_file_index;
			else
			{
				HeapFree(heap, 0, temporal_new_file_name);
				return EBUSY;
			}
		}
	}
	for (SIZE_T file_written = 0; file_written != size;)
	{
		DWORD file_write_result;
		if (WriteFile(file_handle, (LPVOID)((UINT_PTR)data + file_written), (((size - file_written) < (SIZE_T)0x80000000) ? (DWORD)(size - file_written) : (DWORD)0x80000000), (DWORD*)&file_write_result, 0))
			file_written += (SIZE_T)file_write_result;
		else
		{
			CloseHandle(file_handle);
			DeleteFileW(temporal_new_file_name);
			HeapFree(heap, 0, temporal_new_file_name);
			return EIO;
		}
	}
	if (!FlushFileBuffers(file_handle))
	{
		CloseHandle(file_handle);
		DeleteFileW(temporal_new_file_name);
		HeapFree(heap, 0, temporal_new_file_name);
		return EIO;
	}
	CloseHandle(file_handle);
	if (!MoveFileW(temporal_new_file_name, name))
	{
		native_error = GetLastError();
		if (native_error != ERROR_ALREADY_EXISTS)
		{
			DeleteFileW(temporal_new_file_name);
			HeapFree(heap, 0, temporal_new_file_name);
			switch (native_error)
			{
				case ERROR_ACCESS_DENIED:
					return EACCES;
				case ERROR_PATH_NOT_FOUND:
					return ENOENT;
				case ERROR_INVALID_NAME:
					return EINVAL;
				default:
					return EIO;
			}
		}
		if (temporal_file_index == 1295)
		{
			DeleteFileW(temporal_new_file_name);
			HeapFree(heap, 0, temporal_new_file_name);
			return EBUSY;
		}
		++temporal_file_index;
		for (WCHAR* copy_source = (WCHAR*)temporal_new_file_name, *copy_source_end = (WCHAR*)((UINT_PTR)copy_source + directory_name_size + (22 * sizeof(WCHAR))), *copy_destination = temporal_old_file_name; copy_source != copy_source_end; ++copy_source, ++copy_destination)
			*copy_destination = *copy_source;
		write_name = (WCHAR*)((UINT_PTR)temporal_old_file_name + directory_name_size + (15 * sizeof(WCHAR)));
		for (BOOL move_old_file = TRUE; move_old_file;)
		{
			write_name[0] = digit_table[temporal_file_index / 36];
			write_name[1] = digit_table[temporal_file_index % 36];
			if (MoveFileW(name, temporal_old_file_name))
				move_old_file = FALSE;
			else
			{
				native_error = GetLastError();
				if (native_error != ERROR_ALREADY_EXISTS)
				{
					DeleteFileW(temporal_new_file_name);
					HeapFree(heap, 0, temporal_new_file_name);
					switch (native_error)
					{
						case ERROR_ACCESS_DENIED:
							return EACCES;
						case ERROR_PATH_NOT_FOUND:
							return ENOENT;
						case ERROR_INVALID_NAME:
							return EINVAL;
						default:
							return EIO;
					}
				}
				if (temporal_file_index != 1295)
					++temporal_file_index;
				else
				{
					DeleteFileW(temporal_new_file_name);
					HeapFree(heap, 0, temporal_new_file_name);
					return EBUSY;
				}
			}
		}
		if (!MoveFileW(temporal_new_file_name, name))
		{
			native_error = GetLastError();
			MoveFileW(temporal_old_file_name, name);
			DeleteFileW(temporal_new_file_name);
			HeapFree(heap, 0, temporal_new_file_name);
			switch (native_error)
			{
				case ERROR_ACCESS_DENIED:
					return EACCES;
				case ERROR_PATH_NOT_FOUND:
					return ENOENT;
				case ERROR_INVALID_NAME:
					return EINVAL;
				default:
					return EIO;
			}
		}
		DeleteFileW(temporal_old_file_name);
	}
	HeapFree(heap, 0, temporal_new_file_name);
	return 0;
}

int gdt_read_map_file_win32(const WCHAR* name, size_t* size, void** mapping)
{
	DWORD native_error;
	HANDLE file_handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		native_error = GetLastError();
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_INVALID_NAME:
				return EINVAL;
			default:
				return EIO;
		}
	}
	SIZE_T file_size;
#ifdef _WIN64
	if (!GetFileSizeEx(file_handle, (LARGE_INTEGER*)&file_size))
	{
		native_error = GetLastError();
		CloseHandle(file_handle);
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_FUNCTION:
				return EINVAL;
			default:
				return EIO;
		}
	}
#else
	ULARGE_INTEGER raw_file_size;
	if (!GetFileSizeEx(file_handle, (LARGE_INTEGER*)&raw_file_size))
	{
		native_error = GetLastError();
		CloseHandle(file_handle);
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_FUNCTION:
				return EINVAL;
			default:
				return EIO;
		}
	}
	if (raw_file_size.HighPart)
	{
		CloseHandle(file_handle);
		return EFBIG;
	}
	file_size = (SIZE_T)raw_file_size.LowPart;
#endif
	if (!file_size)
	{
		CloseHandle(file_handle);
		*size = 0;
		*mapping = (void*)~0;
		return 0;
	}
	HANDLE file_mapping_handle = CreateFileMappingW(file_handle, 0, PAGE_READONLY, 0, 0, 0);
	if (!file_mapping_handle)
	{
		native_error = GetLastError();
		CloseHandle(file_handle);
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			default:
				return EIO;
		}
	}
	CloseHandle(file_handle);
	void* file_mapping = MapViewOfFile(file_mapping_handle, FILE_MAP_READ, 0, 0, file_size);
	if (!file_mapping)
	{
		native_error = GetLastError();
		CloseHandle(file_mapping_handle);
		switch (native_error)
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			default:
				return EIO;
		}
	}
	CloseHandle(file_mapping_handle);
	*size = file_size;
	*mapping = file_mapping;
	return 0;
}

int gdt_delete_file_win32(const WCHAR* name)
{
	if (DeleteFileW(name))
		return 0;
	DWORD native_error = GetLastError();
	switch (native_error)
	{
		case ERROR_ACCESS_DENIED:
			return EACCES;
		case ERROR_FILE_NOT_FOUND:
			return ENOENT;
		case ERROR_PATH_NOT_FOUND:
			return ENOENT;
		case ERROR_INVALID_NAME:
			return EINVAL;
		default:
			return EIO;
	}
}

int gdt_move_file_win32(const WCHAR* current_name, const WCHAR* new_name)
{
	if (MoveFileW(current_name, new_name))
		return 0;
	DWORD native_error = GetLastError();
	switch (native_error)
	{
		case ERROR_ACCESS_DENIED:
			return EACCES;
		case ERROR_FILE_NOT_FOUND:
			return ENOENT;
		case ERROR_PATH_NOT_FOUND:
			return ENOENT;
		case ERROR_INVALID_NAME:
			return EINVAL;
		case ERROR_ALREADY_EXISTS:
			return EEXIST;
		default:
			return EIO;
	}
}

#else

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
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
#include <stdio.h>

static uint64_t gdt_get_unix_time_stamp()
{
	struct timespec time;
	if (!clock_gettime(CLOCK_REALTIME, &time))
		return (uint64_t)time.tv_sec;
	else
		return 0;
}

int gdt_load_file(const char* name, size_t* size, void** data)
{
	struct stat stats;
	int error;
	int file_descriptor = -1;
	while (file_descriptor == -1)
	{
		file_descriptor = open(name, O_RDONLY);
		if (file_descriptor == -1)
		{
			error = errno;
			if (error != EINTR)
				return error;
		}
	}
	if (fstat(file_descriptor, &stats) == -1)
	{
		error = errno;
		close(file_descriptor);
		return error;
	}
	if (sizeof(off_t) > sizeof(size_t) && stats.st_size > (off_t)SIZE_MAX)
	{
		error = EFBIG;
		close(file_descriptor);
		return error;
	}
	size_t file_size = (size_t)stats.st_size;
	if (!file_size)
	{
		close(file_descriptor);
		*size = 0;
		*data = (void*)~0;
	}
	uintptr_t buffer = (uintptr_t)malloc(file_size);
	if (!buffer)
	{
		close(file_descriptor);
		return ENOMEM;
	}
	for (size_t loaded = 0; loaded != file_size;)
	{
		ssize_t read_result = read(file_descriptor, (void*)(buffer + loaded), ((file_size - loaded) < (size_t)0x40000000) ? (file_size - loaded) : (size_t)0x40000000);
		if (read_result == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				free((void*)buffer);
				close(file_descriptor);
				return error;
			}
			read_result = 0;
		}
		loaded += (size_t)read_result;
	}
	close(file_descriptor);
	*size = file_size;
	*data = (void*)buffer;
	return 0;
}

void gdt_free_file_data(void* data, size_t size)
{
	if (size)
		free(data);
}

int gdt_store_file(const char* name, size_t size, const void* data)
{
	const int large_pid = (sizeof(pid_t) <= sizeof(uint64_t)) ? ((sizeof(pid_t) <= sizeof(uint32_t)) ? 0 : 1) : -1;
	if (large_pid == -1)
		return ENOSYS;
	const uint64_t base_to_12 = 4738381338321616896;
	const uint32_t base_to_6 = 2176782336;
	static const char digit_table[36] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
		'W', 'X', 'Y', 'Z' };
	int error;
	size_t directory_name_size = strlen(name) * sizeof(char);
	while (directory_name_size && *(char*)((uintptr_t)name + directory_name_size - sizeof(char)) != L'/')
		directory_name_size -= sizeof(char);
	size_t temporal_file_name_sizes = directory_name_size + ((large_pid ? 28 : 22) * sizeof(char));
	if (temporal_file_name_sizes < directory_name_size)
		return ENOMEM;
	char* temporal_file_name = (char*)malloc(temporal_file_name_sizes);
	if (!temporal_file_name)
		return ENOMEM;
	uint32_t temporal_file_index = 0;
	char* write_name = temporal_file_name;
	for (char* copy_source = (char*)name, *copy_source_end = (char*)((uintptr_t)copy_source + directory_name_size); copy_source != copy_source_end; ++copy_source, ++write_name)
		*write_name = *copy_source;
	*write_name++ = '~';
	for (uint32_t time = (uint32_t)gdt_get_unix_time_stamp(), digit_shift = base_to_6; digit_shift; digit_shift /= 36)
		*write_name++ = digit_table[(time / digit_shift) % 36];
	pid_t thread_identity = (pid_t)syscall(SYS_gettid);
	if (large_pid)
		for (uint64_t digit_shift = base_to_12; digit_shift; digit_shift /= 36)
			*write_name++ = digit_table[((uint64_t)thread_identity / digit_shift) % 36];
	else
		for (uint32_t digit_shift = base_to_6; digit_shift; digit_shift /= 36)
			*write_name++ = digit_table[((uint32_t)thread_identity / digit_shift) % 36];
	write_name += 2;
	for (char* copy_source = (char*)".TMP", *copy_source_end = copy_source + 5; copy_source != copy_source_end; ++copy_source, ++write_name)
		*write_name = *copy_source;
	write_name = (char*)((uintptr_t)temporal_file_name + directory_name_size + ((large_pid ? 21 : 15) * sizeof(char)));
	int file_descriptor = -1;
	while (file_descriptor == -1)
	{
		write_name[0] = digit_table[temporal_file_index / 36];
		write_name[1] = digit_table[temporal_file_index % 36];
		file_descriptor = open(temporal_file_name, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		if (file_descriptor == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				if (error != EEXIST)
				{
					free(temporal_file_name);
					return error;
				}
				if (temporal_file_index != 1295)
					++temporal_file_index;
				else
				{
					free(temporal_file_name);
					return EBUSY;
				}
			}
		}
	}
	for (size_t written = 0; written != size;)
	{
		ssize_t write_result = write(file_descriptor, (const void*)((uintptr_t)data + written), ((size - written) < (size_t)0x40000000) ? (size - written) : (size_t)0x40000000);
		if (write_result == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				unlink(temporal_file_name);
				close(file_descriptor);
				free(temporal_file_name);
				return error;
			}
			write_result = 0;
		}
		written += (size_t)write_result;
	}
	error = EINTR;
	while (error == EINTR)
		if (fsync(file_descriptor) == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				unlink(temporal_file_name);
				close(file_descriptor);
				free(temporal_file_name);
				return error;
			}
		}
		else
			error = 0;
	close(file_descriptor);
	if (rename(temporal_file_name, name) == -1)
	{
		error = errno;
		unlink(temporal_file_name);
		free(temporal_file_name);
		return error;
	}
	free(temporal_file_name);
	return 0;
}

int gdt_read_map_file(const char* name, size_t* size, void** mapping)
{
	struct stat file_info;
	int error;
	int file_descriptor = -1;
	while (file_descriptor == -1)
	{
		file_descriptor = open(name, O_RDONLY);
		if (file_descriptor == -1)
		{
			error = errno;
			if (error != EINTR)
				return error;
		}
	}
	if (fstat(file_descriptor, &file_info))
	{
		error = errno;
		close(file_descriptor);
		return error;
	}
	if ((sizeof(off_t) > sizeof(size_t)) && (file_info.st_size > (off_t)SIZE_MAX))
	{
		close(file_descriptor);
		return EFBIG;
	}
	size_t file_size = (size_t)file_info.st_size;
	if (!file_size)
	{
		close(file_descriptor);
		*size = 0;
		*mapping = (void*)~0;
	}
	void* file_data = mmap(0, file_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
	if (file_data == MAP_FAILED)
	{
		error = errno;
		close(file_descriptor);
		return error;
	}
	close(file_descriptor);
	*size = file_size;
	*mapping = file_data;
	return 0;
}

void gdt_unmap_file(void* mapping, size_t size)
{
	if (size)
		munmap(mapping, size);
}

int gdt_delete_file(const char* name)
{
	return !unlink(name) ? 0 : errno;
}

int gdt_move_file(const char* current_name, const char* new_name)
{
	return !rename(current_name, new_name) ? 0 : errno;
}

#endif

