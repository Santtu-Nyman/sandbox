/*
	UTF-8 version for Win32 function CommandLineToArgvW by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/sandbox

	Description
		Alternative implementation of Win32 function CommandLineToArgvW.
		The function is not identical to CommandLineToArgvW.
		There are three differences to the functioning of CommandLineToArgvW.
		The strings in argument table are UTF-8 encoded,
		memory is allocated with VirtualAlloc instead LocalAlloc and
		the function returns a Win32 error code.

		This function does only depend on kernel32.dll, but it does need "LtUtf8Utf16Converter.c" for UTF conversions.
		It is pretty much impossible to do any better on dependencies with Windows than this.

		Parameter native_command specifies the native command returned by GetCommandLineW.
		Read documentation of CommandLineToArgvW function from Microsoft for command parsing rules.

		Parameter argument_count_address is pointer to variable that will receive number of arguments.

		Parameter argument_table_address is pointer to variable that will receive address of first elements in the argument table.
		The table has one extra element at the end at after all arguments for making this more useful with the c main function.

		The argument table and the strings are allocated in single block of memory.
		This block can be freed using VirtualFree with type MEM_RELEASE.
		Note that with MEM_RELEASE type VirtualFree size must be zero.

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

#ifndef FL_WIN32_COMMAND_LINE_TO_UTF8_ARGUMENTS_H
#define FL_WIN32_COMMAND_LINE_TO_UTF8_ARGUMENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include <stddef.h>
	
DWORD FlWin32CommandLineToUtf8Arguments(const WCHAR* native_command, size_t* argument_count_address, char*** argument_table_address);

#ifdef __cplusplus
}
#endif

#endif // FL_WIN32_COMMAND_LINE_TO_UTF8_ARGUMENTS_H
