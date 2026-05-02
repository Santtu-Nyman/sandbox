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

// A UNC path with spaces and a long subdirectory is fully qualified.
static void FlWin32FilePathUtIsFullyQualifiedServerFilePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789\\file name.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_ServerFilePath");
}

// A non-C drive-letter path with a directory component and a file name is fully qualified.
static void FlWin32FilePathUtIsFullyQualifiedNonCDriveFilePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"E:\\directory\\file.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_NonCDriveFilePath");
}

// A bare file name with no directory separators or drive letter is not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedSimpleFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_SimpleFileName");
}

// A drive-letter path with a directory component and a file name is fully qualified.
static void FlWin32FilePathUtIsFullyQualifiedSimpleFilePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\directory\\file.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_SimpleFilePath");
}

// A path with a "." component is not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedDotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\.\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_DotComponent");
}

// A path with a ".." component is not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedDotDotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\..\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_DotDotComponent");
}

// A trailing separator creates an empty path component and is therefore not fully qualified.
static void FlWin32FilePathUtIsNotFullyQualifiedTrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar\\";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_TrailingSep");
}

// Forward slashes are accepted as separators; a drive path using "/" is fully qualified.
static void FlWin32FilePathUtIsFullyQualifiedForwardSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:/foo/bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_ForwardSlash");
}

// A lowercase drive letter is still a valid drive letter.
static void FlWin32FilePathUtIsFullyQualifiedLowercaseDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"c:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_LowercaseDrive");
}

// A path of exactly MAX_PATH - 1 characters (259) without an extended prefix is fully qualified.
static void FlWin32FilePathUtIsFullyQualifiedExactMaxPathMinus1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR path[260];
	path[0] = L'C';
	path[1] = L':';
	path[2] = L'\\';
	for (size_t i = 3; i < 259; i++)
		path[i] = L'a';
	FL_UT_CHECK(FlWin32IsPathFullyQualified(259, path) == TRUE, "IsFullyQualified_ExactMaxPathMinus1");
}

// A path of exactly MAX_PATH characters (260) without an extended prefix exceeds the limit.
static void FlWin32FilePathUtIsNotFullyQualifiedExactMaxPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR path[261];
	path[0] = L'C';
	path[1] = L':';
	path[2] = L'\\';
	for (size_t i = 3; i < 260; i++)
		path[i] = L'a';
	FL_UT_CHECK(FlWin32IsPathFullyQualified(260, path) == FALSE, "IsNotFullyQualified_ExactMaxPath");
}

// An extended-prefix path exceeding MAX_PATH is fully qualified because the prefix bypasses the limit.
static void FlWin32FilePathUtIsFullyQualifiedExtendedPrefixLongPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR path[272];
	path[0] = L'\\'; path[1] = L'\\'; path[2] = L'?'; path[3] = L'\\';
	path[4] = L'C'; path[5] = L':'; path[6] = L'\\';
	for (size_t i = 7; i < 271; i++)
		path[i] = L'a';
	FL_UT_CHECK(FlWin32IsPathFullyQualified(271, path) == TRUE, "IsFullyQualified_ExtendedPrefixLongPath");
}

// The NT namespace prefix "\??\" is also valid for UNC paths.
static void FlWin32FilePathUtIsFullyQualifiedNtNamespaceUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\??\\UNC\\server\\share\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == TRUE, "IsFullyQualified_NtNamespaceUnc");
}

// A bare UNC path with no trailing separator on the share is not a valid fully-qualified path.
static void FlWin32FilePathUtIsNotFullyQualifiedUncNoTrailingSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share";
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "IsNotFullyQualified_UncNoTrailingSlash");
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

static void FlWin32FilePathUtIsFullyQualifiedUtf8ServerFilePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789\\file name.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_ServerFilePath");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8NonCDriveFilePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "E:\\directory\\file.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_NonCDriveFilePath");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8SimpleFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_SimpleFileName");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8SimpleFilePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\directory\\file.dat";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_SimpleFilePath");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8DotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\.\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_DotComponent");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8DotDotComponent(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\..\\bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_DotDotComponent");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8TrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar\\";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_TrailingSep");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8ForwardSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:/foo/bar";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_ForwardSlash");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8LowercaseDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "c:\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_LowercaseDrive");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8ExactMaxPathMinus1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char path[260];
	path[0] = 'C';
	path[1] = ':';
	path[2] = '\\';
	for (size_t i = 3; i < 259; i++)
		path[i] = 'a';
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(259, path) == TRUE, "IsFullyQualifiedUtf8_ExactMaxPathMinus1");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8ExactMaxPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char path[261];
	path[0] = 'C';
	path[1] = ':';
	path[2] = '\\';
	for (size_t i = 3; i < 260; i++)
		path[i] = 'a';
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(260, path) == FALSE, "IsNotFullyQualifiedUtf8_ExactMaxPath");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8ExtendedPrefixLongPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char path[272];
	path[0] = '\\'; path[1] = '\\'; path[2] = '?'; path[3] = '\\';
	path[4] = 'C'; path[5] = ':'; path[6] = '\\';
	for (size_t i = 7; i < 271; i++)
		path[i] = 'a';
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(271, path) == TRUE, "IsFullyQualifiedUtf8_ExtendedPrefixLongPath");
}

static void FlWin32FilePathUtIsFullyQualifiedUtf8NtNamespaceUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\??\\UNC\\server\\share\\foo";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == TRUE, "IsFullyQualifiedUtf8_NtNamespaceUnc");
}

static void FlWin32FilePathUtIsNotFullyQualifiedUtf8UncNoTrailingSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share";
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(FL_STR_LEN(path), path) == FALSE, "IsNotFullyQualifiedUtf8_UncNoTrailingSlash");
}

// A UTF-8 path whose byte count is below MAX_PATH but whose UTF-16 length meets MAX_PATH is not fully qualified.
// U+00E9 (é) encodes as 2 UTF-8 bytes but 1 UTF-16 code unit, so 3 + 257*2 = 517 bytes encodes 260 UTF-16 units.
static void FlWin32FilePathUtIsNotFullyQualifiedUtf8MultiByteOverMaxPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char path[518];
	path[0] = 'C';
	path[1] = ':';
	path[2] = '\\';
	for (size_t i = 3; i < 517; i += 2)
	{
		path[i]     = (char)0xC3;
		path[i + 1] = (char)0xA9;
	}
	FL_UT_CHECK(FlWin32IsPathFullyQualifiedUtf8(517, path) == FALSE, "IsNotFullyQualifiedUtf8_MultiByteOverMaxPath");
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

