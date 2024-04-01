#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef _MSC_VER
#pragma warning( push )

// _alloca
// Is malloc.h needed for _alloca? since it is not really normal function
#pragma warning( disable : 6255)

// Large per function stack usage
#pragma warning( disable : 6262)

// GetTickCount
#pragma warning( disable : 28159)

#ifdef SSN_CRT_BASE_SUPPORTED
// I can not write or call __chkstk in C so...
extern void* _ssn_chkstk(size_t size);
#define SSN_PRE_ALLOCA_ROUTINE _ssn_chkstk
#else
#define SSN_PRE_ALLOCA_ROUTINE
#define SSN_ALLOCA _alloca
#endif

#else // _MSC_VER

#define SSN_PRE_ALLOCA_ROUTINE
#define SSN_ALLOCA alloca

#endif

#define WIN32_LEAN_AND_MEAN
#include "ssn_file_api.h"

#define SSN_IGNORE_SYSTEM_FILES
//#define SSN_IGNORE_SYSTEM_VOLUME_INFORMATION_DIRECTORY

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

static size_t ssn_internal_utf8_root_directory_length(size_t path_langth, const char* path)
{
	if (path_langth > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\')
	{
		if (path_langth > 6 && path[4] != '\\' && path[4] != '/' && path[5] == ':' && (path[6] == '\\' || path[6] == '/'))
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
				++share_legth;
			if (!share_legth || path[9 + server_legth + share_legth] != '\\' && path[9 + server_legth + share_legth] != '/')
				return 0;
			return 10 + server_legth + share_legth;
		}
		else
			return 0;
	}
	else
	{
		if (path_langth > 2 && path[0] != '\\' && path[0] != '/' && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
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
				++share_legth;
			if (!share_legth || path[3 + server_legth + share_legth] != '\\' && path[3 + server_legth + share_legth] != '/')
				return 0;
			return 4 + server_legth + share_legth;
		}
		else
			return 0;
	}
}

static size_t ssn_internal_win32_utf16_root_directory_length(size_t path_langth, const WCHAR* path)
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
				++share_legth;
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
				++share_legth;
			if (!share_legth || path[3 + server_legth + share_legth] != L'\\' && path[3 + server_legth + share_legth] != L'/')
				return 0;
			return 4 + server_legth + share_legth;
		}
		else
			return 0;
	}
}

static size_t ssn_internal_win32_utf16_string_length(const WCHAR* string)
{
	const WCHAR* read = string;
	while (*read)
		++read;
	return (size_t)((uintptr_t)read - (uintptr_t)string) / sizeof(WCHAR);
}

static void ssn_internal_win32_copy_utf16_string(size_t length, WCHAR* destination, const WCHAR* source)
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

static void ssn_internal_copy_utf8_string_to_extended_path(size_t length, char* destination, const char* source)
{
	const char* source_end = source + length;
	while (source != source_end)
	{
		char character = *source++;
		if (character == '/')
			character = '\\';
		*destination++ = character;
	}
}

static void ssn_internal_win32_copy_utf16_string_to_extended_path(size_t length, WCHAR* destination, const WCHAR* source)
{
	const WCHAR* source_end = source + length;
	while (source != source_end)
	{
		WCHAR character = *source++;
		if (character == L'/')
			character = L'\\';
		*destination++ = character;
	}
}

static size_t ssn_internal_get_utf8_directory_part_length_from_path(size_t path_length, const char* path)
{
	if (path_length)
	{
		size_t index = path_length - 1;
		for (size_t root_length = ssn_internal_utf8_root_directory_length(path_length, path); index != root_length; --index)
			if (path[index] == '\\' || path[index] == '/')
				return index;
	}
	return 0;
}

static size_t ssn_internal_win32_get_utf16_directory_part_length_from_path(size_t path_length, const WCHAR* path)
{
	if (path_length)
	{
		size_t index = path_length - 1;
		for (size_t root_length = ssn_internal_win32_utf16_root_directory_length(path_length, path); index != root_length; --index)
			if (path[index] == L'\\' || path[index] == L'/')
				return index;
	}
	return 0;
}

