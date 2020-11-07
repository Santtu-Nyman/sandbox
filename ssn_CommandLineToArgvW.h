/*
	Implementation for Win32 function CommandLineToArgvW by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/sandbox

	Description
		Alternative implementation of Win32 function CommandLineToArgvW.

		The point of this implementation is to avoid linking the Shell32.dll.
		This dll has approximately one useful function which is CommandLineToArgvW and this
		is annoying, because in order to use it you need link the whole bloated module.

		My implementation of CommandLineToArgvW does only depend on kernel32.dll.
		It is pretty much impossible to do any better on dependencies with Windows than this.

		Read documentation of CommandLineToArgvW function from Microsoft for understanding how this function works.
		The functioning of CommandLineToArgvW and ssn_CommandLineToArgvW are identical.

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

#ifndef SSN_COMMAND_LINE_TO_ARGV_W_H
#define SSN_COMMAND_LINE_TO_ARGV_W_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>

WCHAR** ssn_CommandLineToArgvW(const WCHAR* lpCmdLine, int* pNumArgs);

#ifdef __cplusplus
}
#endif

#endif