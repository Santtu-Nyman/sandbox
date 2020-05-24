#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ssn_file_api.h"

// Is malloc.h needed for _alloca? since it is not really normal function
// #include <malloc.h>

typedef struct ssn_overlapped_data_t
{
	ULONG_PTR overlapped_internal_low;
	ULONG_PTR overlapped_internal_high;
	ULONGLONG overlapped_offset;
	HANDLE overlapped_event;
	DWORD error;
	DWORD bytes_transfered;
} ssn_overlapped_data_t;

#ifdef _DEBUG
#define SSN_FILE_API_ASSERT(x) do { if (!(x)) DebugBreak(); } while(0)
#else
#define SSN_FILE_API_ASSERT(x) __assume(x)
#endif

static size_t ssn_internal_root_directory_length(size_t path_langth, const char* path)
{
	if (path_langth > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\')
	{
		if (path_langth > 6 && path[4] != '\\' && path[4] != '/' && path[5] == ':' && path[6] == '\\')
			return 7;
		else if ((path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			size_t server_legth = 0;
			while (8 + server_legth < path_langth && path[8 + server_legth] != '\\' && path[8 + server_legth] != '/')
				++server_legth;
			if (!server_legth)
				return 0;
			size_t share_legth = 0;
			while (9 + server_legth + share_legth < path_langth && path[9 + server_legth + share_legth] != '\\' && path[9 + server_legth + share_legth] != '/')
				++server_legth;
			if (!share_legth || path[9 + server_legth + share_legth] != '\\' && path[9 + server_legth + share_legth] != '/')
				return 0;
			return 10 + server_legth + share_legth;
		}
		else
			return 0;
	}
	else
	{
		if (path_langth > 2 && path[0] != '\\' && path[0] != '/' && path[1] == ':' && path[2] == '\\')
			return 3;
		else if (path_langth > 2 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
		{
			size_t server_legth = 0;
			while (2 + server_legth < path_langth && path[2 + server_legth] != '\\' && path[2 + server_legth] != '/')
				++server_legth;
			if (!server_legth)
				return 0;
			size_t share_legth = 0;
			while (3 + server_legth + share_legth < path_langth && path[3 + server_legth + share_legth] != '\\' && path[3 + server_legth + share_legth] != '/')
				++server_legth;
			if (!share_legth || path[3 + server_legth + share_legth] != '\\' && path[3 + server_legth + share_legth] != '/')
				return 0;
			return 4 + server_legth + share_legth;
		}
		else
			return 0;
	}
}

static size_t ssn_internal_win32_root_directory_length(size_t path_langth, const WCHAR* path)
{
	if (path_langth > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		if (path_langth > 6 && path[4] != L'\\' && path[4] != L'/' && path[5] == L':' && path[6] == L'\\')
			return 7;
		else if ((path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			size_t server_legth = 0;
			while (8 + server_legth < path_langth && path[8 + server_legth] != L'\\' && path[8 + server_legth] != L'/')
				++server_legth;
			if (!server_legth)
				return 0;
			size_t share_legth = 0;
			while (9 + server_legth + share_legth < path_langth && path[9 + server_legth + share_legth] != L'\\' && path[9 + server_legth + share_legth] != L'/')
				++server_legth;
			if (!share_legth || path[9 + server_legth + share_legth] != L'\\' && path[9 + server_legth + share_legth] != L'/')
				return 0;
			return 10 + server_legth + share_legth;
		}
		else
			return 0;
	}
	else
	{
		if (path_langth > 2 && path[0] != L'\\' && path[0] != L'/' && path[1] == L':' && path[2] == L'\\')
			return 3;
		else if (path_langth > 2 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
		{
			size_t server_legth = 0;
			while (2 + server_legth < path_langth && path[2 + server_legth] != L'\\' && path[2 + server_legth] != L'/')
				++server_legth;
			if (!server_legth)
				return 0;
			size_t share_legth = 0;
			while (3 + server_legth + share_legth < path_langth && path[3 + server_legth + share_legth] != L'\\' && path[3 + server_legth + share_legth] != L'/')
				++server_legth;
			if (!share_legth || path[3 + server_legth + share_legth] != L'\\' && path[3 + server_legth + share_legth] != L'/')
				return 0;
			return 4 + server_legth + share_legth;
		}
		else
			return 0;
	}
}

static size_t ssn_internal_get_directory_part_length_from_file_name(size_t name_length, const char* name)
{
	if (name_length)
	{
		size_t index = name_length - 1;
		for (size_t root_length = ssn_internal_root_directory_length(name_length, name); index != root_length; --index)
			if (name[index] == '\\' || name[index] == '/')
				return index;
	}
	return 0;
}

static size_t ssn_internal_win32_get_directory_part_length_from_file_name(size_t name_length, const WCHAR* name)
{
	if (name_length)
	{
		size_t index = name_length - 1;
		for (size_t root_length = ssn_internal_win32_root_directory_length(name_length, name); index != root_length; --index)
			if (name[index] == L'\\' || name[index] == L'/')
				return index;
	}
	return 0;
}

static size_t ssn_internal_win32_string_length(const WCHAR* string)
{
	const WCHAR* read = string;
	while (*read)
		++read;
	return (size_t)((uintptr_t)read - (uintptr_t)string) / sizeof(WCHAR);
}

static void ssn_internal_win32_copy_string(size_t length, WCHAR* destination, const WCHAR* source)
{
	const WCHAR* source_end = source + length;
	while (source != source_end)
		*destination++ = *source++;
}

static size_t ssn_internal_utf8_string_length(const char* string)
{
	const char* read = string;
	while (*read)
		++read;
	return (size_t)((uintptr_t)read - (uintptr_t)string);
}

static void ssn_internal_copy_utf8_string(size_t length, char* destination, const char* source)
{
	const char* source_end = source + length;
	while (source != source_end)
		*destination++ = *source++;
}

#define SSN_INTERNAL_MAKE_TMP_FILE_NAME_TRY_LIMIT 0x800
static void ssn_internal_win32_make_tmp_name(WCHAR* buffer, int try_count)
{
	SSN_FILE_API_ASSERT(try_count < SSN_INTERNAL_MAKE_TMP_FILE_NAME_TRY_LIMIT);

	uint32_t thread_id = (uint32_t)GetCurrentThreadId();
	uint32_t timer = (uint32_t)GetTickCount() / 1000;

	// only 43 low bits are used
	uint64_t tmp_number = ((uint64_t)((thread_id & 0xFFFF) ^ ((thread_id >> 16) & 0xFFFF)) << 27) | ((uint64_t)(timer & 0xFFFF) << 11) | (uint64_t)(try_count & 0x7FF);

	static const WCHAR digit_table[] = {
		L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8',
		L'9', L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H',
		L'I', L'J', L'K', L'L', L'M', L'N', L'O', L'P', L'Q',
		L'R', L'S', L'T', L'U', L'V', L'W', L'X', L'Y', L'Z',
		L'!', L'#', L'$', L'%', L'&', L'_' };
	const uint64_t digit_count = sizeof(digit_table) / sizeof(*digit_table);

	for (int i = 0; i != 8; ++i)
	{
		buffer[i] = digit_table[tmp_number % digit_count];
		tmp_number /= digit_count;
	}

	buffer[8] = 0;
}

static void ssn_internal_win32_make_tmp_8dot3_file_name(WCHAR* buffer, int try_count)
{
	ssn_internal_win32_make_tmp_name(buffer, try_count);

	buffer[8] = L'.';
	buffer[9] = L'T';
	buffer[10] = L'M';
	buffer[11] = L'P';
	buffer[12] = 0;
}

static int ssn_internal_win32_undo_create_directory(size_t name_length, WCHAR* name, size_t undo_index)
{
	int error = 0;
	for (size_t iterator = name_length; iterator != (undo_index - 1); --iterator)
	{
		WCHAR name_character = name[iterator];
		if (iterator == name_length || name_character == L'\\' || name_character == L'/')
		{
			name[iterator] = 0;
			BOOL remove_successful = RemoveDirectoryW(name);
			name[iterator] = name_character;
			if (!error && !remove_successful)
			{
				DWORD native_error = GetLastError();
				switch (native_error)
				{
					case ERROR_FILE_NOT_FOUND:
						error = ENOENT;
						break;
					case ERROR_PATH_NOT_FOUND:
						error = ENOENT;
						break;
					case ERROR_ACCESS_DENIED:
						error = EACCES;
						break;
					case ERROR_INVALID_NAME:
						error = ENOENT;
						break;
					default:
						error = EIO;
						break;
				}
			}
		}
	}
	return error;
}

static int ssn_internal_win32_create_directory(size_t name_length, WCHAR* name, size_t* undo_index)
{
	for (size_t root_length = ssn_internal_win32_root_directory_length(name_length, name),
		high_iterator = name_length; high_iterator != (root_length - 1); --high_iterator)
	{
		WCHAR name_character = name[high_iterator];
		if (high_iterator == name_length || name_character == L'\\' || name_character == L'/')
		{
			name[high_iterator] = 0;
			DWORD native_error = CreateDirectoryW(name, 0) ? 0 : GetLastError();
			name[high_iterator] = name_character;

			if (!native_error)
			{
				for (size_t last_create_index = high_iterator, low_iterator = high_iterator + 1; low_iterator != (name_length + 1); ++low_iterator)
				{
					name_character = name[low_iterator];
					if (low_iterator == name_length || name_character == L'\\' || name_character == L'/')
					{
						name[low_iterator] = 0;
						native_error = CreateDirectoryW(name, 0) ? 0 : GetLastError();
						name[low_iterator] = name_character;

						if (native_error && native_error != ERROR_ALREADY_EXISTS)
						{
							ssn_internal_win32_undo_create_directory(last_create_index, name, high_iterator);
							if (native_error == ERROR_ACCESS_DENIED)
								return EACCES;
							else
								return EIO;
						}
						else
							last_create_index = low_iterator;
					}
				}
				*undo_index = high_iterator;
				return 0;
			}
			else if (native_error != ERROR_PATH_NOT_FOUND)
			{
				if (native_error == ERROR_ACCESS_DENIED)
					return EACCES;
				else
					return EIO;
			}
		}
	}
	return ENOENT;
}

int ssn_open_file(const char* name, int permissions_and_flags, ssn_handle_t* handle_address)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 1)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	DWORD desired_access =
		((permissions_and_flags & SSN_READ_PERMISION) ? GENERIC_READ : 0) |
		((permissions_and_flags & SSN_WRITE_PERMISION) ? GENERIC_WRITE : 0);
	if (!desired_access)
		return EINVAL;

	DWORD creation_disposition =
		(permissions_and_flags & SSN_CREATE) ? ((permissions_and_flags & SSN_TRUNCATE) ? CREATE_ALWAYS : CREATE_NEW) : OPEN_EXISTING;

	HANDLE handle = CreateFileW(native_name, desired_access, ((desired_access == GENERIC_READ) ? FILE_SHARE_READ : 0),
		0, creation_disposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);

	if (handle == INVALID_HANDLE_VALUE)
	{
		DWORD native_error = GetLastError();

		if ((native_error == ERROR_PATH_NOT_FOUND) &&
			((permissions_and_flags & (SSN_CREATE_PATH | SSN_CREATE | SSN_WRITE_PERMISION)) == (SSN_CREATE_PATH | SSN_CREATE | SSN_WRITE_PERMISION)))
		{
			size_t create_directory_undo_index;
			size_t directory_part_length = ssn_internal_win32_get_directory_part_length_from_file_name((size_t)(name_length - 1), native_name);
			int error = directory_part_length ? ssn_internal_win32_create_directory(directory_part_length, native_name, &create_directory_undo_index) : ENOENT;
			if (error)
				return error;
			
			handle = CreateFileW(native_name, desired_access, ((desired_access == GENERIC_READ) ? FILE_SHARE_READ : 0),
				0, creation_disposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
			if (handle == INVALID_HANDLE_VALUE)
			{
				native_error = GetLastError();
				ssn_internal_win32_undo_create_directory(directory_part_length, native_name, create_directory_undo_index);
			}
		}

		if (handle == INVALID_HANDLE_VALUE)
			switch (native_error)
			{
				case ERROR_FILE_NOT_FOUND:
					return ENOENT;
				case ERROR_PATH_NOT_FOUND:
					return ENOENT;
				case ERROR_ACCESS_DENIED:
					return EACCES;
				case ERROR_FILE_EXISTS:
					return EEXIST;
				case ERROR_INVALID_NAME:
					return ENOENT;
				default:
					return EIO;
			}
	}

	SSN_FILE_API_ASSERT(handle != INVALID_HANDLE_VALUE);

	*handle_address = (ssn_handle_t)handle;
	return 0;
}

int ssn_close_file(ssn_handle_t handle)
{
	if (!CloseHandle((HANDLE)handle))
		switch (GetLastError())
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_HANDLE:
				return ENOENT;
			default:
				return EIO;
		}

	return 0;
}

int ssn_get_file_size(ssn_handle_t handle, uint64_t* file_size_address)
{
	if (!GetFileSizeEx((HANDLE)handle, (LARGE_INTEGER*)file_size_address))
		switch (GetLastError())
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_HANDLE:
				return ENOENT;
			default:
				return EIO;
		}

	return 0;
}

int ssn_truncate_file(ssn_handle_t handle, uint64_t file_size)
{
	if (file_size & 0x8000000000000000)
		return EINVAL;

	if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &file_size, sizeof(uint64_t)))
		return EIO;

	return 0;
}

