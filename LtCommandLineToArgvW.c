/*
	Implementation for Win32 function CommandLineToArgvW by Santtu Nyman.
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
#include "LtCommandLineToArgvW.h"

WCHAR** LtCommandLineToArgvW(const WCHAR* lpCmdLine, int* pNumArgs)
{
	const size_t max_file_name_length = UNICODE_STRING_MAX_CHARS;
	const size_t int_max = (int)(((unsigned int)~0) >> 1);

	size_t command_length = 0;
	if (lpCmdLine)
	{
		while (lpCmdLine[command_length])
		{
			command_length++;
		}
	}

	if (!command_length)
	{
		WCHAR** executable_name_buffer = (WCHAR**)LocalAlloc(LMEM_FIXED, (2 * sizeof(WCHAR*)) + ((MAX_PATH + 1) * sizeof(WCHAR)));
		if (!executable_name_buffer)
			return 0;
		size_t executable_name_buffer_size = LocalSize(executable_name_buffer);
		if (!executable_name_buffer_size)
		{
			DWORD size_error = GetLastError();
			LocalFree(executable_name_buffer);
			SetLastError(size_error);
			return 0;
		}

		size_t executable_name_length = (size_t)GetModuleFileNameW(0, (WCHAR*)((uintptr_t)executable_name_buffer + (2 * sizeof(WCHAR*))), (DWORD)((executable_name_buffer_size - (2 * sizeof(WCHAR*))) / sizeof(WCHAR)));
		if (!executable_name_length || executable_name_length > (executable_name_buffer_size / sizeof(WCHAR)))
		{
			WCHAR** new_executable_name_buffer = (WCHAR**)LocalReAlloc(executable_name_buffer, (2 * sizeof(WCHAR*)) + ((max_file_name_length + 1) * sizeof(WCHAR)), 0);
			if (!new_executable_name_buffer)
			{
				DWORD realloc_error = GetLastError();
				LocalFree(executable_name_buffer);
				SetLastError(realloc_error);
				return 0;
			}
			executable_name_buffer = new_executable_name_buffer;
			executable_name_buffer_size = LocalSize(executable_name_buffer);
			if (!executable_name_buffer_size)
			{
				DWORD size_error = GetLastError();
				LocalFree(executable_name_buffer);
				SetLastError(size_error);
				return 0;
			}

			executable_name_length = (size_t)GetModuleFileNameW(0, (WCHAR*)((uintptr_t)executable_name_buffer + (2 * sizeof(WCHAR*))), (DWORD)((executable_name_buffer_size - (2 * sizeof(WCHAR*))) / sizeof(WCHAR)));
			if (!executable_name_length || executable_name_length > (executable_name_buffer_size / sizeof(WCHAR)))
			{
				DWORD realloc_error = GetLastError();
				LocalFree(executable_name_buffer);
				SetLastError(realloc_error);
				return 0;
			}
		}

		WCHAR** truncated_executable_name_buffer = (WCHAR**)LocalReAlloc(executable_name_buffer, (2 * sizeof(WCHAR*)) + ((executable_name_length + 1) * sizeof(WCHAR)), 0);
		if (!truncated_executable_name_buffer)
		{
			DWORD truncate_error = GetLastError();
			LocalFree(executable_name_buffer);
			SetLastError(truncate_error);
			return 0;
		}
		executable_name_buffer = truncated_executable_name_buffer;

		executable_name_buffer[0] = (WCHAR*)((uintptr_t)executable_name_buffer + (2 * sizeof(WCHAR*)));
		executable_name_buffer[1] = 0;
		*pNumArgs = 1;
		SetLastError(ERROR_SUCCESS);
		return executable_name_buffer;
	}

	size_t argument_count = 0;
	size_t argument_character_count = 0;
	int in_argument = 1;
	int in_quotes = 0;
	size_t backslash_count = 0;
	for (size_t i = 0; i != command_length; ++i)
	{
		if (lpCmdLine[i] == L'\\')
		{
			in_argument = 1;
			++backslash_count;
		}
		else if (lpCmdLine[i] == L'"')
		{
			in_argument = 1;
			int quoted_double_quote = in_quotes && i + 1 != command_length && lpCmdLine[i + 1] == L'"';
			if (backslash_count && !(backslash_count & 1))
			{
				argument_character_count += (backslash_count / 2);
				in_quotes = !in_quotes;
			}
			else if (backslash_count && (backslash_count & 1))
				argument_character_count += (backslash_count / 2) + 1;
			else
				in_quotes = !in_quotes;
			if (quoted_double_quote)
			{
				++argument_character_count;
				++i;
			}
			backslash_count = 0;
		}
		else if (lpCmdLine[i] == L' ' || lpCmdLine[i] == L'\t')
		{
			if (in_argument)
			{
				if (backslash_count)
				{
					argument_character_count += backslash_count;
					backslash_count = 0;
				}
				if (in_quotes)
					++argument_character_count;
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

	WCHAR** argument_table = (WCHAR**)LocalAlloc(LMEM_FIXED, (((size_t)argument_count + 1) * sizeof(WCHAR*)) + (((size_t)argument_character_count + (size_t)argument_count) * sizeof(WCHAR)));
	if (!argument_table)
		return 0;

	WCHAR* argument_write = (WCHAR*)((uintptr_t)argument_table + (((size_t)argument_count + 1) * sizeof(WCHAR*)));
	in_argument = 1;
	in_quotes = 0;
	backslash_count = 0;
	size_t argument_index = 0;
	argument_table[0] = argument_write;
	for (size_t i = 0; i != command_length; ++i)
	{
		if (lpCmdLine[i] == L'\\')
		{
			if (!in_argument)
			{
				in_argument = 1;
				argument_table[argument_index] = argument_write;
			}
			++backslash_count;
		}
		else if (lpCmdLine[i] == L'"')
		{
			if (!in_argument)
			{
				in_argument = 1;
				argument_table[argument_index] = argument_write;
			}
			int quoted_double_quote = in_quotes && i + 1 != command_length && lpCmdLine[i + 1] == L'"';
			if (backslash_count && !(backslash_count & 1))
			{
				for (WCHAR* fill_end = argument_write + (backslash_count / 2); argument_write != fill_end;)
					*argument_write++ = L'\\';
				in_quotes = !in_quotes;
			}
			else if (backslash_count && (backslash_count & 1))
			{
				for (WCHAR* fill_end = argument_write + (backslash_count / 2); argument_write != fill_end;)
					*argument_write++ = L'\\';
				*argument_write++ = L'\"';
			}
			else
				in_quotes = !in_quotes;
			if (quoted_double_quote)
			{
				*argument_write++ = L'\"';
				++i;
			}
			backslash_count = 0;
		}
		else if (lpCmdLine[i] == L' ' || lpCmdLine[i] == L'\t')
		{
			if (in_argument)
			{
				if (backslash_count)
				{
					for (WCHAR* fill_end = argument_write + backslash_count; argument_write != fill_end;)
						*argument_write++ = L'\\';
					backslash_count = 0;
				}
				if (in_quotes)
					*argument_write++ = lpCmdLine[i];
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
				for (WCHAR* fill_end = argument_write + backslash_count; argument_write != fill_end;)
					*argument_write++ = L'\\';
				backslash_count = 0;
			}
			*argument_write++ = lpCmdLine[i];
		}
	}
	if (in_argument)
	{
		if (backslash_count)
		{
			for (WCHAR* fill_end = argument_write + backslash_count; argument_write != fill_end;)
				*argument_write++ = L'\\';
			backslash_count = 0;
		}
		*argument_write++ = 0;
		++argument_index;
	}
	argument_table[argument_count] = 0;

	if (argument_count > int_max)
	{
		LocalFree(argument_table);
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	*pNumArgs = (int)argument_count;
	SetLastError(ERROR_SUCCESS);
	return argument_table;
}

#ifdef __cplusplus
}
#endif