// An absolute path with the extended prefix "\\?\" that fits within MAX_PATH - 1 characters has the prefix stripped.
static void FlWin32FilePathUtGetFullyQualifiedRemoveExtraExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\C:\\aaa";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\aaa"), "GetFullyQualified_RemoveExtraExtendedPrefix");
}

// Navigating five levels above a deep extended-prefix basePath produces a result short enough to drop the prefix.
static void FlWin32FilePathUtGetFullyQualifiedRemoveExtendedPrefixRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\..\\..\\..\\..";
	static const WCHAR base[] = L"\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd\\eeeeeeeeeeeee\\fffffffff";
	WCHAR buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, 512, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), "GetFullyQualified_RemoveExtendedPrefixRelative");
}

// A long absolute path that exceeds MAX_PATH - 1 characters receives the extended prefix "\\?\".
static void FlWin32FilePathUtGetFullyQualifiedAddExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd";
	static const WCHAR expected[] = L"\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd";
	WCHAR buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", 512, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AddExtendedPrefix");
}

// A relative path that, when appended to basePath, exceeds MAX_PATH - 1 characters receives the extended prefix "\\?\".
static void FlWin32FilePathUtGetFullyQualifiedAddExtendedPrefixRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"ddddddddddddddddddddddddddddddddddddddddddddd";
	static const WCHAR base[] = L"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
	static const WCHAR expected[] = L"\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd";
	WCHAR buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, 512, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AddExtendedPrefixRelative");
}

// A relative path that, when appended to a UNC basePath, exceeds MAX_PATH - 1 characters receives the "\\?\UNC\" extended prefix.
static void FlWin32FilePathUtGetFullyQualifiedAddExtendedPrefixUncRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"ddddddddddddddddddddddddddddddddddddddddddddddddddddddd\\eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\ffffffffffffffffffffffff";
	static const WCHAR base[] = L"\\\\server name\\share name\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccc";
	static const WCHAR expected[] = L"\\\\?\\UNC\\server name\\share name\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddddddddddddd\\eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\ffffffffffffffffffffffff";
	WCHAR buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, 512, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AddExtendedPrefixUncRelative");
}

// A bare file name relative to a non-C drive base resolves to base\file.
static void FlWin32FilePathUtGetFullyQualifiedNonCDriveRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file.dat";
	static const WCHAR base[] = L"E:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"E:\\directory\\file.dat"), "GetFullyQualified_NonCDriveRelativeFileName");
}

// A bare file name relative to a deep UNC base resolves to base\file, preserving spaces.
static void FlWin32FilePathUtGetFullyQualifiedServerRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file name.dat";
	static const WCHAR base[] = L"\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789\\file name.dat"), "GetFullyQualified_ServerRelativeFileName");
}

// A relative path combined with a base path that has a trailing separator resolves correctly.
static void FlWin32FilePathUtGetFullyQualifiedBaseTrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"directory B\\file.dat";
	static const WCHAR base[] = L"D:\\directory A\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"D:\\directory A\\directory B\\file.dat"), "GetFullyQualified_BaseTrailingSep");
}

// ".." in a relative path navigates one level above the base directory.
static void FlWin32FilePathUtGetFullyQualifiedBackFromBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\file.dat";
	static const WCHAR base[] = L"E:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"E:\\file.dat"), "GetFullyQualified_BackFromBase");
}

// ".." in a relative path navigates one level above the base directory when basePath ends with a "." component.
static void FlWin32FilePathUtGetFullyQualifiedBackFromBaseDotSuffix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\file.dat";
	static const WCHAR base[] = L"E:\\directory\\.";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"E:\\file.dat"), "GetFullyQualified_BackFromBaseDotSuffix");
}

// A bare file name relative to a base directory resolves to base\file.
static void FlWin32FilePathUtGetFullyQualifiedSimpleRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file.dat";
	static const WCHAR base[] = L"C:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\directory\\file.dat"), "GetFullyQualified_SimpleRelativeFileName");
}

// The drive root "C:\" resolves to itself.
static void FlWin32FilePathUtGetFullyQualifiedDriveRootAsPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetFullyQualified_DriveRootAsPath");
}

// A lone ".." relative to a one-level-deep base resolves to the drive root.
static void FlWin32FilePathUtGetFullyQualifiedDotDotToDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetFullyQualified_DotDotToDriveRoot");
}

// A ".." relative to the drive root itself navigates above the root and returns 0.
static void FlWin32FilePathUtGetFullyQualifiedDotDotAboveDriveRootRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_DotDotAboveDriveRootRelative");
}

// Multiple ".." components in an absolute path collapse back to the drive root.
static void FlWin32FilePathUtGetFullyQualifiedMultipleDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:\\foo\\bar\\..\\..";;
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetFullyQualified_MultipleDotDot");
}

// An extended-prefix UNC path short enough to fit within MAX_PATH - 1 has "\\?\UNC\" replaced by "\\".
static void FlWin32FilePathUtGetFullyQualifiedRemoveExtendedPrefixUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\UNC\\server\\share\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"\\\\server\\share\\foo"), "GetFullyQualified_RemoveExtendedPrefixUnc");
}

// A volume GUID path is returned with its "\\?\" prefix intact because the GUID form requires it.
static void FlWin32FilePathUtGetFullyQualifiedVolumeGuid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	static const WCHAR expected[] = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_VolumeGuid");
}

// A relative path with a base that uses forward slashes resolves correctly.
static void FlWin32FilePathUtGetFullyQualifiedForwardSlashBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file.dat";
	static const WCHAR base[] = L"C:/directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\directory\\file.dat"), "GetFullyQualified_ForwardSlashBase");
}

// A relative path starting with ".\" resolves the "." to base and appends the child component.
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L".\\foo";
	static const WCHAR base[] = L"C:\\base";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\base\\foo"), "GetFullyQualified_RelativeDotSlash");
}

