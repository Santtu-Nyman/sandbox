/*
	Foundation Library unit test runner by Santtu S. Nyman.

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

		Entry point for the Foundation Library unit test suite.

		Results are reported per feature category, followed by an overall
		summary line.  Each category maps to one UT module.

		To add a new UT module:
		  1. Implement void FlXxxUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
		     in ut/FlXxxUt.c following the pattern in FlCrc32Ut.c.
		  2. Forward-declare the run function below.
		  3. Add a FL_UT_RUN_CATEGORY call for it inside main().
*/

#include "FlUt.h"
#include <stddef.h>
#include <stdio.h>

// Forward declarations of all UT module run functions.
void FlCrc32UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlFraction64UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlMd5UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlSha1UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlSha256UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlSha256HmacUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlPbkdf2Sha256HmacUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlRandomFileNameUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlRandomUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlUuidUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlUnicodeUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlAes256UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlWin32FilePathUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);
void FlWin32CommandLineUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);

// Runs a UT category, prints its result line, and accumulates into the totals.
#define FL_UT_RUN_CATEGORY(category_name, run_function, total_test_count, total_fail_count) \
	do { \
		size_t categoryTestCount = 0; \
		size_t categoryFailCount = 0; \
		(run_function)(&categoryTestCount, &categoryFailCount); \
		size_t categoryPassCount = categoryTestCount - categoryFailCount; \
		if (categoryFailCount == 0) \
			printf("%-24s PASS  (%zu/%zu passed)\n", (category_name), categoryPassCount, categoryTestCount); \
		else \
			printf("%-24s FAIL  (%zu/%zu passed)\n", (category_name), categoryPassCount, categoryTestCount); \
		*(total_test_count) += categoryTestCount; \
		*(total_fail_count) += categoryFailCount; \
	} while (0)

int main(void)
{
	size_t testCount = 0;
	size_t failCount = 0;

	FL_UT_RUN_CATEGORY("CRC32",              FlCrc32UtRun,            &testCount, &failCount);
	FL_UT_RUN_CATEGORY("Fraction64",         FlFraction64UtRun,       &testCount, &failCount);
	FL_UT_RUN_CATEGORY("MD5",                FlMd5UtRun,              &testCount, &failCount);
	FL_UT_RUN_CATEGORY("SHA1",               FlSha1UtRun,             &testCount, &failCount);
	FL_UT_RUN_CATEGORY("SHA256",             FlSha256UtRun,           &testCount, &failCount);
	FL_UT_RUN_CATEGORY("SHA256-HMAC",        FlSha256HmacUtRun,       &testCount, &failCount);
	FL_UT_RUN_CATEGORY("PBKDF2-HMAC-SHA256", FlPbkdf2Sha256HmacUtRun, &testCount, &failCount);
	FL_UT_RUN_CATEGORY("RandomFileName",     FlRandomFileNameUtRun,   &testCount, &failCount);
	FL_UT_RUN_CATEGORY("Random",             FlRandomUtRun,           &testCount, &failCount);
	FL_UT_RUN_CATEGORY("UUID",               FlUuidUtRun,             &testCount, &failCount);
	FL_UT_RUN_CATEGORY("Unicode",            FlUnicodeUtRun,          &testCount, &failCount);
	FL_UT_RUN_CATEGORY("AES-256",            FlAes256UtRun,           &testCount, &failCount);
	FL_UT_RUN_CATEGORY("Win32FilePath",      FlWin32FilePathUtRun,    &testCount, &failCount);
	FL_UT_RUN_CATEGORY("Win32CommandLine",   FlWin32CommandLineUtRun, &testCount, &failCount);

	printf("--------------------------------------------------\n");
	if (failCount == 0)
		printf("Result: PASS  (%zu/%zu passed)\n", testCount, testCount);
	else
		printf("Result: FAIL  (%zu/%zu passed)\n", testCount - failCount, testCount);

	return failCount == 0 ? 0 : 1;
}