static int ssn_internal_win32_encode_file_path(size_t path_size, const char* path, size_t* required_buffer_size_address, size_t buffer_size, WCHAR* buffer)
{
	const size_t int_max_value = (size_t)(((unsigned int)~0) >> 1);

	if (!path_size || path_size > int_max_value)
		return EINVAL;

	size_t native_path_length = (size_t)MultiByteToWideChar(CP_UTF8, 0, path, (int)path_size, 0, 0);
	if (!native_path_length)
		return EINVAL;

	if (native_path_length > 0x7FFF)
		return ENAMETOOLONG;

	WCHAR* native_path = (WCHAR*)_alloca((native_path_length + 1) * sizeof(WCHAR));
	if ((size_t)MultiByteToWideChar(CP_UTF8, 0, path, (int)path_size, native_path, (int)native_path_length) != native_path_length)
		return EIO;
	native_path[native_path_length] = 0;

	size_t root_directory_native_path_length = ssn_internal_win32_utf16_root_directory_length(native_path_length, native_path);
	if (root_directory_native_path_length)
	{
		if ((native_path_length <= (MAX_PATH - 1)) || (native_path_length > 3 && native_path[0] == L'\\' && native_path[1] == L'\\' && native_path[2] == L'?' && native_path[3] == L'\\'))
		{
			size_t original_native_path_size = native_path_length * sizeof(WCHAR);
			*required_buffer_size_address = original_native_path_size;
			if (buffer_size >= original_native_path_size)
			{
				ssn_internal_win32_copy_utf16_string(native_path_length, buffer, native_path);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else if (native_path_length > 2 && native_path[0] != L'\\' && native_path[0] != L'/' && native_path[1] == L':' && (native_path[2] == L'\\' || native_path[2] == L'/'))
		{
			size_t local_native_path_size = (4 + native_path_length) * sizeof(WCHAR);
			*required_buffer_size_address = local_native_path_size;
			if (buffer_size >= local_native_path_size)
			{
				ssn_internal_win32_copy_utf16_string(4, buffer, L"\\\\?\\");
				ssn_internal_win32_copy_utf16_string_to_extended_path(native_path_length, buffer + 4, native_path);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else if (native_path_length > 2 && (native_path[0] == L'\\' || native_path[0] == L'/') && (native_path[1] == L'\\' || native_path[1] == L'/') && (native_path[2] != L'\\' && native_path[2] != L'/'))
		{
			size_t network_native_path_size = (8 + (native_path_length - 2)) * sizeof(WCHAR);
			*required_buffer_size_address = network_native_path_size;
			if (buffer_size >= network_native_path_size)
			{
				ssn_internal_win32_copy_utf16_string(8, buffer, L"\\\\?\\UNC\\");
				ssn_internal_win32_copy_utf16_string_to_extended_path(native_path_length - 2, buffer + 8, native_path + 2);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else
			return EINVAL;
	}

	native_path_length = (size_t)GetFullPathNameW(native_path, 0, 0, 0) - 1;
	if (native_path_length == (size_t)~0 || !native_path_length)
		return EIO;

	if (native_path_length > 0x7FFF)
		return ENAMETOOLONG;

	WCHAR* absolute_native_path = (WCHAR*)_alloca((native_path_length + 1) * sizeof(WCHAR));
	if ((size_t)GetFullPathNameW(native_path, (DWORD)(native_path_length + 1), absolute_native_path, 0) != native_path_length)
		return EIO;

	root_directory_native_path_length = ssn_internal_win32_utf16_root_directory_length(native_path_length, absolute_native_path);
	if (!root_directory_native_path_length)
		return EIO;

	if ((native_path_length <= (MAX_PATH - 1)) || (native_path_length > 3 && absolute_native_path[0] == L'\\' && absolute_native_path[1] == L'\\' && absolute_native_path[2] == L'?' && absolute_native_path[3] == L'\\'))
	{
		size_t original_native_path_size = native_path_length * sizeof(WCHAR);
		*required_buffer_size_address = original_native_path_size;
		if (buffer_size >= original_native_path_size)
		{
			ssn_internal_win32_copy_utf16_string(native_path_length, buffer, absolute_native_path);
			return 0;
		}
		else
			return ENOBUFS;
	}
	else if (native_path_length > 2 && absolute_native_path[0] != L'\\' && absolute_native_path[0] != L'/' && absolute_native_path[1] == L':' && (absolute_native_path[2] == L'\\' || absolute_native_path[2] == L'/'))
	{
		size_t local_native_path_size = (4 + native_path_length) * sizeof(WCHAR);
		*required_buffer_size_address = local_native_path_size;
		if (buffer_size >= local_native_path_size)
		{
			ssn_internal_win32_copy_utf16_string(4, buffer, L"\\\\?\\");
			ssn_internal_win32_copy_utf16_string_to_extended_path(native_path_length, buffer + 4, absolute_native_path);
			return 0;
		}
		else
			return ENOBUFS;
	}
	else if (native_path_length > 2 && (absolute_native_path[0] == L'\\' || absolute_native_path[0] == L'/') && (absolute_native_path[1] == L'\\' || absolute_native_path[1] == L'/') && absolute_native_path[2] != L'\\' && absolute_native_path[2] != L'/')
	{
		size_t network_native_path_size = (8 + (native_path_length - 2)) * sizeof(WCHAR);
		*required_buffer_size_address = network_native_path_size;
		if (buffer_size >= network_native_path_size)
		{
			ssn_internal_win32_copy_utf16_string(8, buffer, L"\\\\?\\UNC\\");
			ssn_internal_win32_copy_utf16_string_to_extended_path(native_path_length - 2, buffer + 8, absolute_native_path + 2);
			return 0;
		}
		else
			return ENOBUFS;
	}
	else
		return EINVAL;
}

static int ssn_internal_win32_decode_file_path(size_t path_size, const WCHAR* path, size_t* required_buffer_size_address, size_t buffer_size, char* buffer)
{
	const size_t int_max_value = (size_t)(((unsigned int)~0) >> 1);

	size_t native_name_length = path_size / sizeof(WCHAR);
	if ((path_size % sizeof(WCHAR)) || !native_name_length || native_name_length > int_max_value)
		return EINVAL;

	if (native_name_length > 0x7FFF)
		return ENAMETOOLONG;

	size_t utf8_path_length = (size_t)WideCharToMultiByte(CP_UTF8, 0, path, (int)native_name_length, 0, 0, 0, 0);
	if (!utf8_path_length)
		return EINVAL;

	char* utf8_path = (char*)_alloca((utf8_path_length + 1));
	if ((size_t)WideCharToMultiByte(CP_UTF8, 0, path, (int)native_name_length, utf8_path, (int)utf8_path_length, 0, 0) != utf8_path_length)
		return EIO;

	if (utf8_path_length > 3 && utf8_path[0] == '\\' && utf8_path[1] == '\\' && utf8_path[2] == '?' && utf8_path[3] == '\\')
	{
		if (utf8_path_length > 5 && utf8_path[4] != '\\' && utf8_path[5] == ':' && utf8_path[6] == '\\')
		{
			size_t local_path_size = (utf8_path_length - 4);
			*required_buffer_size_address = local_path_size;
			if (buffer_size >= local_path_size)
			{
				ssn_internal_copy_utf8_string(utf8_path_length - 4, buffer, utf8_path + 4);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else if (utf8_path_length > 8 && utf8_path[4] == 'U' && utf8_path[5] == 'N' && utf8_path[6] == 'C' && utf8_path[7] == '\\' && utf8_path[8] != '\\')
		{
			size_t network_path_size = (utf8_path_length - 8) + 2;
			*required_buffer_size_address = network_path_size;
			if (buffer_size >= network_path_size)
			{
				buffer[0] = '\\';
				buffer[1] = '\\';
				ssn_internal_copy_utf8_string(utf8_path_length - 8, buffer + 2, utf8_path + 8);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else
			return EINVAL;
	}
	else
	{
		size_t original_path_size = utf8_path_length;
		*required_buffer_size_address = original_path_size;
		if (buffer_size >= original_path_size)
		{
			ssn_internal_copy_utf8_string(utf8_path_length, buffer, utf8_path);
			return 0;
		}
		else
			return ENOBUFS;
	}
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
	for (size_t root_length = ssn_internal_win32_utf16_root_directory_length(name_length, name),
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
							switch (native_error)
							{
								case ERROR_ACCESS_DENIED:
									return EACCES;
								case ERROR_INVALID_NAME:
									return EINVAL;
								case ERROR_ALREADY_EXISTS:
									return EEXIST;
								default:
									return EIO;
							}
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
				switch (native_error)
				{
					case ERROR_ACCESS_DENIED:
						return EACCES;
					case ERROR_INVALID_NAME:
						return EINVAL;
					case ERROR_ALREADY_EXISTS:
						return EEXIST;
					default:
						return EIO;
				}
			}
		}
	}
	return ENOENT;
}

int ssn_open_file(size_t name_length, const char* name, int permissions_and_flags, ssn_handle_t* handle_address)
{
	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca(native_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name + native_name_size) = 0;

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

		if ((native_error == ERROR_PATH_NOT_FOUND) && ((permissions_and_flags & (SSN_CREATE_PATH | SSN_CREATE | SSN_WRITE_PERMISION)) == (SSN_CREATE_PATH | SSN_CREATE | SSN_WRITE_PERMISION)))
		{
			size_t create_directory_undo_index;
			size_t directory_part_length = ssn_internal_win32_get_utf16_directory_part_length_from_path(native_name_size / sizeof(WCHAR), native_name);
			error = directory_part_length ? ssn_internal_win32_create_directory(directory_part_length, native_name, &create_directory_undo_index) : ENOENT;
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

int ssn_load_file(size_t name_length, const char* name, size_t buffer_size, void* buffer, size_t* file_size_address)
{
	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca(native_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name + native_name_size) = 0;

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
	error = ssn_get_file_size((HANDLE)handle, (uint64_t*)&file_size);
	if (error)
	{
		CloseHandle(handle);
		return error;
	}
#else
	SSN_FILE_API_ASSERT(sizeof(size_t) == sizeof(uint32_t));

	uint64_t native_file_size;
	error = ssn_get_file_size((HANDLE)handle, &native_file_size);
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

int ssn_allocate_and_load_file(size_t name_length, const char* name, void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t* file_size_address, void** file_data_address)
{
	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca(native_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name + native_name_size) = 0;

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
	error = ssn_get_file_size((HANDLE)handle, (uint64_t*)&file_size);
	if (error)
	{
		CloseHandle(handle);
		return error;
	}
#else
	SSN_FILE_API_ASSERT(sizeof(size_t) == sizeof(uint32_t));

	uint64_t native_file_size;
	error = ssn_get_file_size((HANDLE)handle, &native_file_size);
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

int ssn_store_file(size_t name_length, const char* name, size_t size, const void* data)
{
	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	const size_t maximum_length_from_path_extension = 6;

	// TMP name is 8 dot 3 type name
	const size_t tmp_file_name_part_length = 12;

	FILE_RENAME_INFO* native_name = (FILE_RENAME_INFO*)_alloca((size_t)(&((const FILE_RENAME_INFO*)0)->FileName) + (native_name_size + sizeof(WCHAR)) + ((native_name_size + tmp_file_name_part_length + maximum_length_from_path_extension + 1) * sizeof(WCHAR)));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name->FileName);
	if (error)
		return error;
	WCHAR* native_tmp_file_name = (WCHAR*)((uintptr_t)native_name + (size_t)(&((const FILE_RENAME_INFO*)0)->FileName) + (native_name_size + sizeof(WCHAR)));

	int native_name_length = (int)native_name_size / sizeof(WCHAR);
	
	// native_name->ReplaceIfExists = TRUE;
	// Write to flags instead to zero all unknown flags values and set ReplaceIfExists to TRUE
	native_name->Flags = 0x00000001;
	native_name->RootDirectory = 0;
	native_name->FileNameLength = (DWORD)native_name_length;
	native_name->FileName[native_name_length] = 0;

	int native_file_name_length = 0;
	while (native_file_name_length != native_name_length && native_name->FileName[(native_name_length - 1) - native_file_name_length] != '\\' && native_name->FileName[(native_name_length - 1) - native_file_name_length] != '/')
		++native_file_name_length;

	if (!native_file_name_length)
		return EINVAL;

	int native_directory_path_length = native_name_length - native_file_name_length;
	int native_tmp_directory_path_length;
	if (((native_directory_path_length + tmp_file_name_part_length) <= (MAX_PATH - 1)) || (native_directory_path_length > 3 && native_name->FileName[0] == L'\\' && native_name->FileName[1] == L'\\' && native_name->FileName[2] == L'?' && native_name->FileName[3] == L'\\'))
	{
		ssn_internal_win32_copy_utf16_string((size_t)native_directory_path_length, native_tmp_file_name, native_name->FileName);
		native_tmp_directory_path_length = native_directory_path_length;
	}
	else
	{
		if (native_directory_path_length > 2 && (native_name->FileName[0] != L'\\' && native_name->FileName[0] != L'/') && (native_name->FileName[1] == L':') && (native_name->FileName[2] == L'\\' || native_name->FileName[2] == L'/'))
		{
			ssn_internal_win32_copy_utf16_string(4, native_tmp_file_name, L"\\\\?\\");
			ssn_internal_win32_copy_utf16_string_to_extended_path(native_directory_path_length, native_tmp_file_name + 4, native_name->FileName);
			native_tmp_directory_path_length = native_directory_path_length + 4;
		}
		else if (native_directory_path_length > 2 && (native_name->FileName[0] == L'\\' || native_name->FileName[0] == L'/') && (native_name->FileName[1] == L'\\' || native_name->FileName[1] == L'/') && (native_name->FileName[2] != L'\\' && native_name->FileName[2] != L'/'))
		{
			ssn_internal_win32_copy_utf16_string(8, native_tmp_file_name, L"\\\\?\\UNC\\");
			ssn_internal_win32_copy_utf16_string_to_extended_path(native_directory_path_length - 2, native_tmp_file_name + 8, native_name->FileName + 2);
			native_tmp_directory_path_length = native_directory_path_length + 6;
		}
		else
			return EINVAL;
	}
	// Add the tmp file name part later.

	size_t create_directory_undo_index = (size_t)~0;
	HANDLE handle = INVALID_HANDLE_VALUE;
	for (int try_count = 0; handle == INVALID_HANDLE_VALUE;)
	{
		ssn_internal_win32_make_tmp_8dot3_file_name(native_tmp_file_name + native_tmp_directory_path_length, try_count);

		handle = CreateFileW(native_tmp_file_name, GENERIC_WRITE | DELETE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
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
						ssn_internal_win32_undo_create_directory((size_t)(native_tmp_directory_path_length - 1), native_tmp_file_name, create_directory_undo_index);

					return EEXIST;
				}
			}
			else if (create_native_error == ERROR_PATH_NOT_FOUND && native_tmp_directory_path_length && create_directory_undo_index == (size_t)~0)
			{
				int create_directory_error = ssn_internal_win32_create_directory((size_t)(native_tmp_directory_path_length - 1), native_tmp_file_name, &create_directory_undo_index);
				if (create_directory_error)
					return create_directory_error;
			}
			else
			{
				if (create_directory_undo_index != (size_t)~0)
					ssn_internal_win32_undo_create_directory((size_t)(native_tmp_directory_path_length - 1), native_tmp_file_name, create_directory_undo_index);

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
		DeleteFileW(native_tmp_file_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(native_tmp_directory_path_length - 1), native_tmp_file_name, create_directory_undo_index);
		return EINVAL;
	}

	if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &size, sizeof(uint64_t)))
#else
	uint64_t native_file_size = (uint64_t)size;
	if (!SetFileInformationByHandle(handle, FileEndOfFileInfo, &native_file_size, sizeof(uint64_t)))
#endif
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_file_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(native_tmp_directory_path_length - 1), native_tmp_file_name, create_directory_undo_index);
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
				DeleteFileW(native_tmp_file_name);
				if (create_directory_undo_index != (size_t)~0)
					ssn_internal_win32_undo_create_directory((size_t)(native_tmp_directory_path_length - 1), native_tmp_file_name, create_directory_undo_index);
				return EIO;
			}
		}
	}

	if (!FlushFileBuffers(handle))
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_file_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(create_directory_undo_index - 1), native_tmp_file_name, create_directory_undo_index);
		return EIO;
	}

	if (!SetFileInformationByHandle(handle, FileRenameInfo, native_name, (DWORD)((&((const FILE_RENAME_INFO*)0)->FileName) + (native_name_size + sizeof(WCHAR)))))
	{
		CloseHandle(handle);
		DeleteFileW(native_tmp_file_name);
		if (create_directory_undo_index != (size_t)~0)
			ssn_internal_win32_undo_create_directory((size_t)(create_directory_undo_index - 1), native_tmp_file_name, create_directory_undo_index);
		return EIO;
	}

	CloseHandle(handle);
	return 0;
}

int ssn_delete_file(size_t name_length, const char* name)
{
	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca(native_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name + native_name_size) = 0;

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

#ifdef SSN_IGNORE_SYSTEM_VOLUME_INFORMATION_DIRECTORY
static int ssn_internal_win32_is_directory_system_volume_information(DWORD directory_attributes, size_t directory_name_length, const WCHAR* directory_name)
{
	// "System Volume Information"
	static const WCHAR system_volume_information_directory_name[] = { L'S', L'Y', L'S', L'T', L'E', L'M', L' ', L'V', L'O', L'L', L'U', L'M', L'E', L' ', L'I', L'N', L'F', L'O', L'R', L'M', L'A', L'T', L'I', L'O', 'N' };
	const size_t system_volume_information_directory_name_length = sizeof(system_volume_information_directory_name) / sizeof(*system_volume_information_directory_name);

	if (directory_name_length && (directory_name[directory_name_length - 1] == L'\\' || directory_name[directory_name_length - 1] == L'/'))
		--directory_name_length;

	if (directory_name_length < (MAX_PATH - 1))
	{
		size_t root_directory_name_length = ssn_internal_win32_utf16_root_directory_length(directory_name_length, directory_name);
		if (root_directory_name_length && directory_name_length == (root_directory_name_length + system_volume_information_directory_name_length))
		{
			int directory_name_match = 1;
			for (size_t i = 0; directory_name_match && i != system_volume_information_directory_name_length; ++i)
			{
				WCHAR character = directory_name[root_directory_name_length + i];
				// Convert to uppercase
				character = ((character < 0x61) || (character > 0x7A)) ? character : (character - 0x20);
				directory_name_match = (int)(character == system_volume_information_directory_name[i]);
			}
			if (directory_name_match)
			{
				if ((directory_attributes != INVALID_FILE_ATTRIBUTES) && ((directory_attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM)) == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM)))
					return 1;
			}
		}
	}

	return 0;
}
#endif

static size_t ssn_internal_win32_get_allocation_granularity()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return (size_t)(system_info.dwAllocationGranularity > system_info.dwPageSize ? system_info.dwAllocationGranularity : system_info.dwPageSize);
}

#define SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY 0x0001

typedef struct ssn_internal_win32_object_t
{
	uint16_t flags;
	uint16_t previous_name_length;
	uint16_t name_length;
	WCHAR name[0];
} ssn_internal_win32_object_t;

#define SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_EXPONENT_LIMIT 7
#define SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY 0x20000

typedef struct ssn_internal_win32_object_list_t
{
	size_t size;
	size_t used_size;
	size_t allocation_granularity;
	size_t object_count;
	ssn_internal_win32_object_t* first;
	ssn_internal_win32_object_t* last;
} ssn_internal_win32_object_list_t;

static ssn_internal_win32_object_list_t* ssn_internal_win32_allocate_object_list()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	size_t initial_size = (SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY + ((size_t)system_info.dwAllocationGranularity - 1)) & ~((size_t)system_info.dwAllocationGranularity - 1);

	ssn_internal_win32_object_list_t* list = (ssn_internal_win32_object_list_t*)VirtualAlloc(0, initial_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!list)
		return 0;

	list->size = initial_size;
	list->used_size = sizeof(ssn_internal_win32_object_list_t);
	list->allocation_granularity = (size_t)system_info.dwAllocationGranularity;
	list->object_count = 0;
	list->first = 0;
	list->last = 0;

	return list;
}

