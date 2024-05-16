/*
	Santtu S. Nyman's random data generation library

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

#ifndef FL_RANDOM_H
#define FL_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>

void FlGenerateRandomData(size_t Size, void* Buffer);
/*
	Procedure:
		FlGenerateRandomData

	Description:
		This procedure generates randon bytes into given buffer.
		This procedure is meant to be used as source for random seeds and not as general purpose RNG,
		because this is a relatively slow procedure. It produces cryptographically secure high quality random bits
		and using it as a general purpose RNG is overkill.

	Parameters:
		Size:
			The number of random bytes to generate.

		Buffer:
			Address of the buffer where the new genereted random bytes are written into.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_RANDOM_H