int ssn_flush_file_buffers(ssn_handle_t handle)
{
	if (!FlushFileBuffers((ssn_handle_t)handle))
		switch (GetLastError())
		{
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_HANDLE:
				return ENOENT;
			default:
				return EIO;
		}

	return 0;
}

static void WINAPI ssn_io_completion_routine(DWORD error, DWORD bytes_transfered, volatile ssn_overlapped_data_t* overlapped_data)
{
	overlapped_data->error = error;
	overlapped_data->bytes_transfered = bytes_transfered;
}

int ssn_read_file(ssn_handle_t handle, uint64_t file_offset, size_t io_size, void* buffer)
{
	volatile ssn_overlapped_data_t overlapped_data;

	DWORD maximum_io_size = 0x80000000;
	for (size_t file_read = 0; file_read != io_size;)
	{
		overlapped_data.overlapped_internal_low = 0;
		overlapped_data.overlapped_internal_high = 0;
		overlapped_data.overlapped_offset = (ULONGLONG)file_offset + (ULONGLONG)file_read;
		overlapped_data.overlapped_event = 0;
		overlapped_data.error = ERROR_UNIDENTIFIED_ERROR;
		overlapped_data.bytes_transfered = 0;

		DWORD io_transfer_size = (((io_size - file_read) < (size_t)maximum_io_size) ? (DWORD)(io_size - file_read) : (DWORD)maximum_io_size);
		DWORD error = ReadFileEx((HANDLE)handle, (void*)((uintptr_t)buffer + file_read), io_transfer_size, (OVERLAPPED*)&overlapped_data,
			(LPOVERLAPPED_COMPLETION_ROUTINE)ssn_io_completion_routine) ? 0 : ERROR_UNIDENTIFIED_ERROR;
		if (!error)
		{
			SleepEx(INFINITE, TRUE);
			error = overlapped_data.error;
			if (error)
			{
				if (error != ERROR_HANDLE_EOF)
					return ENODATA;
				else
					return EIO;
			}
			file_read += (size_t)overlapped_data.bytes_transfered;
		}
		else
		{
			if (io_transfer_size > 0x100000 && maximum_io_size > 0x100000)
				maximum_io_size = 0x100000;
			else
				return EIO;
		}
	}

	return 0;
}

int ssn_write_file(ssn_handle_t handle, uint64_t file_offset, size_t io_size, const void* buffer)
{
	volatile ssn_overlapped_data_t overlapped_data;

	DWORD maximum_io_size = 0x80000000;
	for (size_t file_written = 0; file_written != io_size;)
	{
		overlapped_data.overlapped_internal_low = 0;
		overlapped_data.overlapped_internal_high = 0;
		overlapped_data.overlapped_offset = (ULONGLONG)file_offset + (ULONGLONG)file_written;
		overlapped_data.overlapped_event = 0;
		overlapped_data.error = ERROR_UNIDENTIFIED_ERROR;
		overlapped_data.bytes_transfered = 0;

		DWORD io_transfer_size = (((io_size - file_written) < (size_t)maximum_io_size) ? (DWORD)(io_size - file_written) : (DWORD)maximum_io_size);
		DWORD error = WriteFileEx((HANDLE)handle, (void*)((uintptr_t)buffer + file_written), io_transfer_size,
			(OVERLAPPED*)&overlapped_data, (LPOVERLAPPED_COMPLETION_ROUTINE)ssn_io_completion_routine) ? 0 : ERROR_UNIDENTIFIED_ERROR;
		if (!error)
		{
			SleepEx(INFINITE, TRUE);
			error = overlapped_data.error;
			if (error)
			{
				if (error != ERROR_HANDLE_EOF)
					return ENODATA;
				else
					return EIO;
			}
			file_written += (size_t)overlapped_data.bytes_transfered;
		}
		else
		{
			if (io_transfer_size > 0x100000 && maximum_io_size > 0x100000)
				maximum_io_size = 0x100000;
			else
				return EIO;
		}
	}

	return 0;
}