static int ssn_internal_win32_reallocate_object_list(ssn_internal_win32_object_list_t** object_list_address, size_t required_size)
{
	ssn_internal_win32_object_list_t* list = *object_list_address;
	size_t size = list->size;
	if (required_size > size)
	{
		size_t new_size = size;
		while (new_size < SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY && new_size < required_size)
			new_size <<= 1;
		while (new_size < required_size)
			new_size += SSN_INTERNAL_WIN32_OBJECT_BLOCK_ALLOCATION_GRANULARITY;

		ssn_internal_win32_object_list_t* new_list = (ssn_internal_win32_object_list_t*)VirtualAlloc(0, new_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (new_list)
		{
			new_list->size = new_size;
			new_list->used_size = list->used_size;
			new_list->allocation_granularity = list->allocation_granularity;
			new_list->object_count = list->object_count;
			new_list->first = (ssn_internal_win32_object_t*)((uintptr_t)new_list + (uintptr_t)list->first - (uintptr_t)list);
			new_list->last = (ssn_internal_win32_object_t*)((uintptr_t)new_list + (uintptr_t)list->last - (uintptr_t)list);
			for (uint8_t* source = (uint8_t*)((uintptr_t)list + sizeof(ssn_internal_win32_object_list_t)), *source_end = (uint8_t*)((uintptr_t)list + list->used_size), *destination = (uint8_t*)((uintptr_t)new_list + sizeof(ssn_internal_win32_object_list_t)); source != source_end; ++source, ++destination)
				*destination = *source;
			VirtualFree(list, 0, MEM_RELEASE);
			*object_list_address = new_list;
			return 1;
		}
		else
			return 0;
	}
	else
		return 1;
}

static void ssn_internal_win32_deallocate_object_list(ssn_internal_win32_object_list_t* object_list)
{
	VirtualFree(object_list, 0, MEM_RELEASE);
}

static size_t ssn_internal_win32_list_directory_extend_path_in_place(size_t path_length, WCHAR* path_buffer)
{
	if ((path_length >= 2) && (path_buffer[0] != L'\\' && path_buffer[0] != L'/') && (path_buffer[1] == L':'))
	{
		if (4 + path_length > 0x7FFD)
			return 4 + path_length;
		for (WCHAR* i = path_buffer + path_length; i != path_buffer; --i)
		{
			WCHAR move_character = *i;
			if (move_character == L'/')
				move_character = L'\\';
			*(i + 4) = move_character;
		}
		path_buffer[0] = L'\\';
		path_buffer[1] = L'\\';
		path_buffer[2] = L'?';
		path_buffer[3] = L'\\';
		return 4 + path_length;
	}
	else if ((path_length >= 2) && (path_buffer[0] == L'\\' || path_buffer[0] == L'/') && (path_buffer[1] == L'\\' || path_buffer[1] == L'/'))
	{
		if (6 + path_length > 0x7FFD)
			return 6 + path_length;
		for (WCHAR* e = path_buffer + 2, * i = path_buffer + path_length; i != e; --i)
		{
			WCHAR move_character = *i;
			if (move_character == L'/')
				move_character = L'\\';
			*(i + 6) = move_character;
		}
		path_buffer[0] = L'\\';
		path_buffer[1] = L'\\';
		path_buffer[2] = L'?';
		path_buffer[3] = L'\\';
		path_buffer[4] = L'U';
		path_buffer[5] = L'N';
		path_buffer[6] = L'C';
		path_buffer[7] = L'\\';
		return 6 + path_length;
	}
	else
		return (size_t)~0;
}

static int ssn_internal_win32_list_directory(size_t base_name_length, WCHAR* name_buffer, ssn_internal_win32_object_list_t** object_list_address)
{
	const size_t object_header_size = (size_t)(&((const ssn_internal_win32_object_t*)0)->name);

	if (!base_name_length)
		return EINVAL;

	if (base_name_length > 0x7FFD)
		return ENAMETOOLONG;

	ssn_internal_win32_object_list_t* list = ssn_internal_win32_allocate_object_list();
	if (!list)
		return ENOMEM;

	for (size_t current_directory_name_length = base_name_length, list_offset = (size_t)~0, index = (size_t)~0; index != list->object_count;)
	{
		if (current_directory_name_length && (name_buffer[current_directory_name_length - 1] == L'\\' || name_buffer[current_directory_name_length - 1] == L'/'))
			--current_directory_name_length;
		int current_directory_name_is_extended = (current_directory_name_length > 3) && (name_buffer[0] == L'\\' && name_buffer[1] == L'\\' && name_buffer[2] == L'?' && name_buffer[3] == L'\\');
		if (((current_directory_name_length + 2) > (MAX_PATH - 1)) && !current_directory_name_is_extended)
		{
			current_directory_name_is_extended = 1;
			current_directory_name_length = ssn_internal_win32_list_directory_extend_path_in_place(current_directory_name_length, name_buffer);
			int path_extend_error;
			if (current_directory_name_length == (size_t)~0)
				path_extend_error = EINVAL;
			else if (current_directory_name_length > 0x7FFD)
				path_extend_error = ENAMETOOLONG;
			else
				path_extend_error = 0;
			if (path_extend_error)
			{
				ssn_internal_win32_deallocate_object_list(list);
				return path_extend_error;
			}
		}
		name_buffer[current_directory_name_length] = L'\\';
		name_buffer[current_directory_name_length + 1] = L'*';
		name_buffer[current_directory_name_length + 2] = 0;
		WIN32_FIND_DATAW find_data;
		HANDLE find_handle = FindFirstFileExW(name_buffer, FindExInfoBasic, &find_data, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
		DWORD native_find_error;
		if (find_handle == INVALID_HANDLE_VALUE)
		{
			native_find_error = GetLastError();
			ssn_internal_win32_deallocate_object_list(list);
			switch (native_find_error)
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
		native_find_error = 0;

		while (!native_find_error)
		{
			size_t file_name_length = ssn_internal_win32_utf16_string_length(find_data.cFileName);
			if (current_directory_name_length + 1 + file_name_length > 0x7FFF)
			{
				FindClose(find_handle);
				ssn_internal_win32_deallocate_object_list(list);
				return ENAMETOOLONG;
			}

			int add_find = !(file_name_length == 1 && find_data.cFileName[0] == '.') && !(file_name_length == 2 && find_data.cFileName[0] == '.' && find_data.cFileName[1] == '.');
#ifdef SSN_IGNORE_SYSTEM_FILES
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
				add_find = 0;
#else
#ifdef SSN_IGNORE_SYSTEM_VOLUME_INFORMATION_DIRECTORY
			if (add_find && (find_data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM)) == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM))
			{
				int temporary_extended_current_directory_name = (current_directory_name_length + 1 + file_name_length) > (MAX_PATH - 1) && !current_directory_name_is_extended;
				if (temporary_extended_current_directory_name)
				{
					size_t extended_current_directory_name_length = ssn_internal_win32_list_directory_extend_path_in_place(current_directory_name_length, name_buffer);
					int path_extend_error;
					if (extended_current_directory_name_length == (size_t)~0)
						path_extend_error = EINVAL;
					else if (extended_current_directory_name_length > 0x7FFD)
						path_extend_error = ENAMETOOLONG;
					else
						path_extend_error = 0;
					if (path_extend_error)
					{
						FindClose(find_handle);
						ssn_internal_win32_deallocate_object_list(list);
						return path_extend_error;
					}
				}
				add_find = !ssn_internal_win32_is_directory_system_volume_information(find_data.dwFileAttributes, file_name_length, name_buffer);
				if (temporary_extended_current_directory_name)
				{
					if ((name_buffer[4] == L'U' || name_buffer[4] == L'u') && (name_buffer[5] == L'N' || name_buffer[5] == L'n') && (name_buffer[6] == L'C' || name_buffer[6] == L'c') && name_buffer[7] == L'\\')
					{
						name_buffer[0] = L'\\';
						name_buffer[1] = L'\\';
						for (WCHAR* l = name_buffer + current_directory_name_length, * i = name_buffer + 2; i != l; ++i)
							*i = *(i + 6);
					}
					else
					{
						for (WCHAR* l = name_buffer + current_directory_name_length, *i = name_buffer; i != l; ++i)
							*i = *(i + 4);
					}
				}
			}
#endif // SSN_IGNORE_SYSTEM_VOLUME_INFORMATION_DIRECTORY
#endif // SSN_IGNORE_SYSTEM_FILES
			if (add_find)
			{
				size_t required_extansion_expansion_length = 0;
				if (!current_directory_name_is_extended && ((current_directory_name_length + 1 + file_name_length) > (MAX_PATH - 1)))
				{
					if ((name_buffer[0] == '\\' || name_buffer[0] == '/') && (name_buffer[1] == '\\' || name_buffer[1] == '/'))
						required_extansion_expansion_length = 6;
					else
						required_extansion_expansion_length = 4;
				}
				size_t object_size = (object_header_size + ((required_extansion_expansion_length + current_directory_name_length + 1 + file_name_length) * sizeof(WCHAR)));
				size_t required_list_size = list->used_size + object_size;
				if (required_list_size > list->size)
				{
					if (!ssn_internal_win32_reallocate_object_list(&list, required_list_size))
					{
						ssn_internal_win32_deallocate_object_list(list);
						return ENOMEM;
					}
				}

				ssn_internal_win32_object_t* object = (ssn_internal_win32_object_t*)((uintptr_t)list + list->used_size);
				list->used_size += object_size;

				object->flags = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY : 0;
				object->previous_name_length = list->object_count ? list->last->name_length : 0;
				object->name_length = (uint16_t)(required_extansion_expansion_length + current_directory_name_length + 1 + file_name_length);
				WCHAR* name = object->name;
				if (required_extansion_expansion_length)
				{
					name[0] = L'\\';
					name[1] = L'\\';
					name[2] = L'?';
					name[3] = L'\\';
					if (required_extansion_expansion_length == 4)
					{
						ssn_internal_win32_copy_utf16_string_to_extended_path(current_directory_name_length, name + 4, name_buffer);
						*(name + 4 + current_directory_name_length) = L'\\';
						ssn_internal_win32_copy_utf16_string(file_name_length, name + 4 + current_directory_name_length + 1, find_data.cFileName);
					}
					else
					{
#ifdef _MSC_VER
						__assume(required_extansion_expansion_length == 6);
#endif // _MSC_VER
						name[4] = L'U';
						name[5] = L'N';
						name[6] = L'C';
						name[7] = L'\\';
						ssn_internal_win32_copy_utf16_string_to_extended_path(current_directory_name_length - 2, name + 8, name_buffer + 2);
						*(name + 6 + current_directory_name_length) = L'\\';
						ssn_internal_win32_copy_utf16_string(file_name_length, name + 6 + current_directory_name_length + 1, find_data.cFileName);
					}
				}
				else
				{
					ssn_internal_win32_copy_utf16_string(current_directory_name_length, name, name_buffer);
					*(name + current_directory_name_length) = L'\\';
					ssn_internal_win32_copy_utf16_string(file_name_length, name + current_directory_name_length + 1, find_data.cFileName);
				}

				list->last = object;
				if (!list->object_count)
					list->first = object;
				list->object_count++;
			}

			native_find_error = FindNextFileW(find_handle, &find_data) ? 0 : GetLastError();
		}

		FindClose(find_handle);
		if (native_find_error != ERROR_NO_MORE_FILES)
		{
			ssn_internal_win32_deallocate_object_list(list);
			return EIO;
		}

		// Note: (size_t)~0 + 1 = 0
		++index;
		list_offset = (list_offset != (size_t)~0) ? (list_offset + object_header_size + ((size_t)((ssn_internal_win32_object_t*)((uintptr_t)list + list_offset))->name_length * sizeof(WCHAR))) : (size_t)((uintptr_t)list->first - (uintptr_t)list);
		while (index != list->object_count && !(((ssn_internal_win32_object_t*)((uintptr_t)list + list_offset))->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY))
		{
			++index;
			list_offset += (object_header_size + ((size_t)((ssn_internal_win32_object_t*)((uintptr_t)list + list_offset))->name_length * sizeof(WCHAR)));
		}
		if (index != list->object_count)
		{
			ssn_internal_win32_object_t* current_directory = (ssn_internal_win32_object_t*)((uintptr_t)list + list_offset);
			current_directory_name_length = (size_t)current_directory->name_length;
			for (WCHAR* source = current_directory->name, * source_end = current_directory->name + current_directory_name_length, * destination = name_buffer; source != source_end; ++source, ++destination)
				*destination = *source;
		}
	}

	*object_list_address = list;
	return 0;
}

int ssn_list_directory(size_t name_length, const char* name, size_t buffer_size, ssn_file_entry_t* buffer, size_t* required_buffer_size_address, size_t* directory_count_address, size_t* file_count_address)
{
	const size_t object_header_size = (size_t)(&((const ssn_internal_win32_object_t*)0)->name);

	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name_buffer = (WCHAR*)_alloca((0x7FFF + 1) * sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name_buffer);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name_buffer + native_name_size) = 0;

	ssn_internal_win32_object_list_t* object_list;
	error = ssn_internal_win32_list_directory(native_name_size / sizeof(WCHAR), native_name_buffer, &object_list);
	if (error)
		return error;

	size_t directory_count = 0;
	size_t file_count = 0;
	size_t text_size = 0;
	size_t object_utf8_name_size;
	ssn_internal_win32_object_t* object = object_list->first;
	for (size_t n = object_list->object_count, i = 0; i != n; object = (ssn_internal_win32_object_t*)((uintptr_t)object + object_header_size + ((size_t)object->name_length * sizeof(WCHAR))), ++i)
	{
		error = ssn_internal_win32_decode_file_path((size_t)object->name_length * sizeof(WCHAR), object->name, &object_utf8_name_size, 0, 0);
		if (error && error != ENOBUFS)
		{
			ssn_internal_win32_deallocate_object_list(object_list);
			return error;
		}
		text_size += object_utf8_name_size + 1;
		if (object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY)
			++directory_count;
		else
			++file_count;
	}

	size_t required_size = ((directory_count + file_count) * sizeof(ssn_file_entry_t)) + text_size;
	*required_buffer_size_address = required_size;
	*directory_count_address = directory_count;
	*file_count_address = file_count;
	if (required_size > buffer_size)
	{
		ssn_internal_win32_deallocate_object_list(object_list);
		return ENOBUFS;
	}

	size_t write_offset = (directory_count + file_count) * sizeof(ssn_file_entry_t);
	object = object_list->first;
	for (size_t i = 0; i != directory_count; object = (ssn_internal_win32_object_t*)((uintptr_t)object + object_header_size + ((size_t)object->name_length * sizeof(WCHAR))))
		if (object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY)
		{
			if (write_offset == required_size)
			{
				ssn_internal_win32_deallocate_object_list(object_list);
				return EIO;
			}
			buffer[i].name = (char*)((uintptr_t)buffer + write_offset);
			error = ssn_internal_win32_decode_file_path((size_t)object->name_length * sizeof(WCHAR), object->name, &object_utf8_name_size, required_size - (write_offset + 1), buffer[i].name);
			if (error)
			{
				ssn_internal_win32_deallocate_object_list(object_list);
				return error;
			}
			buffer[i].name_length = object_utf8_name_size;
			*(buffer[i].name + object_utf8_name_size) = 0;
			write_offset += object_utf8_name_size + 1;
			++i;
		}

	object = object_list->first;
	for (size_t i = 0; i != file_count; object = (ssn_internal_win32_object_t*)((uintptr_t)object + object_header_size + ((size_t)object->name_length * sizeof(WCHAR))))
		if (!(object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY))
		{
			if (write_offset == required_size)
			{
				ssn_internal_win32_deallocate_object_list(object_list);
				return EIO;
			}
			buffer[directory_count + i].name = (char*)((uintptr_t)buffer + write_offset);
			error = ssn_internal_win32_decode_file_path((size_t)object->name_length * sizeof(WCHAR), object->name, &object_utf8_name_size, required_size - (write_offset + 1), buffer[directory_count + i].name);
			if (error)
			{
				ssn_internal_win32_deallocate_object_list(object_list);
				return error;
			}
			buffer[directory_count + i].name_length = object_utf8_name_size;
			*(buffer[directory_count + i].name + object_utf8_name_size) = 0;
			write_offset += object_utf8_name_size + 1;
			++i;
		}

	ssn_internal_win32_deallocate_object_list(object_list);
	return 0;
}

