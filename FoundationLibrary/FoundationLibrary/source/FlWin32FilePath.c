/*
	Win32 file path manipulation library version 1.0.0 2023-02-25 by Santtu S. Nyman.

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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define WIN32_LEAN_AND_MEAN
#include "FlWin32FilePath.h"
#include "FlUtf8Utf16Converter.h"

#define FL_IS_HEX_WCHAR(C) (((int)(C) >= (int)L'0' && (int)(C) <= (int)L'9') || ((int)(C) >= (int)L'A' && (int)(C) <= (int)L'F') || ((int)(C) >= (int)L'a' && (int)(C) <= (int)L'f'))
#define FL_IS_HEX_CHAR(C) (((int)(C) >= (int)'0' && (int)(C) <= (int)'9') || ((int)(C) >= (int)'A' && (int)(C) <= (int)'F') || ((int)(C) >= (int)'a' && (int)(C) <= (int)'f'))

static SIZE_T FlWin32AbsolutePathVolumeDirectoryPartLength(SIZE_T pathLength, const WCHAR* path)
{
	if (pathLength > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\')
	{
		if (pathLength > 6 && (path[4] != L'\\' && path[4] != L'/') && path[5] == L':' && (path[6] == L'\\' || path[6] == L'/'))
		{
			return 7;
		}
		else if (pathLength > 48 &&
			(path[4] == L'V' || path[4] == L'v') &&
			(path[5] == L'O' || path[5] == L'o') &&
			(path[6] == L'L' || path[6] == L'l') &&
			(path[7] == L'U' || path[7] == L'u') &&
			(path[8] == L'M' || path[8] == L'm') &&
			(path[9] == L'E' || path[9] == L'e') &&
			path[10] == L'{' &&
			FL_IS_HEX_WCHAR(path[11]) &&
			FL_IS_HEX_WCHAR(path[12]) &&
			FL_IS_HEX_WCHAR(path[13]) &&
			FL_IS_HEX_WCHAR(path[14]) &&
			FL_IS_HEX_WCHAR(path[15]) &&
			FL_IS_HEX_WCHAR(path[16]) &&
			FL_IS_HEX_WCHAR(path[17]) &&
			FL_IS_HEX_WCHAR(path[18]) &&
			path[19] == L'-' &&
			FL_IS_HEX_WCHAR(path[20]) &&
			FL_IS_HEX_WCHAR(path[21]) &&
			FL_IS_HEX_WCHAR(path[22]) &&
			FL_IS_HEX_WCHAR(path[23]) &&
			path[24] == L'-' &&
			FL_IS_HEX_WCHAR(path[25]) &&
			FL_IS_HEX_WCHAR(path[26]) &&
			FL_IS_HEX_WCHAR(path[27]) &&
			FL_IS_HEX_WCHAR(path[28]) &&
			path[29] == L'-' &&
			FL_IS_HEX_WCHAR(path[30]) &&
			FL_IS_HEX_WCHAR(path[31]) &&
			FL_IS_HEX_WCHAR(path[32]) &&
			FL_IS_HEX_WCHAR(path[33]) &&
			path[34] == L'-' &&
			FL_IS_HEX_WCHAR(path[35]) &&
			FL_IS_HEX_WCHAR(path[36]) &&
			FL_IS_HEX_WCHAR(path[37]) &&
			FL_IS_HEX_WCHAR(path[38]) &&
			FL_IS_HEX_WCHAR(path[39]) &&
			FL_IS_HEX_WCHAR(path[40]) &&
			FL_IS_HEX_WCHAR(path[41]) &&
			FL_IS_HEX_WCHAR(path[42]) &&
			FL_IS_HEX_WCHAR(path[43]) &&
			FL_IS_HEX_WCHAR(path[44]) &&
			FL_IS_HEX_WCHAR(path[45]) &&
			FL_IS_HEX_WCHAR(path[46]) &&
			path[47] == L'}' &&
			path[48] == L'\\')
		{
			return 49;
		}
		else if (pathLength > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			size_t serverLength = 0;
			while (8 + serverLength < pathLength && (path[8 + serverLength] != L'\\' && path[8 + serverLength] != L'/'))
			{
				++serverLength;
			}
			if (!serverLength)
			{
				return 0;
			}
			size_t shareLength = 0;
			while (9 + serverLength + shareLength < pathLength && (path[9 + serverLength + shareLength] != L'\\' && path[9 + serverLength + shareLength] != L'/'))
			{
				++shareLength;
			}
			if (!shareLength || (path[9 + serverLength + shareLength] != L'\\' && path[9 + serverLength + shareLength] != L'/'))
			{
				return 0;
			}
			return 10 + serverLength + shareLength;
		}
		else
		{
			return 0;
		}
	}
	else if (pathLength > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\')
	{
		if (pathLength > 6 && (path[4] != L'\\' && path[4] != L'/') && path[5] == L':' && (path[6] == L'\\' || path[6] == L'/'))
		{
			return 7;
		}
		else if (pathLength > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			size_t serverLength = 0;
			while (8 + serverLength < pathLength && (path[8 + serverLength] != L'\\' && path[8 + serverLength] != L'/'))
			{
				++serverLength;
			}
			if (!serverLength)
			{
				return 0;
			}
			size_t shareLength = 0;
			while (9 + serverLength + shareLength < pathLength && (path[9 + serverLength + shareLength] != L'\\' && path[9 + serverLength + shareLength] != L'/'))
			{
				++shareLength;
			}
			if (!shareLength || (path[9 + serverLength + shareLength] != L'\\' && path[9 + serverLength + shareLength] != L'/'))
			{
				return 0;
			}
			return 10 + serverLength + shareLength;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (pathLength > 2 && path[0] != L'\\' && path[0] != L'/' && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/'))
		{
			return 3;
		}
		else if (pathLength > 2 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
		{
			size_t serverLength = 0;
			while (2 + serverLength < pathLength && (path[2 + serverLength] != L'\\' && path[2 + serverLength] != L'/'))
			{
				++serverLength;
			}
			if (!serverLength)
			{
				return 0;
			}
			size_t shareLength = 0;
			while (3 + serverLength + shareLength < pathLength && (path[3 + serverLength + shareLength] != L'\\' && path[3 + serverLength + shareLength] != L'/'))
			{
				++shareLength;
			}
			if (!shareLength || (path[3 + serverLength + shareLength] != L'\\' && path[3 + serverLength + shareLength] != L'/'))
			{
				return 0;
			}
			return 4 + serverLength + shareLength;
		}
		else
		{
			return 0;
		}
	}
}

BOOL FlWin32IsPathFullyQualified(_In_ SIZE_T pathLength, _In_reads_(pathLength) const WCHAR* path)
{
	size_t pathVolumeDirectoryPartLength = FlWin32AbsolutePathVolumeDirectoryPartLength(pathLength, path);
	if (!pathVolumeDirectoryPartLength)
	{
		return FALSE;
	}

	BOOL extendedPrefix = ((pathLength > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\') || (pathLength > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\'));
	if ((pathLength > (MAX_PATH - 1)) && !extendedPrefix)
	{
		return FALSE;
	}

	if (pathLength > pathVolumeDirectoryPartLength && (path[pathLength - 1] == L'\\' || path[pathLength - 1] == L'/'))
	{
		return FALSE;
	}

	for (size_t offset = pathVolumeDirectoryPartLength; offset != pathLength;)
	{
		size_t componentLength = 0;
		while (offset + componentLength != pathLength && (path[offset + componentLength] != L'\\' && path[offset + componentLength] != L'/'))
		{
			componentLength++;
		}
		if (!componentLength || (componentLength == 1 && path[offset] == L'.') || (componentLength == 2 && path[offset] == L'.' && path[offset + 1] == L'.'))
		{
			return FALSE;
		}
		offset += componentLength + (((offset + componentLength) != pathLength) ? 1 : 0);
	}
	return TRUE;
}

SIZE_T FlWin32GetFullyQualifiedPath(_In_ SIZE_T pathLength, _In_reads_(pathLength) const WCHAR* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const WCHAR* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_(pathBufferSize,return) WCHAR* pathBuffer)
{
	size_t pathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLength(pathLength, path);
	// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
	if (pathLength && (pathLength > pathVolumePartLength) && ((path[pathLength - 1] == L'\\') || (path[pathLength - 1] == L'/')))
	{
		pathLength--;
	}
	if (!basePathLength && !pathLength)
	{
		return 0;
	}

	if (pathVolumePartLength)
	{
		// Absolute Path to fully qualified Path

		BOOL extendedPrefix = ((pathLength > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\') || (pathLength > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\'));
		BOOL networkPath = FALSE;
		if (extendedPrefix)
		{
			if (pathLength > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
			{
				networkPath = TRUE;
			}
		}
		else
		{
			if (pathLength > 1 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
			{
				networkPath = TRUE;
			}
		}

		size_t fullyQualifiedLength = pathVolumePartLength;
		for (size_t offset = pathLength, componentCount = 0, componentEraseCount = 0; offset != pathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != pathVolumePartLength && (path[offset - componentLength - 1] != L'\\' && path[offset - componentLength - 1] != L'/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == L'.' && path[componentOffset + 1] == L'.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					fullyQualifiedLength += componentLength + 1;
					componentCount++;
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != pathVolumePartLength)
			{
				offset--;
			}
			else
			{
				if (componentEraseCount)
				{
					return 0;
				}
				if (componentCount)
				{
					fullyQualifiedLength--;
				}
			}
		}

		BOOL addExtendedPrefix = FALSE;
		BOOL removeExtendedPrefix = FALSE;
		BOOL volumeGuidPath = extendedPrefix && !networkPath && pathVolumePartLength == 49;
		if ((!extendedPrefix && (fullyQualifiedLength > (MAX_PATH - 1))) || (extendedPrefix && ((networkPath ? (fullyQualifiedLength - 6) : (fullyQualifiedLength - 4)) > (MAX_PATH - 1))))
		{
			if (!extendedPrefix)
			{
				addExtendedPrefix = TRUE;
				fullyQualifiedLength += networkPath ? 6 : 4;
			}
		}
		else
		{
			if (extendedPrefix && !volumeGuidPath)
			{
				removeExtendedPrefix = TRUE;
				fullyQualifiedLength -= networkPath ? 6 : 4;
			}
		}

		if (fullyQualifiedLength > pathBufferSize)
		{
			return fullyQualifiedLength;
		}

		size_t writeOffset = fullyQualifiedLength;
		for (size_t offset = pathLength, componentEraseCount = 0; offset != pathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != pathVolumePartLength && (path[offset - componentLength - 1] != L'\\' && path[offset - componentLength - 1] != L'/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == L'.' && path[componentOffset + 1] == L'.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					if (writeOffset != fullyQualifiedLength)
					{
						writeOffset--;
						pathBuffer[writeOffset] = '\\';
					}
					writeOffset -= componentLength;
					memcpy(pathBuffer + writeOffset, path + componentOffset, componentLength * sizeof(WCHAR));
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != pathVolumePartLength)
			{
				offset--;
			}
		}

		for (size_t pathVolumePartCopyEnd = (addExtendedPrefix ? (networkPath ? 6 : 4) : ((removeExtendedPrefix && networkPath) ? 2 : 0)), volumePartReadOffset = pathVolumePartLength - 1; writeOffset != pathVolumePartCopyEnd;)
		{
			WCHAR pathVolumePartCharacter = path[volumePartReadOffset];
			volumePartReadOffset--;
			if (pathVolumePartCharacter == L'/')
			{
				pathVolumePartCharacter = L'\\';
			}
			writeOffset--;
			pathBuffer[writeOffset] = pathVolumePartCharacter;
		}
		if (addExtendedPrefix)
		{
			if (networkPath)
			{
				memcpy(pathBuffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
			}
			memcpy(pathBuffer, L"\\\\?\\", 4 * sizeof(WCHAR));
		}
		else if (removeExtendedPrefix && networkPath)
		{
			memcpy(pathBuffer, L"\\\\", 2 * sizeof(WCHAR));
		}

		return fullyQualifiedLength;
	}
	else
	{
		// Relative Path to fully qualified Path

		// Get volume Path part of the base directory
		size_t basePathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLength(basePathLength, basePath);
		// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
		if (basePathLength && (basePathLength > basePathVolumePartLength) && ((basePath[basePathLength - 1] == L'\\') || (basePath[basePathLength - 1] == L'/')))
		{
			basePathLength--;
		}
		if (!basePathLength)
		{
			return 0;
		}

		BOOL basePathExtendedPrefix = ((basePathLength > 4 && basePath[0] == L'\\' && basePath[1] == L'?' && basePath[2] == L'?' && basePath[3] == L'\\') || (basePathLength > 4 && basePath[0] == L'\\' && basePath[1] == L'\\' && basePath[2] == L'?' && basePath[3] == L'\\'));
		BOOL basePathNetworkPath = FALSE;
		if (basePathExtendedPrefix)
		{
			if (basePathLength > 7 && (basePath[4] == L'U' || basePath[4] == L'u') && (basePath[5] == L'N' || basePath[5] == L'n') && (basePath[6] == L'C' || basePath[6] == L'c') && (basePath[7] == L'\\' || basePath[7] == L'/'))
			{
				basePathNetworkPath = TRUE;
			}
		}
		else
		{
			if (basePathLength > 1 && (basePath[0] == L'\\' || basePath[0] == L'/') && (basePath[1] == L'\\' || basePath[1] == L'/'))
			{
				basePathNetworkPath = TRUE;
			}
		}

		size_t relativePathComponentEraseCount = 0;
		size_t relativePathComponentCount = 0;
		size_t relativePathLength = 0;
		for (size_t offset = pathLength; offset;)
		{
			size_t componentLength = 0;
			while (offset - componentLength && (path[offset - componentLength - 1] != L'\\' && path[offset - componentLength - 1] != L'/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == L'.' && path[componentOffset + 1] == L'.')
			{
				relativePathComponentEraseCount++;
			}
			else
			{
				if (!relativePathComponentEraseCount)
				{
					relativePathLength += componentLength + 1;
					relativePathComponentCount++;
				}
				else
				{
					relativePathComponentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset)
			{
				offset--;
			}
			else
			{
				if (relativePathComponentCount)
				{
					relativePathLength--;
				}
			}
		}

		// Get the base Path component count and length after making it fully qualified. Also erase components as required from the relative sub Path
		size_t basePathComponentCount = 0;
		size_t basePathFullyQualifiedLength = basePathVolumePartLength;
		if (basePathLength == basePathVolumePartLength && relativePathComponentEraseCount)
		{
			return 0;
		}
		for (size_t offset = basePathLength, componentEraseCount = relativePathComponentEraseCount; offset != basePathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != basePathVolumePartLength && (basePath[offset - componentLength - 1] != L'\\' && basePath[offset - componentLength - 1] != L'/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && basePath[componentOffset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && basePath[componentOffset] == L'.' && basePath[componentOffset + 1] == L'.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					basePathFullyQualifiedLength += componentLength + 1;
					basePathComponentCount++;
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != basePathVolumePartLength)
			{
				offset--;
			}
			else
			{
				if (componentEraseCount)
				{
					return 0;
				}
				if (basePathComponentCount)
				{
					basePathFullyQualifiedLength--;
				}
			}
		}
		
		size_t fullyQualifiedLength = basePathFullyQualifiedLength + ((relativePathLength && (basePathFullyQualifiedLength != basePathVolumePartLength)) ? 1 : 0) + relativePathLength;
		
		BOOL addExtendedPrefix = FALSE;
		BOOL removeExtendedPrefix = FALSE;
		if ((!basePathExtendedPrefix && (fullyQualifiedLength > (MAX_PATH - 1))) || (basePathExtendedPrefix && ((basePathNetworkPath ? (fullyQualifiedLength - 6) : (fullyQualifiedLength - 4)) > (MAX_PATH - 1))))
		{
			if (!basePathExtendedPrefix)
			{
				addExtendedPrefix = TRUE;
				basePathFullyQualifiedLength += basePathNetworkPath ? 6 : 4;
				fullyQualifiedLength += basePathNetworkPath ? 6 : 4;
			}
		}
		else
		{
			if (basePathExtendedPrefix)
			{
				removeExtendedPrefix = TRUE;
				basePathFullyQualifiedLength -= basePathNetworkPath ? 6 : 4;
				fullyQualifiedLength -= basePathNetworkPath ? 6 : 4;
			}
		}
		
		if (fullyQualifiedLength > pathBufferSize)
		{
			return fullyQualifiedLength;
		}

		size_t writeOffset = fullyQualifiedLength;
		for (size_t offset = pathLength, componentEraseCount = 0; offset;)
		{
			size_t componentLength = 0;
			while (offset - componentLength && (path[offset - componentLength - 1] != L'\\' && path[offset - componentLength - 1] != L'/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == L'.' && path[componentOffset + 1] == L'.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					if (writeOffset != fullyQualifiedLength)
					{
						writeOffset--;
						pathBuffer[writeOffset] = '\\';
					}
					writeOffset -= componentLength;
					memcpy(pathBuffer + writeOffset, path + componentOffset, componentLength * sizeof(WCHAR));
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset)
			{
				offset--;
			}
		}

		if (relativePathLength && basePathComponentCount)
		{
			writeOffset--;
			pathBuffer[writeOffset] = '\\';
		}

		for (size_t offset = basePathLength, componentEraseCount = relativePathComponentEraseCount; offset != basePathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != basePathVolumePartLength && (basePath[offset - componentLength - 1] != L'\\' && basePath[offset - componentLength - 1] != L'/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && basePath[componentOffset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && basePath[componentOffset] == L'.' && basePath[componentOffset + 1] == L'.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					if (writeOffset != basePathFullyQualifiedLength)
					{
						writeOffset--;
						pathBuffer[writeOffset] = '\\';
					}
					writeOffset -= componentLength;
					memcpy(pathBuffer + writeOffset, basePath + componentOffset, componentLength * sizeof(WCHAR));
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != basePathVolumePartLength)
			{
				offset--;
			}
		}

		for (size_t pathVolumePartCopyEnd = (addExtendedPrefix ? (basePathNetworkPath ? 6 : 4) : ((removeExtendedPrefix && basePathNetworkPath) ? 2 : 0)), volumePartReadOffset = basePathVolumePartLength - 1; writeOffset != pathVolumePartCopyEnd;)
		{
			WCHAR pathVolumePartCharacter = basePath[volumePartReadOffset];
			volumePartReadOffset--;
			if (pathVolumePartCharacter == L'/')
			{
				pathVolumePartCharacter = L'\\';
			}
			writeOffset--;
			pathBuffer[writeOffset] = pathVolumePartCharacter;
		}
		if (addExtendedPrefix)
		{
			if (basePathNetworkPath)
			{
				memcpy(pathBuffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
			}
			memcpy(pathBuffer, L"\\\\?\\", 4 * sizeof(WCHAR));
		}
		else if (removeExtendedPrefix && basePathNetworkPath)
		{
			memcpy(pathBuffer, L"\\\\", 2 * sizeof(WCHAR));
		}

		return fullyQualifiedLength;
	}
}

SIZE_T FlWin32GetVolumeDirectoryPath(_In_ SIZE_T pathLength, _In_reads_(pathLength) const WCHAR* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const WCHAR* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_(pathBufferSize,return) WCHAR* pathBuffer)
{
	size_t pathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLength(pathLength, path);
	size_t componentEraseCountFromRelativePath = 0;
	if (!pathVolumePartLength)
	{
		pathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLength(basePathLength, basePath);
		if (!pathVolumePartLength)
		{
			return 0;
		}

		// Remove trailing '\'
		if (pathLength && ((path[pathLength - 1] == L'\\') || (path[pathLength - 1] == L'/')))
		{
			pathLength--;
		}
		// Get nunber of path components to erase from the end of the base path
		for (size_t realComponentCount = 0, offset = 0; offset < pathLength;)
		{
			size_t componentLength = 0;
			while ((offset + componentLength < pathLength) && (path[offset + componentLength] != L'\\' && path[offset + componentLength] != L'/'))
			{
				componentLength++;
			}
			if (componentLength == 1 && path[offset] == L'.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[offset] == L'.' && path[offset + 1] == L'.')
			{
				if (realComponentCount)
				{
					realComponentCount--;
				}
				else
				{
					componentEraseCountFromRelativePath++;
				}
			}
			else
			{
				realComponentCount++;
			}
			offset += componentLength;
			if (offset < pathLength)
			{
				offset++;
			}
		}

		pathLength = basePathLength;
		path = basePath;
	}

	// Remove trailing '\' if path is longer than volume directory Path.
	if ((pathLength > pathVolumePartLength) && ((path[pathLength - 1] == '\\') || (path[pathLength - 1] == '/')))
	{
		pathLength--;
	}

	// Check if the the path traces back out of root.
	size_t realComponentCount = 0;
	for (size_t offset = pathVolumePartLength; offset < pathLength;)
	{
		size_t componentLength = 0;
		while ((offset + componentLength < pathLength) && (path[offset + componentLength] != L'\\' && path[offset + componentLength] != L'/'))
		{
			componentLength++;
		}
		if (componentLength == 1 && path[offset] == L'.')
		{
			// just skip "." components
		}
		else if (componentLength == 2 && path[offset] == L'.' && path[offset + 1] == L'.')
		{
			if (!realComponentCount)
			{
				// Trying to erase past root directory
				return 0;
			}
			realComponentCount--;
		}
		else
		{
			realComponentCount++;
		}
		offset += componentLength;
		if (offset < pathLength)
		{
			offset++;
		}
	}
	if (componentEraseCountFromRelativePath > realComponentCount)
	{
		// Trying to erase past root directory from the relative path
		return 0;
	}

	BOOL extendedPrefix = ((pathLength > 4 && path[0] == L'\\' && path[1] == L'?' && path[2] == L'?' && path[3] == L'\\') || (pathLength > 4 && path[0] == L'\\' && path[1] == L'\\' && path[2] == L'?' && path[3] == L'\\'));
	BOOL networkPath = FALSE;
	if (extendedPrefix)
	{
		if (pathLength > 7 && (path[4] == L'U' || path[4] == L'u') && (path[5] == L'N' || path[5] == L'n') && (path[6] == L'C' || path[6] == L'c') && (path[7] == L'\\' || path[7] == L'/'))
		{
			networkPath = TRUE;
		}
	}
	else
	{
		if (pathLength > 1 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/'))
		{
			networkPath = TRUE;
		}
	}

	size_t fullyQualifiedLength = pathVolumePartLength;
	BOOL addExtendedPrefix = FALSE;
	BOOL removeExtendedPrefix = FALSE;
	BOOL volumeGuidPath = extendedPrefix && !networkPath && pathVolumePartLength == 49;
	if ((!extendedPrefix && (fullyQualifiedLength > (MAX_PATH - 1))) || (extendedPrefix && ((networkPath ? (fullyQualifiedLength - 6) : (fullyQualifiedLength - 4)) > (MAX_PATH - 1))))
	{
		if (!extendedPrefix)
		{
			addExtendedPrefix = TRUE;
			fullyQualifiedLength += networkPath ? 6 : 4;
		}
	}
	else
	{
		if (extendedPrefix && !volumeGuidPath)
		{
			removeExtendedPrefix = TRUE;
			fullyQualifiedLength -= networkPath ? 6 : 4;
		}
	}

	if (fullyQualifiedLength > pathBufferSize)
	{
		return fullyQualifiedLength;
	}

	for (size_t pathVolumePartCopyEnd = (addExtendedPrefix ? (networkPath ? 6 : 4) : ((removeExtendedPrefix && networkPath) ? 2 : 0)), writeOffset = fullyQualifiedLength, readOffset = pathVolumePartLength - 1; writeOffset != pathVolumePartCopyEnd;)
	{
		WCHAR pathVolumePartCharacter = path[readOffset];
		readOffset--;
		if (pathVolumePartCharacter == L'/')
		{
			pathVolumePartCharacter = L'\\';
		}
		writeOffset--;
		pathBuffer[writeOffset] = pathVolumePartCharacter;
	}
	if (addExtendedPrefix)
	{
		if (networkPath)
		{
			memcpy(pathBuffer + 4, L"UNC\\", 4 * sizeof(WCHAR));
		}
		memcpy(pathBuffer, L"\\\\?\\", 4 * sizeof(WCHAR));
	}
	else if (removeExtendedPrefix && networkPath)
	{
		memcpy(pathBuffer, L"\\\\", 2 * sizeof(WCHAR));
	}

	return fullyQualifiedLength;
}

static SIZE_T FlWin32AbsolutePathVolumeDirectoryPartLengthUtf8(SIZE_T pathLength, const char* path)
{
	if (pathLength > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\')
	{
		if (pathLength > 6 && (path[4] != '\\' && path[4] != '/') && path[5] == ':' && (path[6] == '\\' || path[6] == '/'))
		{
			return 7;
		}
		else if (pathLength > 48 &&
			(path[4] == 'V' || path[4] == 'v') &&
			(path[5] == 'O' || path[5] == 'o') &&
			(path[6] == 'L' || path[6] == 'l') &&
			(path[7] == 'U' || path[7] == 'u') &&
			(path[8] == 'M' || path[8] == 'm') &&
			(path[9] == 'E' || path[9] == 'e') &&
			path[10] == '{' &&
			FL_IS_HEX_CHAR(path[11]) &&
			FL_IS_HEX_CHAR(path[12]) &&
			FL_IS_HEX_CHAR(path[13]) &&
			FL_IS_HEX_CHAR(path[14]) &&
			FL_IS_HEX_CHAR(path[15]) &&
			FL_IS_HEX_CHAR(path[16]) &&
			FL_IS_HEX_CHAR(path[17]) &&
			FL_IS_HEX_CHAR(path[18]) &&
			path[19] == '-' &&
			FL_IS_HEX_CHAR(path[20]) &&
			FL_IS_HEX_CHAR(path[21]) &&
			FL_IS_HEX_CHAR(path[22]) &&
			FL_IS_HEX_CHAR(path[23]) &&
			path[24] == '-' &&
			FL_IS_HEX_CHAR(path[25]) &&
			FL_IS_HEX_CHAR(path[26]) &&
			FL_IS_HEX_CHAR(path[27]) &&
			FL_IS_HEX_CHAR(path[28]) &&
			path[29] == '-' &&
			FL_IS_HEX_CHAR(path[30]) &&
			FL_IS_HEX_CHAR(path[31]) &&
			FL_IS_HEX_CHAR(path[32]) &&
			FL_IS_HEX_CHAR(path[33]) &&
			path[34] == '-' &&
			FL_IS_HEX_CHAR(path[35]) &&
			FL_IS_HEX_CHAR(path[36]) &&
			FL_IS_HEX_CHAR(path[37]) &&
			FL_IS_HEX_CHAR(path[38]) &&
			FL_IS_HEX_CHAR(path[39]) &&
			FL_IS_HEX_CHAR(path[40]) &&
			FL_IS_HEX_CHAR(path[41]) &&
			FL_IS_HEX_CHAR(path[42]) &&
			FL_IS_HEX_CHAR(path[43]) &&
			FL_IS_HEX_CHAR(path[44]) &&
			FL_IS_HEX_CHAR(path[45]) &&
			FL_IS_HEX_CHAR(path[46]) &&
			path[47] == '}' &&
			path[48] == '\\')
		{
			return 49;
		}
		else if (pathLength > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			size_t serverLength = 0;
			while (8 + serverLength < pathLength && (path[8 + serverLength] != '\\' && path[8 + serverLength] != '/'))
			{
				++serverLength;
			}
			if (!serverLength)
			{
				return 0;
			}
			size_t shareLength = 0;
			while (9 + serverLength + shareLength < pathLength && (path[9 + serverLength + shareLength] != '\\' && path[9 + serverLength + shareLength] != '/'))
			{
				++shareLength;
			}
			if (!shareLength || (path[9 + serverLength + shareLength] != '\\' && path[9 + serverLength + shareLength] != '/'))
			{
				return 0;
			}
			return 10 + serverLength + shareLength;
		}
		else
		{
			return 0;
		}
	}
	else if (pathLength > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\')
	{
		if (pathLength > 6 && (path[4] != '\\' && path[4] != '/') && path[5] == ':' && (path[6] == '\\' || path[6] == '/'))
		{
			return 7;
		}
		else if (pathLength > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			size_t serverLength = 0;
			while (8 + serverLength < pathLength && (path[8 + serverLength] != '\\' && path[8 + serverLength] != '/'))
			{
				++serverLength;
			}
			if (!serverLength)
			{
				return 0;
			}
			size_t shareLength = 0;
			while (9 + serverLength + shareLength < pathLength && (path[9 + serverLength + shareLength] != '\\' && path[9 + serverLength + shareLength] != '/'))
			{
				++shareLength;
			}
			if (!shareLength || (path[9 + serverLength + shareLength] != '\\' && path[9 + serverLength + shareLength] != '/'))
			{
				return 0;
			}
			return 10 + serverLength + shareLength;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (pathLength > 2 && path[0] != '\\' && path[0] != '/' && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
		{
			return 3;
		}
		else if (pathLength > 2 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
		{
			size_t serverLength = 0;
			while (2 + serverLength < pathLength && (path[2 + serverLength] != '\\' && path[2 + serverLength] != '/'))
			{
				++serverLength;
			}
			if (!serverLength)
			{
				return 0;
			}
			size_t shareLength = 0;
			while (3 + serverLength + shareLength < pathLength && (path[3 + serverLength + shareLength] != '\\' && path[3 + serverLength + shareLength] != '/'))
			{
				++shareLength;
			}
			if (!shareLength || (path[3 + serverLength + shareLength] != '\\' && path[3 + serverLength + shareLength] != '/'))
			{
				return 0;
			}
			return 4 + serverLength + shareLength;
		}
		else
		{
			return 0;
		}
	}
}

BOOL FlWin32IsPathFullyQualifiedUtf8(_In_ SIZE_T pathLength, _In_reads_(pathLength) const char* path)
{
	size_t pathVolumeDirectoryPartLength = FlWin32AbsolutePathVolumeDirectoryPartLengthUtf8(pathLength, path);
	if (!pathVolumeDirectoryPartLength)
	{
		return FALSE;
	}

	size_t utf16FullyQualifiedLength = FlConvertUtf8ToUtf16Le(pathLength, path, 0, 0);
	BOOL extendedPrefix = ((pathLength > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\') || (pathLength > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\'));
	if ((utf16FullyQualifiedLength > (MAX_PATH - 1)) && !extendedPrefix)
	{
		return FALSE;
	}

	if (pathLength > pathVolumeDirectoryPartLength && (path[pathLength - 1] == '\\' || path[pathLength - 1] == '/'))
	{
		return FALSE;
	}

	for (size_t offset = pathVolumeDirectoryPartLength; offset != pathLength;)
	{
		size_t componentLength = 0;
		while (offset + componentLength != pathLength && (path[offset + componentLength] != '\\' && path[offset + componentLength] != '/'))
		{
			componentLength++;
		}
		if (!componentLength || (componentLength == 1 && path[offset] == '.') || (componentLength == 2 && path[offset] == '.' && path[offset + 1] == '.'))
		{
			return FALSE;
		}
		offset += componentLength + (((offset + componentLength) != pathLength) ? 1 : 0);
	}
	return TRUE;
}

SIZE_T FlWin32GetFullyQualifiedPathUtf8(_In_ SIZE_T pathLength, _In_reads_(pathLength) const char* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const char* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_(pathBufferSize,return) char* pathBuffer)
{
	size_t pathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLengthUtf8(pathLength, path);
	// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
	if (pathLength && (pathLength > pathVolumePartLength) && ((path[pathLength - 1] == '\\') || (path[pathLength - 1] == '/')))
	{
		pathLength--;
	}
	if (!basePathLength && !pathLength)
	{
		return 0;
	}

	if (pathVolumePartLength)
	{
		// Absolute Path to fully qualified Path

		BOOL extendedPrefix = ((pathLength > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\') || (pathLength > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\'));
		BOOL networkPath = FALSE;
		if (extendedPrefix)
		{
			if (pathLength > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
			{
				networkPath = TRUE;
			}
		}
		else
		{
			if (pathLength > 1 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
			{
				networkPath = TRUE;
			}
		}

		size_t fullyQualifiedLength = pathVolumePartLength;
		size_t fullyQualifiedUtf16Length = FlConvertUtf8ToUtf16Le(pathVolumePartLength, path, 0, 0);
		for (size_t offset = pathLength, componentCount = 0, componentEraseCount = 0; offset != pathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != pathVolumePartLength && (path[offset - componentLength - 1] != '\\' && path[offset - componentLength - 1] != '/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == '.' && path[componentOffset + 1] == '.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					fullyQualifiedLength += componentLength + 1;
					fullyQualifiedUtf16Length += FlConvertUtf8ToUtf16Le(componentLength, path + componentOffset, 0, 0) + 1;
					componentCount++;
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != pathVolumePartLength)
			{
				offset--;
			}
			else
			{
				if (componentEraseCount)
				{
					return 0;
				}
				if (componentCount)
				{
					fullyQualifiedLength--;
					fullyQualifiedUtf16Length--;
				}
			}
		}

		BOOL addExtendedPrefix = FALSE;
		BOOL removeExtendedPrefix = FALSE;
		BOOL volumeGuidPath = extendedPrefix && !networkPath && pathVolumePartLength == 49;
		if ((!extendedPrefix && (fullyQualifiedUtf16Length > (MAX_PATH - 1))) || (extendedPrefix && ((networkPath ? (fullyQualifiedUtf16Length - 6) : (fullyQualifiedUtf16Length - 4)) > (MAX_PATH - 1))))
		{
			if (!extendedPrefix)
			{
				addExtendedPrefix = TRUE;
				size_t lengthAdjust = networkPath ? 6 : 4;
				fullyQualifiedLength += lengthAdjust;
				fullyQualifiedUtf16Length += lengthAdjust;
			}
		}
		else
		{
			if (extendedPrefix && !volumeGuidPath)
			{
				removeExtendedPrefix = TRUE;
				size_t lengthAdjust = networkPath ? 6 : 4;
				fullyQualifiedLength -= lengthAdjust;
				fullyQualifiedUtf16Length -= lengthAdjust;
			}
		}

		if (fullyQualifiedLength > pathBufferSize)
		{
			return fullyQualifiedLength;
		}

		size_t writeOffset = fullyQualifiedLength;
		for (size_t offset = pathLength, componentEraseCount = 0; offset != pathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != pathVolumePartLength && (path[offset - componentLength - 1] != '\\' && path[offset - componentLength - 1] != '/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == '.' && path[componentOffset + 1] == '.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					if (writeOffset != fullyQualifiedLength)
					{
						writeOffset--;
						pathBuffer[writeOffset] = '\\';
					}
					writeOffset -= componentLength;
					memcpy(pathBuffer + writeOffset, path + componentOffset, componentLength * sizeof(char));
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != pathVolumePartLength)
			{
				offset--;
			}
		}

		for (size_t pathVolumePartCopyEnd = (addExtendedPrefix ? (networkPath ? 6 : 4) : ((removeExtendedPrefix && networkPath) ? 2 : 0)), volumePartReadOffset = pathVolumePartLength - 1; writeOffset != pathVolumePartCopyEnd;)
		{
			char pathVolumePartCharacter = path[volumePartReadOffset];
			volumePartReadOffset--;
			if (pathVolumePartCharacter == '/')
			{
				pathVolumePartCharacter = '\\';
			}
			writeOffset--;
			pathBuffer[writeOffset] = pathVolumePartCharacter;
		}
		if (addExtendedPrefix)
		{
			if (networkPath)
			{
				memcpy(pathBuffer + 4, "UNC\\", 4 * sizeof(char));
			}
			memcpy(pathBuffer, "\\\\?\\", 4 * sizeof(char));
		}
		else if (removeExtendedPrefix && networkPath)
		{
			memcpy(pathBuffer, "\\\\", 2 * sizeof(char));
		}

		return fullyQualifiedLength;
	}
	else
	{
		// Relative Path to fully qualified Path

		// Get volume Path part of the base directory
		size_t basePathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLengthUtf8(basePathLength, basePath);
		// Remove trailing '\' if Path is longer than volume directory Path. Volume directory length is zero when Path is relative
		if (basePathLength && (basePathLength > basePathVolumePartLength) && ((basePath[basePathLength - 1] == '\\') || (basePath[basePathLength - 1] == '/')))
		{
			basePathLength--;
		}
		if (!basePathLength)
		{
			return 0;
		}

		BOOL basePathExtendedPrefix = ((basePathLength > 4 && basePath[0] == '\\' && basePath[1] == '?' && basePath[2] == '?' && basePath[3] == '\\') || (basePathLength > 4 && basePath[0] == '\\' && basePath[1] == '\\' && basePath[2] == '?' && basePath[3] == '\\'));
		BOOL basePathNetworkPath = FALSE;
		if (basePathExtendedPrefix)
		{
			if (basePathLength > 7 && (basePath[4] == 'U' || basePath[4] == 'u') && (basePath[5] == 'N' || basePath[5] == 'n') && (basePath[6] == 'C' || basePath[6] == 'c') && (basePath[7] == '\\' || basePath[7] == '/'))
			{
				basePathNetworkPath = TRUE;
			}
		}
		else
		{
			if (basePathLength > 1 && (basePath[0] == '\\' || basePath[0] == '/') && (basePath[1] == '\\' || basePath[1] == '/'))
			{
				basePathNetworkPath = TRUE;
			}
		}

		size_t relativePathComponentEraseCount = 0;
		size_t relativePathComponentCount = 0;
		size_t relativePathLength = 0;
		size_t relativePathUtf16Length = 0;
		for (size_t offset = pathLength; offset;)
		{
			size_t componentLength = 0;
			while (offset - componentLength && (path[offset - componentLength - 1] != '\\' && path[offset - componentLength - 1] != '/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == '.' && path[componentOffset + 1] == '.')
			{
				relativePathComponentEraseCount++;
			}
			else
			{
				if (!relativePathComponentEraseCount)
				{
					relativePathLength += componentLength + 1;
					relativePathUtf16Length += FlConvertUtf8ToUtf16Le(componentLength, path + componentOffset, 0, 0) + 1;
					relativePathComponentCount++;
				}
				else
				{
					relativePathComponentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset)
			{
				offset--;
			}
			else
			{
				if (relativePathComponentCount)
				{
					relativePathLength--;
					relativePathUtf16Length--;
				}
			}
		}

		// Get the base Path component count and length after making it fully qualified. Also erase components as required from the relative sub Path
		size_t basePathComponentCount = 0;
		size_t basePathFullyQualifiedLength = basePathVolumePartLength;
		size_t basePathFullyQualifiedUtf16Length = FlConvertUtf8ToUtf16Le(basePathVolumePartLength, basePath, 0, 0);
		if (basePathLength == basePathVolumePartLength && relativePathComponentEraseCount)
		{
			return 0;
		}
		for (size_t offset = basePathLength, componentEraseCount = relativePathComponentEraseCount; offset != basePathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != basePathVolumePartLength && (basePath[offset - componentLength - 1] != '\\' && basePath[offset - componentLength - 1] != '/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && basePath[componentOffset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && basePath[componentOffset] == '.' && basePath[componentOffset + 1] == '.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					basePathFullyQualifiedLength += componentLength + 1;
					basePathFullyQualifiedUtf16Length += FlConvertUtf8ToUtf16Le(componentLength, basePath + componentOffset, 0, 0) + 1;
					basePathComponentCount++;
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != basePathVolumePartLength)
			{
				offset--;
			}
			else
			{
				if (componentEraseCount)
				{
					return 0;
				}
				if (basePathComponentCount)
				{
					basePathFullyQualifiedLength--;
					basePathFullyQualifiedUtf16Length--;
				}
			}
		}

		BOOL addBasePathSeparator = relativePathLength && (basePathFullyQualifiedLength != basePathVolumePartLength);
		size_t fullyQualifiedUtf16Length = basePathFullyQualifiedUtf16Length + (addBasePathSeparator ? 1 : 0) + relativePathUtf16Length;
		size_t fullyQualifiedLength = basePathFullyQualifiedLength + (addBasePathSeparator ? 1 : 0) + relativePathLength;

		BOOL addExtendedPrefix = FALSE;
		BOOL removeExtendedPrefix = FALSE;
		if ((!basePathExtendedPrefix && (fullyQualifiedUtf16Length > (MAX_PATH - 1))) || (basePathExtendedPrefix && ((basePathNetworkPath ? (fullyQualifiedUtf16Length - 6) : (fullyQualifiedUtf16Length - 4)) > (MAX_PATH - 1))))
		{
			if (!basePathExtendedPrefix)
			{
				addExtendedPrefix = TRUE;
				size_t lengthAdjust = basePathNetworkPath ? 6 : 4;
				basePathFullyQualifiedLength += lengthAdjust;
				fullyQualifiedLength += lengthAdjust;
				fullyQualifiedUtf16Length += lengthAdjust;
			}
		}
		else
		{
			if (basePathExtendedPrefix)
			{
				removeExtendedPrefix = TRUE;
				size_t lengthAdjust = basePathNetworkPath ? 6 : 4;
				basePathFullyQualifiedLength -= lengthAdjust;
				fullyQualifiedLength -= lengthAdjust;
				fullyQualifiedUtf16Length -= lengthAdjust;
			}
		}

		if (fullyQualifiedLength > pathBufferSize)
		{
			return fullyQualifiedLength;
		}

		size_t writeOffset = fullyQualifiedLength;
		for (size_t offset = pathLength, componentEraseCount = 0; offset;)
		{
			size_t componentLength = 0;
			while (offset - componentLength && (path[offset - componentLength - 1] != '\\' && path[offset - componentLength - 1] != '/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && path[componentOffset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[componentOffset] == '.' && path[componentOffset + 1] == '.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					if (writeOffset != fullyQualifiedLength)
					{
						writeOffset--;
						pathBuffer[writeOffset] = '\\';
					}
					writeOffset -= componentLength;
					memcpy(pathBuffer + writeOffset, path + componentOffset, componentLength * sizeof(char));
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset)
			{
				offset--;
			}
		}

		if (relativePathLength && basePathComponentCount)
		{
			writeOffset--;
			pathBuffer[writeOffset] = '\\';
		}

		for (size_t offset = basePathLength, componentEraseCount = relativePathComponentEraseCount; offset != basePathVolumePartLength;)
		{
			size_t componentLength = 0;
			while (offset - componentLength != basePathVolumePartLength && (basePath[offset - componentLength - 1] != '\\' && basePath[offset - componentLength - 1] != '/'))
			{
				componentLength++;
			}
			size_t componentOffset = offset - componentLength;
			if (componentLength == 1 && basePath[componentOffset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && basePath[componentOffset] == '.' && basePath[componentOffset + 1] == '.')
			{
				componentEraseCount++;
			}
			else
			{
				if (!componentEraseCount)
				{
					if (writeOffset != basePathFullyQualifiedLength)
					{
						writeOffset--;
						pathBuffer[writeOffset] = '\\';
					}
					writeOffset -= componentLength;
					memcpy(pathBuffer + writeOffset, basePath + componentOffset, componentLength * sizeof(char));
				}
				else
				{
					componentEraseCount--;
				}
			}
			offset -= componentLength;
			if (offset != basePathVolumePartLength)
			{
				offset--;
			}
		}

		for (size_t pathVolumePartCopyEnd = (addExtendedPrefix ? (basePathNetworkPath ? 6 : 4) : ((removeExtendedPrefix && basePathNetworkPath) ? 2 : 0)), volumePartReadOffset = basePathVolumePartLength - 1; writeOffset != pathVolumePartCopyEnd;)
		{
			char pathVolumePartCharacter = basePath[volumePartReadOffset];
			volumePartReadOffset--;
			if (pathVolumePartCharacter == '/')
			{
				pathVolumePartCharacter = '\\';
			}
			writeOffset--;
			pathBuffer[writeOffset] = pathVolumePartCharacter;
		}
		if (addExtendedPrefix)
		{
			if (basePathNetworkPath)
			{
				memcpy(pathBuffer + 4, "UNC\\", 4 * sizeof(char));
			}
			memcpy(pathBuffer, "\\\\?\\", 4 * sizeof(char));
		}
		else if (removeExtendedPrefix && basePathNetworkPath)
		{
			memcpy(pathBuffer, "\\\\", 2 * sizeof(char));
		}

		return fullyQualifiedLength;
	}
}

SIZE_T FlWin32GetVolumeDirectoryPathUtf8(_In_ SIZE_T pathLength, _In_reads_(pathLength) const char* path, _In_ SIZE_T basePathLength, _In_reads_(basePathLength) const char* basePath, _In_ SIZE_T pathBufferSize, _Out_writes_to_(pathBufferSize,return) char* pathBuffer)
{
	size_t pathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLengthUtf8(pathLength, path);
	size_t componentEraseCountFromRelativePath = 0;
	if (!pathVolumePartLength)
	{
		pathVolumePartLength = FlWin32AbsolutePathVolumeDirectoryPartLengthUtf8(basePathLength, basePath);
		if (!pathVolumePartLength)
		{
			return 0;
		}

		// Remove trailing '\'
		if (pathLength && ((path[pathLength - 1] == '\\') || (path[pathLength - 1] == '/')))
		{
			pathLength--;
		}
		// Get nunber of path components to erase from the end of the base path
		for (size_t realComponentCount = 0, offset = 0; offset < pathLength;)
		{
			size_t componentLength = 0;
			while ((offset + componentLength < pathLength) && (path[offset + componentLength] != '\\' && path[offset + componentLength] != '/'))
			{
				componentLength++;
			}
			if (componentLength == 1 && path[offset] == '.')
			{
				// just skip "." components
			}
			else if (componentLength == 2 && path[offset] == '.' && path[offset + 1] == '.')
			{
				if (realComponentCount)
				{
					realComponentCount--;
				}
				else
				{
					componentEraseCountFromRelativePath++;
				}
			}
			else
			{
				realComponentCount++;
			}
			offset += componentLength;
			if (offset < pathLength)
			{
				offset++;
			}
		}

		pathLength = basePathLength;
		path = basePath;
	}

	// Remove trailing '\' if path is longer than volume directory Path.
	if ((pathLength > pathVolumePartLength) && ((path[pathLength - 1] == '\\') || (path[pathLength - 1] == '/')))
	{
		pathLength--;
	}

	// Check if the the path traces back out of root.
	size_t realComponentCount = 0;
	for (size_t offset = pathVolumePartLength; offset < pathLength;)
	{
		size_t componentLength = 0;
		while ((offset + componentLength < pathLength) && (path[offset + componentLength] != '\\' && path[offset + componentLength] != '/'))
		{
			componentLength++;
		}
		if (componentLength == 1 && path[offset] == '.')
		{
			// just skip "." components
		}
		else if (componentLength == 2 && path[offset] == '.' && path[offset + 1] == '.')
		{
			if (!realComponentCount)
			{
				// Trying to erase past root directory
				return 0;
			}
			realComponentCount--;
		}
		else
		{
			realComponentCount++;
		}
		offset += componentLength;
		if (offset < pathLength)
		{
			offset++;
		}
	}
	if (componentEraseCountFromRelativePath > realComponentCount)
	{
		// Trying to erase past root directory from the relative path
		return 0;
	}

	BOOL extendedPrefix = ((pathLength > 4 && path[0] == '\\' && path[1] == '?' && path[2] == '?' && path[3] == '\\') || (pathLength > 4 && path[0] == '\\' && path[1] == '\\' && path[2] == '?' && path[3] == '\\'));
	BOOL networkPath = FALSE;
	if (extendedPrefix)
	{
		if (pathLength > 7 && (path[4] == 'U' || path[4] == 'u') && (path[5] == 'N' || path[5] == 'n') && (path[6] == 'C' || path[6] == 'c') && (path[7] == '\\' || path[7] == '/'))
		{
			networkPath = TRUE;
		}
	}
	else
	{
		if (pathLength > 1 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
		{
			networkPath = TRUE;
		}
	}

	size_t fullyQualifiedLength = pathVolumePartLength;
	size_t utf16FullyQualifiedLength = FlConvertUtf8ToUtf16Le(fullyQualifiedLength, path, 0, 0);
	BOOL addExtendedPrefix = FALSE;
	BOOL removeExtendedPrefix = FALSE;
	BOOL volumeGuidPath = extendedPrefix && !networkPath && pathVolumePartLength == 49;
	if ((!extendedPrefix && (utf16FullyQualifiedLength > (MAX_PATH - 1))) || (extendedPrefix && ((networkPath ? (utf16FullyQualifiedLength - 6) : (utf16FullyQualifiedLength - 4)) > (MAX_PATH - 1))))
	{
		if (!extendedPrefix)
		{
			addExtendedPrefix = TRUE;
			size_t lengthAdjust = networkPath ? 6 : 4;
			fullyQualifiedLength += lengthAdjust;
			utf16FullyQualifiedLength += lengthAdjust;
		}
	}
	else
	{
		if (extendedPrefix && !volumeGuidPath)
		{
			removeExtendedPrefix = TRUE;
			size_t lengthAdjust = networkPath ? 6 : 4;
			fullyQualifiedLength -= lengthAdjust;
			utf16FullyQualifiedLength -= lengthAdjust;
		}
	}

	if (fullyQualifiedLength > pathBufferSize)
	{
		return fullyQualifiedLength;
	}

	for (size_t pathVolumePartCopyEnd = (addExtendedPrefix ? (networkPath ? 6 : 4) : ((removeExtendedPrefix && networkPath) ? 2 : 0)), writeOffset = fullyQualifiedLength, readOffset = pathVolumePartLength - 1; writeOffset != pathVolumePartCopyEnd;)
	{
		char pathVolumePartCharacter = path[readOffset];
		readOffset--;
		if (pathVolumePartCharacter == '/')
		{
			pathVolumePartCharacter = '\\';
		}
		writeOffset--;
		pathBuffer[writeOffset] = pathVolumePartCharacter;
	}
	if (addExtendedPrefix)
	{
		if (networkPath)
		{
			memcpy(pathBuffer + 4, "UNC\\", 4 * sizeof(char));
		}
		memcpy(pathBuffer, "\\\\?\\", 4 * sizeof(char));
	}
	else if (removeExtendedPrefix && networkPath)
	{
		memcpy(pathBuffer, "\\\\", 2 * sizeof(char));
	}

	return fullyQualifiedLength;
}

#ifdef __cplusplus
}
#endif // __cplusplus
