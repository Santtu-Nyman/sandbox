/*
	Foundation Library unit test infrastructure by Santtu S. Nyman.

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

		Shared infrastructure for all Foundation Library unit test modules.

		Each UT module exposes a single run function with the signature:

			void FlXxxUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount);

		The function increments *testCount for every check performed and
		*failCount for every check that fails.  The FL_UT_CHECK macro depends
		on the parameter names being exactly testCount and failCount so all
		UT test functions must use those names for the counter parameters.
*/

#ifndef FL_UT_H
#define FL_UT_H

#include <stddef.h>
#include <stdio.h>
#include "FlSAL.h"

// Evaluates condition, increments *testCount, and on failure increments
// *failCount and prints a diagnostic line.  testCount and failCount must be
// the names of the size_t pointer parameters in scope where this macro is used.
#define FL_UT_CHECK(condition, test_name) \
	do { \
		++(*testCount); \
		if (!(condition)) \
		{ \
			++(*failCount); \
			printf("FAIL [%s]: %s\n", (test_name), #condition); \
		} \
	} while (0)

#endif // FL_UT_H