int ssn_load_file(const char* name, size_t buffer_size, void* buffer, size_t* file_size_address)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 1)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	HANDLE handle = CreateFileW(native_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, buffer_size ? FILE_FLAG_SEQUENTIAL_SCAN : 0, 0);
	if (handle == INVALID_HANDLE_VALUE)
	{
		DWORD create_native_error = GetLastError();
		switch (create_native_error)
		{
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_FILE_EXISTS:
				return EEXIST;
			case ERROR_INVALID_NAME:
				return ENOENT;
			default:
				return EIO;
		}
	}

#ifdef _WIN64
	SSN_FILE_API_ASSERT(sizeof(size_t) == sizeof(uint64_t));

	size_t file_size;
	int error = ssn_get_file_size((HANDLE)handle, (uint64_t*)&file_size);
	if (error)
	{
		CloseHandle(handle);
		return error;
	}
#else
	SSN_FILE_API_ASSERT(sizeof(size_t) == sizeof(uint32_t));

	uint64_t native_file_size;
	int error = ssn_get_file_size((HANDLE)handle, &native_file_size);
	if (error)
	{
		CloseHandle(handle);
		return error;
	}
	if (native_file_size > (uint64_t)0xFFFFFFFF)
	{
		CloseHandle(handle);
		return E2BIG;
	}
	size_t file_size = (size_t)native_file_size;
#endif

	*file_size_address = file_size;
	if (file_size > buffer_size)
	{
		CloseHandle(handle);
		return ENOBUFS;
	}

	DWORD maximum_io_size = 0x80000000;
	DWORD transfer_size;
	for (size_t file_read = 0; file_read != file_size;)
	{
		DWORD io_transfer_size = ((file_size - file_read) < (size_t)maximum_io_size) ? (DWORD)(file_size - file_read) : maximum_io_size;
		if (ReadFile(handle, (void*)((uintptr_t)buffer + file_read), io_transfer_size, &transfer_size, 0) && transfer_size)
			file_read += (size_t)transfer_size;
		else
		{
			if (io_transfer_size > 0x100000 && maximum_io_size > 0x100000)
				maximum_io_size = 0x100000;
			else
			{
				CloseHandle(handle);
				return EIO;
			}
		}
	}

	CloseHandle(handle);
	return 0;
}

int ssn_allocate_and_load_file(const char* name, void* allocator_context, allocator_callback_t allocator_procedure, deallocator_callback_t deallocator_procedure, size_t* file_size_address, void** file_data_address)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 1)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	HANDLE handle = CreateFileW(native_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (handle == INVALID_HANDLE_VALUE)
	{
		DWORD create_native_error = GetLastError();
		switch (create_native_error)
		{
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_FILE_EXISTS:
				return EEXIST;
			case ERROR_INVALID_NAME:
				return ENOENT;
			default:
				return EIO;
		}
	}

#ifdef _WIN64
	SSN_FILE_API_ASSERT(sizeof(size_t) == sizeof(uint64_t));

	size_t file_size;
	int error = ssn_get_file_size((HANDLE)handle, (uint64_t*)&file_size);
	if (error)
	{
		CloseHandle(handle);
		return error;
	}
#else
	SSN_FILE_API_ASSERT(sizeof(size_t) == sizeof(uint32_t));

	uint64_t native_file_size;
	int error = ssn_get_file_size((HANDLE)handle, &native_file_size);
	if (error)
	{
		CloseHandle(handle);
		return error;
	}
	if (native_file_size > (uint64_t)0xFFFFFFFF)
	{
		CloseHandle(handle);
		return E2BIG;
	}
	size_t file_size = (size_t)native_file_size;
#endif

	*file_size_address = file_size;

	void* file_data = allocator_procedure(allocator_context, file_size);
	if (!file_data)
	{
		CloseHandle(handle);
		return ENOBUFS;
	}

	DWORD maximum_io_size = 0x80000000;
	DWORD transfer_size;
	for (size_t file_read = 0; file_read != file_size;)
	{
		DWORD io_transfer_size = ((file_size - file_read) < (size_t)maximum_io_size) ? (DWORD)(file_size - file_read) : maximum_io_size;
		if (ReadFile(handle, (void*)((uintptr_t)file_data + file_read), io_transfer_size, &transfer_size, 0) && transfer_size)
			file_read += (size_t)transfer_size;
		else
		{
			if (io_transfer_size > 0x100000 && maximum_io_size > 0x100000)
				maximum_io_size = 0x100000;
			else
			{
				deallocator_procedure(allocator_context, file_size, file_data);
				CloseHandle(handle);
				return EIO;
			}
		}
	}

	CloseHandle(handle);
	*file_data_address = file_data;
	return 0;
}

int ssn_store_file(const char* name, size_t size, const void* data)
{
	size_t utf8_name_length = ssn_internal_utf8_string_length(name);

	size_t utf8_name_file_length = 0;
	while (utf8_name_file_length != utf8_name_length && name[(utf8_name_length - 1) - utf8_name_file_length] != '\\' && name[(utf8_name_length - 1) - utf8_name_file_length] != '/')
		++utf8_name_file_length;

	if (!utf8_name_file_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(utf8_name_file_length <= utf8_name_length);

	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);
	int name_file_length = MultiByteToWideChar(CP_UTF8, 0, name + (utf8_name_length - utf8_name_file_length), -1, 0, 0);
	int name_directory_length = name_length - name_file_length;
	int tmp_name_length = name_directory_length + 13;

	if (name_length < 1 || name_file_length < 1 || name_length < name_file_length)
		return EINVAL;

	if (name_length > 0x8000 || tmp_name_length > 0x8000)
		return ENAMETOOLONG;

	FILE_RENAME_INFO* native_name = (FILE_RENAME_INFO*)_alloca((size_t)(&((const FILE_RENAME_INFO*)0)->FileName) + ((size_t)name_length + (size_t)tmp_name_length) * sizeof(WCHAR));
	native_name->ReplaceIfExists = TRUE;
	native_name->RootDirectory = 0;
	native_name->FileNameLength = (DWORD)(name_length - 1);
	WCHAR* native_tmp_name = (WCHAR*)((uintptr_t)native_name + (size_t)(&((const FILE_RENAME_INFO*)0)->FileName) + ((size_t)name_length * sizeof(WCHAR)));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name->FileName, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name->FileName[name_length - 1]);

	if (name_directory_length)
	{
		if (native_name->FileName[name_length - name_file_length - 1] != L'\\' && native_name->FileName[name_length - name_file_length - 1] != L'/')
			return EINVAL;

		for (int copy_index = 0; copy_index != name_directory_length; ++copy_index)
			native_tmp_name[copy_index] = native_name->FileName[copy_index];
	}

	size_t create_directory_undo_index = (size_t)~0;
	HANDLE handle = INVALID_HANDLE_VALUE;
	for (int try_count = 0; handle == INVALID_HANDLE_VALUE;)
	{
		ssn_internal_win32_make_tmp_8dot3_file_name(native_tmp_name + name_directory_length, try_count);

		handle = CreateFileW(native_tmp_name, GENERIC_WRITE | DELETE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
		if (handle == INVALID_HANDLE_VALUE)
		{
			DWORD create_native_error = GetLastError();
			if (create_native_error == ERROR_FILE_EXISTS)
			{
				if (try_count != SSN_INTERNAL_MAKE_TMP_FILE_NAME_TRY_LIMIT)
					++try_count;
				else
				{
					if (create_directory_undo_index != (size_t)~0)
						ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);

					return EEXIST;
				}
			}
			else if (create_native_error == ERROR_PATH_NOT_FOUND && name_directory_length && create_directory_undo_index == (size_t)~0)
			{
				int create_directory_error = ssn_internal_win32_create_directory((size_t)(name_directory_length - 1), native_tmp_name, &create_directory_undo_index);
				if (create_directory_error)
					return create_directory_error;
			}
			else
			{
				if (create_directory_undo_index != (size_t)~0)
					ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);

				switch (create_native_error)
				{
					case ERROR_FILE_NOT_FOUND:
						return ENOENT;
					case ERROR_PATH_NOT_FOUND:
						return ENOENT;
					case ERROR_ACCESS_DENIED:
						return EACCES;
					case ERROR_INVALID_NAME:
						return ENOENT;
					default:
						return EIO;
				}
			}
		}
	}

