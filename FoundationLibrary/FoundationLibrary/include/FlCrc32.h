/*
	Public domain CRC-32/ISO-HDLC implementation by Santtu S. Nyman.

	Version history
		version 1.0.0 2024-04-01
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

#ifndef FL_CRC32_H
#define FL_CRC32_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include "FlSAL.h"

uint32_t FlCrc32Create(void);
/*
	Function:
		FlCrc32Create

	Description:
		This function creates and returns the initial CRC-32/ISO-HDLC calculation context.
		The returned context must be passed to FlCrc32Data to process input data,
		or directly to FlCrc32Finish to obtain the CRC-32 checksum of empty input.

	Return value:
		Returns the initial CRC-32/ISO-HDLC calculation context.
*/

uint32_t FlCrc32Data(_In_ uint32_t crc32Context, _In_ size_t dataSize, _In_reads_bytes_(dataSize) const void* data);
/*
	Function:
		FlCrc32Data

	Description:
		This function processes a chunk of input data and returns the updated CRC-32/ISO-HDLC calculation context.
		The caller may process an arbitrary number of input chunks by calling this function repeatedly,
		passing the returned context value as the context for the next call.
		The chunks must be provided in order.
		Calling this function with dataSize equal to zero leaves the context unchanged.

	Parameters:
		crc32Context:
			The current CRC-32/ISO-HDLC calculation context returned by FlCrc32Create or a previous call to FlCrc32Data.

		dataSize:
			Size of the data chunk to process in bytes.

		data:
			Pointer to the data chunk to process.

	Return value:
		Returns the updated CRC-32/ISO-HDLC calculation context.
*/

uint32_t FlCrc32Finish(_In_ uint32_t crc32Context);
/*
	Function:
		FlCrc32Finish

	Description:
		This function finalizes the CRC-32/ISO-HDLC calculation and returns the resulting checksum.
		Calling this function immediately after FlCrc32Create returns the CRC-32 checksum of empty input.

	Parameters:
		crc32Context:
			The current CRC-32/ISO-HDLC calculation context returned by FlCrc32Create or FlCrc32Data.

	Return value:
		Returns the finalized 32-bit CRC-32/ISO-HDLC checksum of all input data processed.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_CRC32_H