// A result of exactly MAX_PATH - 1 characters (259) is returned without the "\\?\" extended prefix.
static void FlWin32FilePathUtGetFullyQualifiedExactlyMaxPathMinus1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR path[260];
	WCHAR expected[260];
	path[0] = L'C'; path[1] = L':'; path[2] = L'\\';
	expected[0] = L'C'; expected[1] = L':'; expected[2] = L'\\';
	for (size_t i = 3; i < 259; i++)
	{
		path[i]     = L'a';
		expected[i] = L'a';
	}
	expected[259] = L'\0';
	WCHAR buf[520];
	SIZE_T result = FlWin32GetFullyQualifiedPath(259, path, 0, L"", 520, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_ExactlyMaxPathMinus1");
}

// A result of exactly MAX_PATH characters (260) is returned with the "\\?\" extended prefix prepended.
static void FlWin32FilePathUtGetFullyQualifiedExactlyMaxPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	WCHAR path[261];
	WCHAR expected[265];
	path[0] = L'C'; path[1] = L':'; path[2] = L'\\';
	for (size_t i = 3; i < 260; i++)
		path[i] = L'a';
	expected[0] = L'\\'; expected[1] = L'\\'; expected[2] = L'?'; expected[3] = L'\\';
	expected[4] = L'C'; expected[5] = L':'; expected[6] = L'\\';
	for (size_t i = 7; i < 264; i++)
		expected[i] = L'a';
	expected[264] = L'\0';
	WCHAR buf[520];
	SIZE_T result = FlWin32GetFullyQualifiedPath(260, path, 0, L"", 520, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_ExactlyMaxPath");
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

static void FlWin32FilePathUtGetFullyQualifiedUtf8RemoveNtPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\??\\C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo"), "GetFullyQualifiedUtf8_RemoveNtPrefix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8Unc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\server\\share\\foo"), "GetFullyQualifiedUtf8_Unc");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteIgnoresBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "D:\\foo";
	static const char base[] = "C:\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "D:\\foo"), "GetFullyQualifiedUtf8_AbsoluteIgnoresBase");
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

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotOnly(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = ".";
	static const char base[] = "C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\foo"), "GetFullyQualifiedUtf8_RelativeDotOnly");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8BufferSizing(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar";
	char buf[MAX_PATH];
	SIZE_T needed = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", 0, buf);
	FL_UT_CHECK(needed > 0, "GetFullyQualifiedUtf8_BufferSizing_NeededSize");
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == needed && flFilePathEq8(result, buf, "C:\\foo\\bar"), "GetFullyQualifiedUtf8_BufferSizing_Success");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8BothEmpty(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(0, "", 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_BothEmpty");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeEmptyBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_RelativeEmptyBase");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\..";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_DotDotAboveRoot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtraExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\C:\\aaa";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\aaa"), "GetFullyQualifiedUtf8_RemoveExtraExtendedPrefix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefixRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\..\\..\\..\\..";
	static const char base[] = "\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd\\eeeeeeeeeeeee\\fffffffff";
	char buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, 512, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), "GetFullyQualifiedUtf8_RemoveExtendedPrefixRelative");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AddExtendedPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd";
	static const char expected[] = "\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd";
	char buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", 512, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AddExtendedPrefix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AddExtendedPrefixRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "ddddddddddddddddddddddddddddddddddddddddddddd";
	static const char base[] = "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
	static const char expected[] = "\\\\?\\C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddd";
	char buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, 512, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AddExtendedPrefixRelative");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AddExtendedPrefixUncRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "ddddddddddddddddddddddddddddddddddddddddddddddddddddddd\\eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\ffffffffffffffffffffffff";
	static const char base[] = "\\\\server name\\share name\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccc";
	static const char expected[] = "\\\\?\\UNC\\server name\\share name\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccc\\ddddddddddddddddddddddddddddddddddddddddddddddddddddddd\\eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\ffffffffffffffffffffffff";
	char buf[512];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, 512, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AddExtendedPrefixUncRelative");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8NonCDriveRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file.dat";
	static const char base[] = "E:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "E:\\directory\\file.dat"), "GetFullyQualifiedUtf8_NonCDriveRelativeFileName");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ServerRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file name.dat";
	static const char base[] = "\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789\\file name.dat"), "GetFullyQualifiedUtf8_ServerRelativeFileName");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8BaseTrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "directory B\\file.dat";
	static const char base[] = "D:\\directory A\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "D:\\directory A\\directory B\\file.dat"), "GetFullyQualifiedUtf8_BaseTrailingSep");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8BackFromBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\file.dat";
	static const char base[] = "E:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "E:\\file.dat"), "GetFullyQualifiedUtf8_BackFromBase");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8BackFromBaseDotSuffix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\file.dat";
	static const char base[] = "E:\\directory\\.";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "E:\\file.dat"), "GetFullyQualifiedUtf8_BackFromBaseDotSuffix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8SimpleRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file.dat";
	static const char base[] = "C:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\directory\\file.dat"), "GetFullyQualifiedUtf8_SimpleRelativeFileName");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DriveRootAsPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetFullyQualifiedUtf8_DriveRootAsPath");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotToDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetFullyQualifiedUtf8_DotDotToDriveRoot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveDriveRootRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_DotDotAboveDriveRootRelative");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8MultipleDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar\\..\\..";;
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetFullyQualifiedUtf8_MultipleDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefixUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\UNC\\server\\share\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\server\\share\\foo"), "GetFullyQualifiedUtf8_RemoveExtendedPrefixUnc");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8VolumeGuid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	static const char expected[] = "\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_VolumeGuid");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ForwardSlashBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file.dat";
	static const char base[] = "C:/directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\directory\\file.dat"), "GetFullyQualifiedUtf8_ForwardSlashBase");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = ".\\foo";
	static const char base[] = "C:\\base";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\base\\foo"), "GetFullyQualifiedUtf8_RelativeDotSlash");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ExactlyMaxPathMinus1(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char path[260];
	char expected[260];
	path[0] = 'C'; path[1] = ':'; path[2] = '\\';
	expected[0] = 'C'; expected[1] = ':'; expected[2] = '\\';
	for (size_t i = 3; i < 259; i++)
	{
		path[i]     = 'a';
		expected[i] = 'a';
	}
	expected[259] = '\0';
	char buf[520];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(259, path, 0, "", 520, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_ExactlyMaxPathMinus1");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ExactlyMaxPath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	char path[261];
	char expected[265];
	path[0] = 'C'; path[1] = ':'; path[2] = '\\';
	for (size_t i = 3; i < 260; i++)
		path[i] = 'a';
	expected[0] = '\\'; expected[1] = '\\'; expected[2] = '?'; expected[3] = '\\';
	expected[4] = 'C'; expected[5] = ':'; expected[6] = '\\';
	for (size_t i = 7; i < 264; i++)
		expected[i] = 'a';
	expected[264] = '\0';
	char buf[520];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(260, path, 0, "", 520, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_ExactlyMaxPath");
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

// A relative path appended to a long basePath yields the short volume root without an extended prefix.
static void FlWin32FilePathUtGetVolumeDirectoryExtendedPrefixRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"ddddddddddddddddddddddddddddddddddddddddddddd";
	static const WCHAR base[] = L"C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_ExtendedPrefixRelative");
}

