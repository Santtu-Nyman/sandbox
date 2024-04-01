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

#define WIN32_LEAN_AND_MEAN
#include "LtWin32FilePath.h"
#include "LtUtf8Utf16Converter.h"

#define LT_IS_HEX_WCHAR(C) (((int)(C) >= (int)L'0' && (int)(C) <= (int)L'9') || ((int)(C) >= (int)L'A' && (int)(C) <= (int)L'F') || ((int)(C) >= (int)L'a' && (int)(C) <= (int)L'f'))
#define LT_IS_HEX_CHAR(C) (((int)(C) >= (int)'0' && (int)(C) <= (int)'9') || ((int)(C) >= (int)'A' && (int)(C) <= (int)'F') || ((int)(C) >= (int)'a' && (int)(C) <= (int)'f'))

static SIZE_T lt_win32_absolute_path_volume_directory_part_length(SIZE_T path_length, const WCHAR* path)
{
	if (path_length > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		if (path_length > 6 && (path[4] != L'\\' && path[4] != L'/') && path[5] == L':' && (path[6] == L'\\' || path[6] == L'/'))
		{
			return 7;
		}
		else if (path_length > 48 &&
			(path[4] == L'V' || path[4] == L'v') &&
			(path[5] == L'O' || path[5] == L'o') &&
			(path[6] == L'L' || path[6] == L'l') &&
			(path[7] == L'U' || path[7] == L'u') &&
			(path[8] == L'M' || path[8] == L'm') &&
			(path[9] == L'E' || path[9] == L'e') &&
			path[10] == L'{' &&
			LT_IS_HEX_WCHAR(path[11]) &&
			LT_IS_HEX_WCHAR(path[12]) &&
			LT_IS_HEX_WCHAR(path[13]) &&
			LT_IS_HEX_WCHAR(path[14]) &&
			LT_IS_HEX_WCHAR(path[15]) &&
			LT_IS_HEX_WCHAR(path[16]) &&
			LT_IS_HEX_WCHAR(path[17]) &&
			LT_IS_HEX_WCHAR(path[18]) &&
			path[19] == L'-' &&
			LT_IS_HEX_WCHAR(path[20]) &&
			LT_IS_HEX_WCHAR(path[21]) &&
			LT_IS_HEX_WCHAR(path[22]) &&
			LT_IS_HEX_WCHAR(path[23]) &&
			path[24] == L'-' &&
			LT_IS_HEX_WCHAR(path[25]) &&
			LT_IS_HEX_WCHAR(path[26]) &&
			LT_IS_HEX_WCHAR(path[27]) &&
			LT_IS_HEX_WCHAR(path[28]) &&
			path[29] == L'-' &&
			LT_IS_HEX_WCHAR(path[30]) &&
			LT_IS_HEX_WCHAR(path[31]) &&
			LT_IS_HEX_WCHAR(path[32]) &&
			LT_IS_HEX_WCHAR(path[33]) &&
			path[34] == L'-' &&
			LT_IS_HEX_WCHAR(path[35]) &&
			LT_IS_HEX_WCHAR(path[36]) &&
			LT_IS_HEX_WCHAR(path[37]) &&
			LT_IS_HEX_WCHAR(path[38]) &&
			LT_IS_HEX_WCHAR(path[39]) &&
			LT_IS_HEX_WCHAR(path[40]) &&
			LT_IS_HEX_WCHAR(path[41]) &&
			LT_IS_HEX_WCHAR(path[42]) &&
			LT_IS_HEX_WCHAR(path[43]) &&
			LT_IS_HEX_WCHAR(path[44]) &&
			LT_IS_HEX_WCHAR(path[45]) &&
			LT_IS_HEX_WCHAR(path[46]) &&
			path[47] == L'}' &&
			path[48] == L'\\')
		{
			return 49;
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
		if (path_length > 6 && (path[4] != L'\\' && path[4] != L'/') && path[5] == L':' && (path[6] == L'\\' || path[6] == L'/'))
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
		if (path_length > 2 && path[0] != L'\\' && path[0] != L'/' && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/'))
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

BOOL LtWin32IsPathFullyQualified(SIZE_T PathLength, const WCHAR* Path)
{
	size_t path_volume_directory_part_length = lt_win32_absolute_path_volume_directory_part_length(PathLength, Path);
	if (!path_volume_directory_part_length)
	{
		return FALSE;
	}

	BOOL extended_prefix = ((PathLength > 4 && Path[0] == L'\\' && Path[1] == L'?' && Path[2] == L'?' && Path[3] == L'\\') || (PathLength > 4 && Path[0] == L'\\' && Path[1] == L'\\' && Path[2] == L'?' && Path[3] == L'\\'));
	if ((path_volume_directory_part_length > (MAX_PATH - 1)) && !extended_prefix)
	{
		return FALSE;
	}

	for (size_t offset = path_volume_directory_part_length; offset != PathLength;)
	{
		size_t component_lenght = 0;
		while (offset + component_lenght != PathLength && (Path[offset + component_lenght] != L'\\' && Path[offset + component_lenght] != L'/'))
		{
			component_lenght++;
		}
		if (!component_lenght || (component_lenght == 1 && Path[0] == L'.') || (component_lenght == 2 && Path[0] == L'.' && Path[1] == L'.'))
		{
			return FALSE;
		}
		offset += component_lenght + (((offset + component_lenght) != PathLength) ? 1 : 0);
	}
	return TRUE;
}

SIZE_T LtWin32GetFullyQualifiedPath(SIZE_T PathLength, const WCHAR* Path, SIZE_T BasePathLength, const WCHAR* BasePath, SIZE_T PathBufferSize, WCHAR* PathBuffer)
{
	size_t path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length(PathLength, Path);
	// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
	if (PathLength && (PathLength > path_volume_part_length) && ((Path[PathLength - 1] == L'\\') || (Path[PathLength - 1] == L'/')))
	{
		PathLength--;
	}
	if (!BasePathLength && !PathLength)
	{
		return 0;
	}

	if (path_volume_part_length)
	{
		// Absolute Path to fully qualified Path

		BOOL extended_prefix = ((PathLength > 4 && Path[0] == L'\\' && Path[1] == L'?' && Path[2] == L'?' && Path[3] == L'\\') || (PathLength > 4 && Path[0] == L'\\' && Path[1] == L'\\' && Path[2] == L'?' && Path[3] == L'\\'));
		BOOL network_path = FALSE;
		if (extended_prefix)
		{
			if (PathLength > 7 && (Path[4] == L'U' || Path[4] == L'u') && (Path[5] == L'N' || Path[5] == L'n') && (Path[6] == L'C' || Path[6] == L'c') && (Path[7] == L'\\' || Path[7] == L'/'))
			{
				network_path = TRUE;
			}
		}
		else
		{
			if (PathLength > 1 && (Path[0] == L'\\' || Path[0] == L'/') && (Path[1] == L'\\' || Path[1] == L'/'))
			{
				network_path = TRUE;
			}
		}

		size_t fully_qualified_length = path_volume_part_length;
		for (size_t offset = PathLength, component_count = 0, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (Path[offset - component_lenght - 1] != L'\\' && Path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == L'.' && Path[component_offset + 1] == L'.')
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

		if (fully_qualified_length > PathBufferSize)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = PathLength, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (Path[offset - component_lenght - 1] != L'\\' && Path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == L'.' && Path[component_offset + 1] == L'.')
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
						PathBuffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(PathBuffer + write_offset, Path + component_offset, component_lenght * sizeof(WCHAR));
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
			WCHAR path_volume_part_character = Path[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == L'/')
			{
				path_volume_part_character = L'\\';
			}
			write_offset--;
			PathBuffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (network_path)
			{
				memcpy(PathBuffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
			}
			memcpy(PathBuffer, L"\\\\?\\", 4 * sizeof(WCHAR));
		}
		else if (remove_extended_prefix && network_path)
		{
			memcpy(PathBuffer, L"\\\\", 2 * sizeof(WCHAR));
		}

		return fully_qualified_length;
	}
	else
	{
		// Relative Path to fully qualified Path

		// Get volume Path part of the base directory
		size_t base_path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length(BasePathLength, BasePath);
		// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
		if (BasePathLength && (BasePathLength > base_path_volume_part_length) && ((BasePath[BasePathLength - 1] == L'\\') || (BasePath[BasePathLength - 1] == L'/')))
		{
			BasePathLength--;
		}
		if (!BasePathLength)
		{
			return 0;
		}

		BOOL base_path_extended_prefix = ((BasePathLength > 4 && BasePath[0] == L'\\' && BasePath[1] == L'?' && BasePath[2] == L'?' && BasePath[3] == L'\\') || (BasePathLength > 4 && BasePath[0] == L'\\' && BasePath[1] == L'\\' && BasePath[2] == L'?' && BasePath[3] == L'\\'));
		BOOL base_path_network_path = FALSE;
		if (base_path_extended_prefix)
		{
			if (BasePathLength > 7 && (BasePath[4] == L'U' || BasePath[4] == L'u') && (BasePath[5] == L'N' || BasePath[5] == L'n') && (BasePath[6] == L'C' || BasePath[6] == L'c') && (BasePath[7] == L'\\' || BasePath[7] == L'/'))
			{
				base_path_network_path = TRUE;
			}
		}
		else
		{
			if (BasePathLength > 1 && (BasePath[0] == L'\\' || BasePath[0] == L'/') && (BasePath[1] == L'\\' || BasePath[1] == L'/'))
			{
				base_path_network_path = TRUE;
			}
		}

		size_t relative_path_component_erase_count = 0;
		size_t relative_path_component_count = 0;
		size_t relative_path_length = 0;
		for (size_t offset = PathLength; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (Path[offset - component_lenght - 1] != L'\\' && Path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == L'.' && Path[component_offset + 1] == L'.')
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

		// Get the base Path component count and length after making it fully qualified. Also erase components as required from the relative sub Path
		size_t base_path_component_count = 0;
		size_t base_path_fully_qualified_length = base_path_volume_part_length;
		for (size_t offset = BasePathLength, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (BasePath[offset - component_lenght - 1] != L'\\' && BasePath[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && BasePath[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && BasePath[component_offset] == L'.' && BasePath[component_offset + 1] == L'.')
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
		
		if (fully_qualified_length > PathBufferSize)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = PathLength, component_erase_count = 0; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (Path[offset - component_lenght - 1] != L'\\' && Path[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == L'.' && Path[component_offset + 1] == L'.')
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
						PathBuffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(PathBuffer + write_offset, Path + component_offset, component_lenght * sizeof(WCHAR));
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
			PathBuffer[write_offset] = '\\';
		}

		for (size_t offset = BasePathLength, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (BasePath[offset - component_lenght - 1] != L'\\' && BasePath[offset - component_lenght - 1] != L'/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && BasePath[component_offset] == L'.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && BasePath[component_offset] == L'.' && BasePath[component_offset + 1] == L'.')
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
						PathBuffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(PathBuffer + write_offset, BasePath + component_offset, component_lenght * sizeof(WCHAR));
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
			WCHAR path_volume_part_character = BasePath[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == L'/')
			{
				path_volume_part_character = L'\\';
			}
			write_offset--;
			PathBuffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (base_path_network_path)
			{
				memcpy(PathBuffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
			}
			memcpy(PathBuffer, L"\\\\?\\", 4 * sizeof(WCHAR));
		}
		else if (remove_extended_prefix && base_path_network_path)
		{
			memcpy(PathBuffer, L"\\\\", 2 * sizeof(WCHAR));
		}

		return fully_qualified_length;
	}
}

SIZE_T LtWin32GetVolumeDirectoryPath(SIZE_T PathLength, const WCHAR* Path, SIZE_T BasePathLength, const WCHAR* BasePath, SIZE_T PathBufferSize, WCHAR* PathBuffer)
{
	size_t path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length(PathLength, Path);
	if (!BasePathLength && !PathLength)
	{
		return 0;
	}
	if (!path_volume_part_length)
	{
		PathLength = BasePathLength;
		Path = BasePath;
		path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length(PathLength, Path);
		if (!path_volume_part_length)
		{
			return 0;
		}
	}

	BOOL extended_prefix = ((PathLength > 4 && Path[0] == L'\\' && Path[1] == L'?' && Path[2] == L'?' && Path[3] == L'\\') || (PathLength > 4 && Path[0] == L'\\' && Path[1] == L'\\' && Path[2] == L'?' && Path[3] == L'\\'));
	BOOL network_path = FALSE;
	if (extended_prefix)
	{
		if (PathLength > 7 && (Path[4] == L'U' || Path[4] == L'u') && (Path[5] == L'N' || Path[5] == L'n') && (Path[6] == L'C' || Path[6] == L'c') && (Path[7] == L'\\' || Path[7] == L'/'))
		{
			network_path = TRUE;
		}
	}
	else
	{
		if (PathLength > 1 && (Path[0] == L'\\' || Path[0] == L'/') && (Path[1] == L'\\' || Path[1] == L'/'))
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

	if (fully_qualified_length > PathBufferSize)
	{
		return fully_qualified_length;
	}

	for (size_t path_volume_part_copy_end = (add_extended_prefix ? (network_path ? 6 : 4) : ((remove_extended_prefix && network_path) ? 2 : 0)), write_offset = fully_qualified_length, read_offset = path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
	{
		WCHAR path_volume_part_character = Path[read_offset];
		read_offset--;
		if (path_volume_part_character == L'/')
		{
			path_volume_part_character = L'\\';
		}
		write_offset--;
		PathBuffer[write_offset] = path_volume_part_character;
	}
	if (add_extended_prefix)
	{
		if (network_path)
		{
			memcpy(PathBuffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
		}
		memcpy(PathBuffer, L"\\\\?\\", 4 * sizeof(WCHAR));
	}
	else if (remove_extended_prefix && network_path)
	{
		memcpy(PathBuffer, L"\\\\", 2 * sizeof(WCHAR));
	}

	return fully_qualified_length;
}

static SIZE_T lt_win32_absolute_path_volume_directory_part_length_utf8(SIZE_T path_length, const char* path)
{
	if (path_length > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\')
	{
		if (path_length > 6 && (path[4] != '\\' && path[4] != '/') && path[5] == ':' && (path[6] == '\\' || path[6] == '/'))
		{
			return 7;
		}
		else if (path_length > 48 &&
			(path[4] == 'V' || path[4] == 'v') &&
			(path[5] == 'O' || path[5] == 'o') &&
			(path[6] == 'L' || path[6] == 'l') &&
			(path[7] == 'U' || path[7] == 'u') &&
			(path[8] == 'M' || path[8] == 'm') &&
			(path[9] == 'E' || path[9] == 'e') &&
			path[10] == '{' &&
			LT_IS_HEX_CHAR(path[11]) &&
			LT_IS_HEX_CHAR(path[12]) &&
			LT_IS_HEX_CHAR(path[13]) &&
			LT_IS_HEX_CHAR(path[14]) &&
			LT_IS_HEX_CHAR(path[15]) &&
			LT_IS_HEX_CHAR(path[16]) &&
			LT_IS_HEX_CHAR(path[17]) &&
			LT_IS_HEX_CHAR(path[18]) &&
			path[19] == '-' &&
			LT_IS_HEX_CHAR(path[20]) &&
			LT_IS_HEX_CHAR(path[21]) &&
			LT_IS_HEX_CHAR(path[22]) &&
			LT_IS_HEX_CHAR(path[23]) &&
			path[24] == '-' &&
			LT_IS_HEX_CHAR(path[25]) &&
			LT_IS_HEX_CHAR(path[26]) &&
			LT_IS_HEX_CHAR(path[27]) &&
			LT_IS_HEX_CHAR(path[28]) &&
			path[29] == '-' &&
			LT_IS_HEX_CHAR(path[30]) &&
			LT_IS_HEX_CHAR(path[31]) &&
			LT_IS_HEX_CHAR(path[32]) &&
			LT_IS_HEX_CHAR(path[33]) &&
			path[34] == '-' &&
			LT_IS_HEX_CHAR(path[35]) &&
			LT_IS_HEX_CHAR(path[36]) &&
			LT_IS_HEX_CHAR(path[37]) &&
			LT_IS_HEX_CHAR(path[38]) &&
			LT_IS_HEX_CHAR(path[39]) &&
			LT_IS_HEX_CHAR(path[40]) &&
			LT_IS_HEX_CHAR(path[41]) &&
			LT_IS_HEX_CHAR(path[42]) &&
			LT_IS_HEX_CHAR(path[43]) &&
			LT_IS_HEX_CHAR(path[44]) &&
			LT_IS_HEX_CHAR(path[45]) &&
			LT_IS_HEX_CHAR(path[46]) &&
			path[47] == '}' &&
			path[48] == '\\')
		{
			return 49;
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
		if (path_length > 6 && (path[4] != '\\' && path[4] != '/') && path[5] == ':' && (path[6] == '\\' || path[6] == '/'))
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
		if (path_length > 2 && path[0] != '\\' && path[0] != '/' && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
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

BOOL LtWin32IsPathFullyQualifiedUtf8(SIZE_T PathLength, const char* Path)
{
	size_t path_volume_directory_part_length = lt_win32_absolute_path_volume_directory_part_length_utf8(PathLength, Path);
	if (!path_volume_directory_part_length)
	{
		return FALSE;
	}

	size_t utf16_fully_qualified_length = LtConvertUtf8ToUtf16Le(PathLength, Path, 0, 0);
	BOOL extended_prefix = ((PathLength > 4 && Path[0] == '\\' && Path[1] == '?' && Path[2] == '?' && Path[3] == '\\') || (PathLength > 4 && Path[0] == '\\' && Path[1] == '\\' && Path[2] == '?' && Path[3] == '\\'));
	if ((utf16_fully_qualified_length > (MAX_PATH - 1)) && !extended_prefix)
	{
		return FALSE;
	}

	for (size_t offset = path_volume_directory_part_length; offset != PathLength;)
	{
		size_t component_lenght = 0;
		while (offset + component_lenght != PathLength && (Path[offset + component_lenght] != '\\' && Path[offset + component_lenght] != '/'))
		{
			component_lenght++;
		}
		if (!component_lenght || (component_lenght == 1 && Path[0] == '.') || (component_lenght == 2 && Path[0] == '.' && Path[1] == '.'))
		{
			return FALSE;
		}
		offset += component_lenght + (((offset + component_lenght) != PathLength) ? 1 : 0);
	}
	return TRUE;
}

SIZE_T LtWin32GetFullyQualifiedPathUtf8(SIZE_T PathLength, const char* Path, SIZE_T BasePathLength, const char* BasePath, SIZE_T PathBufferSize, char* PathBuffer)
{
	size_t path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length_utf8(PathLength, Path);
	// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
	if (PathLength && (PathLength > path_volume_part_length) && ((Path[PathLength - 1] == '\\') || (Path[PathLength - 1] == '/')))
	{
		PathLength--;
	}
	if (!BasePathLength && !PathLength)
	{
		return 0;
	}

	if (path_volume_part_length)
	{
		// Absolute Path to fully qualified Path

		BOOL extended_prefix = ((PathLength > 4 && Path[0] == '\\' && Path[1] == '?' && Path[2] == '?' && Path[3] == '\\') || (PathLength > 4 && Path[0] == '\\' && Path[1] == '\\' && Path[2] == '?' && Path[3] == '\\'));
		BOOL network_path = FALSE;
		if (extended_prefix)
		{
			if (PathLength > 7 && (Path[4] == 'U' || Path[4] == 'u') && (Path[5] == 'N' || Path[5] == 'n') && (Path[6] == 'C' || Path[6] == 'c') && (Path[7] == '\\' || Path[7] == '/'))
			{
				network_path = TRUE;
			}
		}
		else
		{
			if (PathLength > 1 && (Path[0] == '\\' || Path[0] == '/') && (Path[1] == '\\' || Path[1] == '/'))
			{
				network_path = TRUE;
			}
		}

		size_t fully_qualified_length = path_volume_part_length;
		size_t fully_qualified_utf16_length = LtConvertUtf8ToUtf16Le(path_volume_part_length, Path, 0, 0);
		for (size_t offset = PathLength, component_count = 0, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (Path[offset - component_lenght - 1] != '\\' && Path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == '.' && Path[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					fully_qualified_length += component_lenght + 1;
					fully_qualified_utf16_length += LtConvertUtf8ToUtf16Le(component_lenght, Path + component_offset, 0, 0) + 1;
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

		if (fully_qualified_length > PathBufferSize)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = PathLength, component_erase_count = 0; offset != path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != path_volume_part_length && (Path[offset - component_lenght - 1] != '\\' && Path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == '.' && Path[component_offset + 1] == '.')
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
						PathBuffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(PathBuffer + write_offset, Path + component_offset, component_lenght * sizeof(char));
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
			char path_volume_part_character = Path[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == '/')
			{
				path_volume_part_character = '\\';
			}
			write_offset--;
			PathBuffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (network_path)
			{
				memcpy(PathBuffer + 4, "UNC\\", 4 * sizeof(char));
			}
			memcpy(PathBuffer, "\\\\?\\", 4 * sizeof(char));
		}
		else if (remove_extended_prefix && network_path)
		{
			memcpy(PathBuffer, "\\\\", 2 * sizeof(char));
		}

		return fully_qualified_length;
	}
	else
	{
		// Relative Path to fully qualified Path

		// Get volume Path part of the base directory
		size_t base_path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length_utf8(BasePathLength, BasePath);
		// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
		if (BasePathLength && (BasePathLength > base_path_volume_part_length) && ((BasePath[BasePathLength - 1] == '\\') || (BasePath[BasePathLength - 1] == '/')))
		{
			BasePathLength--;
		}
		if (!BasePathLength)
		{
			return 0;
		}

		BOOL base_path_extended_prefix = ((BasePathLength > 4 && BasePath[0] == '\\' && BasePath[1] == '?' && BasePath[2] == '?' && BasePath[3] == '\\') || (BasePathLength > 4 && BasePath[0] == '\\' && BasePath[1] == '\\' && BasePath[2] == '?' && BasePath[3] == '\\'));
		BOOL base_path_network_path = FALSE;
		if (base_path_extended_prefix)
		{
			if (BasePathLength > 7 && (BasePath[4] == 'U' || BasePath[4] == 'u') && (BasePath[5] == 'N' || BasePath[5] == 'n') && (BasePath[6] == 'C' || BasePath[6] == 'c') && (BasePath[7] == '\\' || BasePath[7] == '/'))
			{
				base_path_network_path = TRUE;
			}
		}
		else
		{
			if (BasePathLength > 1 && (BasePath[0] == '\\' || BasePath[0] == '/') && (BasePath[1] == '\\' || BasePath[1] == '/'))
			{
				base_path_network_path = TRUE;
			}
		}

		size_t relative_path_component_erase_count = 0;
		size_t relative_path_component_count = 0;
		size_t relative_path_length = 0;
		size_t relative_path_utf16_length = 0;
		for (size_t offset = PathLength; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (Path[offset - component_lenght - 1] != '\\' && Path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == '.' && Path[component_offset + 1] == '.')
			{
				relative_path_component_erase_count++;
			}
			else
			{
				if (!relative_path_component_erase_count)
				{
					relative_path_length += component_lenght + 1;
					relative_path_utf16_length += LtConvertUtf8ToUtf16Le(component_lenght, Path + component_offset, 0, 0) + 1;
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

		// Get the base Path component count and length after making it fully qualified. Also erase components as required from the relative sub Path
		size_t base_path_component_count = 0;
		size_t base_path_fully_qualified_length = base_path_volume_part_length;
		size_t base_path_fully_qualified_utf16_length = LtConvertUtf8ToUtf16Le(base_path_volume_part_length, BasePath, 0, 0);
		for (size_t offset = BasePathLength, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (BasePath[offset - component_lenght - 1] != '\\' && BasePath[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && BasePath[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && BasePath[component_offset] == '.' && BasePath[component_offset + 1] == '.')
			{
				component_erase_count++;
			}
			else
			{
				if (!component_erase_count)
				{
					base_path_fully_qualified_length += component_lenght + 1;
					base_path_fully_qualified_utf16_length += LtConvertUtf8ToUtf16Le(component_lenght, BasePath + component_offset, 0, 0) + 1;
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

		if (fully_qualified_length > PathBufferSize)
		{
			return fully_qualified_length;
		}

		size_t write_offset = fully_qualified_length;
		for (size_t offset = PathLength, component_erase_count = 0; offset;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght && (Path[offset - component_lenght - 1] != '\\' && Path[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && Path[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && Path[component_offset] == '.' && Path[component_offset + 1] == '.')
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
						PathBuffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(PathBuffer + write_offset, Path + component_offset, component_lenght * sizeof(char));
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
			PathBuffer[write_offset] = '\\';
		}

		for (size_t offset = BasePathLength, component_erase_count = relative_path_component_erase_count; offset != base_path_volume_part_length;)
		{
			size_t component_lenght = 0;
			while (offset - component_lenght != base_path_volume_part_length && (BasePath[offset - component_lenght - 1] != '\\' && BasePath[offset - component_lenght - 1] != '/'))
			{
				component_lenght++;
			}
			size_t component_offset = offset - component_lenght;
			if (component_lenght == 1 && BasePath[component_offset] == '.')
			{
				// just skip "." components
			}
			else if (component_lenght == 2 && BasePath[component_offset] == '.' && BasePath[component_offset + 1] == '.')
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
						PathBuffer[write_offset] = '\\';
					}
					write_offset -= component_lenght;
					memcpy(PathBuffer + write_offset, BasePath + component_offset, component_lenght * sizeof(char));
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
			char path_volume_part_character = BasePath[volume_part_read_offset];
			volume_part_read_offset--;
			if (path_volume_part_character == '/')
			{
				path_volume_part_character = '\\';
			}
			write_offset--;
			PathBuffer[write_offset] = path_volume_part_character;
		}
		if (add_extended_prefix)
		{
			if (base_path_network_path)
			{
				memcpy(PathBuffer + 4, "UNC\\", 4 * sizeof(char));
			}
			memcpy(PathBuffer, "\\\\?\\", 4 * sizeof(char));
		}
		else if (remove_extended_prefix && base_path_network_path)
		{
			memcpy(PathBuffer, "\\\\", 2 * sizeof(char));
		}

		return fully_qualified_length;
	}
}

SIZE_T LtWin32GetVolumeDirectoryPathUtf8(SIZE_T PathLength, const char* Path, SIZE_T BasePathLength, const char* BasePath, SIZE_T PathBufferSize, char* PathBuffer)
{
	size_t path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length_utf8(PathLength, Path);
	if (!BasePathLength && !PathLength)
	{
		return 0;
	}
	if (!path_volume_part_length)
	{
		PathLength = BasePathLength;
		Path = BasePath;
		path_volume_part_length = lt_win32_absolute_path_volume_directory_part_length_utf8(PathLength, Path);
		if (!path_volume_part_length)
		{
			return 0;
		}
	}

	BOOL extended_prefix = ((PathLength > 4 && Path[0] == '\\' && Path[1] == '?' && Path[2] == '?' && Path[3] == '\\') || (PathLength > 4 && Path[0] == '\\' && Path[1] == '\\' && Path[2] == '?' && Path[3] == '\\'));
	BOOL network_path = FALSE;
	if (extended_prefix)
	{
		if (PathLength > 7 && (Path[4] == 'U' || Path[4] == 'u') && (Path[5] == 'N' || Path[5] == 'n') && (Path[6] == 'C' || Path[6] == 'c') && (Path[7] == '\\' || Path[7] == '/'))
		{
			network_path = TRUE;
		}
	}
	else
	{
		if (PathLength > 1 && (Path[0] == '\\' || Path[0] == '/') && (Path[1] == '\\' || Path[1] == '/'))
		{
			network_path = TRUE;
		}
	}

	size_t fully_qualified_length = path_volume_part_length;
	size_t utf16_fully_qualified_length = LtConvertUtf8ToUtf16Le(fully_qualified_length, Path, 0, 0);
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

	if (fully_qualified_length > PathBufferSize)
	{
		return fully_qualified_length;
	}

	for (size_t path_volume_part_copy_end = (add_extended_prefix ? (network_path ? 6 : 4) : ((remove_extended_prefix && network_path) ? 2 : 0)), write_offset = fully_qualified_length, read_offset = path_volume_part_length - 1; write_offset != path_volume_part_copy_end;)
	{
		char path_volume_part_character = Path[read_offset];
		read_offset--;
		if (path_volume_part_character == '/')
		{
			path_volume_part_character = '\\';
		}
		write_offset--;
		PathBuffer[write_offset] = path_volume_part_character;
	}
	if (add_extended_prefix)
	{
		if (network_path)
		{
			memcpy(PathBuffer + 4, "UNC\\", 4 * sizeof(char));
		}
		memcpy(PathBuffer, "\\\\?\\", 4 * sizeof(char));
	}
	else if (remove_extended_prefix && network_path)
	{
		memcpy(PathBuffer, "\\\\", 2 * sizeof(char));
	}

	return fully_qualified_length;
}

#ifdef __cplusplus
}
#endif // __cplusplus