#ifdef _WIN64
	if (size & 0x8000000000000000)
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);
		return EINVAL;
	}

	if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &size, sizeof(uint64_t)))
#else
	uint64_t native_file_size = (uint64_t)size;
	if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &native_file_size, sizeof(uint64_t)))
#endif
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);
		return EIO;
	}

	DWORD maximum_io_size = 0x80000000;
	DWORD transfer_size;
	for (size_t file_written = 0; file_written != size;)
	{
		DWORD io_transfer_size = ((size - file_written) < (size_t)maximum_io_size) ? (DWORD)(size - file_written) : maximum_io_size;
		if (WriteFile(handle, (const void*)((uintptr_t)data + file_written), io_transfer_size, &transfer_size, 0) && transfer_size)
			file_written += (size_t)transfer_size;
		else
		{
			if (io_transfer_size > 0x100000 && maximum_io_size > 0x100000)
				maximum_io_size = 0x100000;
			else
			{
				CloseHandle(handle);
				DeleteFileW(native_tmp_name);
				if (create_directory_undo_index != (size_t)~0)
					ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);
				return EIO;
			}
		}
	}

	if (!FlushFileBuffers(handle))
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);
		return EIO;
	}

	if (!SetFileInformationByHandle(handle, FileRenameInfo, native_name, (size_t)(&((const FILE_RENAME_INFO*)0)->FileName) + ((size_t)name_length * sizeof(WCHAR))))
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(name_directory_length - 1), native_tmp_name, create_directory_undo_index);
		return EIO;
	}

	CloseHandle(handle);
	return 0;
}

int ssn_delete_file(const char* name)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 1)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	if (!DeleteFileW(native_name))
	{
		DWORD native_error = GetLastError();
		switch (native_error)
		{
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_NAME:
				return ENOENT;
			default:
				return EIO;
		}
	}

	return 0;
}

static size_t ssn_internal_win32_get_allocation_granularity()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (size_t)(system_info.dwAllocationGranularity > system_info.dwPageSize ? system_info.dwAllocationGranularity : system_info.dwPageSize);
}

typedef struct ssn_internal_win32_object_t
{
	uint16_t previous_length;
	uint16_t type_and_length;
	WCHAR name[1];
} ssn_internal_win32_object_t;

#define SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_EXPONENT_LIMIT 7
#define SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY 0x20000

typedef struct ssn_internal_win32_object_block_t
{
	struct ssn_internal_win32_object_block_t* previous_block;
	struct ssn_internal_win32_object_block_t* next_block;
	size_t block_size;
	size_t allocation_exponent;
	size_t allocation_count;
	size_t object_count;
	ssn_internal_win32_object_t* object_list_end;
	ssn_internal_win32_object_t object_list_begin[1];
} ssn_internal_win32_object_block_t;

static void ssn_internal_win32_deallocate_object_list(ssn_internal_win32_object_block_t* last_block)
{
	const size_t allocation_exponent_limit = SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_EXPONENT_LIMIT;
	while (last_block)
	{
		SSN_FILE_API_ASSERT(last_block->block_size && last_block->allocation_count && last_block->allocation_exponent <= allocation_exponent_limit);

		ssn_internal_win32_object_block_t* previous_block = last_block->previous_block;

		uintptr_t base_address = (uintptr_t)last_block;
		size_t allocation_exponent = last_block->allocation_exponent;
		size_t allocation_count = last_block->allocation_count;
		for (size_t i = allocation_count; i--;)
		{
			uintptr_t allocation_offset = 0;
			for (size_t j = 0; j != i; ++j)
				allocation_offset += ((size_t)1 << ((allocation_exponent + j) < allocation_exponent_limit ? (allocation_exponent + j) : allocation_exponent_limit)) * SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY;
			VirtualFree((void*)((uintptr_t)last_block + allocation_offset), 0, MEM_RELEASE);
		}
		
		last_block = previous_block;
	}
}

