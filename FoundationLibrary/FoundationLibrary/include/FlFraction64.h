/*
	Santtu S. Nyman's public domain 64 bit fraction encode and decode procedures.

	Version history
		version 1.0.0 2026-03-08
			First public version.

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
*/

#ifndef FL_FRACTION_64_H
#define FL_FRACTION_64_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include "FlSAL.h"

#define FL_FRACTION_64_MAX_LENGTH 64
#define FL_FRACTION_64_FULL_PRECISION 65

uint64_t FlDecodeFraction64(_In_ size_t length, _In_reads_(length) const char* text);
/*
	Procedure:
		FlDecodeFraction64

	Description:
		This procedure decodes decimal fractional value encoded in UTF-8 text to closest possible 64 bit fraction.

	Parameters:
		length:
			The number of decimal digits in the decimal fractional in bytes.

		text:
			Pointer to a decimal fractional value encoded in UTF-8 text.
			The length of this decimal fractional is given by length parameter.

	Return:
		Returns the 64 bit fraction value decoded from the text decoded.
*/

size_t FlEncodeFraction64(_In_ uint64_t value, _In_ int precision, _Out_writes_to_(FL_FRACTION_64_MAX_LENGTH,return) char* text);
/*
	Procedure:
		FlEncodeFraction64

	Description:
		This procedure encodes 64 bit fractional value into a decimal fractional in UTF-8 text.
		This encoded value will be exact or can be rounded to a specific precision.

	Parameters:
		value:
			The 64 bit fraction to be encoded into decimal text.

		precision:
			Specifies the number of digits to use for rounding.
			The length of the number will be limited to this value.
			Setting this parameter to FL_FRACTION_64_FULL_PRECISION encodes the full fraction without any rounding.
			This value must be greater than or equal to 0 and less than or equal to FL_FRACTION_64_MAX_LENGTH (64) or FL_FRACTION_64_FULL_PRECISION.

		text:
			Pointer to a buffer that will receive the UTF-8 text.
			The buffer must be at least FL_FRACTION_64_MAX_LENGTH (64) characters long when precision set to FL_FRACTION_64_FULL_PRECISION and
			when precision is not FL_FRACTION_64_FULL_PRECISION the buffer must be at least long enough for precision number of characters.

	Return:
		Returns the length of the UTF-8 text decimal fractional in bytes.
		The return value will be 0 if the rounding of the number results in the value being rounded up to an integer value of 1.
		The value returned by this function can never be higher than FL_FRACTION_64_MAX_LENGTH (64) for any input.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_FRACTION_64_H