// A bare file name relative to a non-C drive base yields the volume root of the base.
static void FlWin32FilePathUtGetVolumeDirectoryNonCDriveRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file.dat";
	static const WCHAR base[] = L"E:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"E:\\"), "GetVolumeDirectory_NonCDriveRelativeFileName");
}

// An absolute path on a different drive than basePath yields the volume root of path, not basePath.
static void FlWin32FilePathUtGetVolumeDirectoryOtherDriveAbsolutePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"Z:\\file.dat";
	static const WCHAR base[] = L"E:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"Z:\\"), "GetVolumeDirectory_OtherDriveAbsolutePath");
}

// A bare file name relative to a deep UNC base yields the UNC share root.
static void FlWin32FilePathUtGetVolumeDirectoryServerRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file name.dat";
	static const WCHAR base[] = L"\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"\\\\example.com\\share name\\"), "GetVolumeDirectory_ServerRelativeFileName");
}

// A relative path combined with a base path that has a trailing separator yields the correct volume root.
static void FlWin32FilePathUtGetVolumeDirectoryBaseTrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"directory B\\file.dat";
	static const WCHAR base[] = L"D:\\directory A\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"D:\\"), "GetVolumeDirectory_BaseTrailingSep");
}

// ".." in a relative path navigates above the base directory; volume root is still determined from basePath.
static void FlWin32FilePathUtGetVolumeDirectoryBackFromBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\file.dat";
	static const WCHAR base[] = L"E:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"E:\\"), "GetVolumeDirectory_BackFromBase");
}

// Same as above but basePath ends with a "." component.
static void FlWin32FilePathUtGetVolumeDirectoryBackFromBaseDotSuffix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\file.dat";
	static const WCHAR base[] = L"E:\\directory\\.";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"E:\\"), "GetVolumeDirectory_BackFromBaseDotSuffix");
}

// A bare file name relative to a base directory yields the volume root of the base.
static void FlWin32FilePathUtGetVolumeDirectorySimpleRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"file.dat";
	static const WCHAR base[] = L"C:\\directory";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_SimpleRelativeFileName");
}

// The NT namespace prefix "\??\" is stripped; the drive volume root is returned.
static void FlWin32FilePathUtGetVolumeDirectoryNtNamespace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\??\\C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_NtNamespace");
}

// A volume GUID path yields the GUID volume root, which retains the "\\?\" prefix.
static void FlWin32FilePathUtGetVolumeDirectoryVolumeGuid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	static const WCHAR expected[] = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetVolumeDirectory_VolumeGuid");
}

// A drive-relative path such as "C:foo" has no recognized absolute volume root and returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryDriveRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_DriveRelative");
}

// A path using forward slashes yields the normalized drive volume root.
static void FlWin32FilePathUtGetVolumeDirectoryForwardSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C:/foo/bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, L"C:\\"), "GetVolumeDirectory_ForwardSlash");
}

// A UNC path with no trailing separator on the share has no recognizable volume root and returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryUncNoTrailingSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, L"", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_UncNoTrailingSlash");
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

static void FlWin32FilePathUtGetVolumeDirectoryUtf8DriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_DriveRoot");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8Unc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share\\foo\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\server\\share\\"), "GetVolumeDirectoryUtf8_Unc");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8ExtendedUnc(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\UNC\\server\\share\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\server\\share\\"), "GetVolumeDirectoryUtf8_ExtendedUnc");
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

static void FlWin32FilePathUtGetVolumeDirectoryUtf8BufferSizing(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:\\foo\\bar";
	char buf[MAX_PATH];
	SIZE_T needed = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", 0, buf);
	FL_UT_CHECK(needed > 0, "GetVolumeDirectoryUtf8_BufferSizing_NeededSize");
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == needed && flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_BufferSizing_Success");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8ExtendedPrefixRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "ddddddddddddddddddddddddddddddddddddddddddddd";
	static const char base[] = "C:\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_ExtendedPrefixRelative");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8NonCDriveRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file.dat";
	static const char base[] = "E:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "E:\\"), "GetVolumeDirectoryUtf8_NonCDriveRelativeFileName");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8OtherDriveAbsolutePath(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "Z:\\file.dat";
	static const char base[] = "E:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "Z:\\"), "GetVolumeDirectoryUtf8_OtherDriveAbsolutePath");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8ServerRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file name.dat";
	static const char base[] = "\\\\example.com\\share name\\directory name\\01234567801234567890123456780123456789";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "\\\\example.com\\share name\\"), "GetVolumeDirectoryUtf8_ServerRelativeFileName");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8BaseTrailingSep(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "directory B\\file.dat";
	static const char base[] = "D:\\directory A\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "D:\\"), "GetVolumeDirectoryUtf8_BaseTrailingSep");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8BackFromBase(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\file.dat";
	static const char base[] = "E:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "E:\\"), "GetVolumeDirectoryUtf8_BackFromBase");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8BackFromBaseDotSuffix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\file.dat";
	static const char base[] = "E:\\directory\\.";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "E:\\"), "GetVolumeDirectoryUtf8_BackFromBaseDotSuffix");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8SimpleRelativeFileName(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "file.dat";
	static const char base[] = "C:\\directory";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_SimpleRelativeFileName");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8NtNamespace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\??\\C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_NtNamespace");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8VolumeGuid(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\foo";
	static const char expected[] = "\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetVolumeDirectoryUtf8_VolumeGuid");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8DriveRelative(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_DriveRelative");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8ForwardSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "C:/foo/bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, "C:\\"), "GetVolumeDirectoryUtf8_ForwardSlash");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8UncNoTrailingSlash(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, "", MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_UncNoTrailingSlash");
}