static int ssn_internal_win32_list_directory(size_t name_length, const WCHAR* name, size_t* directory_count_address, size_t* file_count_address,
	ssn_internal_win32_object_block_t** last_block_address)
{
	const size_t block_header_size = (size_t)(&((const ssn_internal_win32_object_block_t*)0)->object_list_begin);
	const size_t object_header_size = (size_t)(&((const ssn_internal_win32_object_t*)0)->name);
	const size_t allocation_exponent_limit = SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_EXPONENT_LIMIT;

	if (name_length && (name[name_length - 1] == L'\\' || name[name_length - 1] == L'/'))
		--name_length;

	if (name_length > 0x7FFD)
		return ENAMETOOLONG;

	SSN_FILE_API_ASSERT(sizeof(WCHAR) == sizeof(uint16_t) && (2 * sizeof(WCHAR)) == object_header_size);

	const size_t minimum_allocation_granularity = SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY;
	SSN_FILE_API_ASSERT(minimum_allocation_granularity >= (block_header_size + object_header_size + (0x8000 * sizeof(WCHAR))));
	size_t allocation_granularity = (ssn_internal_win32_get_allocation_granularity() + (minimum_allocation_granularity - 1)) & ~(minimum_allocation_granularity - 1);

	size_t block_count = 1;
	size_t allocation_exponent = 0;
	ssn_internal_win32_object_block_t* current_block = VirtualAlloc(0, allocation_granularity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!current_block)
		return ENOMEM;
	
	current_block->previous_block = 0;
	current_block->next_block = 0;
	current_block->block_size = allocation_granularity;
	current_block->allocation_exponent = allocation_exponent;
	current_block->allocation_count = 1;
	current_block->object_count = 0;
	current_block->object_list_end = 0;

	ssn_internal_win32_copy_string(name_length + 1, (WCHAR*)((uintptr_t)current_block + block_header_size), name);

	ssn_internal_win32_object_t* next_object = current_block->object_list_begin;

	ssn_internal_win32_object_t* current_directory = 0;
	ssn_internal_win32_object_block_t* current_directory_block = current_block;

	size_t directory_count = 0;
	size_t file_count = 0;

	WIN32_FIND_DATAW file_data;
	while (current_directory != next_object)
	{
		size_t object_name_directory_length = current_directory ? (current_directory->type_and_length & 0x7FFF) : name_length;
		WCHAR* search_path = current_directory ? current_directory->name : (WCHAR*)current_block->object_list_begin;

		SSN_FILE_API_ASSERT(
			(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 1) * sizeof(WCHAR))) == (uint16_t*)&search_path[object_name_directory_length + 1] &&
			(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 2) * sizeof(WCHAR))) == (uint16_t*)&search_path[object_name_directory_length + 2]);
		uint16_t search_tmp_buffer[2] = {
			*(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 1) * sizeof(WCHAR))),
			*(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 2) * sizeof(WCHAR))) };

		search_path[object_name_directory_length + 0] = L'\\';
		search_path[object_name_directory_length + 1] = L'*';
		search_path[object_name_directory_length + 2] = 0;
		HANDLE handle = FindFirstFileW(search_path, &file_data);
		DWORD native_search_error = handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
		*(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 0) * sizeof(WCHAR))) = 0;
		*(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 1) * sizeof(WCHAR))) = search_tmp_buffer[0];
		*(uint16_t*)((uintptr_t)search_path + ((object_name_directory_length + 2) * sizeof(WCHAR))) = search_tmp_buffer[1];

		while (!native_search_error)
		{
			size_t object_name_file_length = ssn_internal_win32_string_length(file_data.cFileName);
			if ((file_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && !(file_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
				(!(object_name_file_length == 1 && file_data.cFileName[0] == L'.') && !(object_name_file_length == 2 && file_data.cFileName[0] == L'.' && file_data.cFileName[1] == L'.')))
			{
				if (object_name_directory_length + 1 + object_name_file_length > 0x7FFF)
				{
					ssn_internal_win32_deallocate_object_list(current_block);
					return ENAMETOOLONG;
				}

				int is_directory = ((file_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
				size_t block_remaining = (size_t)(((uintptr_t)current_block + current_block->block_size) - (uintptr_t)next_object);
				size_t block_required = object_header_size + ((object_name_directory_length + 1 + object_name_file_length + 1 + (is_directory ? 1 : 0)) * sizeof(WCHAR));
				if (block_remaining < block_required)
				{
					SSN_FILE_API_ASSERT(current_block->block_size);

					if (allocation_exponent < allocation_exponent_limit)
						++allocation_exponent;

					void* block_extension_base = (void*)((uintptr_t)current_block + current_block->block_size);
					SSN_FILE_API_ASSERT(allocation_exponent <= allocation_exponent_limit);
					size_t new_block_size = ((size_t)1 << allocation_exponent) * allocation_granularity;
					SSN_FILE_API_ASSERT(new_block_size <= ((size_t)1 << SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_EXPONENT_LIMIT) * allocation_granularity);
					ssn_internal_win32_object_block_t* new_block = VirtualAlloc(block_extension_base, new_block_size - 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
					if (!new_block)
					{
						new_block = VirtualAlloc(0, new_block_size - 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
						if (!new_block)
						{
							ssn_internal_win32_deallocate_object_list(current_block);
							return ENOMEM;
						}
					}

					if ((uintptr_t)new_block == (uintptr_t)block_extension_base)
					{
						current_block->block_size += (uint32_t)new_block_size;
						current_block->allocation_count++;
					}
					else
					{
						current_block->next_block = new_block;

						new_block->previous_block = current_block;
						new_block->next_block = 0;
						new_block->block_size = new_block_size;
						new_block->allocation_exponent = allocation_exponent;
						new_block->allocation_count = 1;
						new_block->object_count = 0;
						new_block->object_list_end = 0;

						++block_count;

						current_block = new_block;
						next_object = current_block->object_list_begin;
					}
				}
				SSN_FILE_API_ASSERT((size_t)(((uintptr_t)current_block + current_block->block_size) - (uintptr_t)next_object) >= block_required);

				next_object->previous_length = current_block->object_list_end ? (current_block->object_list_end->type_and_length & 0x7FFF) : 0;
				next_object->type_and_length = (uint16_t)(object_name_directory_length + 1 + object_name_file_length) | (uint16_t)(is_directory ? 0x8000 : 0);
				ssn_internal_win32_copy_string(object_name_directory_length, next_object->name, current_directory ? current_directory->name : name);
				next_object->name[object_name_directory_length] = L'\\';
				ssn_internal_win32_copy_string(object_name_file_length + 1, next_object->name + object_name_directory_length + 1, file_data.cFileName);

				current_block->object_list_end = next_object;
				current_block->object_count++;

				if (is_directory)
					++directory_count;
				else
					++file_count;
				
				next_object = (ssn_internal_win32_object_t*)((uintptr_t)next_object + object_header_size + (((uintptr_t)(next_object->type_and_length & 0x7FFF) + 1) * sizeof(WCHAR)));
			}

			native_search_error = FindNextFileW(handle, &file_data) ? 0 : GetLastError();
		}
		if (handle != INVALID_HANDLE_VALUE)
			FindClose(handle);
		if (native_search_error != ERROR_FILE_NOT_FOUND && native_search_error != ERROR_NO_MORE_FILES)
		{
			ssn_internal_win32_deallocate_object_list(current_block);
			return EIO;
		}

		if (current_directory)
		{
			if (current_directory != current_directory_block->object_list_end)
				current_directory = (ssn_internal_win32_object_t*)((uintptr_t)current_directory + object_header_size + (((uintptr_t)(current_directory->type_and_length & 0x7FFF) + 1) * sizeof(WCHAR)));
			else if (current_directory_block->next_block)
			{
				current_directory_block = current_directory_block->next_block;
				current_directory = current_directory_block->object_list_begin;
			}
			else
			{
				current_directory_block = current_block;
				current_directory = next_object;
			}
		}
		else
			current_directory = current_directory_block->object_list_begin;

		while (current_directory != next_object && !(current_directory->type_and_length & 0x8000))
		{
			if (current_directory != current_directory_block->object_list_end)
				current_directory = (ssn_internal_win32_object_t*)((uintptr_t)current_directory + object_header_size + (((uintptr_t)(current_directory->type_and_length & 0x7FFF) + 1) * sizeof(WCHAR)));
			else if (current_directory_block->next_block)
			{
				current_directory_block = current_directory_block->next_block;
				current_directory = current_directory_block->object_list_begin;
			}
			else
			{
				current_directory_block = current_block;
				current_directory = next_object;
			}
		}
	}

	*directory_count_address = directory_count;
	*file_count_address = file_count;
	*last_block_address = current_block;
	return 0;
}

#ifdef _DEBUG
static int ssn_internal_win32_debug_validate_list_directory(size_t directory_count, size_t file_count, ssn_internal_win32_object_block_t* last_block)
{
	WCHAR** name_table = (WCHAR**)VirtualAlloc(0, (directory_count + file_count) * sizeof(WCHAR*), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	SSN_FILE_API_ASSERT(name_table);

	size_t test_directory_count = 0;
	size_t test_file_count = 0;
	ssn_internal_win32_object_block_t* block = last_block;
	while (block->previous_block)
		block = block->previous_block;

	for (size_t c = 0, i = 0; block; ++i)
	{
		ssn_internal_win32_object_t* object = block->object_list_begin;
		for (size_t j = 0; j != block->object_count; ++j)
		{
			DWORD file_attributes = GetFileAttributesW(object->name);
			if (file_attributes == INVALID_FILE_ATTRIBUTES)
			{
				VirtualFree(name_table, 0, MEM_RELEASE);
				return 0;
			}

			if (object->type_and_length & 0x8000)
			{
				if (!(file_attributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					VirtualFree(name_table, 0, MEM_RELEASE);
					return 0;
				}

				++test_directory_count;
			}
			else
			{
				if (file_attributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					VirtualFree(name_table, 0, MEM_RELEASE);
					return 0;
				}

				++test_file_count;
			}

			if (c >= test_directory_count + test_file_count)
				return 0;

			name_table[c] = object->name;
			for (size_t k = 0; k != c; ++k)
				if (!lstrcmpW(name_table[c], name_table[k]))
				{
					VirtualFree(name_table, 0, MEM_RELEASE);
					return 0;
				}
			++c;

			object = (ssn_internal_win32_object_t*)((uintptr_t)object + (size_t)(&((const ssn_internal_win32_object_t*)0)->name) + (((uintptr_t)(object->type_and_length & 0x7FFF) + 1) * sizeof(WCHAR)));
		}
		block = block->next_block;
	}

	if (test_directory_count != directory_count || test_file_count != file_count)
	{
		VirtualFree(name_table, 0, MEM_RELEASE);
		return 0;
	}

	VirtualFree(name_table, 0, MEM_RELEASE);
	return 1;
}
#endif // _DEBUG

int ssn_delete_directory(const char* name)
{
	size_t utf8_path_length = ssn_internal_utf8_string_length(name);

	size_t utf8_name_length = 0;
	if (utf8_path_length)
	{
		if (name[utf8_path_length - 1] != '\\' && name[utf8_path_length - 1] != '/')
		{
			while (utf8_name_length != utf8_path_length && name[(utf8_path_length - 1) - utf8_name_length] != '\\' && name[(utf8_path_length - 1) - utf8_name_length] != '/')
				++utf8_name_length;
		}
		else
		{
			utf8_name_length = 1;
			while (utf8_name_length != utf8_path_length && name[(utf8_path_length - 1) - utf8_name_length] != '\\' && name[(utf8_path_length - 1) - utf8_name_length] != '/')
				++utf8_name_length;
		}
	}

	if (!utf8_name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(utf8_name_length <= utf8_path_length);

	int path_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name + (utf8_path_length - utf8_name_length), -1, 0, 0);
	int name_directory_length = path_length - name_length;
	int tmp_name_length = name_directory_length + 9;

	if (path_length < 2 || name_length < 2 || path_length < name_length)
		return EINVAL;

	if (path_length > 0x8000 || tmp_name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca(((size_t)path_length + (size_t)tmp_name_length) * sizeof(WCHAR));
	WCHAR* native_tmp_name = native_name + path_length;

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, path_length) != path_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[path_length - 1]);

	if (ssn_internal_win32_root_directory_length((size_t)(path_length - 1), native_name) == (size_t)(path_length - 1))
		return EINVAL;

	if (native_name[path_length - 2] == L'\\' || native_name[path_length - 2] == L'/')
	{
		native_name[path_length - 2] = 0;
		--path_length;
		--name_length;

		if (name_length == 1)
			return EINVAL;
	}

	if (name_directory_length)
	{
		if (native_name[path_length - name_length - 1] != L'\\' && native_name[path_length - name_length - 1] != L'/')
			return EINVAL;

		for (int copy_index = 0; copy_index != name_directory_length; ++copy_index)
			native_tmp_name[copy_index] = native_name[copy_index];
	}

	DWORD directory_atributes = GetFileAttributesW(native_name);
	if (directory_atributes == INVALID_FILE_ATTRIBUTES)
	{
		DWORD native_atributes_error = GetLastError();
		switch (native_atributes_error)
		{
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_NAME:
				return ENOENT;
			default:
				return EIO;
		}
	}

	if (!(directory_atributes & FILE_ATTRIBUTE_DIRECTORY) || (directory_atributes & FILE_ATTRIBUTE_REPARSE_POINT))
		return EINVAL;

	for (int tmp_rename = 1, try_count = 0; tmp_rename;)
	{
		ssn_internal_win32_make_tmp_name(native_tmp_name + name_directory_length, try_count);

		if (MoveFileW(native_name, native_tmp_name))
			tmp_rename = 0;
		else
		{
			DWORD native_move_error = GetLastError();
			if (native_move_error == ERROR_ALREADY_EXISTS)
			{
				if (try_count != SSN_INTERNAL_MAKE_TMP_FILE_NAME_TRY_LIMIT)
					++try_count;
				else
					return EEXIST;
			}
			else
				switch (native_move_error)
				{
					case ERROR_FILE_NOT_FOUND:
						return ENOENT;
					case ERROR_PATH_NOT_FOUND:
						return ENOENT;
					case ERROR_ACCESS_DENIED:
						return EACCES;
					case ERROR_INVALID_NAME:
						return ENOENT;
					default:
						return EIO;
				}
		}
	}

	size_t sub_directory_count;
	size_t sub_file_count;
	ssn_internal_win32_object_block_t* last_block;
	int error = ssn_internal_win32_list_directory((size_t)(tmp_name_length - 1), native_tmp_name, &sub_directory_count, &sub_file_count, &last_block);
	if (error)
	{
		MoveFileW(native_tmp_name, native_name);
		return error;
	}

	for (ssn_internal_win32_object_block_t* block = last_block; block; block = block->previous_block)
	{
		ssn_internal_win32_object_t* object = block->object_list_end;
		for (size_t n = block->object_count, i = 0; i != n; ++i)
		{
			if (object->type_and_length & 0x8000)
				RemoveDirectoryW(object->name);
			else
				DeleteFileW(object->name);
			object = (ssn_internal_win32_object_t*)((uintptr_t)object - ((size_t)(&((const ssn_internal_win32_object_t*)0)->name) + (((size_t)object->previous_length + 1) * sizeof(WCHAR))));
		}
	}
	if (!RemoveDirectoryW(native_tmp_name))
	{
		DWORD native_remove_error = GetLastError();
		switch (native_remove_error)
		{
			case ERROR_FILE_NOT_FOUND:
				error = 0;// directory was somehow removed?
				break;
			case ERROR_PATH_NOT_FOUND:
				error = ENOENT;
				break;
			case ERROR_ACCESS_DENIED:
				error = EACCES;
				break;
			case ERROR_INVALID_NAME:
				error = ENOENT;
			default:
				error = EIO;
				break;
		}
		if (error)
			MoveFileW(native_tmp_name, native_name);// this is very bad. some files and directories were deleted, but operation failed.
	}

	ssn_internal_win32_deallocate_object_list(last_block);
	return error;
}

int ssn_move_file(const char* old_name, const char* new_name)
{
	int old_name_length = MultiByteToWideChar(CP_UTF8, 0, old_name, -1, 0, 0);
	int new_name_length = MultiByteToWideChar(CP_UTF8, 0, new_name, -1, 0, 0);

	if (old_name_length < 1 || new_name_length < 1)
		return EINVAL;

	if (old_name_length > 0x8000 || new_name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_old_name = (WCHAR*)_alloca((size_t)(old_name_length + new_name_length) * sizeof(WCHAR));
	WCHAR* native_new_name = native_old_name + old_name_length;

	if (MultiByteToWideChar(CP_UTF8, 0, old_name, -1, native_old_name, old_name_length) != old_name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_old_name[old_name_length - 1]);

	if (MultiByteToWideChar(CP_UTF8, 0, new_name, -1, native_new_name, new_name_length) != new_name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_new_name[new_name_length - 1]);

	if (!MoveFileW(native_old_name, native_new_name))
	{
		DWORD native_error = GetLastError();
		switch (native_error)
		{
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_FILE_EXISTS:
				return EEXIST;
			case ERROR_INVALID_NAME:
				return ENOENT;
			default:
				return EIO;
		}
	}

	return 0;
}

int ssn_move_directory(const char* old_name, const char* new_name)
{
	int old_name_length = MultiByteToWideChar(CP_UTF8, 0, old_name, -1, 0, 0);
	int new_name_length = MultiByteToWideChar(CP_UTF8, 0, new_name, -1, 0, 0);

	if (old_name_length < 2 || new_name_length < 2)
		return EINVAL;

	if (old_name_length > 0x8000 || new_name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_old_name = (WCHAR*)_alloca((size_t)(old_name_length + new_name_length) * sizeof(WCHAR));
	WCHAR* native_new_name = native_old_name + old_name_length;

	if (MultiByteToWideChar(CP_UTF8, 0, old_name, -1, native_old_name, old_name_length) != old_name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_old_name[old_name_length - 1]);

	if (ssn_internal_win32_root_directory_length((size_t)(old_name_length - 1), native_old_name) == (size_t)(old_name_length - 1))
		return EINVAL;

	if (native_old_name[old_name_length - 2] == L'\\' || native_old_name[old_name_length - 2] == L'/')
		native_old_name[old_name_length - 2] = 0;

	if (MultiByteToWideChar(CP_UTF8, 0, new_name, -1, native_new_name, new_name_length) != new_name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_new_name[new_name_length - 1]);

	if (ssn_internal_win32_root_directory_length((size_t)(new_name_length - 1), native_new_name) == (size_t)(new_name_length - 1))
		return EINVAL;

	if (native_new_name[new_name_length - 2] == L'\\' || native_new_name[new_name_length - 2] == L'/')
		native_new_name[new_name_length - 2] = 0;

	if (!MoveFileW(native_old_name, native_new_name))
	{
		DWORD native_error = GetLastError();
		switch (native_error)
		{
			case ERROR_FILE_NOT_FOUND:
				return ENOENT;
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_FILE_EXISTS:
				return EEXIST;
			case ERROR_INVALID_NAME:
				return ENOENT;
			default:
				return EIO;
		}
	}

	return 0;
}

int ssn_create_directory(const char* name)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 2)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	if (ssn_internal_win32_root_directory_length((size_t)(name_length - 1), native_name) == (size_t)(name_length - 1))
		return EINVAL;

	if (native_name[name_length - 2] == L'\\' || native_name[name_length - 2] == L'/')
		native_name[name_length - 2] = 0;

	if (!CreateDirectoryW(native_name, 0))
	{
		DWORD native_error = GetLastError();
		switch (native_error)
		{
			case ERROR_PATH_NOT_FOUND:
				return ENOENT;
			case ERROR_ACCESS_DENIED:
				return EACCES;
			case ERROR_INVALID_NAME:
				return ENOENT;
			case ERROR_ALREADY_EXISTS:
				return EEXIST;
			default:
				return EIO;
		}
	}

	return 0;
}

int ssn_list_directory(const char* name, size_t buffer_size, char** buffer, size_t* required_buffer_size_address, size_t* directory_count_address, size_t* file_count_address)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 2)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	if (ssn_internal_win32_root_directory_length((size_t)(name_length - 1), native_name) != (size_t)(name_length - 1) &&
		(native_name[name_length - 2] == L'\\' || native_name[name_length - 2] == L'/'))
	{
		native_name[name_length - 2] = 0;
		--name_length;
	}

	size_t sub_directory_count;
	size_t sub_file_count;
	ssn_internal_win32_object_block_t* last_block;
	int error = ssn_internal_win32_list_directory((size_t)(name_length - 1), native_name, &sub_directory_count, &sub_file_count, &last_block);
	if (error)
		return error;

	size_t sub_directory_index = 0;
	size_t sub_file_index = 0;
	size_t result_size = (sub_directory_count + sub_file_count) * sizeof(char*);
	for (ssn_internal_win32_object_block_t* block = last_block; block; block = block->previous_block)
	{
		ssn_internal_win32_object_t* object = block->object_list_end;
		for (size_t n = block->object_count, i = 0; i != n; ++i)
		{
			int object_native_name_length = (int)(object->type_and_length & 0x7FFF);
			int object_utf8_name_length = WideCharToMultiByte(CP_UTF8, 0, object->name, object_native_name_length, 0, 0, 0, 0);
			if (object_utf8_name_length < 1)
			{
				ssn_internal_win32_deallocate_object_list(last_block);
				return EIO;
			}
			char** table_entry;
			if (object->type_and_length & 0x8000)
				table_entry = buffer + sub_directory_index++;
			else
				table_entry = buffer + sub_directory_count + sub_file_index++;
			if (buffer_size >= result_size + (size_t)(object_utf8_name_length + 1))
			{
				*table_entry = (char*)((uintptr_t)buffer + result_size);
				if (WideCharToMultiByte(CP_UTF8, 0, object->name, object_native_name_length, (char*)((uintptr_t)buffer + result_size), object_utf8_name_length, 0, 0) != object_utf8_name_length)
				{
					ssn_internal_win32_deallocate_object_list(last_block);
					return EIO;
				}
				*(char*)((uintptr_t)buffer + result_size + object_utf8_name_length) = 0;
			}
			result_size += (size_t)(object_utf8_name_length + 1);
			object = (ssn_internal_win32_object_t*)((uintptr_t)object - ((size_t)(&((const ssn_internal_win32_object_t*)0)->name) + (((size_t)object->previous_length + 1) * sizeof(WCHAR))));
		}
	}
	ssn_internal_win32_deallocate_object_list(last_block);

	*required_buffer_size_address = result_size;
	*directory_count_address = sub_directory_count;
	*file_count_address = sub_file_count;
	return (result_size > buffer_size) ? ENOBUFS : 0;
}

int ssn_allocate_and_list_directory(const char* name, void* allocator_context, allocator_callback_t allocator_procedure, deallocator_callback_t deallocator_procedure, size_t* entry_table_size_address, char*** entry_table_address, size_t* directory_count_address, size_t* file_count_address)
{
	int name_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);

	if (name_length < 2)
		return EINVAL;

	if (name_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca((size_t)name_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, native_name, name_length) != name_length)
		return EINVAL;

	SSN_FILE_API_ASSERT(!native_name[name_length - 1]);

	if (ssn_internal_win32_root_directory_length((size_t)(name_length - 1), native_name) != (size_t)(name_length - 1) &&
		(native_name[name_length - 2] == L'\\' || native_name[name_length - 2] == L'/'))
	{
		native_name[name_length - 2] = 0;
		--name_length;
	}

	size_t sub_directory_count;
	size_t sub_file_count;
	ssn_internal_win32_object_block_t* last_block;
	int error = ssn_internal_win32_list_directory((size_t)(name_length - 1), native_name, &sub_directory_count, &sub_file_count, &last_block);
	if (error)
		return error;

	size_t buffer_size = (sub_directory_count + sub_file_count) * sizeof(char*);
	for (ssn_internal_win32_object_block_t* block = last_block; block; block = block->previous_block)
	{
		ssn_internal_win32_object_t* object = block->object_list_end;
		for (size_t n = block->object_count, i = 0; i != n; ++i)
		{
			int object_native_name_length = (int)(object->type_and_length & 0x7FFF);
			int object_utf8_name_length = WideCharToMultiByte(CP_UTF8, 0, object->name, object_native_name_length, 0, 0, 0, 0);
			if (object_utf8_name_length < 1)
			{
				ssn_internal_win32_deallocate_object_list(last_block);
				return EIO;
			}
			buffer_size += (size_t)(object_utf8_name_length + 1);
			object = (ssn_internal_win32_object_t*)((uintptr_t)object - ((size_t)(&((const ssn_internal_win32_object_t*)0)->name) + (((size_t)object->previous_length + 1) * sizeof(WCHAR))));
		}
	}

	char** buffer = (char**)allocator_procedure(allocator_context, buffer_size);
	if (!buffer)
		return ENOBUFS;

	size_t sub_directory_index = 0;
	size_t sub_file_index = 0;
	size_t result_size = (sub_directory_count + sub_file_count) * sizeof(char*);
	for (ssn_internal_win32_object_block_t* block = last_block; block; block = block->previous_block)
	{
		ssn_internal_win32_object_t* object = block->object_list_end;
		for (size_t n = block->object_count, i = 0; i != n; ++i)
		{
			int object_native_name_length = (int)(object->type_and_length & 0x7FFF);
			int object_utf8_name_length = WideCharToMultiByte(CP_UTF8, 0, object->name, object_native_name_length, 0, 0, 0, 0);
			if (object_utf8_name_length < 1)
			{
				deallocator_procedure(allocator_context, buffer_size, buffer);
				ssn_internal_win32_deallocate_object_list(last_block);
				return EIO;
			}
			char** table_entry;
			if (object->type_and_length & 0x8000)
				table_entry = buffer + sub_directory_index++;
			else
				table_entry = buffer + sub_directory_count + sub_file_index++;
			if (buffer_size < result_size + (size_t)(object_utf8_name_length + 1))
			{
				deallocator_procedure(allocator_context, buffer_size, buffer);
				ssn_internal_win32_deallocate_object_list(last_block);
				return EIO;
			}
			*table_entry = (char*)((uintptr_t)buffer + result_size);
			if (WideCharToMultiByte(CP_UTF8, 0, object->name, object_native_name_length, (char*)((uintptr_t)buffer + result_size), object_utf8_name_length, 0, 0) != object_utf8_name_length)
			{
				deallocator_procedure(allocator_context, buffer_size, buffer);
				ssn_internal_win32_deallocate_object_list(last_block);
				return EIO;
			}
			*(char*)((uintptr_t)buffer + result_size + object_utf8_name_length) = 0;
			result_size += (size_t)(object_utf8_name_length + 1);
			object = (ssn_internal_win32_object_t*)((uintptr_t)object - ((size_t)(&((const ssn_internal_win32_object_t*)0)->name) + (((size_t)object->previous_length + 1) * sizeof(WCHAR))));
		}
	}
	ssn_internal_win32_deallocate_object_list(last_block);

	*entry_table_size_address = buffer_size;
	*entry_table_address = buffer;
	*directory_count_address = sub_directory_count;
	*file_count_address = sub_file_count;
	return 0;
}

int ssn_get_executable_file_name(size_t buffer_size, char* buffer, size_t* file_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x8000);
	if (!native_name_length || native_name_length >= 0x8000)
		return EIO;

	int name_length = WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, 0, 0, 0, 0);
	if (name_length < 1)
		return EIO;
	
	*file_name_length_address = (size_t)name_length;
	if ((size_t)(name_length + 1) > buffer_size)
		return ENOBUFS;

	if (WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, buffer, name_length, 0, 0) != name_length)
		return EIO;
	*(buffer + name_length) = 0;

	return 0;
}

