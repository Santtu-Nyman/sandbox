/*
	Win32 command line parsing utility by by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/sandbox

	Description
		This file provides an API for Windows command line parsing with both wide char (UTF-16) and UTF-8

		FlCommandLineToArgvW is an alternative implementation of Win32 function CommandLineToArgvW.
		The point of this is to be able to do the normal command line parsing wihtout needing to link Shell32.dll.

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

#ifndef FL_WIN32_COMMAND_LINE_H
#define FL_WIN32_COMMAND_LINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include <stddef.h>
#include "FlSAL.h"
#include "flHRESULT.h"

HRESULT FlWin32CommandLineToArgumentsUtf16(_In_ _Null_terminated_ const WCHAR* win32CommandLine, _In_ size_t bufferSize, _Out_writes_bytes_to_(bufferSize, *bufferRequiredAddress) WCHAR** argumentBuffer, _Out_ size_t* bufferRequiredAddress, _Out_ size_t* argumentCountAddress);
/*
	Function:
		FlWin32CommandLineToArgumentsUtf16

	Description:
		This function parses a null-terminated UTF-16 Win32 command line string into an array of
		null-terminated UTF-16 argument strings.
		The result is written into a single caller-provided buffer as a contiguous block: a pointer
		table of (argumentCount + 1 for C STD library compatibility) WCHAR* entries followed immediately by the packed, null-terminated
		argument strings. The final pointer entry in the table is NULL.
		If win32CommandLine is NULL or an empty string, the function obtains the current executable
		path via GetModuleFileNameW and produces a single argument containing that path.
		To determine the required buffer size without writing any output, call with bufferSize set to 0
		and argumentBuffer set to NULL. The function returns HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
		and writes the required size to *bufferRequiredAddress and the argument count to *argumentCountAddress.

	Parameters:
		win32CommandLine:
			Null-terminated UTF-16 Win32 command line string to parse.
			May be NULL or an empty string, in which case the current executable path is used.

		bufferSize:
			Size in bytes of the buffer pointed to by argumentBuffer.
			Set to 0 for a size query.

		argumentBuffer:
			Caller-provided buffer that receives the argument pointer table followed by the
			null-terminated argument strings.
			May be NULL when bufferSize is 0.

		bufferRequiredAddress:
			Receives the required buffer size in bytes.
			On S_OK this is the number of bytes written; on ERROR_INSUFFICIENT_BUFFER this is the
			number of bytes needed to hold the result.

		argumentCountAddress:
			Receives the number of arguments parsed.

	Return value:
		Returns S_OK on success.
		Returns HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) when bufferSize is smaller than the
		required size; argumentBuffer is not modified and *bufferRequiredAddress and
		*argumentCountAddress are set to the values needed.
		Returns another HRESULT error code on failure.
*/

HRESULT FlWin32CommandLineToArgumentsUtf8(_In_ _Null_terminated_ const WCHAR* win32CommandLine, _In_ size_t bufferSize, _Out_writes_bytes_to_(bufferSize, *bufferRequiredAddress) char** argumentBuffer, _Out_ size_t* bufferRequiredAddress, _Out_ size_t* argumentCountAddress);
/*
	Function:
		FlWin32CommandLineToArgumentsUtf8

	Description:
		UTF-8 version of FlWin32CommandLineToArgumentsUtf16.
		This function parses a null-terminated UTF-16 Win32 command line string into an array of
		null-terminated UTF-8 argument strings.
		The result is written into a single caller-provided buffer as a contiguous block: a pointer
		table of (argumentCount + 1 for C STD library compatibility) char* entries followed immediately by the packed, null-terminated
		UTF-8 argument strings. The final pointer entry in the table is NULL.
		If win32CommandLine is NULL or an empty string, the function obtains the current executable
		path via GetModuleFileNameW and produces a single argument containing that path converted to UTF-8.
		To determine the required buffer size without writing any output, call with bufferSize set to 0
		and argumentBuffer set to NULL. The function returns HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
		and writes the required size to *bufferRequiredAddress and the argument count to *argumentCountAddress.

	Parameters:
		win32CommandLine:
			Null-terminated UTF-16 Win32 command line string to parse.
			May be NULL or an empty string, in which case the current executable path is used.

		bufferSize:
			Size in bytes of the buffer pointed to by argumentBuffer.
			Set to 0 for a size query.

		argumentBuffer:
			Caller-provided buffer that receives the argument pointer table followed by the
			null-terminated UTF-8 argument strings.
			May be NULL when bufferSize is 0.

		bufferRequiredAddress:
			Receives the required buffer size in bytes.
			On S_OK this is the number of bytes written; on ERROR_INSUFFICIENT_BUFFER this is the
			number of bytes needed to hold the result.

		argumentCountAddress:
			Receives the number of arguments parsed.

	Return value:
		Returns S_OK on success.
		Returns HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) when bufferSize is smaller than the
		required size; argumentBuffer is not modified and *bufferRequiredAddress and
		*argumentCountAddress are set to the values needed.
		Returns another HRESULT error code on failure.
*/

WCHAR** FlCommandLineToArgvW(_In_ _Null_terminated_ const WCHAR* win32CommandLine, _Out_ int* argumentCountAddress);
/*
	Function:
		FlCommandLineToArgvW

	Description:
		This function is an alternative implementation of the Win32 function CommandLineToArgvW
		that does not require linking Shell32.dll.
		It parses a null-terminated UTF-16 Win32 command line string into an array of null-terminated
		UTF-16 argument strings. The returned buffer is allocated with LocalAlloc and must be freed
		by the caller using LocalFree.

	Parameters:
		win32CommandLine:
			Null-terminated UTF-16 Win32 command line string to parse.
			May be NULL or an empty string, in which case the current executable path is used as
			the sole argument.

		argumentCountAddress:
			Receives the number of arguments parsed.

	Return value:
		Returns a pointer to a NULL-terminated array of WCHAR* argument pointers on success.
		The returned buffer must be freed with LocalFree.
		Returns NULL on failure; the error code can be retrieved with GetLastError.
*/

#ifdef __cplusplus
}
#endif

#endif // FL_WIN32_COMMAND_LINE_H
