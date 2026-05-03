/*
	Win32 file path manipulation library by Santtu S. Nyman.

	Version history
		version 1.0.2 2026-05-01
			Bug fixes.
		version 1.0.1 2024-04-01
			Bug fixes.
		version 1.0.0 2023-02-25
			First publicly available version.

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

#ifndef FL_WIN32_FILE_PATH_H
#define FL_WIN32_FILE_PATH_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <Windows.h>
#include "FlSAL.h"

BOOL FlWin32IsPathFullyQualified(_In_ SIZE_T pathLength, _In_reads_(pathLength) const WCHAR* path);
/*
	Function:
		FlWin32IsPathFullyQualified

	Description:
		This function tests whether a UTF-16 path is fully qualified.
		A fully qualified path has a valid volume root (a drive-letter root such as "C:\",
		a UNC share root such as "\\server\share\", or an extended-prefix variant using
		"\\?\" or "\??\" including volume GUID paths), contains no "." or ".." components,
		contains no empty path components, and does not exceed MAX_PATH - 1 characters
		unless the path already carries an extended prefix ("\\?\" or "\??\").
		Both "\" and "/" are accepted as path separator characters.
		The path does not need to be null-terminated.

	Parameters:
		pathLength:
			Number of WCHAR characters in path.

		path:
			The UTF-16 path to test.

	Return value:
		Returns TRUE if the path is fully qualified, FALSE otherwise.
*/

_Success_(return != 0 && return <= pathBufferSize) SIZE_T FlWin32GetFullyQualifiedPath(_In_ SIZE_T pathLength, _In_reads_(pathLength) const WCHAR* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const WCHAR* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_opt_(pathBufferSize, return) WCHAR* pathBuffer);
/*
	Function:
		FlWin32GetFullyQualifiedPath

	Description:
		This function resolves a UTF-16 path to a fully qualified path.
		If path is absolute (contains a volume root) it is normalized in place: "." components
		are removed, ".." components are resolved, trailing separators are stripped, and all
		separator characters are normalized to "\".
		If path is relative it is resolved against basePath, which must be absolute. Leading ".."
		components in path consume trailing components of basePath.
		The "\\?\" extended prefix is added automatically when the result would exceed
		MAX_PATH - 1 characters, and removed when the result fits within MAX_PATH - 1 characters.
		For UNC paths the extended form is "\\?\UNC\server\share\...".
		basePath is ignored when path is already absolute.
		Both "\" and "/" are accepted as separator characters in both path and basePath.
		Neither path nor basePath need to be null-terminated.

	Parameters:
		pathLength:
			Number of WCHAR characters in path.

		path:
			The UTF-16 path to resolve. May be relative or absolute.

		basePathLength:
			Number of WCHAR characters in basePath.

		basePath:
			The absolute base directory path used when path is relative.
			Ignored when path is already absolute.

		pathBufferSize:
			Capacity of pathBuffer in WCHAR characters.

		pathBuffer:
			Buffer that receives the fully qualified path.
			The result is not null-terminated.

	Return value:
		Returns the number of WCHAR characters written to pathBuffer.
		If pathBufferSize is smaller than the required length, no data is written and
		the required length is returned so the caller can allocate a sufficient buffer
		and retry.
		Returns 0 on error: when ".." components navigate above the volume root, or when
		both path and basePath are empty or unusable.
*/

_Success_(return != 0 && return <= pathBufferSize) SIZE_T FlWin32GetVolumeDirectoryPath(_In_ SIZE_T pathLength, _In_reads_(pathLength) const WCHAR* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const WCHAR* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_opt_(pathBufferSize, return) WCHAR* pathBuffer);
/*
	Function:
		FlWin32GetVolumeDirectoryPath

	Description:
		This function extracts the volume root directory from a UTF-16 path.
		The volume root is the shortest prefix that identifies the volume: a drive-letter root
		such as "C:\", a UNC share root such as "\\server\share\", or an extended-prefix
		variant such as "\\?\C:\" or "\\?\UNC\server\share\".
		If path is relative and therefore has no volume root, basePath is used instead to
		determine the volume.
		The "\\?\" extended prefix is added automatically when the result would exceed
		MAX_PATH - 1 characters, and removed when the result fits within MAX_PATH - 1 characters.
		Separator characters in the result are normalized to "\".
		Neither path nor basePath need to be null-terminated.

	Parameters:
		pathLength:
			Number of WCHAR characters in path.

		path:
			The UTF-16 path from which to extract the volume root.
			May be relative or absolute.

		basePathLength:
			Number of WCHAR characters in basePath.

		basePath:
			Fallback absolute path used to determine the volume when path is relative.

		pathBufferSize:
			Capacity of pathBuffer in WCHAR characters.

		pathBuffer:
			Buffer that receives the volume root path.
			The result is not null-terminated.

	Return value:
		Returns the number of WCHAR characters written to pathBuffer.
		If pathBufferSize is smaller than the required length, no data is written and
		the required length is returned so the caller can allocate a sufficient buffer
		and retry.
		Returns 0 on error: when both path and basePath are empty, or when neither
		contains a recognizable volume root.
*/