int ssn_allocate_and_get_executable_file_name(void* allocator_context, allocator_callback_t allocator_procedure, deallocator_callback_t deallocator_procedure, char** file_name_address, size_t* file_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x8000);
	if (!native_name_length || native_name_length >= 0x8000)
		return EIO;

	int name_length = WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, 0, 0, 0, 0);
	if (name_length < 1)
		return EIO;

	*file_name_length_address = (size_t)name_length;
	char* file_name = (char*)allocator_procedure(allocator_context, (size_t)(name_length + 1));
	if (!file_name)
		return ENOBUFS;

	if (WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, file_name, name_length, 0, 0) != name_length)
	{
		deallocator_procedure(allocator_context, (size_t)(name_length + 1), file_name);
		return EIO;
	}
	*(file_name + name_length) = 0;

	*file_name_address = file_name;
	return 0;
}

int ssn_get_program_directory(size_t buffer_size, char* buffer, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x8000);
	if (!native_name_length || native_name_length >= 0x8000)
		return EIO;

	while (native_name_length && (*(native_name + native_name_length - 1) != L'\\' && *(native_name + native_name_length - 1) != L'/'))
		--native_name_length;
	if (native_name_length && ssn_internal_win32_root_directory_length(native_name_length, native_name) != native_name_length)
		--native_name_length;

	int name_length = WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, 0, 0, 0, 0);
	if (name_length < 1)
		return EIO;

	*directory_name_length_address = (size_t)name_length;
	if ((size_t)(name_length + 1) > buffer_size)
		return ENOBUFS;

	if (WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, buffer, name_length, 0, 0) != name_length)
		return EIO;
	*(buffer + name_length) = 0;

	return 0;
}