// ============================================================================
// Invalid root directory path detection
// ============================================================================

// "\\" has no server name and is not a valid root.
static void FlWin32FilePathUtInvalidRootUncNoServer(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_UncNoServer");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_UncNoServer");
}

// "CC:\": two-letter prefix before the colon is not a single drive letter.
static void FlWin32FilePathUtInvalidRootTwoLetterDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"CC:\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_TwoLetterDrive");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_TwoLetterDrive");
}

// "\\?\CC:\": extended prefix with two-letter drive is invalid.
static void FlWin32FilePathUtInvalidRootExtendedTwoLetterDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\CC:\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedTwoLetterDrive");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedTwoLetterDrive");
}

// "\:\": backslash as drive letter is not a valid drive root.
static void FlWin32FilePathUtInvalidRootBackslashAsDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\:\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_BackslashAsDrive");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_BackslashAsDrive");
}

// "\\\\?:\": four backslashes followed by "?:\" are not an extended prefix; UNC server name is empty.
static void FlWin32FilePathUtInvalidRootFourBackslashPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\\\\\?:\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_FourBackslashPrefix");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_FourBackslashPrefix");
}

// "C\": drive letter with no colon is not a valid drive root.
static void FlWin32FilePathUtInvalidRootDriveMissingColon(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"C\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_DriveMissingColon");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_DriveMissingColon");
}

// "\\?\C\": extended-prefix drive root missing the colon.
static void FlWin32FilePathUtInvalidRootExtendedDriveMissingColon(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\C\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedDriveMissingColon");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedDriveMissingColon");
}

// "\\?\UNX\server\share\": extended prefix with misspelled UNC keyword.
static void FlWin32FilePathUtInvalidRootExtendedUncTypo(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\UNX\\server\\share\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedUncTypo");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedUncTypo");
}

// "\\?\UNC\": extended UNC prefix with no server name.
static void FlWin32FilePathUtInvalidRootExtendedUncNoServer(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\UNC\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedUncNoServer");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedUncNoServer");
}

// "\\?\V____e{...}\": extended prefix with "Volume" misspelled using underscores.
static void FlWin32FilePathUtInvalidRootExtendedVolumeMisspelled(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\V____e{12345678-1234-1234-1234-123456789abc}\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeMisspelled");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeMisspelled");
}

// "\\?\Volume_12345678-1234-1234-1234-123456789abc_\": volume GUID with underscores instead of braces.
static void FlWin32FilePathUtInvalidRootExtendedVolumeGuidNoBraces(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\Volume_12345678-1234-1234-1234-123456789abc_\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeGuidNoBraces");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeGuidNoBraces");
}

// "\\?\Volume{...}_\": extra character after the closing brace of a volume GUID.
static void FlWin32FilePathUtInvalidRootExtendedVolumeGuidExtraChar(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}_\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeGuidExtraChar");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeGuidExtraChar");
}

// "\\?\VoluXe{...}\": typo in "Volume" (X instead of m).
static void FlWin32FilePathUtInvalidRootExtendedVolumeTypo(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\VoluXe{12345678-1234-1234-1234-123456789abc}\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeTypo");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeTypo");
}

// "\\?\Volume{...}*\": invalid character after the closing brace of a volume GUID.
static void FlWin32FilePathUtInvalidRootExtendedVolumeGuidInvalidChar(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc}*\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeGuidInvalidChar");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeGuidInvalidChar");
}

// "\\?\Volume{...*\": volume GUID with missing closing brace.
static void FlWin32FilePathUtInvalidRootExtendedVolumeGuidMissingBrace(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\Volume{12345678-1234-1234-1234-123456789abc*\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeGuidMissingBrace");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeGuidMissingBrace");
}

// "\\?\Volume*\": wildcard in the volume keyword prevents GUID parsing.
static void FlWin32FilePathUtInvalidRootExtendedVolumeWildcard(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\Volume*\\";
	WCHAR buf[MAX_PATH];
	FL_UT_CHECK(FlWin32IsPathFullyQualified(FL_WSTR_LEN(path), path) == FALSE, "InvalidRoot_IsFullyQualified_ExtendedVolumeWildcard");
	FL_UT_CHECK(FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf) == 0, "InvalidRoot_GetFullyQualified_ExtendedVolumeWildcard");
}

// ============================================================================
// Backtracking above volume root
// ============================================================================

// ".." from a UNC share root cannot navigate above the share root and returns 0.
static void FlWin32FilePathUtGetFullyQualifiedUncRootDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\server\\share\\..";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_UncRootDotDot");
}

// ".." from an extended-prefix drive root cannot navigate above the root and returns 0.
static void FlWin32FilePathUtGetFullyQualifiedExtendedDriveDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\C:\\..";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_ExtendedDriveDotDot");
}

// ".." from an extended UNC share root cannot navigate above the share root and returns 0.
static void FlWin32FilePathUtGetFullyQualifiedExtendedUncDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"\\\\?\\UNC\\server\\share\\..";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_ExtendedUncDotDot");
}

// A relative ".." when the base is a UNC share root navigates above the root and returns 0.
static void FlWin32FilePathUtGetFullyQualifiedDotDotAboveUncRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"\\\\server\\share\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_DotDotAboveUncRoot");
}

// A relative ".." when the base is an extended-prefix drive root navigates above the root and returns 0.
static void FlWin32FilePathUtGetFullyQualifiedDotDotAboveExtendedDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"\\\\?\\C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_DotDotAboveExtendedDriveRoot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8UncRootDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\server\\share\\..";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_UncRootDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ExtendedDriveDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\C:\\..";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_ExtendedDriveDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8ExtendedUncDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "\\\\?\\UNC\\server\\share\\..";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_ExtendedUncDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveUncRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "\\\\server\\share\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_DotDotAboveUncRoot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveExtendedDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "\\\\?\\C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_DotDotAboveExtendedDriveRoot");
}

// GetVolumeDirectoryPath: a relative ".." above the drive root returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryDotDotAboveDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_DotDotAboveDriveRoot");
}