BOOL FlWin32IsPathFullyQualifiedUtf8(_In_ SIZE_T pathLength, _In_reads_(pathLength) const char* path);
/*
	Function:
		FlWin32IsPathFullyQualifiedUtf8

	Description:
		UTF-8 version of FlWin32IsPathFullyQualified.
		This function tests whether a UTF-8 path is fully qualified.
		A fully qualified path has a valid volume root (a drive-letter root such as "C:\",
		a UNC share root such as "\\server\share\", or an extended-prefix variant using
		"\\?\" or "\??\" including volume GUID paths), contains no "." or ".." components,
		contains no empty path components, and does not exceed MAX_PATH - 1 characters
		when measured in UTF-16 code units unless the path already carries an extended
		prefix ("\\?\" or "\??\").
		Both "\" and "/" are accepted as path separator characters.
		The path does not need to be null-terminated.

	Parameters:
		pathLength:
			Number of bytes in path.

		path:
			The UTF-8 path to test.

	Return value:
		Returns TRUE if the path is fully qualified, FALSE otherwise.
*/

_Success_(return != 0 && return <= pathBufferSize) SIZE_T FlWin32GetFullyQualifiedPathUtf8(_In_ SIZE_T pathLength, _In_reads_(pathLength) const char* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const char* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_opt_(pathBufferSize, return) char* pathBuffer);
/*
	Function:
		FlWin32GetFullyQualifiedPathUtf8

	Description:
		UTF-8 version of FlWin32GetFullyQualifiedPath.
		This function resolves a UTF-8 path to a fully qualified path.
		If path is absolute (contains a volume root) it is normalized in place: "." components
		are removed, ".." components are resolved, trailing separators are stripped, and all
		separator characters are normalized to "\".
		If path is relative it is resolved against basePath, which must be absolute. Leading ".."
		components in path consume trailing components of basePath.
		The "\\?\" extended prefix is added automatically when the result would exceed
		MAX_PATH - 1 UTF-16 code units, and removed when the result fits within MAX_PATH - 1
		UTF-16 code units. The MAX_PATH decision uses the UTF-16 encoded length, not the
		UTF-8 byte length.
		For UNC paths the extended form is "\\?\UNC\server\share\...".
		basePath is ignored when path is already absolute.
		Both "\" and "/" are accepted as separator characters in both path and basePath.
		Neither path nor basePath need to be null-terminated.

	Parameters:
		pathLength:
			Number of bytes in path.

		path:
			The UTF-8 path to resolve. May be relative or absolute.

		basePathLength:
			Number of bytes in basePath.

		basePath:
			The absolute base directory path used when path is relative.
			Ignored when path is already absolute.

		pathBufferSize:
			Capacity of pathBuffer in bytes.

		pathBuffer:
			Buffer that receives the fully qualified path.
			The result is not null-terminated.

	Return value:
		Returns the number of bytes written to pathBuffer.
		If pathBufferSize is smaller than the required length, no data is written and
		the required length is returned so the caller can allocate a sufficient buffer
		and retry.
		Returns 0 on error: when ".." components navigate above the volume root, or when
		both path and basePath are empty or unusable.
*/

_Success_(return != 0 && return <= pathBufferSize) SIZE_T FlWin32GetVolumeDirectoryPathUtf8(_In_ SIZE_T pathLength, _In_reads_(pathLength) const char* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const char* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_opt_(pathBufferSize, return) char* pathBuffer);
/*
	Function:
		FlWin32GetVolumeDirectoryPathUtf8

	Description:
		UTF-8 version of FlWin32GetVolumeDirectoryPath.
		This function extracts the volume root directory from a UTF-8 path.
		The volume root is the shortest prefix that identifies the volume: a drive-letter root
		such as "C:\", a UNC share root such as "\\server\share\", or an extended-prefix
		variant such as "\\?\C:\" or "\\?\UNC\server\share\".
		If path is relative and therefore has no volume root, basePath is used instead to
		determine the volume.
		The "\\?\" extended prefix is added automatically when the result would exceed
		MAX_PATH - 1 UTF-16 code units, and removed when the result fits within MAX_PATH - 1
		UTF-16 code units. The MAX_PATH decision uses the UTF-16 encoded length, not the
		UTF-8 byte length.
		Separator characters in the result are normalized to "\".
		Neither path nor basePath need to be null-terminated.

	Parameters:
		pathLength:
			Number of bytes in path.

		path:
			The UTF-8 path from which to extract the volume root.
			May be relative or absolute.

		basePathLength:
			Number of bytes in basePath.

		basePath:
			Fallback absolute path used to determine the volume when path is relative.

		pathBufferSize:
			Capacity of pathBuffer in bytes.

		pathBuffer:
			Buffer that receives the volume root path.
			The result is not null-terminated.

	Return value:
		Returns the number of bytes written to pathBuffer.
		If pathBufferSize is smaller than the required length, no data is written and
		the required length is returned so the caller can allocate a sufficient buffer
		and retry.
		Returns 0 on error: when both path and basePath are empty, or when neither
		contains a recognizable volume root.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_WIN32_FILE_PATH_H
