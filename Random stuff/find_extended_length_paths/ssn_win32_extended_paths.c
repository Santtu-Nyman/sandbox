#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ssn_win32_extended_paths.h"

static size_t ssn_win32_internal_utf8_string_length(const char* string)
{
	const char* read = string;
	while (*read)
		++read;
	return (size_t)((uintptr_t)read - (uintptr_t)string);
}

static void ssn_win32_internal_copy_utf8_string(size_t length, char* destination, const char* source)
{
	const char* source_end = source + length;
	while (source != source_end)
		*destination++ = *source++;
}

static size_t ssn_win32_internal_utf16_string_length(const WCHAR* string)
{
	const WCHAR* read = string;
	while (*read)
		++read;
	return (size_t)((uintptr_t)read - (uintptr_t)string) / sizeof(WCHAR);
}

static void ssn_win32_internal_copy_utf16_string(size_t length, WCHAR* destination, const WCHAR* source)
{
	const WCHAR* source_end = source + length;
	while (source != source_end)
		*destination++ = *source++;
}

static size_t ssn_win32_internal_utf16_root_directory_length(size_t path_langth, const WCHAR* path)
{
	if (path_langth > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		if (path_langth > 6 && path[4] != L'\\' && path[4] != L'/' && path[5] == L':' && (path[6] == L'\\' || path[6] == L'/'))
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
		if (path_langth > 2 && path[0] != L'\\' && path[0] != L'/' && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/'))
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

static void ssn_win32_internal_copy_utf16_string_to_extended_path(size_t length, WCHAR* destination, const WCHAR* source)
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

int ssn_win32_extend_path(const WCHAR* path, size_t* required_buffer_size_address, size_t buffer_size, WCHAR* buffer)
{
	size_t path_langth = ssn_win32_internal_utf16_string_length(path);

	if (!path_langth)
		return EINVAL;

	if (path_langth > 0x7FFF)
		return ENAMETOOLONG;

	if (path_langth > 3 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		size_t original_path_size = (path_langth + 1) * sizeof(WCHAR);
		*required_buffer_size_address = original_path_size;
		if (buffer_size >= original_path_size)
		{
			ssn_win32_internal_copy_utf16_string(path_langth + 1, buffer, path);
			return 0;
		}
		else
			return ENOBUFS;
	}

	size_t root_directory_path_length = ssn_win32_internal_utf16_root_directory_length(path_langth, path);
	if (root_directory_path_length)
	{
		if (path_langth > 2 && path[0] != L'\\' && path[0] != L'/' && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/'))
		{
			size_t local_path_size = (4 + path_langth + 1) * sizeof(WCHAR);
			*required_buffer_size_address = local_path_size;
			if (buffer_size >= local_path_size)
			{
				ssn_win32_internal_copy_utf16_string(4, buffer, L"\\\\?\\");
				ssn_win32_internal_copy_utf16_string_to_extended_path(path_langth + 1, buffer + 4, path);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else if (path_langth > 2 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/') && path[2] != L'\\' && path[2] != L'/')
		{
			size_t network_path_size = (8 + (path_langth - 2) + 1) * sizeof(WCHAR);
			*required_buffer_size_address = network_path_size;
			if (buffer_size >= network_path_size)
			{
				ssn_win32_internal_copy_utf16_string(8, buffer, L"\\\\?\\UNC\\");
				ssn_win32_internal_copy_utf16_string_to_extended_path(path_langth - 1, buffer + 8, path + 2);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else
			return EINVAL;
	}

	path_langth = GetFullPathNameW(path, 0, 0, 0);
	if (!path_langth)
		return EINVAL;
	--path_langth;

	WCHAR* absolute_path = (WCHAR*)_alloca(path_langth * sizeof(WCHAR));
	if (GetFullPathNameW(path, (DWORD)(path_langth + 1), absolute_path, 0) != path_langth)
		return EIO;

	if (path_langth > 3 && absolute_path[0] == L'\\' && absolute_path[1] == L'\\' && absolute_path[2] == L'?' && absolute_path[3] == L'\\')
	{
		size_t absolute_path_size = (path_langth + 1) * sizeof(WCHAR);
		*required_buffer_size_address = absolute_path_size;
		if (buffer_size >= absolute_path_size)
		{
			ssn_win32_internal_copy_utf16_string(path_langth + 1, buffer, absolute_path);
			return 0;
		}
		else
			return ENOBUFS;
	}

	root_directory_path_length = ssn_win32_internal_utf16_root_directory_length(path_langth, absolute_path);
	if (!root_directory_path_length)
		return EIO;

	if (path_langth > 2 && absolute_path[0] != L'\\' && absolute_path[0] != L'/' && absolute_path[1] == L':' && (absolute_path[2] == L'\\' || absolute_path[2] == L'/'))
	{
		size_t local_path_size = ((4 + path_langth) + 1) * sizeof(WCHAR);
		*required_buffer_size_address = local_path_size;
		if (buffer_size >= local_path_size)
		{
			ssn_win32_internal_copy_utf16_string(4, buffer, L"\\\\?\\");
			ssn_win32_internal_copy_utf16_string_to_extended_path(path_langth + 1, buffer + 4, absolute_path);
			return 0;
		}
		else
			return ENOBUFS;
	}
	else if (path_langth > 2 && (absolute_path[0] == L'\\' || absolute_path[0] == L'/') && (absolute_path[1] == L'\\' || absolute_path[1] == L'/') && absolute_path[2] != L'\\' && absolute_path[2] != L'/')
	{
		size_t network_path_size = (8 + (path_langth - 2) + 1) * sizeof(WCHAR);
		*required_buffer_size_address = network_path_size;
		if (buffer_size >= network_path_size)
		{
			ssn_win32_internal_copy_utf16_string(8, buffer, L"\\\\?\\UNC\\");
			ssn_win32_internal_copy_utf16_string_to_extended_path(path_langth - 1, buffer + 8, absolute_path + 2);
			return 0;
		}
		else
			return ENOBUFS;
	}
	else
		return EINVAL;
}

int ssn_win32_unextend_path(const WCHAR* path, size_t* required_buffer_size_address, size_t buffer_size, WCHAR* buffer)
{
	size_t path_langth = ssn_win32_internal_utf16_string_length(path);

	if (!path_langth)
		return EINVAL;

	if (path_langth > 0x7FFF)
		return ENAMETOOLONG;

	if (path_langth > 3 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		if (path_langth > 5 && path[4] != L'\\' && path[5] == L':' && path[6] == L'\\')
		{
			size_t local_path_size = ((path_langth - 4) + 1) * sizeof(WCHAR);
			*required_buffer_size_address = local_path_size;
			if (buffer_size >= local_path_size)
			{
				ssn_win32_internal_copy_utf16_string(path_langth - 3, buffer, path + 4);
				return 0;
			}
			else
				return ENOBUFS;
		}
		else if (path_langth > 8 && path[4] == L'U' && path[5] == L'N' && path[6] == L'C' && path[7] == L'\\' && path[8] != L'\\')
		{
			size_t network_path_size = ((path_langth - 8) + 3) * sizeof(WCHAR);
			*required_buffer_size_address = network_path_size;
			if (buffer_size >= network_path_size)
			{
				buffer[0] = L'\\';
				buffer[1] = L'\\';
				ssn_win32_internal_copy_utf16_string(path_langth - 7, buffer + 2, path + 8);
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
		size_t original_path_size = (path_langth + 1) * sizeof(WCHAR);
		*required_buffer_size_address = original_path_size;
		if (buffer_size >= original_path_size)
		{
			ssn_win32_internal_copy_utf16_string(path_langth + 1, buffer, path);
			return 0;
		}
		else
			return ENOBUFS;
	}
}

int ssn_win32_extend_path_utf8(const char* path, size_t* required_buffer_size_address, size_t buffer_size, char* buffer)
{
	int path_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, 0, 0);

	if (path_length < 1)
		return EINVAL;

	if (path_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_path = (WCHAR*)_alloca((size_t)path_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, native_path, path_length) != path_length)
		return EINVAL;

	size_t native_extend_path_size;
	int error = ssn_win32_extend_path(native_path, &native_extend_path_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	WCHAR* native_extend_path = (WCHAR*)_alloca(native_extend_path_size);

	error = ssn_win32_extend_path(native_path, &native_extend_path_size, native_extend_path_size, native_extend_path);
	if (error)
		return error;

	int extend_path_length = WideCharToMultiByte(CP_UTF8, 0, native_extend_path, -1, 0, 0, 0, 0);
	if (!extend_path_length)
		return EIO;

	size_t required_buffer_size = (size_t)extend_path_length;
	*required_buffer_size_address = required_buffer_size;
	if (required_buffer_size <= buffer_size)
	{
		if (WideCharToMultiByte(CP_UTF8, 0, native_extend_path, -1, buffer, extend_path_length, 0, 0) != extend_path_length)
			return EIO;
		return 0;
	}
	else
		return ENOBUFS;
}

int ssn_win32_unextend_path_utf8(const char* path, size_t* required_buffer_size_address, size_t buffer_size, char* buffer)
{
	int path_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, 0, 0);

	if (path_length < 1)
		return EINVAL;

	if (path_length > 0x8000)
		return ENAMETOOLONG;

	WCHAR* native_path = (WCHAR*)_alloca((size_t)path_length * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, native_path, path_length) != path_length)
		return EINVAL;

	size_t native_unextend_path_size;
	int error = ssn_win32_unextend_path(native_path, &native_unextend_path_size, 0, 0);
	if (error != ENOBUFS)
		return error;

	WCHAR* native_unextend_path = (WCHAR*)_alloca(native_unextend_path_size);

	error = ssn_win32_unextend_path(native_path, &native_unextend_path_size, native_unextend_path_size, native_unextend_path);
	if (error)
		return error;

	int unextend_path_length = WideCharToMultiByte(CP_UTF8, 0, native_unextend_path, -1, 0, 0, 0, 0);
	if (!unextend_path_length)
		return EIO;

	size_t required_buffer_size = (size_t)unextend_path_length;
	*required_buffer_size_address = required_buffer_size;
	if (required_buffer_size <= buffer_size)
	{
		if (WideCharToMultiByte(CP_UTF8, 0, native_unextend_path, -1, buffer, unextend_path_length, 0, 0) != unextend_path_length)
			return EIO;
		return 0;
	}
	else
		return ENOBUFS;
}

#ifdef __cplusplus
}
#endif // __cplusplus