// GetVolumeDirectoryPath: a relative ".." above the UNC share root returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryDotDotAboveUncRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"\\\\server\\share\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_DotDotAboveUncRoot");
}

// GetVolumeDirectoryPath: a relative ".." above an extended-prefix drive root returns 0.
static void FlWin32FilePathUtGetVolumeDirectoryDotDotAboveExtendedDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..";
	static const WCHAR base[] = L"\\\\?\\C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_DotDotAboveExtendedDriveRoot");
}

// GetVolumeDirectoryPath: three ".." components with a base two levels deep goes above the drive root.
static void FlWin32FilePathUtGetVolumeDirectoryTooManyDotDotDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\..\\..";;
	static const WCHAR base[] = L"C:\\A\\B";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_TooManyDotDotDrive");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8DotDotAboveDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_DotDotAboveDriveRoot");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8DotDotAboveUncRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "\\\\server\\share\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_DotDotAboveUncRoot");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8DotDotAboveExtendedDriveRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..";
	static const char base[] = "\\\\?\\C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_DotDotAboveExtendedDriveRoot");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8TooManyDotDotDrive(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\..\\..";;
	static const char base[] = "C:\\A\\B";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_TooManyDotDotDrive");
}

// ============================================================================
// Simple absolute path dot removal
// ============================================================================

// A "." component in the middle of an absolute path is removed.
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteDotMiddle(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"C:\\A\\.\\B";
	static const WCHAR expected[] = L"C:\\A\\B";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AbsoluteDotMiddle");
}

// A "." component immediately after the volume root separator is removed.
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteDotAtRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"C:\\.\\foo";
	static const WCHAR expected[] = L"C:\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AbsoluteDotAtRoot");
}

// A "." component in a UNC absolute path is removed.
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteUncDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\server\\share\\.\\foo";
	static const WCHAR expected[] = L"\\\\server\\share\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AbsoluteUncDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotMiddle(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "C:\\A\\.\\B";
	static const char expected[] = "C:\\A\\B";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AbsoluteDotMiddle");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotAtRoot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "C:\\.\\foo";
	static const char expected[] = "C:\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AbsoluteDotAtRoot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteUncDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\server\\share\\.\\foo";
	static const char expected[] = "\\\\server\\share\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AbsoluteUncDot");
}

// ============================================================================
// Simple relative path dot removal
// ============================================================================

// A ".\foo" relative path prepends "./" which is stripped, leaving "foo" appended to base.
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L".\\foo";
	static const WCHAR base[]     = L"C:\\dir";
	static const WCHAR expected[] = L"C:\\dir\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_RelativeDotPrefix");
}

// A lone "." relative path resolves to the base directory itself.
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotAlone(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L".";
	static const WCHAR base[]     = L"C:\\dir";
	static const WCHAR expected[] = L"C:\\dir";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_RelativeDotAlone");
}

// A "foo\." relative path: the trailing "." is removed; result is base appended with "foo".
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotSuffix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"foo\\.";
	static const WCHAR base[]     = L"C:\\dir";
	static const WCHAR expected[] = L"C:\\dir\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_RelativeDotSuffix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotPrefix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = ".\\foo";
	static const char base[]     = "C:\\dir";
	static const char expected[] = "C:\\dir\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_RelativeDotPrefix");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotAlone(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = ".";
	static const char base[]     = "C:\\dir";
	static const char expected[] = "C:\\dir";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_RelativeDotAlone");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotSuffix(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "foo\\.";
	static const char base[]     = "C:\\dir";
	static const char expected[] = "C:\\dir\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_RelativeDotSuffix");
}

// ============================================================================
// Absolute path multiple dot removal
// ============================================================================

// "C:\A\.\B\..\C" — dot is removed and the ".." cancels "B"; result is "C:\A\C".
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteDotAndDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"C:\\A\\.\\B\\..\\C";
	static const WCHAR expected[] = L"C:\\A\\C";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AbsoluteDotAndDotDot");
}

// "C:\A\..\.\B" — ".." cancels "A" then "." is removed; result is "C:\B".
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteDotDotAfterDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"C:\\A\\..\\.\\B";
	static const WCHAR expected[] = L"C:\\B";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AbsoluteDotDotAfterDot");
}

// "C:\A\B\..\.\C" — ".." cancels "B", "." is removed; result is "C:\A\C".
static void FlWin32FilePathUtGetFullyQualifiedAbsoluteMultipleDotAndDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"C:\\A\\B\\..\\.\\C";
	static const WCHAR expected[] = L"C:\\A\\C";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_AbsoluteMultipleDotAndDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotAndDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "C:\\A\\.\\B\\..\\C";
	static const char expected[] = "C:\\A\\C";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AbsoluteDotAndDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotDotAfterDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "C:\\A\\..\\.\\B";
	static const char expected[] = "C:\\B";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AbsoluteDotDotAfterDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteMultipleDotAndDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "C:\\A\\B\\..\\.\\C";
	static const char expected[] = "C:\\A\\C";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_AbsoluteMultipleDotAndDotDot");
}

// ============================================================================
// Relative path multiple dot removal
// ============================================================================

// ".\foo\..\bar" with base "C:\dir" — dot is stripped, "foo" then ".." cancel; result is "C:\dir\bar".
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotDotAndDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L".\\foo\\..\\bar";
	static const WCHAR base[]     = L"C:\\dir";
	static const WCHAR expected[] = L"C:\\dir\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_RelativeDotDotAndDot");
}

// "..\.\foo" with base "C:\dir\sub" — ".." pops "sub", "." stripped; result is "C:\dir\foo".
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotAfterDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"..\\.\\foo";
	static const WCHAR base[]     = L"C:\\dir\\sub";
	static const WCHAR expected[] = L"C:\\dir\\foo";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_RelativeDotAfterDotDot");
}

