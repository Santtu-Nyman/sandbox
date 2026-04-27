/*
	Win32 file path unit tests by Santtu S. Nyman.

	License:

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

	Description:

		Unit tests for FlWin32IsPathFullyQualified, FlWin32GetFullyQualifiedPath,
		FlWin32GetVolumeDirectoryPath and their UTF-8 counterparts.
*/

#define WIN32_LEAN_AND_MEAN
#include "FlUt.h"
#include "../include/FlWin32FilePath.h"
#include <stddef.h>
#include <string.h>
#include <wchar.h>

// Number of characters in a string literal, excluding the null terminator.
#define FL_WSTR_LEN(s) ((sizeof(s) / sizeof(WCHAR)) - 1)
#define FL_STR_LEN(s)  (sizeof(s) - 1)

static BOOL flFilePathEqW(SIZE_T len, const WCHAR* a, const WCHAR* expected)
{
	SIZE_T expectedLen = wcslen(expected);
	if (len != expectedLen)
		return FALSE;
	return len == 0 || memcmp(a, expected, len * sizeof(WCHAR)) == 0;
}

static BOOL flFilePathEq8(SIZE_T len, const char* a, const char* expected)
{
	SIZE_T expectedLen = strlen(expected);
	if (len != expectedLen)
		return FALSE;
	return len == 0 || memcmp(a, expected, len) == 0;
}

// ============================================================================
// FlWin32IsPathFullyQualified
// ============================================================================

// "C:\" is the drive root and is a valid fully-qualified path.
static void FlWin32FilePathUtIsFullyQualifiedDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_DriveRoot");
}

// Drive-letter path with one component.
static void FlWin32FilePathUtIsFullyQualifiedDriveSimple(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_DriveSimple");
}

// Drive-letter path several components deep.
static void FlWin32FilePathUtIsFullyQualifiedDriveDeep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar\\baz.txt";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_DriveDeep");
}

// "\\server\share\" is the UNC share root; it has no path components beyond the volume root.
static void FlWin32FilePathUtIsFullyQualifiedUncRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share\\";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_UncRoot");
}

// UNC path with path components below the share root.
static void FlWin32FilePathUtIsFullyQualifiedUncPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share\\folder\\file.txt";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_UncPath");
}

// Extended prefix "\\?\" with a drive-letter root.
static void FlWin32FilePathUtIsFullyQualifiedExtendedDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\C:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_ExtendedDrive");
}

// Extended prefix "\\?\" with UNC share.
static void FlWin32FilePathUtIsFullyQualifiedExtendedUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\UNC\\server\\share\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_ExtendedUnc");
}

// NT namespace prefix "\??\" with a drive-letter root.
static void FlWin32FilePathUtIsFullyQualifiedNtNamespace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\??\\C:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_NtNamespace");
}

// Extended prefix "\\?\" with a volume GUID root.
static void FlWin32FilePathUtIsFullyQualifiedVolumeGuid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_VolumeGuid");
}

// A relative path has no volume root and is therefore not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"foo\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_Relative");
}

// A root-relative path ("\foo") has no drive or UNC prefix and is not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedRootRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_RootRelative");
}

// "C:foo" has a drive letter but no backslash separator, making it drive-relative.
static void FlWin32FilePathUtIsNotFullyQualifiedDriveRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_DriveRelative");
}

// An empty path is not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FL_UT_CHECK(FlWin32IsPathFullyQualified(0, L"") == FALSE, "IsNotFullyQualified_Empty");
}

// A double backslash inside the path body creates an empty component, which is invalid.
static void FlWin32FilePathUtIsNotFullyQualifiedEmptyComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_EmptyComponent");
}

// ============================================================================
// FlWin32IsPathFullyQualifiedUtf8
// ============================================================================

static void FlWin32FilePathUtIsFullyQualifiedUtf8DriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_DriveRoot");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8DriveSimple(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_DriveSimple");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8DriveDeep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar\\baz.txt";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_DriveDeep");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8UncRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share\\";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_UncRoot");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8UncPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share\\folder\\file.txt";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_UncPath");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8ExtendedDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\C:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_ExtendedDrive");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8ExtendedUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\UNC\\server\\share\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_ExtendedUnc");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8NtNamespace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\??\\C:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_NtNamespace");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8VolumeGuid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_VolumeGuid");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8Relative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "foo\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_Relative");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8RootRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_RootRelative");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8DriveRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_DriveRelative");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8Empty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(0, "") == FALSE, "IsNotFullyQualifiedUtf8_Empty");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8EmptyComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_EmptyComponent");
}

// ============================================================================
// FlWin32GetFullyQualifiedPath
// ============================================================================

