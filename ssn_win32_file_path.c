/*
	Win32 file path manipulation library version 1.0.0 2023-02-25 by Santtu S. Nyman.

	License
		This is free and unencumbered software released into the public domain.
		Anyone is free to copy, modify, publish, use, compile, sell, or
		distribute this software, either in source code form or as a compiled
		binary, for any purpose, commercial or non-commercial, and by any
		means.
		In jurisdictions that recognize copyright laws, the author or authors
		of this software dedicate any and all copyright interest in the
		software to the public domain. We make this dedication for the benefit
		of the public at large and to the detriment of our heirs and
		successors. We intend this dedication to be an overt act of
		relinquishment in perpetuity of all present and future rights to this
		software under copyright law.
		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
		IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
		OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
		ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
		OTHER DEALINGS IN THE SOFTWARE.
		For more information, please refer to <https://unlicense.org>
*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "ssn_win32_file_path.h"
#include "ssn_utf8_utf16_converter.h"

static SIZE_T ssn_win32_absolute_path_volume_directory_part_length(SIZE_T path_length, const WCHAR* path)
{
	if (path_length > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		if (path_length > 6 && (path[4] != L'\\' && path[4] != L'/') && path[5] == L':' && path[6] == L'\\')
		{
			return 7;
		}
		else if (path_length > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			size_t server_length = 0;
			while (8 + server_length < path_length && (path[8 + server_length] != L'\\' && path[8 + server_length] != L'/'))
			{
				++server_length;
			}
			if (!server_length)
			{
				return 0;
			}
			size_t share_length = 0;
			while (9 + server_length + share_length < path_length && (path[9 + server_length + share_length] != L'\\' && path[9 + server_length + share_length] != L'/'))
			{
				++share_length;
			}
			if (!share_length || (path[9 + server_length + share_length] != L'\\' && path[9 + server_length + share_length] != L'/'))
			{
				return 0;
			}
			return 10 + server_length + share_length;
		}
		else
		{
			return 0;
		}
	}
	else if (path_length > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\')
	{
		if (path_length > 6 && (path[4] != L'\\' && path[4] != L'/') && path[5] == L':' && path[6] == L'\\')
		{
			return 7;
		}
		else if (path_length > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			size_t server_length = 0;
			while (8 + server_length < path_length && (path[8 + server_length] != L'\\' && path[8 + server_length] != L'/'))
			{
				++server_length;
			}
			if (!server_length)
			{
				return 0;
			}
			size_t share_length = 0;
			while (9 + server_length + share_length < path_length && (path[9 + server_length + share_length] != L'\\' && path[9 + server_length + share_length] != L'/'))
			{
				++share_length;
			}
			if (!share_length || (path[9 + server_length + share_length] != L'\\' && path[9 + server_length + share_length] != L'/'))
			{
				return 0;
			}
			return 10 + server_length + share_length;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (path_length > 2 && path[0] != L'\\' && path[0] != L'/' && path[1] == L':' && path[2] == L'\\')
		{
			return 3;
		}
		else if (path_length > 2 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
		{
			size_t server_length = 0;
			while (2 + server_length < path_length && (path[2 + server_length] != L'\\' && path[2 + server_length] != L'/'))
			{
				++server_length;
			}
			if (!server_length)
			{
				return 0;
			}
			size_t share_length = 0;
			while (3 + server_length + share_length < path_length && (path[3 + server_length + share_length] != L'\\' && path[3 + server_length + share_length] != L'/'))
			{
				++share_length;
			}
			if (!share_length || (path[3 + server_length + share_length] != L'\\' && path[3 + server_length + share_length] != L'/'))
			{
				return 0;
			}
			return 4 + server_length + share_length;
		}
		else
		{
			return 0;
		}
	}
}

BOOL ssn_win32_is_path_fully_qualified(SIZE_T path_length, const WCHAR* path)
{
	size_t path_volume_directory_part_length = ssn_win32_absolute_path_volume_directory_part_length(path_length, path);
	if (!path_volume_directory_part_length)
	{
		return FALSE;
	}

	BOOL extended_prefix = ((path_length > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\') || (path_length > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\'));
	if ((path_volume_directory_part_length > (MAX_PATH - 1)) && !extended_prefix)
	{
		return FALSE;
	}

	for (size_t offset = path_volume_directory_part_length; offset != path_length;)
	{
		size_t component_lenght = 0;
		while (offset + component_lenght != path_length && (path[offset + component_lenght] != L'\\' && path[offset + component_lenght] != L'/'))
		{
			component_lenght++;
		}
		if (!component_lenght || (component_lenght == 1 && path[0] == L'.') || (component_lenght == 2 && path[0] == L'.' && path[1] == L'.'))
		{
			return FALSE;
		}
		offset += component_lenght + (((offset + component_lenght) != path_length) ? 1 : 0);
	}
	return TRUE;
}

SIZE_T ssn_win32_get_fully_qualified_path(SIZE_T path_length, const WCHAR* path, SIZE_T base_path_length, const WCHAR* base_path, SIZE_T path_buffer_size, WCHAR* path_buffer)
{
	size_t path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length(path_length, path);
	// Remove trailing '\' if path is longer than volume directory path. Volume directory length is zero when path is relative
	if (path_length && (path_length > path_volume_part_length) && ((path[path_length - 1] == L'\\') || (path[path_length - 1] == L'/')))
	{
		path_length--;
	}
	if (!base_path_length && !path_length)
	{
		return 0;
	}

	if (path_volume_part_length)
	{
		// Absolute path to fully qualified path

		BOOL extended_prefix = ((path_length > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\') || (path_length > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\'));
		BOOL network_path = FALSE;
		if (extended_prefix)
		{
			if (path_length > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
			{
				network_path = TRUE;
			}
		}
		else
		{
			if (path_length > 1 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
			{
				network_path = TRUE;
			}
		}

		size_t fully_qualified_length = path_volume_part_length;
		for (size_t offset = path_length, component_count = 0, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (path[offset - component_lenght - 1] != L'\\' && path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == L'.' && path[component_offset + 1] == L'.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					fully_qualified_length += component_lenght + 1;
					component_count++;
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != path_volume_part_length)
			{
				offset--;
			}
			else
			{
				if (component_erase_count)
				{
					return 0;
				}
				if (component_count)
				{
					fully_qualified_length--;
				}
			}
		}

		BOOL add_extended_prefix = FALSE;
		BOOL remove_extended_prefix = FALSE;
		if ((!extended_prefix && (fully_qualified_length > (MAX_PATH - 1))) || (extended_prefix && ((network_path ? (fully_qualified_length - 6) : (fully_qualified_length - 4)) > (MAX_PATH - 1))))
		{
			if (!extended_prefix)
			{
				add_extended_prefix = TRUE;
				fully_qualified_length += network_path ? 6 : 4;
			}
		}
		else 
		{
			if (extended_prefix)
			{
				remove_extended_prefix = TRUE;
				fully_qualified_length -= network_path ? 6 : 4;
			}
		}

		if (fully_qualified_length > path_buffer_size)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = path_length, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (path[offset - component_lenght - 1] != L'\\' && path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == L'.' && path[component_offset + 1] == L'.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					if (write_offset != fully_qualified_length)
					{
						write_offset--;
						path_buffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(path_buffer + write_offset, path + component_offset, component_lenght * sizeof(WCHAR));
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != path_volume_part_length)
			{
				offset--;
			}
		}

		for (size_t path_volume_part_copy_end = (add_extended_prefix ? (network_path ? 6 : 4) : ((remove_extended_prefix && network_path) ? 2 : 0)), volume_part_read_offset = path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
		{
			WCHAR path_volume_part_character = path[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == L'/')
			{
				path_volume_part_character == L'\\';
			}
			write_offset--;
			path_buffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (network_path)
			{
				memcpy(path_buffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
			}
			memcpy(path_buffer, L"\\\\?\\", 4 * sizeof(WCHAR));
		}
		else if (remove_extended_prefix && network_path)
		{
			memcpy(path_buffer, L"\\\\", 2 * sizeof(WCHAR));
		}

		return fully_qualified_length;
	}
	else
	{
		// Relative path to fully qualified path

		// Get volume path part of the base directory
		size_t base_path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length(base_path_length, base_path);
		// Remove trailing '\' if path is longer than volume directory path. Volume directory length is zero when path is relative
		if (base_path_length && (base_path_length > base_path_volume_part_length) && ((base_path[base_path_length - 1] == L'\\') || (base_path[base_path_length - 1] == L'/')))
		{
			base_path_length--;
		}
		if (!base_path_length)
		{
			return 0;
		}

		BOOL base_path_extended_prefix = ((base_path_length > 4 && base_path[0] == L'\\' && base_path[1] == L'?' && base_path[2] == L'?' && base_path[3] == L'\\') || (base_path_length > 4 && base_path[0] == L'\\' && base_path[1] == L'\\' && base_path[2] == L'?' && base_path[3] == L'\\'));
		BOOL base_path_network_path = FALSE;
		if (base_path_extended_prefix)
		{
			if (base_path_length > 7 && (base_path[4] == L'U' || base_path[4] == L'u') && (base_path[5] == L'N' || base_path[5] == L'n') && (base_path[6] == L'C' || base_path[6] == L'c') && (base_path[7] == L'\\' || base_path[7] == L'/'))
			{
				base_path_network_path = TRUE;
			}
		}
		else
		{
			if (base_path_length > 1 && (base_path[0] == L'\\' || base_path[0] == L'/') && (base_path[1] == L'\\' || base_path[1] == L'/'))
			{
				base_path_network_path = TRUE;
			}
		}

		size_t relative_path_component_erase_count = 0;
		size_t relative_path_component_count = 0;
		size_t relative_path_length = 0;
		for (size_t offset = path_length; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (path[offset - component_lenght - 1] != L'\\' && path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == L'.' && path[component_offset + 1] == L'.')
			{
				relative_path_component_erase_count++;
			}
			else
			{
				if (!relative_path_component_erase_count)
				{
					relative_path_length += component_lenght + 1;
					relative_path_component_count++;
				}
				else
				{
					relative_path_component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset)
			{
				offset--;
			}
			else
			{
				if (relative_path_component_count)
				{
					relative_path_length--;
				}
			}
		}

		// Get the base path component count and length after making it fully qualified. Also erase components as required from the relative sub path
		size_t base_path_component_count = 0;
		size_t base_path_fully_qualified_length = base_path_volume_part_length;
		for (size_t offset = base_path_length, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (base_path[offset - component_lenght - 1] != L'\\' && base_path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && base_path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && base_path[component_offset] == L'.' && base_path[component_offset + 1] == L'.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					base_path_fully_qualified_length += component_lenght + 1;
					base_path_component_count++;
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != base_path_volume_part_length)
			{
				offset--;
			}
			else
			{
				if (component_erase_count)
				{
					return 0;
				}
				if (base_path_component_count)
				{
					base_path_fully_qualified_length--;
				}
			}
		}
		
		size_t fully_qualified_length = base_path_fully_qualified_length + ((relative_path_length && (base_path_fully_qualified_length != base_path_volume_part_length)) ? 1 : 0) + relative_path_length;
		
		BOOL add_extended_prefix = FALSE;
		BOOL remove_extended_prefix = FALSE;
		if ((!base_path_extended_prefix && (fully_qualified_length > (MAX_PATH - 1))) || (base_path_extended_prefix && ((base_path_network_path ? (fully_qualified_length - 6) : (fully_qualified_length - 4)) > (MAX_PATH - 1))))
		{
			if (!base_path_extended_prefix)
			{
				add_extended_prefix = TRUE;
				base_path_fully_qualified_length += base_path_network_path ? 6 : 4;
				fully_qualified_length += base_path_network_path ? 6 : 4;
			}
		}
		else
		{
			if (base_path_extended_prefix)
			{
				remove_extended_prefix = TRUE;
				base_path_fully_qualified_length -= base_path_network_path ? 6 : 4;
				fully_qualified_length -= base_path_network_path ? 6 : 4;
			}
		}
		
		if (fully_qualified_length > path_buffer_size)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = path_length, component_erase_count = 0; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (path[offset - component_lenght - 1] != L'\\' && path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == L'.' && path[component_offset + 1] == L'.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					if (write_offset != fully_qualified_length)
					{
						write_offset--;
						path_buffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(path_buffer + write_offset, path + component_offset, component_lenght * sizeof(WCHAR));
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset)
			{
				offset--;
			}
		}

		if (relative_path_length && base_path_component_count)
		{
			write_offset--;
			path_buffer[write_offset] = '\\';
		}

		for (size_t offset = base_path_length, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (base_path[offset - component_lenght - 1] != L'\\' && base_path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && base_path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && base_path[component_offset] == L'.' && base_path[component_offset + 1] == L'.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					if (write_offset != base_path_fully_qualified_length)
					{
						write_offset--;
						path_buffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(path_buffer + write_offset, base_path + component_offset, component_lenght * sizeof(WCHAR));
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != base_path_volume_part_length)
			{
				offset--;
			}
		}

		for (size_t path_volume_part_copy_end = (add_extended_prefix ? (base_path_network_path ? 6 : 4) : ((remove_extended_prefix && base_path_network_path) ? 2 : 0)), volume_part_read_offset = base_path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
		{
			WCHAR path_volume_part_character = base_path[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == L'/')
			{
				path_volume_part_character == L'\\';
			}
			write_offset--;
			path_buffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (base_path_network_path)
			{
				memcpy(path_buffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
			}
			memcpy(path_buffer, L"\\\\?\\", 4 * sizeof(WCHAR));
		}
		else if (remove_extended_prefix && base_path_network_path)
		{
			memcpy(path_buffer, L"\\\\", 2 * sizeof(WCHAR));
		}

		return fully_qualified_length;
	}
}

SIZE_T ssn_win32_get_volume_directory_path(SIZE_T path_length, const WCHAR* path, SIZE_T base_path_length, const WCHAR* base_path, SIZE_T path_buffer_size, WCHAR* path_buffer)
{
	size_t path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length(path_length, path);
	if (!base_path_length && !path_length)
	{
		return 0;
	}
	if (!path_volume_part_length)
	{
		path_length = base_path_length;
		path = base_path;
		path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length(path_length, path);
		if (!path_volume_part_length)
		{
			return 0;
		}
	}

	BOOL extended_prefix = ((path_length > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\') || (path_length > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\'));
	BOOL network_path = FALSE;
	if (extended_prefix)
	{
		if (path_length > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			network_path = TRUE;
		}
	}
	else
	{
		if (path_length > 1 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
		{
			network_path = TRUE;
		}
	}

	size_t fully_qualified_length = path_volume_part_length;
	BOOL add_extended_prefix = FALSE;
	BOOL remove_extended_prefix = FALSE;
	if ((!extended_prefix && (fully_qualified_length > (MAX_PATH - 1))) || (extended_prefix && ((network_path ? (fully_qualified_length - 6) : (fully_qualified_length - 4)) > (MAX_PATH - 1))))
	{
		if (!extended_prefix)
		{
			add_extended_prefix = TRUE;
			fully_qualified_length += network_path ? 6 : 4;
		}
	}
	else
	{
		if (extended_prefix)
		{
			remove_extended_prefix = TRUE;
			fully_qualified_length -= network_path ? 6 : 4;
		}
	}

	if (fully_qualified_length > path_buffer_size)
	{
		return fully_qualified_length;
	}

	for (size_t path_volume_part_copy_end = (add_extended_prefix ? (network_path ? 6 : 4) : ((remove_extended_prefix && network_path) ? 2 : 0)), write_offset = fully_qualified_length, read_offset = path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
	{
		WCHAR path_volume_part_character = path[read_offset];
		read_offset--;
		if (path_volume_part_character == L'/')
		{
			path_volume_part_character == L'\\';
		}
		write_offset--;
		path_buffer[write_offset] = path_volume_part_character;
	}
	if (add_extended_prefix)
	{
		if (network_path)
		{
			memcpy(path_buffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
		}
		memcpy(path_buffer, L"\\\\?\\", 4 * sizeof(WCHAR));
	}
	else if (remove_extended_prefix && network_path)
	{
		memcpy(path_buffer, L"\\\\", 2 * sizeof(WCHAR));
	}

	return fully_qualified_length;
}

static SIZE_T ssn_win32_absolute_path_volume_directory_part_length_utf8(SIZE_T path_length, const char* path)
{
	if (path_length > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\')
	{
		if (path_length > 6 && (path[4] != '\\' && path[4] != '/') && path[5] == ':' && path[6] == '\\')
		{
			return 7;
		}
		else if (path_length > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			size_t server_length = 0;
			while (8 + server_length < path_length && (path[8 + server_length] != '\\' && path[8 + server_length] != '/'))
			{
				++server_length;
			}
			if (!server_length)
			{
				return 0;
			}
			size_t share_length = 0;
			while (9 + server_length + share_length < path_length && (path[9 + server_length + share_length] != '\\' && path[9 + server_length + share_length] != '/'))
			{
				++share_length;
			}
			if (!share_length || (path[9 + server_length + share_length] != '\\' && path[9 + server_length + share_length] != '/'))
			{
				return 0;
			}
			return 10 + server_length + share_length;
		}
		else
		{
			return 0;
		}
	}
	else if (path_length > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\')
	{
		if (path_length > 6 && (path[4] != '\\' && path[4] != '/') && path[5] == ':' && path[6] == '\\')
		{
			return 7;
		}
		else if (path_length > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			size_t server_length = 0;
			while (8 + server_length < path_length && (path[8 + server_length] != '\\' && path[8 + server_length] != '/'))
			{
				++server_length;
			}
			if (!server_length)
			{
				return 0;
			}
			size_t share_length = 0;
			while (9 + server_length + share_length < path_length && (path[9 + server_length + share_length] != '\\' && path[9 + server_length + share_length] != '/'))
			{
				++share_length;
			}
			if (!share_length || (path[9 + server_length + share_length] != '\\' && path[9 + server_length + share_length] != '/'))
			{
				return 0;
			}
			return 10 + server_length + share_length;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (path_length > 2 && path[0] != '\\' && path[0] != '/' && path[1] == ':' && path[2] == '\\')
		{
			return 3;
		}
		else if (path_length > 2 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
		{
			size_t server_length = 0;
			while (2 + server_length < path_length && (path[2 + server_length] != '\\' && path[2 + server_length] != '/'))
			{
				++server_length;
			}
			if (!server_length)
			{
				return 0;
			}
			size_t share_length = 0;
			while (3 + server_length + share_length < path_length && (path[3 + server_length + share_length] != '\\' && path[3 + server_length + share_length] != '/'))
			{
				++share_length;
			}
			if (!share_length || (path[3 + server_length + share_length] != '\\' && path[3 + server_length + share_length] != '/'))
			{
				return 0;
			}
			return 4 + server_length + share_length;
		}
		else
		{
			return 0;
		}
	}
}

BOOL ssn_win32_is_path_fully_qualified_utf8(SIZE_T path_length, const char* path)
{
	size_t path_volume_directory_part_length = ssn_win32_absolute_path_volume_directory_part_length_utf8(path_length, path);
	if (!path_volume_directory_part_length)
	{
		return FALSE;
	}

	size_t utf16_fully_qualified_length = ssn_convert_utf8_to_utf16le(path_length, path, 0, 0);
	BOOL extended_prefix = ((path_length > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\') || (path_length > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\'));
	if ((utf16_fully_qualified_length > (MAX_PATH - 1)) && !extended_prefix)
	{
		return FALSE;
	}

	for (size_t offset = path_volume_directory_part_length; offset != path_length;)
	{
		size_t component_lenght = 0;
		while (offset + component_lenght != path_length && (path[offset + component_lenght] != '\\' && path[offset + component_lenght] != '/'))
		{
			component_lenght++;
		}
		if (!component_lenght || (component_lenght == 1 && path[0] == '.') || (component_lenght == 2 && path[0] == '.' && path[1] == '.'))
		{
			return FALSE;
		}
		offset += component_lenght + (((offset + component_lenght) != path_length) ? 1 : 0);
	}
	return TRUE;
}

SIZE_T ssn_win32_get_fully_qualified_path_utf8(SIZE_T path_length, const char* path, SIZE_T base_path_length, const char* base_path, SIZE_T path_buffer_size, char* path_buffer)
{
	size_t path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length_utf8(path_length, path);
	// Remove trailing '\' if path is longer than volume directory path. Volume directory length is zero when path is relative
	if (path_length && (path_length > path_volume_part_length) && ((path[path_length - 1] == '\\') || (path[path_length - 1] == '/')))
	{
		path_length--;
	}
	if (!base_path_length && !path_length)
	{
		return 0;
	}

	if (path_volume_part_length)
	{
		// Absolute path to fully qualified path

		BOOL extended_prefix = ((path_length > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\') || (path_length > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\'));
		BOOL network_path = FALSE;
		if (extended_prefix)
		{
			if (path_length > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
			{
				network_path = TRUE;
			}
		}
		else
		{
			if (path_length > 1 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
			{
				network_path = TRUE;
			}
		}

		size_t fully_qualified_length = path_volume_part_length;
		size_t fully_qualified_utf16_length = ssn_convert_utf8_to_utf16le(path_volume_part_length, path, 0, 0);
		for (size_t offset = path_length, component_count = 0, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (path[offset - component_lenght - 1] != '\\' && path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == '.' && path[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					fully_qualified_length += component_lenght + 1;
					fully_qualified_utf16_length += ssn_convert_utf8_to_utf16le(component_lenght, path + component_offset, 0, 0) + 1;
					component_count++;
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != path_volume_part_length)
			{
				offset--;
			}
			else
			{
				if (component_erase_count)
				{
					return 0;
				}
				if (component_count)
				{
					fully_qualified_length--;
					fully_qualified_utf16_length--;
				}
			}
		}

		BOOL add_extended_prefix = FALSE;
		BOOL remove_extended_prefix = FALSE;
		if ((!extended_prefix && (fully_qualified_utf16_length > (MAX_PATH - 1))) || (extended_prefix && ((network_path ? (fully_qualified_utf16_length - 6) : (fully_qualified_utf16_length - 4)) > (MAX_PATH - 1))))
		{
			if (!extended_prefix)
			{
				add_extended_prefix = TRUE;
				size_t length_adjust = network_path ? 6 : 4;
				fully_qualified_length += length_adjust;
				fully_qualified_utf16_length += length_adjust;
			}
		}
		else
		{
			if (extended_prefix)
			{
				remove_extended_prefix = TRUE;
				size_t length_adjust = network_path ? 6 : 4;
				fully_qualified_length -= length_adjust;
				fully_qualified_utf16_length -= length_adjust;
			}
		}

		if (fully_qualified_length > path_buffer_size)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = path_length, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (path[offset - component_lenght - 1] != '\\' && path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == '.' && path[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					if (write_offset != fully_qualified_length)
					{
						write_offset--;
						path_buffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(path_buffer + write_offset, path + component_offset, component_lenght * sizeof(char));
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != path_volume_part_length)
			{
				offset--;
			}
		}

		for (size_t path_volume_part_copy_end = (add_extended_prefix ? (network_path ? 6 : 4) : ((remove_extended_prefix && network_path) ? 2 : 0)), volume_part_read_offset = path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
		{
			char path_volume_part_character = path[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == '/')
			{
				path_volume_part_character == '\\';
			}
			write_offset--;
			path_buffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (network_path)
			{
				memcpy(path_buffer + 4, "UNC\\", 4 * sizeof(char));
			}
			memcpy(path_buffer, "\\\\?\\", 4 * sizeof(char));
		}
		else if (remove_extended_prefix && network_path)
		{
			memcpy(path_buffer, "\\\\", 2 * sizeof(char));
		}

		return fully_qualified_length;
	}
	else
	{
		// Relative path to fully qualified path

		// Get volume path part of the base directory
		size_t base_path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length_utf8(base_path_length, base_path);
		// Remove trailing '\' if path is longer than volume directory path. Volume directory length is zero when path is relative
		if (base_path_length && (base_path_length > base_path_volume_part_length) && ((base_path[base_path_length - 1] == '\\') || (base_path[base_path_length - 1] == '/')))
		{
			base_path_length--;
		}
		if (!base_path_length)
		{
			return 0;
		}

		BOOL base_path_extended_prefix = ((base_path_length > 4 && base_path[0] == '\\' && base_path[1] == '?' && base_path[2] == '?' && base_path[3] == '\\') || (base_path_length > 4 && base_path[0] == '\\' && base_path[1] == '\\' && base_path[2] == '?' && base_path[3] == '\\'));
		BOOL base_path_network_path = FALSE;
		if (base_path_extended_prefix)
		{
			if (base_path_length > 7 && (base_path[4] == 'U' || base_path[4] == 'u') && (base_path[5] == 'N' || base_path[5] == 'n') && (base_path[6] == 'C' || base_path[6] == 'c') && (base_path[7] == '\\' || base_path[7] == '/'))
			{
				base_path_network_path = TRUE;
			}
		}
		else
		{
			if (base_path_length > 1 && (base_path[0] == '\\' || base_path[0] == '/') && (base_path[1] == '\\' || base_path[1] == '/'))
			{
				base_path_network_path = TRUE;
			}
		}

		size_t relative_path_component_erase_count = 0;
		size_t relative_path_component_count = 0;
		size_t relative_path_length = 0;
		size_t relative_path_utf16_length = 0;
		for (size_t offset = path_length; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (path[offset - component_lenght - 1] != '\\' && path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == '.' && path[component_offset + 1] == '.')
			{
				relative_path_component_erase_count++;
			}
			else
			{
				if (!relative_path_component_erase_count)
				{
					relative_path_length += component_lenght + 1;
					relative_path_utf16_length += ssn_convert_utf8_to_utf16le(component_lenght, path + component_offset, 0, 0) + 1;
					relative_path_component_count++;
				}
				else
				{
					relative_path_component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset)
			{
				offset--;
			}
			else
			{
				if (relative_path_component_count)
				{
					relative_path_length--;
					relative_path_utf16_length--;
				}
			}
		}

		// Get the base path component count and length after making it fully qualified. Also erase components as required from the relative sub path
		size_t base_path_component_count = 0;
		size_t base_path_fully_qualified_length = base_path_volume_part_length;
		size_t base_path_fully_qualified_utf16_length = ssn_convert_utf8_to_utf16le(base_path_volume_part_length, base_path, 0, 0);
		for (size_t offset = base_path_length, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (base_path[offset - component_lenght - 1] != '\\' && base_path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && base_path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && base_path[component_offset] == '.' && base_path[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					base_path_fully_qualified_length += component_lenght + 1;
					base_path_fully_qualified_utf16_length += ssn_convert_utf8_to_utf16le(component_lenght, base_path + component_offset, 0, 0) + 1;
					base_path_component_count++;
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != base_path_volume_part_length)
			{
				offset--;
			}
			else
			{
				if (component_erase_count)
				{
					return 0;
				}
				if (base_path_component_count)
				{
					base_path_fully_qualified_length--;
					base_path_fully_qualified_utf16_length--;
				}
			}
		}

		BOOL add_base_path_separator = relative_path_length && (base_path_fully_qualified_length != base_path_volume_part_length);
		size_t fully_qualified_utf16_length = base_path_fully_qualified_utf16_length + (add_base_path_separator ? 1 : 0) + relative_path_utf16_length;
		size_t fully_qualified_length = base_path_fully_qualified_length + (add_base_path_separator ? 1 : 0) + relative_path_length;

		BOOL add_extended_prefix = FALSE;
		BOOL remove_extended_prefix = FALSE;
		if ((!base_path_extended_prefix && (fully_qualified_utf16_length > (MAX_PATH - 1))) || (base_path_extended_prefix && ((base_path_network_path ? (fully_qualified_utf16_length - 6) : (fully_qualified_utf16_length - 4)) > (MAX_PATH - 1))))
		{
			if (!base_path_extended_prefix)
			{
				add_extended_prefix = TRUE;
				size_t length_adjust = base_path_network_path ? 6 : 4;
				base_path_fully_qualified_length += length_adjust;
				fully_qualified_length += length_adjust;
				fully_qualified_utf16_length += length_adjust;
			}
		}
		else
		{
			if (base_path_extended_prefix)
			{
				remove_extended_prefix = TRUE;
				size_t length_adjust = base_path_network_path ? 6 : 4;
				base_path_fully_qualified_length -= length_adjust;
				fully_qualified_length -= length_adjust;
				fully_qualified_utf16_length -= length_adjust;
			}
		}

		if (fully_qualified_length > path_buffer_size)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = path_length, component_erase_count = 0; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (path[offset - component_lenght - 1] != '\\' && path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && path[component_offset] == '.' && path[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					if (write_offset != fully_qualified_length)
					{
						write_offset--;
						path_buffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(path_buffer + write_offset, path + component_offset, component_lenght * sizeof(char));
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset)
			{
				offset--;
			}
		}

		if (relative_path_length && base_path_component_count)
		{
			write_offset--;
			path_buffer[write_offset] = '\\';
		}

		for (size_t offset = base_path_length, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (base_path[offset - component_lenght - 1] != '\\' && base_path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && base_path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && base_path[component_offset] == '.' && base_path[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					if (write_offset != base_path_fully_qualified_length)
					{
						write_offset--;
						path_buffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(path_buffer + write_offset, base_path + component_offset, component_lenght * sizeof(char));
				}
				else
				{
					component_erase_count--;
				}
			}
			offset -= component_lenght;
			if (offset != base_path_volume_part_length)
			{
				offset--;
			}
		}

		for (size_t path_volume_part_copy_end = (add_extended_prefix ? (base_path_network_path ? 6 : 4) : ((remove_extended_prefix && base_path_network_path) ? 2 : 0)), volume_part_read_offset = base_path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
		{
			char path_volume_part_character = base_path[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == '/')
			{
				path_volume_part_character == '\\';
			}
			write_offset--;
			path_buffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (base_path_network_path)
			{
				memcpy(path_buffer + 4, "UNC\\", 4 * sizeof(char));
			}
			memcpy(path_buffer, "\\\\?\\", 4 * sizeof(char));
		}
		else if (remove_extended_prefix && base_path_network_path)
		{
			memcpy(path_buffer, "\\\\", 2 * sizeof(char));
		}

		return fully_qualified_length;
	}
}

SIZE_T ssn_win32_get_volume_directory_path_utf8(SIZE_T path_length, const char* path, SIZE_T base_path_length, const char* base_path, SIZE_T path_buffer_size, char* path_buffer)
{
	size_t path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length_utf8(path_length, path);
	if (!base_path_length && !path_length)
	{
		return 0;
	}
	if (!path_volume_part_length)
	{
		path_length = base_path_length;
		path = base_path;
		path_volume_part_length = ssn_win32_absolute_path_volume_directory_part_length_utf8(path_length, path);
		if (!path_volume_part_length)
		{
			return 0;
		}
	}

	BOOL extended_prefix = ((path_length > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\') || (path_length > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\'));
	BOOL network_path = FALSE;
	if (extended_prefix)
	{
		if (path_length > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			network_path = TRUE;
		}
	}
	else
	{
		if (path_length > 1 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
		{
			network_path = TRUE;
		}
	}

	size_t fully_qualified_length = path_volume_part_length;
	size_t utf16_fully_qualified_length = ssn_convert_utf8_to_utf16le(fully_qualified_length, path, 0, 0);
	BOOL add_extended_prefix = FALSE;
	BOOL remove_extended_prefix = FALSE;
	if ((!extended_prefix && (utf16_fully_qualified_length > (MAX_PATH - 1))) || (extended_prefix && ((network_path ? (utf16_fully_qualified_length - 6) : (utf16_fully_qualified_length - 4)) > (MAX_PATH - 1))))
	{
		if (!extended_prefix)
		{
			add_extended_prefix = TRUE;
			size_t length_adjust = network_path ? 6 : 4;
			fully_qualified_length += length_adjust;
			utf16_fully_qualified_length += length_adjust;
		}
	}
	else
	{
		if (extended_prefix)
		{
			remove_extended_prefix = TRUE;
			size_t length_adjust = network_path ? 6 : 4;
			fully_qualified_length -= length_adjust;
			utf16_fully_qualified_length -= length_adjust;
		}
	}

	if (fully_qualified_length > path_buffer_size)
	{
		return fully_qualified_length;
	}

	for (size_t path_volume_part_copy_end = (add_extended_prefix ? (network_path ? 6 : 4) : ((remove_extended_prefix && network_path) ? 2 : 0)), write_offset = fully_qualified_length, read_offset = path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
	{
		char path_volume_part_character = path[read_offset];
		read_offset--;
		if (path_volume_part_character == '/')
		{
			path_volume_part_character == '\\';
		}
		write_offset--;
		path_buffer[write_offset] = path_volume_part_character;
	}
	if (add_extended_prefix)
	{
		if (network_path)
		{
			memcpy(path_buffer + 4, "UNC\\", 4 * sizeof(char));
		}
		memcpy(path_buffer, "\\\\?\\", 4 * sizeof(char));
	}
	else if (remove_extended_prefix && network_path)
	{
		memcpy(path_buffer, "\\\\", 2 * sizeof(char));
	}

	return fully_qualified_length;
}

#ifdef __cplusplus
}
#endif // __cplusplus