// "foo\.\..\bar" with base "C:\dir" — "." stripped, "foo\.." cancelled; result is "C:\dir\bar".
static void FlWin32FilePathUtGetFullyQualifiedRelativeDotBetween(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"foo\\.\\..\\bar";
	static const WCHAR base[]     = L"C:\\dir";
	static const WCHAR expected[] = L"C:\\dir\\bar";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetFullyQualified_RelativeDotBetween");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotDotAndDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = ".\\foo\\..\\bar";
	static const char base[]     = "C:\\dir";
	static const char expected[] = "C:\\dir\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_RelativeDotDotAndDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotAfterDotDot(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "..\\.\\foo";
	static const char base[]     = "C:\\dir\\sub";
	static const char expected[] = "C:\\dir\\foo";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_RelativeDotAfterDotDot");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotBetween(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "foo\\.\\..\\bar";
	static const char base[]     = "C:\\dir";
	static const char expected[] = "C:\\dir\\bar";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetFullyQualifiedUtf8_RelativeDotBetween");
}

// ============================================================================
// Trying to switch server share from relative path
// ============================================================================

// Two ".." from one level below a UNC share root exceed the root; GetFullyQualifiedPath returns 0.
static void FlWin32FilePathUtGetFullyQualifiedSwitchServerShare(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\..\\serverB\\shareB\\foo";
	static const WCHAR base[] = L"\\\\serverA\\shareA\\folder";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualified_SwitchServerShare");
}

static void FlWin32FilePathUtGetFullyQualifiedUtf8SwitchServerShare(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\..\\serverB\\shareB\\foo";
	static const char base[] = "\\\\serverA\\shareA\\folder";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetFullyQualifiedPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetFullyQualifiedUtf8_SwitchServerShare");
}

// GetVolumeDirectoryPath: navigating above a UNC root via relative path returns 0.
static void FlWin32FilePathUtGetVolumeDirectorySwitchServerShare(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[] = L"..\\..\\serverB\\shareB\\foo";
	static const WCHAR base[] = L"\\\\serverA\\shareA\\folder";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, FL_WSTR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectory_SwitchServerShare");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8SwitchServerShare(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[] = "..\\..\\serverB\\shareB\\foo";
	static const char base[] = "\\\\serverA\\shareA\\folder";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, FL_STR_LEN(base), base, MAX_PATH, buf);
	FL_UT_CHECK(result == 0, "GetVolumeDirectoryUtf8_SwitchServerShare");
}

// ============================================================================
// Weird server paths
// ============================================================================

// "\\C:\a\b\c" — server="C:", share="a"; volume root is "\\C:\a\".
static void FlWin32FilePathUtGetVolumeDirectoryWeirdServerDriveAsSvr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\C:\\a\\b\\c";
	static const WCHAR expected[] = L"\\\\C:\\a\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetVolumeDirectory_WeirdServerDriveAsSvr");
}

// "\\X\C:\a\b\c" — server="X", share="C:"; volume root is "\\X\C:\".
static void FlWin32FilePathUtGetVolumeDirectoryWeirdServerDriveAsShr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\X\\C:\\a\\b\\c";
	static const WCHAR expected[] = L"\\\\X\\C:\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetVolumeDirectory_WeirdServerDriveAsShr");
}

// "\\X\UNC\a\b\c" — server="X", share="UNC"; volume root is "\\X\UNC\".
static void FlWin32FilePathUtGetVolumeDirectoryWeirdServerUncAsShr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\X\\UNC\\a\\b\\c";
	static const WCHAR expected[] = L"\\\\X\\UNC\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetVolumeDirectory_WeirdServerUncAsShr");
}

// "\\X\Volume{GUID}\a\b\c" — server="X", share is a GUID-like string; volume root parsed as plain UNC.
static void FlWin32FilePathUtGetVolumeDirectoryWeirdServerVolumeAsShr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const WCHAR path[]     = L"\\\\X\\Volume{12345678-1234-1234-1234-123456789abc}\\a\\b\\c";
	static const WCHAR expected[] = L"\\\\X\\Volume{12345678-1234-1234-1234-123456789abc}\\";
	WCHAR buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPath(FL_WSTR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEqW(result, buf, expected), "GetVolumeDirectory_WeirdServerVolumeAsShr");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerDriveAsSvr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\C:\\a\\b\\c";
	static const char expected[] = "\\\\C:\\a\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetVolumeDirectoryUtf8_WeirdServerDriveAsSvr");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerDriveAsShr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\X\\C:\\a\\b\\c";
	static const char expected[] = "\\\\X\\C:\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetVolumeDirectoryUtf8_WeirdServerDriveAsShr");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerUncAsShr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\X\\UNC\\a\\b\\c";
	static const char expected[] = "\\\\X\\UNC\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetVolumeDirectoryUtf8_WeirdServerUncAsShr");
}