// A clean absolute drive path is returned unchanged.
static void FlWin32FilePathUtGetFullyQualifiedClean(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo\\bar"), "GetFullyQualified_Clean");
}

// A trailing separator is stripped from an absolute path.
static void FlWin32FilePathUtGetFullyQualifiedTrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo\\bar"), "GetFullyQualified_TrailingSep");
}

// A "." component in an absolute path is removed.
static void FlWin32FilePathUtGetFullyQualifiedDotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\.\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo\\bar"), "GetFullyQualified_DotComponent");
}

// A ".." component in an absolute path consumes the preceding component.
static void FlWin32FilePathUtGetFullyQualifiedDotDotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\..\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\bar"), "GetFullyQualified_DotDotComponent");
}

// Forward slashes are accepted as separators and normalized to backslashes.
static void FlWin32FilePathUtGetFullyQualifiedForwardSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:/foo/bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo\\bar"), "GetFullyQualified_ForwardSlash");
}

// A UNC absolute path is returned with normalized separators.
static void FlWin32FilePathUtGetFullyQualifiedUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"\\\\server\\share\\foo"), "GetFullyQualified_Unc");
}

// An extended prefix "\\?\" is removed when the result fits within MAX_PATH - 1 characters.
static void FlWin32FilePathUtGetFullyQualifiedRemoveExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo"), "GetFullyQualified_RemoveExtendedPrefix");
}

// The NT namespace prefix "\??\" is removed when the result fits within MAX_PATH - 1 characters.
static void FlWin32FilePathUtGetFullyQualifiedRemoveNtPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\??\\C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo"), "GetFullyQualified_RemoveNtPrefix");
}

// When path is absolute, basePath is ignored.
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteIgnoresBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"D:\\foo";
	static const WCHAR base[] = L"C:\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"D:\\foo"), "GetFullyQualified_AbsoluteIgnoresBase");
}

// A relative path is appended to the base path.
static void FlWin32FilePathUtGetFullyQualifiedRelativeToBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"foo\\bar";
	static const WCHAR base[] = L"C:\\baz";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\baz\\foo\\bar"), "GetFullyQualified_RelativeToBase");
}

// ".." in a relative path consumes a component from the base path.
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\foo";
	static const WCHAR base[] = L"C:\\baz\\qux";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\baz\\foo"), "GetFullyQualified_RelativeDotDot");
}

// A lone "." relative path resolves to the base path itself.
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotOnly(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L".";
	static const WCHAR base[] = L"C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\foo"), "GetFullyQualified_RelativeDotOnly");
}

// When the buffer is too small no data is written and the required size is returned.
// A subsequent call with a sufficient buffer succeeds and produces the correct result.
static void FlWin32FilePathUtGetFullyQualifiedBufferSizing(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T needed = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", 0, buf);
	FL_UT_CHECK(needed > 0, "GetFullyQualified_BufferSizing_NeededSize");
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == needed && flFilePathEqW(result, buf, L"C:\\foo\\bar"), "GetFullyQualified_BufferSizing_Success");
}

// Both path and basePath empty returns 0.
static void FlWin32FilePathUtGetFullyQualifiedBothEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(0, L"", 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_BothEmpty");
}

// A relative path with an empty base path returns 0.
static void FlWin32FilePathUtGetFullyQualifiedRelativeEmptyBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_RelativeEmptyBase");
}

// A ".." that would navigate above the volume root returns 0.
static void FlWin32FilePathUtGetFullyQualifiedDotDotAboveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\..";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_DotDotAboveRoot");
}

// ============================================================================
// FlWin32GetFullyQualifiedPathUtf8
// ============================================================================

static void FlWin32FilePathUtGetFullyQualifiedUtf8Clean(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo\\bar"), "GetFullyQualifiedUtf8_Clean");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8TrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo\\bar"), "GetFullyQualifiedUtf8_TrailingSep");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\.\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo\\bar"), "GetFullyQualifiedUtf8_DotComponent");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\..\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\bar"), "GetFullyQualifiedUtf8_DotDotComponent");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ForwardSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:/foo/bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo\\bar"), "GetFullyQualifiedUtf8_ForwardSlash");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo"), "GetFullyQualifiedUtf8_RemoveExtendedPrefix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeToBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "foo\\bar";
	static const char base[] = "C:\\baz";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\baz\\foo\\bar"), "GetFullyQualifiedUtf8_RelativeToBase");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\foo";
	static const char base[] = "C:\\baz\\qux";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\baz\\foo"), "GetFullyQualifiedUtf8_RelativeDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8BothEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(0, "", 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_BothEmpty");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\..";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_DotDotAboveRoot");
}

