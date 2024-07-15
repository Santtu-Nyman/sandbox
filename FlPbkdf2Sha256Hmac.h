/*
	Santtu S. Nyman's public domain PBKDF2-HMAC-SHA256 implementation.

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

#ifndef FL_PBKDF2_HMAC_SHA256_H
#define FL_PBKDF2_HMAC_SHA256_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>

void FlPbkdf2Sha256Hmac(uint64_t IterationCount, size_t PasswordSize, const void* Password, size_t SaltSize, const void* Salt, size_t DerivedKeySize, void* DerivedKey);
/*
	Procedure:
		FlPbkdf2Sha256Hmac

	Description:
		This procedure calculates PBKDF2-HMAC-SHA256

	Parameters:
			
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_PBKDF2_HMAC_SHA256_H