int ssn_allocate_and_get_program_directory(void* allocator_context, allocator_callback_t allocator_procedure, deallocator_callback_t deallocator_procedure, char** directory_name_address, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x8000);
	if (!native_name_length || native_name_length >= 0x8000)
		return EIO;

	while (native_name_length && (*(native_name + native_name_length - 1) != L'\\' && *(native_name + native_name_length - 1) != L'/'))
		--native_name_length;
	if (native_name_length && ssn_internal_win32_root_directory_length(native_name_length, native_name) != native_name_length)
		--native_name_length;

	int name_length = WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, 0, 0, 0, 0);
	if (name_length < 1)
		return EIO;

	*directory_name_length_address = (size_t)name_length;
	char* buffer = allocator_procedure(allocator_context, (size_t)(name_length + 1));
	if (!buffer)
		return ENOBUFS;

	if (WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, buffer, name_length, 0, 0) != name_length)
	{
		deallocator_procedure(allocator_context, (size_t)(name_length + 1), buffer);
		return EIO;
	}
	*(buffer + name_length) = 0;

	*directory_name_address = buffer;
	return 0;
}

int ssn_get_data_directory(size_t buffer_size, const char* sub_directory_name, char* buffer, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetEnvironmentVariableW(L"LOCALAPPDATA", native_name, 0x8000);
	
	if (!native_name_length || native_name_length >= 0x8000)
	{
		HMODULE Userenv = LoadLibraryW(L"Userenv.dll");
		if (!Userenv)
			return ENOSYS;

		BOOL (WINAPI* GetUserProfileDirectoryW)(HANDLE hToken, WCHAR* lpProfileDir, DWORD* lpcchSize) = (BOOL (WINAPI*)(HANDLE, WCHAR*, DWORD*))GetProcAddress(Userenv, "GetUserProfileDirectoryW");
		if (!GetUserProfileDirectoryW)
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}

		DWORD user_diectory_name_length = 0x8000;
		if (!GetUserProfileDirectoryW(GetCurrentProcessToken(), native_name, &user_diectory_name_length))
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}
		native_name_length = ssn_internal_win32_string_length(native_name);

		FreeLibrary(Userenv);
	}

	size_t sub_directory_name_length = ssn_internal_utf8_string_length(sub_directory_name);
	if (sub_directory_name_length && native_name_length && (native_name[native_name_length - 1] == L'\\' || native_name[native_name_length - 1] == L'/'))
		--native_name_length;

	int name_length = WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, 0, 0, 0, 0);
	if (name_length < 1)
		return EIO;

	size_t directory_name_length = (size_t)name_length + (sub_directory_name_length ? sub_directory_name_length + 1 : 0);
	*directory_name_length_address = directory_name_length;
	if (directory_name_length + 1 > buffer_size)
		return ENOBUFS;

	if (WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, buffer, name_length, 0, 0) != name_length)
		return EIO;
	if (sub_directory_name_length)
	{
		*(buffer + name_length) = '\\';
		ssn_internal_copy_utf8_string(sub_directory_name_length, buffer + name_length + 1, sub_directory_name);
	}
	*(buffer + directory_name_length) = 0;
	return 0;
}