// ============================================================================
// FlWin32GetVolumeDirectoryPath
// ============================================================================

// A drive-letter path yields the three-character drive root.
static void FlWin32FilePathUtGetVolumeDirectoryDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_Drive");
}

// Passing the drive root itself yields the same drive root.
static void FlWin32FilePathUtGetVolumeDirectoryDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_DriveRoot");
}

// A UNC path yields the share root "\\server\share\".
static void FlWin32FilePathUtGetVolumeDirectoryUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share\\foo\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"\\\\server\\share\\"), "GetVolumeDirectory_Unc");
}

// An extended-prefix UNC path yields the plain share root (prefix removed for short paths).
static void FlWin32FilePathUtGetVolumeDirectoryExtendedUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\UNC\\server\\share\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"\\\\server\\share\\"), "GetVolumeDirectory_ExtendedUnc");
}

// The extended prefix "\\?\" is removed when the result fits within MAX_PATH - 1 characters.
static void FlWin32FilePathUtGetVolumeDirectoryRemoveExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_RemoveExtendedPrefix");
}

// When path is relative the volume is taken from basePath.
static void FlWin32FilePathUtGetVolumeDirectoryFromBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"foo\\bar";
	static const WCHAR base[] = L"C:\\baz";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_FromBase");
}

// Both path and basePath empty returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryBothEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(0, L"", 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_BothEmpty");
}

// A relative path with no base returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryRelativeNoBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"foo\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_RelativeNoBase");
}

// When the buffer is too small no data is written and the required size is returned.
static void FlWin32FilePathUtGetVolumeDirectoryBufferSizing(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T needed = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", 0, buf);
	FL_UT_CHECK(needed > 0, "GetVolumeDirectory_BufferSizing_NeededSize");
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == needed && flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_BufferSizing_Success");
}

// ============================================================================
// FlWin32GetVolumeDirectoryPathUtf8
// ============================================================================

static void FlWin32FilePathUtGetVolumeDirectoryUtf8Drive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_Drive");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8Unc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share\\foo\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\server\\share\\"), "GetVolumeDirectoryUtf8_Unc");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8RemoveExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_RemoveExtendedPrefix");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8FromBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "foo\\bar";
	static const char base[] = "C:\\baz";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_FromBase");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8BothEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(0, "", 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_BothEmpty");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8RelativeNoBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "foo\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_RelativeNoBase");
}

// ============================================================================
// Run function
// ============================================================================

void FlWin32FilePathUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// FlWin32IsPathFullyQualified
	FlWin32FilePathUtIsFullyQualifiedDriveRoot(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedDriveSimple(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedDriveDeep(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUncRoot(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUncPath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedExtendedDrive(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedExtendedUnc(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedNtNamespace(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedVolumeGuid(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedRelative(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedRootRelative(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedDriveRelative(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedEmpty(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedEmptyComponent(testCount, failCount);

	// FlWin32IsPathFullyQualifiedUtf8
	FlWin32FilePathUtIsFullyQualifiedUtf8DriveRoot(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8DriveSimple(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8DriveDeep(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8UncRoot(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8UncPath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8ExtendedDrive(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8ExtendedUnc(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8NtNamespace(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8VolumeGuid(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8Relative(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8RootRelative(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8DriveRelative(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8Empty(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8EmptyComponent(testCount, failCount);

	// FlWin32GetFullyQualifiedPath
	FlWin32FilePathUtGetFullyQualifiedClean(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedTrailingSep(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotComponent(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotDotComponent(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedForwardSlash(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUnc(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRemoveExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRemoveNtPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAbsoluteIgnoresBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeToBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotOnly(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedBufferSizing(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedBothEmpty(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeEmptyBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotDotAboveRoot(testCount, failCount);

	// FlWin32GetFullyQualifiedPathUtf8
	FlWin32FilePathUtGetFullyQualifiedUtf8Clean(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8TrailingSep(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotComponent(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotComponent(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ForwardSlash(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeToBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8BothEmpty(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveRoot(testCount, failCount);

	// FlWin32GetVolumeDirectoryPath
	FlWin32FilePathUtGetVolumeDirectoryDrive(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUnc(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryExtendedUnc(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryRemoveExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryFromBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryBothEmpty(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryRelativeNoBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryBufferSizing(testCount, failCount);

	// FlWin32GetVolumeDirectoryPathUtf8
	FlWin32FilePathUtGetVolumeDirectoryUtf8Drive(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8Unc(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8RemoveExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8FromBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8BothEmpty(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8RelativeNoBase(testCount, failCount);
}
