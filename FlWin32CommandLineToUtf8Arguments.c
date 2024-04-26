/*
	UTF-8 version for Win32 function CommandLineToArgvW by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/sandbox

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
#endif

#define WIN32_LEAN_AND_MEAN
#include "FlWin32CommandLineToUtf8Arguments.h"
#include "FlUtf8Utf16Converter.h"

DWORD FlWin32CommandLineToUtf8Arguments(const WCHAR* native_command, size_t* argument_count_address, char*** argument_table_address)
{
	const size_t max_file_name_length = UNICODE_STRING_MAX_CHARS;

	SYSTEM_INFO system_info;
	memset(&system_info, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&system_info);
	size_t page_size = (size_t)system_info.dwPageSize;
	if (!page_size)
	{
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
		page_size = 0x1000;
#else
		page_size = 0x10000;
#endif
	}

	size_t native_command_length = 0;
	if (native_command)
	{
		while (native_command[native_command_length])
		{
			native_command_length++;
		}
	}

	if (!native_command_length)
	{
		size_t utf16_executable_name_size = (((MAX_PATH + 1) * sizeof(WCHAR)) + (page_size - 1)) & ~(page_size - 1);
		WCHAR* utf16_executable_name = (WCHAR*)VirtualAlloc(0, utf16_executable_name_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!utf16_executable_name)
		{
			return ERROR_OUTOFMEMORY;
		}

		size_t utf16_executable_name_length = (size_t)GetModuleFileNameW(0, utf16_executable_name, (DWORD)(utf16_executable_name_size / sizeof(WCHAR)));
		if (!utf16_executable_name_length || utf16_executable_name_length > (utf16_executable_name_size / sizeof(WCHAR)))
		{
			VirtualFree(utf16_executable_name, 0, MEM_RELEASE);
			utf16_executable_name_size = (((max_file_name_length + 1) * sizeof(WCHAR)) + (page_size - 1)) & ~(page_size - 1);
			utf16_executable_name = (WCHAR*)VirtualAlloc(0, utf16_executable_name_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (!utf16_executable_name)
			{
				return ERROR_OUTOFMEMORY;
			}

			utf16_executable_name_length = (size_t)GetModuleFileNameW(0, utf16_executable_name, (DWORD)(utf16_executable_name_size / sizeof(WCHAR)));
			if (!utf16_executable_name_length || utf16_executable_name_length > (utf16_executable_name_size / sizeof(WCHAR)))
			{
				DWORD GetModuleFileNameError = GetLastError();
				if (GetModuleFileNameError == ERROR_SUCCESS)
				{
					GetModuleFileNameError = ERROR_UNIDENTIFIED_ERROR;
				}
				VirtualFree(utf16_executable_name, 0, MEM_RELEASE);
				return GetModuleFileNameError;
			}
		}
		
		size_t utf8_executable_name_length = FlConvertUtf16LeToUtf8(utf16_executable_name_length, utf16_executable_name, 0, 0);
		size_t utf8_table_size = (((2 * sizeof(char*)) + (utf8_executable_name_length + 1)) + (page_size - 1)) & ~(page_size - 1);
		char** utf8_table = (char**)VirtualAlloc(0, utf8_table_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!utf8_table)
		{
			VirtualFree(utf16_executable_name, 0, MEM_RELEASE);
			return ERROR_OUTOFMEMORY;
		}
		char* utf8_executable_name = (char*)((uintptr_t)utf8_table + (2 * sizeof(char*)));
		utf8_table[0] = utf8_executable_name;
		utf8_table[1] = 0;
		FlConvertUtf16LeToUtf8(utf16_executable_name_length, utf16_executable_name, utf8_executable_name_length, utf8_executable_name);
		utf8_executable_name[utf8_executable_name_length] = 0;

		VirtualFree(utf16_executable_name, 0, MEM_RELEASE);

		*argument_count_address = 1;
		*argument_table_address = utf8_table;
		return 0;
	}

	size_t command_length = FlConvertUtf16LeToUtf8(native_command_length, native_command, 0, 0);
	size_t command_size = (((size_t)command_length + 1) + (page_size - 1)) & ~(page_size - 1);
	char* command = (char*)VirtualAlloc(0, command_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!command)
	{
		return ERROR_OUTOFMEMORY;
	}
	FlConvertUtf16LeToUtf8(native_command_length, native_command, command_length, command);
	command[command_length] = 0;

	size_t argument_count = 0;
	size_t argument_character_count = 0;
	size_t in_argument = 1;
	size_t in_quotes = 0;
	size_t backslash_count = 0;
	for (size_t i = 0; i != command_length; ++i)
	{
		if (command[i] == '\\')
		{
			in_argument = 1;
			++backslash_count;
		}
		else if (command[i] == '"')
		{
			in_argument = 1;
			int quoted_double_quote = in_quotes && i + 1 != command_length && command[i + 1] == '"';
			if (backslash_count && !(backslash_count & 1))
			{
				argument_character_count += (backslash_count / 2);
				in_quotes = !in_quotes;
			}
			else if (backslash_count && (backslash_count & 1))
			{
				argument_character_count += (backslash_count / 2) + 1;
			}
			else
			{
				in_quotes = !in_quotes;
			}
			if (quoted_double_quote)
			{
				++argument_character_count;
				++i;
			}
			backslash_count = 0;
		}
		else if (command[i] == ' ' || command[i] == '\t')
		{
			if (in_argument)
			{
				if (backslash_count)
				{
					argument_character_count += backslash_count;
					backslash_count = 0;
				}
				if (in_quotes)
				{
					++argument_character_count;
				}
				else
				{
					in_argument = 0;
					++argument_count;
				}
			}
		}
		else
		{
			in_argument = 1;
			if (backslash_count)
			{
				argument_character_count += backslash_count;
				backslash_count = 0;
			}
			++argument_character_count;
		}
	}
	if (in_argument)
	{
		if (backslash_count)
		{
			argument_character_count += backslash_count;
			backslash_count = 0;
		}
		++argument_count;
	}

	size_t argument_table_size = (((((size_t)argument_count + 1) * sizeof(char*)) + (((size_t)argument_character_count + (size_t)argument_count))) + (page_size - 1)) & ~(page_size - 1);
	char** argument_table = (char**)VirtualAlloc(0, argument_table_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!argument_table)
	{
		VirtualFree(command, 0, MEM_RELEASE);
		return ERROR_OUTOFMEMORY;
	}

	char* argument_write = (char*)((uintptr_t)argument_table + (((size_t)argument_count + 1) * sizeof(char*)));
	in_argument = 1;
	in_quotes = 0;
	backslash_count = 0;
	size_t argument_index = 0;
	argument_table[0] = argument_write;
	for (int i = 0; i != command_length; ++i)
	{
		if (command[i] == '\\')
		{
			if (!in_argument)
			{
				in_argument = 1;
				argument_table[argument_index] = argument_write;
			}
			++backslash_count;
		}
		else if (command[i] == '"')
		{
			if (!in_argument)
			{
				in_argument = 1;
				argument_table[argument_index] = argument_write;
			}
			int quoted_double_quote = in_quotes && i + 1 != command_length && command[i + 1] == '"';
			if (backslash_count && !(backslash_count & 1))
			{
				for (char* fill_end = argument_write + (backslash_count / 2); argument_write != fill_end;)
				{
					*argument_write++ = '\\';
				}
				in_quotes = !in_quotes;
			}
			else if (backslash_count && (backslash_count & 1))
			{
				for (char* fill_end = argument_write + (backslash_count / 2); argument_write != fill_end;)
				{
					*argument_write++ = '\\';
				}
				*argument_write++ = '\"';
			}
			else
			{
				in_quotes = !in_quotes;
			}
			if (quoted_double_quote)
			{
				*argument_write++ = '\"';
				++i;
			}
			backslash_count = 0;
		}
		else if (command[i] == ' ' || command[i] == '\t')
		{
			if (in_argument)
			{
				if (backslash_count)
				{
					for (char* fill_end = argument_write + backslash_count; argument_write != fill_end;)
					{
						*argument_write++ = '\\';
					}
					backslash_count = 0;
				}
				if (in_quotes)
				{
					*argument_write++ = command[i];
				}
				else
				{
					in_argument = 0;
					*argument_write++ = 0;
					++argument_index;
				}
			}
		}
		else
		{
			if (!in_argument)
			{
				in_argument = 1;
				argument_table[argument_index] = argument_write;
			}
			if (backslash_count)
			{
				for (char* fill_end = argument_write + backslash_count; argument_write != fill_end;)
				{
					*argument_write++ = '\\';
				}
				backslash_count = 0;
			}
			*argument_write++ = command[i];
		}
	}
	if (in_argument)
	{
		if (backslash_count)
		{
			for (char* fill_end = argument_write + backslash_count; argument_write != fill_end;)
			{
				*argument_write++ = '\\';
			}
			backslash_count = 0;
		}
		*argument_write++ = 0;
		++argument_index;
	}
	argument_table[argument_count] = 0;

	VirtualFree(command, 0, MEM_RELEASE);

	*argument_count_address = argument_count;
	*argument_table_address = argument_table;
	return 0;
}

#ifdef __cplusplus
}
#endif