int ssn_allocate_and_get_data_directory(void* allocator_context, allocator_callback_t allocator_procedure, deallocator_callback_t deallocator_procedure, const char* sub_directory_name, char** directory_name_address, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetEnvironmentVariableW(L"LOCALAPPDATA", native_name, 0x8000);

	if (!native_name_length || native_name_length >= 0x8000)
	{
		HMODULE Userenv = LoadLibraryW(L"Userenv.dll");
		if (!Userenv)
			return ENOSYS;

		BOOL(WINAPI* GetUserProfileDirectoryW)(HANDLE hToken, WCHAR* lpProfileDir, DWORD* lpcchSize) = (BOOL(WINAPI*)(HANDLE, WCHAR*, DWORD*))GetProcAddress(Userenv, "GetUserProfileDirectoryW");
		if (!GetUserProfileDirectoryW)
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}

		DWORD user_diectory_name_length = 0x8000;
		if (!GetUserProfileDirectoryW(GetCurrentProcessToken(), native_name, &user_diectory_name_length))
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}
		native_name_length = ssn_internal_win32_string_length(native_name);

		FreeLibrary(Userenv);
	}

	size_t sub_directory_name_length = ssn_internal_utf8_string_length(sub_directory_name);
	if (sub_directory_name_length && native_name_length && (native_name[native_name_length - 1] == L'\\' || native_name[native_name_length - 1] == L'/'))
		--native_name_length;

	int name_length = WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, 0, 0, 0, 0);
	if (name_length < 1)
		return EIO;

	size_t directory_name_length = (size_t)name_length + (sub_directory_name_length ? sub_directory_name_length + 1 : 0);
	*directory_name_length_address = directory_name_length;
	char* directory_name = allocator_procedure(allocator_context, directory_name_length + 1);
	if (!directory_name)
		return ENOBUFS;

	if (WideCharToMultiByte(CP_UTF8, 0, native_name, (int)native_name_length, directory_name, name_length, 0, 0) != name_length)
	{
		deallocator_procedure(allocator_context, directory_name_length + 1, directory_name);
		return EIO;
	}
	if (sub_directory_name_length)
	{
		*(directory_name + name_length) = '\\';
		ssn_internal_copy_utf8_string(sub_directory_name_length, directory_name + name_length + 1, sub_directory_name);
	}
	*(directory_name + directory_name_length) = 0;
	*directory_name_address = directory_name;
	return 0;
}

int ssn_make_path(const char* parent_directory, const char* sub_directory, size_t buffer_size, char* buffer, size_t* directory_name_length_address)
{
	size_t parent_directory_length = ssn_internal_utf8_string_length(parent_directory);
	size_t sub_directory_length = ssn_internal_utf8_string_length(sub_directory);
	if ((parent_directory_length && (parent_directory[parent_directory_length - 1] == L'\\' || parent_directory[parent_directory_length - 1] == L'/')) &&
		(sub_directory_length || ssn_internal_root_directory_length(parent_directory_length, parent_directory) != parent_directory_length))
		--parent_directory_length;

	size_t name_length = (size_t)parent_directory_length + (parent_directory_length && sub_directory_length ? 1 : 0) + sub_directory_length;
	*directory_name_length_address = name_length;
	if (name_length + 1 > buffer_size)
		return ENOBUFS;

	ssn_internal_copy_utf8_string(parent_directory_length, buffer, parent_directory);
	if (parent_directory_length && sub_directory_length)
		*(buffer + parent_directory_length) = '\\';
	ssn_internal_copy_utf8_string(sub_directory_length, buffer + parent_directory_length + (parent_directory_length && sub_directory_length ? 1 : 0), sub_directory);
	*(buffer + name_length) = 0;
	return 0;
}

int ssn_allocate_and_make_path(void* allocator_context, allocator_callback_t allocator_procedure, deallocator_callback_t deallocator_procedure, const char* parent_directory, const char* sub_directory, char** directory_name_address, size_t* directory_name_length_address)
{
	size_t parent_directory_length = ssn_internal_utf8_string_length(parent_directory);
	size_t sub_directory_length = ssn_internal_utf8_string_length(sub_directory);
	if ((parent_directory_length && (parent_directory[parent_directory_length - 1] == L'\\' || parent_directory[parent_directory_length - 1] == L'/')) &&
		(sub_directory_length || ssn_internal_root_directory_length(parent_directory_length, parent_directory) != parent_directory_length))
		--parent_directory_length;

	size_t name_length = (size_t)parent_directory_length + (parent_directory_length && sub_directory_length ? 1 : 0) + sub_directory_length;
	*directory_name_length_address = name_length;
	char* buffer = allocator_procedure(allocator_context, name_length + 1);
	if (!buffer)
		return ENOBUFS;

	ssn_internal_copy_utf8_string(parent_directory_length, buffer, parent_directory);
	if (parent_directory_length && sub_directory_length)
		*(buffer + parent_directory_length) = '\\';
	ssn_internal_copy_utf8_string(sub_directory_length, buffer + parent_directory_length + (parent_directory_length && sub_directory_length ? 1 : 0), sub_directory);
	*(buffer + name_length) = 0;
	*directory_name_address = buffer;
	return 0;
}

#ifdef __cplusplus
}
#endif // __cplusplus