int ssn_delete_directory(size_t name_length, const char* name)
{
	const size_t object_header_size = (size_t)(&((const ssn_internal_win32_object_t*)0)->name);

	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	const size_t maximum_length_from_path_extension = 6;

	// TMP name is 8 dot 3 type name
	const size_t tmp_file_name_part_length = 12;

	// Note: Pretty large allocation 64 to 192 KiB
	WCHAR* native_name_buffer = (WCHAR*)_alloca(((0x7FFF + 1) * sizeof(WCHAR)) + (native_name_size + sizeof(WCHAR)) + (native_name_size + ((maximum_length_from_path_extension + 1 + tmp_file_name_part_length + 1) * sizeof(WCHAR*))));
	WCHAR* native_name = (WCHAR*)((uintptr_t)native_name_buffer + ((0x7FFF + 1) * sizeof(WCHAR)));
	WCHAR* native_tmp_name = (WCHAR*)((uintptr_t)native_name_buffer + ((0x7FFF + 1) * sizeof(WCHAR)) + (native_name_size + sizeof(WCHAR)));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name + native_name_size) = 0;

	int native_name_length = (int)(native_name_size / sizeof(WCHAR));
	if (ssn_internal_win32_utf16_root_directory_length((size_t)native_name_length, native_name) == (size_t)native_name_length)
		return EINVAL;

	int native_file_name_length = 0;
	while (native_file_name_length != native_name_length && native_name[(native_name_length - 1) - native_file_name_length] != '\\' && native_name[(native_name_length - 1) - native_file_name_length] != '/')
		++native_file_name_length;

	if (!native_file_name_length)
		return EINVAL;

	int native_directory_path_length = native_name_length - native_file_name_length;
	int native_tmp_directory_path_length;
	if (((native_directory_path_length + tmp_file_name_part_length) <= (MAX_PATH - 1)) || (native_directory_path_length > 3 && native_name[0] == L'\\' && native_name[1] == L'\\' && native_name[2] == L'?' && native_name[3] == L'\\'))
	{
		ssn_internal_win32_copy_utf16_string((size_t)native_directory_path_length, native_tmp_name, native_name);
		native_tmp_directory_path_length = native_directory_path_length;
	}
	else
	{
		if (native_directory_path_length > 2 && (native_name[0] != L'\\' && native_name[0] != L'/') && (native_name[1] == L':') && (native_name[2] == L'\\' || native_name[2] == L'/'))
		{
			ssn_internal_win32_copy_utf16_string(4, native_tmp_name, L"\\\\?\\");
			ssn_internal_win32_copy_utf16_string_to_extended_path(native_directory_path_length, native_tmp_name + 4, native_name);
			native_tmp_directory_path_length = native_directory_path_length + 4;
		}
		else if (native_directory_path_length > 2 && (native_name[0] == L'\\' || native_name[0] == L'/') && (native_name[1] == L'\\' || native_name[1] == L'/') && (native_name[2] != L'\\' && native_name[2] != L'/'))
		{
			ssn_internal_win32_copy_utf16_string(8, native_tmp_name, L"\\\\?\\UNC\\");
			ssn_internal_win32_copy_utf16_string_to_extended_path(native_directory_path_length - 2, native_tmp_name + 8, native_name + 2);
			native_tmp_directory_path_length = native_directory_path_length + 6;
		}
		else
			return EINVAL;
	}

	DWORD native_error = ERROR_FILE_EXISTS;
	for (int try_count = 0; native_error == ERROR_FILE_EXISTS; ++try_count)
	{
		if (try_count == SSN_INTERNAL_MAKE_TMP_FILE_NAME_TRY_LIMIT)
			return EEXIST;
		ssn_internal_win32_make_tmp_8dot3_file_name(native_tmp_name + native_tmp_directory_path_length, try_count);
		native_error = MoveFileW(native_name, native_tmp_name) ? 0 : GetLastError();
		if (native_error && native_error != ERROR_FILE_EXISTS)
		{
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
	}

	ssn_internal_win32_copy_utf16_string((size_t)(native_tmp_directory_path_length + tmp_file_name_part_length + 1), native_name_buffer, native_tmp_name);
	ssn_internal_win32_object_list_t* object_list;
	error = ssn_internal_win32_list_directory((size_t)(native_tmp_directory_path_length + tmp_file_name_part_length), native_name_buffer, &object_list);
	if (error)
	{
		MoveFileW(native_tmp_name, native_name);
		return error;
	}

	native_error = 0;
	ssn_internal_win32_object_t* object = object_list->last;
	for (size_t n = object_list->object_count, i = 0; !native_error && i != n; object = (ssn_internal_win32_object_t*)((uintptr_t)object - (object_header_size + ((size_t)object->previous_name_length * sizeof(WCHAR)))), ++i)
	{
		ssn_internal_win32_copy_utf16_string((size_t)object->name_length, native_name_buffer, object->name);
		*(native_name_buffer + (size_t)object->name_length) = 0;
		if (object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY)
			native_error = RemoveDirectoryW(native_name_buffer) ? 0 : GetLastError();
		else
			native_error = DeleteFileW(native_name_buffer) ? 0 : GetLastError();
	}
	if (!native_error)
		native_error = RemoveDirectoryW(native_tmp_name) ? 0 : GetLastError();

	if (native_error)
	{
		// Very bad partial delete error. Can not complete operation or recover to old state.
		MoveFileW(native_tmp_name, native_name);
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

int ssn_move_file(size_t old_name_length, const char* old_name, size_t new_name_length, const char* new_name)
{
	if (old_name_length < 1 || new_name_length < 1)
		return EINVAL;

	size_t native_old_name_size;
	int error = ssn_internal_win32_encode_file_path(old_name_length, old_name, &native_old_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;
	size_t native_new_name_size;
	error = ssn_internal_win32_encode_file_path(new_name_length, new_name, &native_new_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_old_name_size > 0x7FFF * sizeof(WCHAR) || native_new_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_old_name = (WCHAR*)_alloca(native_old_name_size + sizeof(WCHAR) + native_new_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(old_name_length, old_name, &native_old_name_size, native_old_name_size, native_old_name);
	if (error)
		return error;
	WCHAR* native_new_name = (WCHAR*)((uintptr_t)native_old_name + native_old_name_size + sizeof(WCHAR));
	*(WCHAR*)((uintptr_t)native_old_name + native_old_name_size) = 0;
	error = ssn_internal_win32_encode_file_path(new_name_length, new_name, &native_new_name_size, native_new_name_size, native_new_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_new_name + native_new_name_size) = 0;

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

int ssn_move_directory(size_t old_name_length, const char* old_name, size_t new_name_length, const char* new_name)
{
	if (old_name_length < 1 || new_name_length < 1)
		return EINVAL;

	size_t native_old_name_size;
	int error = ssn_internal_win32_encode_file_path(old_name_length, old_name, &native_old_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;
	size_t native_new_name_size;
	error = ssn_internal_win32_encode_file_path(new_name_length, new_name, &native_new_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_old_name_size > 0x7FFF * sizeof(WCHAR) || native_new_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_old_name = (WCHAR*)_alloca(native_old_name_size + sizeof(WCHAR) + native_new_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(old_name_length, old_name, &native_old_name_size, native_old_name_size, native_old_name);
	if (error)
		return error;
	WCHAR* native_new_name = (WCHAR*)((uintptr_t)native_old_name + native_old_name_size + sizeof(WCHAR));
	*(WCHAR*)((uintptr_t)native_old_name + native_old_name_size) = 0;
	error = ssn_internal_win32_encode_file_path(new_name_length, new_name, &native_new_name_size, native_new_name_size, native_new_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_new_name + native_new_name_size) = 0;

	size_t native_old_name_length = native_old_name_size / sizeof(WCHAR);
	size_t native_path_root_directory_length = ssn_internal_win32_utf16_root_directory_length(native_old_name_length, native_old_name);
	if (!native_path_root_directory_length || native_path_root_directory_length == native_old_name_length)
		return EINVAL;
	if (native_old_name[native_old_name_length - 1] == L'\\' || native_old_name[native_old_name_length - 1] == L'/')
		native_old_name[--native_old_name_length] = 0;

	size_t native_new_name_length = native_new_name_size / sizeof(WCHAR);
	native_path_root_directory_length = ssn_internal_win32_utf16_root_directory_length(native_new_name_length, native_new_name);
	if (!native_path_root_directory_length || native_path_root_directory_length == native_new_name_length)
		return EINVAL;
	if (native_new_name[native_new_name_length - 1] == L'\\' || native_new_name[native_new_name_length - 1] == L'/')
		native_new_name[--native_new_name_length] = 0;

	if (!MoveFileW(native_old_name, native_new_name))
	{
		DWORD native_error = GetLastError();
		if (native_error != ERROR_PATH_NOT_FOUND)
			switch (native_error)
			{
				case ERROR_FILE_NOT_FOUND:
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

		size_t create_directory_undo_index;
		size_t directory_part_length = ssn_internal_win32_get_utf16_directory_part_length_from_path(native_new_name_length, native_new_name);
		error = directory_part_length ? ssn_internal_win32_create_directory(directory_part_length, native_new_name, &create_directory_undo_index) : ENOENT;
		if (error)
			return error;

		if (!MoveFileW(native_old_name, native_new_name))
		{
			native_error = GetLastError();
			ssn_internal_win32_undo_create_directory(directory_part_length, native_new_name, create_directory_undo_index);
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
	}

	return 0;
}

int ssn_create_directory(size_t name_length, const char* name)
{
	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name = (WCHAR*)_alloca(native_name_size + sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name + native_name_size) = 0;

	size_t unused_directory_create_undo_index;
	error = ssn_internal_win32_create_directory(name_length, native_name, &unused_directory_create_undo_index);
	return error;
}

int ssn_allocate_and_list_directory(size_t name_length, const char* name, void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t* entry_table_size_address, ssn_file_entry_t** entry_table_address, size_t* directory_count_address, size_t* file_count_address)
{
	const size_t object_header_size = (size_t)(&((const ssn_internal_win32_object_t*)0)->name);

	if (name_length < 1)
		return EINVAL;

	size_t native_name_size;
	int error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	if (native_name_size > 0x7FFF * sizeof(WCHAR))
		return ENAMETOOLONG;

	WCHAR* native_name_buffer = (WCHAR*)_alloca((0x7FFF + 1) * sizeof(WCHAR));
	error = ssn_internal_win32_encode_file_path(name_length, name, &native_name_size, native_name_size, native_name_buffer);
	if (error)
		return error;
	*(WCHAR*)((uintptr_t)native_name_buffer + native_name_size) = 0;

	ssn_internal_win32_object_list_t* object_list;
	error = ssn_internal_win32_list_directory(native_name_size / sizeof(WCHAR), native_name_buffer, &object_list);
	if (error)
		return error;

	size_t directory_count = 0;
	size_t file_count = 0;
	size_t text_size = 0;
	size_t object_utf8_name_size;
	ssn_internal_win32_object_t* object = object_list->first;
	for (size_t n = object_list->object_count, i = 0; i != n; object = (ssn_internal_win32_object_t*)((uintptr_t)object + object_header_size + ((size_t)object->name_length * sizeof(WCHAR))), ++i)
	{
		error = ssn_internal_win32_decode_file_path((size_t)object->name_length * sizeof(WCHAR), object->name, &object_utf8_name_size, 0, 0);
		if (error && error != ENOBUFS)
		{
			ssn_internal_win32_deallocate_object_list(object_list);
			return error;
		}
		text_size += object_utf8_name_size + 1;
		if (object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY)
			++directory_count;
		else
			++file_count;
	}

	size_t required_size = ((directory_count + file_count) * sizeof(ssn_file_entry_t)) + text_size;
	ssn_file_entry_t* buffer = allocator_procedure(allocator_context, required_size);
	if (!buffer)
	{
		ssn_internal_win32_deallocate_object_list(object_list);
		return ENOBUFS;
	}

	size_t write_offset = (directory_count + file_count) * sizeof(ssn_file_entry_t);
	object = object_list->first;
	for (size_t i = 0; i != directory_count; object = (ssn_internal_win32_object_t*)((uintptr_t)object + object_header_size + ((size_t)object->name_length * sizeof(WCHAR))))
		if (object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY)
		{
			if (write_offset == required_size)
			{
				deallocator_procedure(allocator_context, required_size, buffer);
				ssn_internal_win32_deallocate_object_list(object_list);
				return EIO;
			}
			buffer[i].name = (char*)((uintptr_t)buffer + write_offset);
			error = ssn_internal_win32_decode_file_path((size_t)object->name_length * sizeof(WCHAR), object->name, &object_utf8_name_size, required_size - (write_offset + 1), buffer[i].name);
			if (error)
			{
				deallocator_procedure(allocator_context, required_size, buffer);
				ssn_internal_win32_deallocate_object_list(object_list);
				return error;
			}
			buffer[i].name_length = object_utf8_name_size;
			*(buffer[i].name + object_utf8_name_size) = 0;
			write_offset += object_utf8_name_size + 1;
			++i;
		}

	object = object_list->first;
	for (size_t i = 0; i != file_count; object = (ssn_internal_win32_object_t*)((uintptr_t)object + object_header_size + ((size_t)object->name_length * sizeof(WCHAR))))
		if (!(object->flags & SSN_INTERNAL_WIN32_OBJECT_FLAG_DIRECTORY))
		{
			if (write_offset == required_size)
			{
				deallocator_procedure(allocator_context, required_size, buffer);
				ssn_internal_win32_deallocate_object_list(object_list);
				return EIO;
			}
			buffer[directory_count + i].name = (char*)((uintptr_t)buffer + write_offset);
			error = ssn_internal_win32_decode_file_path((size_t)object->name_length * sizeof(WCHAR), object->name, &object_utf8_name_size, required_size - (write_offset + 1), buffer[directory_count + i].name);
			if (error)
			{
				deallocator_procedure(allocator_context, required_size, buffer);
				ssn_internal_win32_deallocate_object_list(object_list);
				return error;
			}
			buffer[directory_count + i].name_length = object_utf8_name_size;
			*(buffer[directory_count + i].name + object_utf8_name_size) = 0;
			write_offset += object_utf8_name_size + 1;
			++i;
		}

	ssn_internal_win32_deallocate_object_list(object_list);
	*entry_table_size_address = required_size;
	*entry_table_address = buffer;
	*directory_count_address = directory_count;
	*file_count_address = file_count;
	return 0;
}

int ssn_get_executable_file_name(size_t buffer_size, char* buffer, size_t* file_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca((0x7FFF + 1) * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x7FFF + 1);
	if (!native_name_length || native_name_length > 0x7FFF)
		return EIO;

	return ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, file_name_length_address, buffer_size, buffer);
}

int ssn_allocate_and_get_executable_file_name(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, char** file_name_address, size_t* file_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca((0x7FFF + 1) * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x7FFF + 1);
	if (!native_name_length || native_name_length > 0x7FFF)
		return EIO;

	int error = ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, file_name_length_address, 0, 0);
	if (error != ENOBUFS)
		return error ? error : EIO;

	char* file_name = (char*)allocator_procedure(allocator_context, *file_name_length_address);
	if (!file_name)
		return ENOBUFS;

	error = ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, file_name_length_address, *file_name_length_address, file_name);
	if (error)
	{
		deallocator_procedure(allocator_context, *file_name_length_address, file_name);
		return error;
	}

	return 0;
}

int ssn_get_program_directory(size_t buffer_size, char* buffer, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x7FFF + 1);
	if (!native_name_length || native_name_length > 0x7FFF)
		return EIO;

	size_t directory_part_length = ssn_internal_win32_get_utf16_directory_part_length_from_path(native_name_length, native_name);
	if (!directory_part_length)
		return EIO;

	return ssn_internal_win32_decode_file_path(directory_part_length * sizeof(WCHAR), native_name, directory_name_length_address, buffer_size, buffer);
}

int ssn_allocate_and_get_program_directory(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, char** directory_name_address, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca(0x8000 * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetModuleFileNameW(0, native_name, 0x7FFF + 1);
	if (!native_name_length || native_name_length > 0x7FFF)
		return EIO;

	size_t directory_part_length = ssn_internal_win32_get_utf16_directory_part_length_from_path(native_name_length, native_name);
	if (!directory_part_length)
		return EIO;

	size_t program_directory_name_length;
	int error = ssn_internal_win32_decode_file_path(directory_part_length * sizeof(WCHAR), native_name, &program_directory_name_length, 0, 0);
	*directory_name_length_address = program_directory_name_length;
	if (error != ENOBUFS)
		return error;

	char* buffer = allocator_procedure(allocator_context, program_directory_name_length);
	if (!buffer)
		return ENOBUFS;

	error = ssn_internal_win32_decode_file_path(directory_part_length * sizeof(WCHAR), native_name, directory_name_length_address, program_directory_name_length, buffer);
	if (error)
	{
		deallocator_procedure(allocator_context, program_directory_name_length, buffer);
		return EIO;
	}

	*directory_name_address = buffer;
	return 0;
}

int ssn_get_data_directory(size_t buffer_size, size_t sub_directory_name_length, const char* sub_directory_name, char* buffer, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca((0x7FFF + 1) * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetEnvironmentVariableW(L"LOCALAPPDATA", native_name, 0x7FFF + 1);
	
	if (!native_name_length || native_name_length > 0x7FFF)
	{
		HMODULE Userenv = LoadLibraryW(L"Userenv.dll");
		if (!Userenv)
			return ENOSYS;

		BOOL (WINAPI* Userenv_GetUserProfileDirectoryW)(HANDLE hToken, WCHAR* lpProfileDir, DWORD* lpcchSize) = (BOOL (WINAPI*)(HANDLE, WCHAR*, DWORD*))GetProcAddress(Userenv, "GetUserProfileDirectoryW");
		if (!Userenv_GetUserProfileDirectoryW)
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}

		DWORD user_diectory_name_length = 0x7FFF + 1;
		if (!Userenv_GetUserProfileDirectoryW(GetCurrentProcessToken(), native_name, &user_diectory_name_length))
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}
		native_name_length = ssn_internal_win32_utf16_string_length(native_name);

		FreeLibrary(Userenv);
	}

	if (native_name_length && (native_name[native_name_length - 1] == L'\\' || native_name[native_name_length - 1] == L'/'))
		--native_name_length;

	size_t base_directory_name_length;
	int error = ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, &base_directory_name_length, 0, 0);
	if (error != ENOBUFS)
		return error;

	size_t data_directory_name_length = base_directory_name_length + 1 + sub_directory_name_length;
	*directory_name_length_address = data_directory_name_length;
	if (data_directory_name_length > buffer_size)
		return ENOBUFS;

	error = ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, &base_directory_name_length, base_directory_name_length, buffer);
	if (error)
		return error;
	*(buffer + base_directory_name_length) = '\\';
	ssn_internal_copy_utf8_string(sub_directory_name_length, buffer + base_directory_name_length + 1, sub_directory_name);

	return 0;
}

int ssn_allocate_and_get_data_directory(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t sub_directory_name_length, const char* sub_directory_name, char** directory_name_address, size_t* directory_name_length_address)
{
	WCHAR* native_name = (WCHAR*)_alloca((0x7FFF + 1) * sizeof(WCHAR));
	size_t native_name_length = (size_t)GetEnvironmentVariableW(L"LOCALAPPDATA", native_name, 0x7FFF + 1);

	if (!native_name_length || native_name_length > 0x7FFF)
	{
		HMODULE Userenv = LoadLibraryW(L"Userenv.dll");
		if (!Userenv)
			return ENOSYS;

		BOOL(WINAPI * Userenv_GetUserProfileDirectoryW)(HANDLE hToken, WCHAR * lpProfileDir, DWORD * lpcchSize) = (BOOL(WINAPI*)(HANDLE, WCHAR*, DWORD*))GetProcAddress(Userenv, "GetUserProfileDirectoryW");
		if (!Userenv_GetUserProfileDirectoryW)
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}

		DWORD user_diectory_name_length = 0x7FFF + 1;
		if (!Userenv_GetUserProfileDirectoryW(GetCurrentProcessToken(), native_name, &user_diectory_name_length))
		{
			FreeLibrary(Userenv);
			return ENOSYS;
		}
		native_name_length = ssn_internal_win32_utf16_string_length(native_name);

		FreeLibrary(Userenv);
	}

	if (native_name_length && (native_name[native_name_length - 1] == L'\\' || native_name[native_name_length - 1] == L'/'))
		--native_name_length;

	size_t base_directory_name_length;
	int error = ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, &base_directory_name_length, 0, 0);
	if (error != ENOBUFS)
		return error;

	size_t data_directory_name_length = base_directory_name_length + 1 + sub_directory_name_length;
	*directory_name_length_address = data_directory_name_length;
	
	char* directory_name = allocator_procedure(allocator_context, data_directory_name_length);
	if (!directory_name)
		return ENOBUFS;

	error = ssn_internal_win32_decode_file_path(native_name_length * sizeof(WCHAR), native_name, &base_directory_name_length, base_directory_name_length, directory_name);
	if (error)
	{
		deallocator_procedure(allocator_context, data_directory_name_length, directory_name);
		return error;
	}
	*(directory_name + base_directory_name_length) = '\\';
	ssn_internal_copy_utf8_string(sub_directory_name_length, directory_name + base_directory_name_length + 1, sub_directory_name);

	*directory_name_address = directory_name;
	return 0;
}

int ssn_make_path(size_t parent_directory_length, const char* parent_directory, size_t sub_directory_length, const char* sub_directory, size_t buffer_size, char* buffer, size_t* directory_name_length_address)
{
	if ((parent_directory_length && (parent_directory[parent_directory_length - 1] == L'\\' || parent_directory[parent_directory_length - 1] == L'/')) &&
		(sub_directory_length || ssn_internal_utf8_root_directory_length(parent_directory_length, parent_directory) != parent_directory_length))
		--parent_directory_length;

	int add_slash = (int)(parent_directory_length && sub_directory_length);
	size_t name_length = (size_t)parent_directory_length + add_slash + sub_directory_length;
	*directory_name_length_address = name_length;
	if (name_length > buffer_size)
		return ENOBUFS;

	ssn_internal_copy_utf8_string(parent_directory_length, buffer, parent_directory);
	if (add_slash)
		*(buffer + parent_directory_length) = '\\';
	ssn_internal_copy_utf8_string(sub_directory_length, buffer + parent_directory_length + add_slash, sub_directory);
	return 0;
}

int ssn_allocate_and_make_path(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t parent_directory_length, const char* parent_directory, size_t sub_directory_length, const char* sub_directory, char** directory_name_address, size_t* directory_name_length_address)
{
	if ((parent_directory_length && (parent_directory[parent_directory_length - 1] == L'\\' || parent_directory[parent_directory_length - 1] == L'/')) &&
		(sub_directory_length || ssn_internal_utf8_root_directory_length(parent_directory_length, parent_directory) != parent_directory_length))
		--parent_directory_length;

	int add_slash = (int)(parent_directory_length && sub_directory_length);
	size_t name_length = (size_t)parent_directory_length + add_slash + sub_directory_length;
	*directory_name_length_address = name_length;

	char* buffer = allocator_procedure(allocator_context, name_length);
	if (!buffer)
		return ENOBUFS;

	ssn_internal_copy_utf8_string(parent_directory_length, buffer, parent_directory);
	if (add_slash)
		*(buffer + parent_directory_length) = '\\';
	ssn_internal_copy_utf8_string(sub_directory_length, buffer + parent_directory_length + add_slash, sub_directory);
	*directory_name_address = buffer;
	return 0;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

#ifdef __cplusplus
}
#endif // __cplusplus