static void FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerVolumeAsShr(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static const char path[]     = "\\\\X\\Volume{12345678-1234-1234-1234-123456789abc}\\a\\b\\c";
	static const char expected[] = "\\\\X\\Volume{12345678-1234-1234-1234-123456789abc}\\";
	char buf[MAX_PATH];
	SIZE_T result = FlWin32GetVolumeDirectoryPathUtf8(FL_STR_LEN(path), path, 0, NULL, MAX_PATH, buf);
	FL_UT_CHECK(flFilePathEq8(result, buf, expected), "GetVolumeDirectoryUtf8_WeirdServerVolumeAsShr");
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
	FlWin32FilePathUtIsFullyQualifiedNonCDriveFilePath(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedSimpleFileName(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedSimpleFilePath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedServerFilePath(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedDotComponent(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedDotDotComponent(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedTrailingSep(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedForwardSlash(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedLowercaseDrive(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedExactMaxPathMinus1(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedExactMaxPath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedExtendedPrefixLongPath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedNtNamespaceUnc(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUncNoTrailingSlash(testCount, failCount);

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
	FlWin32FilePathUtIsFullyQualifiedUtf8NonCDriveFilePath(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8SimpleFileName(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8SimpleFilePath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8ServerFilePath(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8DotComponent(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8DotDotComponent(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8TrailingSep(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8ForwardSlash(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8LowercaseDrive(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8ExactMaxPathMinus1(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8ExactMaxPath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8ExtendedPrefixLongPath(testCount, failCount);
	FlWin32FilePathUtIsFullyQualifiedUtf8NtNamespaceUnc(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8UncNoTrailingSlash(testCount, failCount);
	FlWin32FilePathUtIsNotFullyQualifiedUtf8MultiByteOverMaxPath(testCount, failCount);

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
	FlWin32FilePathUtGetFullyQualifiedRemoveExtraExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRemoveExtendedPrefixRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAddExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAddExtendedPrefixRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAddExtendedPrefixUncRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedNonCDriveRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedBaseTrailingSep(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedBackFromBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedBackFromBaseDotSuffix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedServerRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedSimpleRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDriveRootAsPath(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotDotToDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotDotAboveDriveRootRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedMultipleDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRemoveExtendedPrefixUnc(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedVolumeGuid(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedForwardSlashBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotSlash(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedExactlyMaxPathMinus1(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedExactlyMaxPath(testCount, failCount);

	// FlWin32GetFullyQualifiedPathUtf8
	FlWin32FilePathUtGetFullyQualifiedUtf8Clean(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8TrailingSep(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotComponent(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotComponent(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ForwardSlash(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RemoveNtPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8Unc(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteIgnoresBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeToBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotOnly(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8BufferSizing(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8BothEmpty(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeEmptyBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtraExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefixRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AddExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AddExtendedPrefixRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AddExtendedPrefixUncRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8NonCDriveRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8BaseTrailingSep(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8BackFromBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8BackFromBaseDotSuffix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ServerRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8SimpleRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DriveRootAsPath(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotToDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveDriveRootRelative(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8MultipleDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RemoveExtendedPrefixUnc(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8VolumeGuid(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ForwardSlashBase(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotSlash(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ExactlyMaxPathMinus1(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ExactlyMaxPath(testCount, failCount);

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
	FlWin32FilePathUtGetVolumeDirectoryExtendedPrefixRelative(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryNonCDriveRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryBaseTrailingSep(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryBackFromBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryBackFromBaseDotSuffix(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryOtherDriveAbsolutePath(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryServerRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectorySimpleRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryNtNamespace(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryVolumeGuid(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryDriveRelative(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryForwardSlash(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUncNoTrailingSlash(testCount, failCount);

	// FlWin32GetVolumeDirectoryPathUtf8
	FlWin32FilePathUtGetVolumeDirectoryUtf8Drive(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8DriveRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8Unc(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8ExtendedUnc(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8RemoveExtendedPrefix(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8FromBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8BothEmpty(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8RelativeNoBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8BufferSizing(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8ExtendedPrefixRelative(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8NonCDriveRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8BaseTrailingSep(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8BackFromBase(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8BackFromBaseDotSuffix(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8OtherDriveAbsolutePath(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8ServerRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8SimpleRelativeFileName(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8NtNamespace(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8VolumeGuid(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8DriveRelative(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8ForwardSlash(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8UncNoTrailingSlash(testCount, failCount);

	// Backtracking above volume root - FlWin32GetFullyQualifiedPath
	FlWin32FilePathUtGetFullyQualifiedUncRootDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedExtendedDriveDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedExtendedUncDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotDotAboveUncRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedDotDotAboveExtendedDriveRoot(testCount, failCount);

	// Backtracking above volume root - FlWin32GetFullyQualifiedPathUtf8
	FlWin32FilePathUtGetFullyQualifiedUtf8UncRootDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ExtendedDriveDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8ExtendedUncDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveUncRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8DotDotAboveExtendedDriveRoot(testCount, failCount);

	// Backtracking above volume root - FlWin32GetVolumeDirectoryPath
	FlWin32FilePathUtGetVolumeDirectoryDotDotAboveDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryDotDotAboveUncRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryDotDotAboveExtendedDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryTooManyDotDotDrive(testCount, failCount);

	// Backtracking above volume root - FlWin32GetVolumeDirectoryPathUtf8
	FlWin32FilePathUtGetVolumeDirectoryUtf8DotDotAboveDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8DotDotAboveUncRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8DotDotAboveExtendedDriveRoot(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8TooManyDotDotDrive(testCount, failCount);

	// Simple absolute path dot removal
	FlWin32FilePathUtGetFullyQualifiedAbsoluteDotMiddle(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAbsoluteDotAtRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAbsoluteUncDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotMiddle(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotAtRoot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteUncDot(testCount, failCount);

	// Simple relative path dot removal
	FlWin32FilePathUtGetFullyQualifiedRelativeDotPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotAlone(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotSuffix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotPrefix(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotAlone(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotSuffix(testCount, failCount);

	// Absolute path multiple dot removal
	FlWin32FilePathUtGetFullyQualifiedAbsoluteDotAndDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAbsoluteDotDotAfterDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedAbsoluteMultipleDotAndDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotAndDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteDotDotAfterDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8AbsoluteMultipleDotAndDotDot(testCount, failCount);

	// Relative path multiple dot removal
	FlWin32FilePathUtGetFullyQualifiedRelativeDotDotAndDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotAfterDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedRelativeDotBetween(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotDotAndDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotAfterDotDot(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8RelativeDotBetween(testCount, failCount);

	// Trying to switch server share from relative path
	FlWin32FilePathUtGetFullyQualifiedSwitchServerShare(testCount, failCount);
	FlWin32FilePathUtGetFullyQualifiedUtf8SwitchServerShare(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectorySwitchServerShare(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8SwitchServerShare(testCount, failCount);

	// Weird server paths
	FlWin32FilePathUtGetVolumeDirectoryWeirdServerDriveAsSvr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryWeirdServerDriveAsShr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryWeirdServerUncAsShr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryWeirdServerVolumeAsShr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerDriveAsSvr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerDriveAsShr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerUncAsShr(testCount, failCount);
	FlWin32FilePathUtGetVolumeDirectoryUtf8WeirdServerVolumeAsShr(testCount, failCount);

	// Invalid root directory path detection
	FlWin32FilePathUtInvalidRootUncNoServer(testCount, failCount);
	FlWin32FilePathUtInvalidRootTwoLetterDrive(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedTwoLetterDrive(testCount, failCount);
	FlWin32FilePathUtInvalidRootBackslashAsDrive(testCount, failCount);
	FlWin32FilePathUtInvalidRootFourBackslashPrefix(testCount, failCount);
	FlWin32FilePathUtInvalidRootDriveMissingColon(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedDriveMissingColon(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedUncTypo(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedUncNoServer(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeMisspelled(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeGuidNoBraces(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeGuidExtraChar(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeTypo(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeGuidInvalidChar(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeGuidMissingBrace(testCount, failCount);
	FlWin32FilePathUtInvalidRootExtendedVolumeWildcard(testCount, failCount);
}
