#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

int main(int argc, char** argv)
{
	char* InputFileName = 0;
	char* OutputFileName = 0;
	FILE* Input = 0;
	FILE* Output = 0;
	bool DefaultOutputFileName = false;
	if (argc == 2)
	{
		Input = fopen(argv[1], "rb");
		if (Input)
			InputFileName = argv[1];
	}
	bool COutputMode = false;
	if (!Input)
		for (int Index = 1; (!Input || !Output || !COutputMode) && Index != argc; ++Index)
			if (Index + 1 != argc && !Input && !strcmp("-i", argv[Index]))
			{
				Input = fopen(argv[Index + 1], "rb");
				if (!Input)
				{
					printf("Error can't open input file \"%s\"\n", argv[Index + 1]);
					if (Output)
						fclose(Output);
					return -1;
				}
				InputFileName = argv[Index + 1];
				++Index;
			}
			else if (Index + 1 != argc && !Output && !strcmp("-o", argv[Index]))
			{
				Output = fopen(argv[Index + 1], "wb");
				if (!Output)
				{
					printf("Error can't open output file \"%s\"\n", argv[Index + 1]);
					if (Input)
						fclose(Input);
					return -1;
				}
				OutputFileName = argv[Index + 1];
				++Index;
			}
			else if (!strcmp("-c", argv[Index]))
				COutputMode = true;
	if (!Input)
	{
		printf("Error no input file\n");
		if (Output)
			fclose(Output);
		return -1;
	}
	if (!Output)
	{
		size_t InputFileNameLength = strlen(InputFileName);
		OutputFileName = (char*)malloc(InputFileNameLength + 25);
		if (!OutputFileName)
		{
			printf("Error memory allocation failed\n");
			fclose(Input);
		}
		memcpy(OutputFileName, InputFileName, InputFileNameLength);
		time_t RawTime;
		struct tm* Time = time(&RawTime) != -1 ? localtime(&RawTime) : 0;
		if (Time && strftime(OutputFileName + InputFileNameLength, 21, ".%F-%T", Time) == 20)
		{
			*(OutputFileName + InputFileNameLength + 14) = '-';
			*(OutputFileName + InputFileNameLength + 17) = '-';
		}
		else
			memcpy(OutputFileName + InputFileNameLength, ".XXXX-XX-XX-XX-XX-XX", 20);
		memcpy(OutputFileName + InputFileNameLength + 20, ".txt", 5);
		Output = fopen(OutputFileName, "wb");
		if (!Output)
		{
			printf("Error can't open output file \"%s\"\n", OutputFileName);
			free(OutputFileName);
			fclose(Input);
			return -1;
		}
		DefaultOutputFileName = true;
	}
	const size_t BytesPerLine = 64;
	const size_t DataBlockSize = 0x10000;
	const uint8_t AsciiHexDigits[16] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };
	if (COutputMode)
	{
		uint8_t* InputBuffer = (uint8_t*)malloc(DataBlockSize + ((6 * DataBlockSize) + (2 * ((DataBlockSize / BytesPerLine) + 1))));
		if (!InputBuffer)
		{
			printf("Error memory allocation failed\n");
			fclose(Input);
			fclose(Output);
			if (DefaultOutputFileName)
			{
				remove(OutputFileName);
				free(OutputFileName);
			}
			return -1;
		}
		uint8_t* OutputBuffer = InputBuffer + DataBlockSize;
		for (bool Continuing = false, Reading = true; Reading;)
		{
			size_t DataRead = 0;
			while (Reading && DataRead != DataBlockSize)
			{
				size_t ReadResult = fread((void*)(InputBuffer + DataRead), 1, DataBlockSize - DataRead, Input);
				if (ReadResult)
					DataRead += ReadResult;
				else if (feof(Input))
					Reading = false;
				else if (ferror(Input))
				{
					printf("Error reading input file \"%s\" failed\n", InputFileName);
					free(InputBuffer);
					fclose(Input);
					fclose(Output);
					if (DefaultOutputFileName)
					{
						remove(OutputFileName);
						free(OutputFileName);
					}
					return -1;
				}
			}
			uint8_t* Read = InputBuffer;
			uint8_t* Write = OutputBuffer;
			for (size_t Lines = DataRead / BytesPerLine, LineIndex = 0; LineIndex != Lines; ++LineIndex)
			{
				if (Continuing)
				{
					*Write++ = 0x2C;
					*Write++ = 0x0A;
				}
				else
					Continuing = true;
				for (uint8_t* ReadEnd = Read + BytesPerLine; Read != ReadEnd; ++Read)
				{
					*Write++ = 0x30;
					*Write++ = 0x78;
					*Write++ = AsciiHexDigits[*Read >> 4];
					*Write++ = AsciiHexDigits[*Read & 0xF];
					*Write++ = 0x2C;
					*Write++ = 0x20;
				}
				Write -= 2;
			}
			if (Read != InputBuffer + DataRead)
			{
				if (Continuing)
				{
					*Write++ = 0x2C;
					*Write++ = 0x0A;
				}
				else
					Continuing = true;
				for (uint8_t* ReadEnd = InputBuffer + DataRead; Read != ReadEnd; ++Read)
				{
					*Write++ = 0x30;
					*Write++ = 0x78;
					*Write++ = AsciiHexDigits[*Read >> 4];
					*Write++ = AsciiHexDigits[*Read & 0xF];
					*Write++ = 0x2C;
					*Write++ = 0x20;
				}
				Write -= 2;
			}
			for (size_t WriteSize = (size_t)((uintptr_t)Write - (uintptr_t)OutputBuffer), DataWritten = 0; DataWritten != WriteSize;)
			{
				size_t WriteResult = fwrite((void*)(OutputBuffer + DataWritten), 1, WriteSize - DataWritten, Output);
				if (!WriteResult && ferror(Output))
				{
					printf("Error writing output file \"%s\" failed\n", OutputFileName);
					free(InputBuffer);
					fclose(Input);
					fclose(Output);
					if (DefaultOutputFileName)
					{
						remove(OutputFileName);
						free(OutputFileName);
					}
					return -1;
				}
				DataWritten += WriteResult;
			}
		}
		free(InputBuffer);
	}
	else
	{
		uint8_t* InputBuffer = (uint8_t*)malloc(DataBlockSize + ((2 * DataBlockSize) + ((DataBlockSize / BytesPerLine) + 1)));
		if (!InputBuffer)
		{
			printf("Error memory allocation failed\n");
			fclose(Input);
			fclose(Output);
			if (DefaultOutputFileName)
			{
				remove(OutputFileName);
				free(OutputFileName);
			}
			return -1;
		}
		uint8_t* OutputBuffer = InputBuffer + DataBlockSize;
		for (bool Reading = true; Reading;)
		{
			size_t DataRead = 0;
			while (Reading && DataRead != DataBlockSize)
			{
				size_t ReadResult = fread((void*)(InputBuffer + DataRead), 1, DataBlockSize - DataRead, Input);
				if (ReadResult)
					DataRead += ReadResult;
				else if (feof(Input))
					Reading = false;
				else if (ferror(Input))
				{
					printf("Error reading input file \"%s\" failed\n", InputFileName);
					free(InputBuffer);
					fclose(Input);
					fclose(Output);
					if (DefaultOutputFileName)
					{
						remove(OutputFileName);
						free(OutputFileName);
					}
					return -1;
				}
			}
			uint8_t* Read = InputBuffer;
			uint8_t* Write = OutputBuffer;
			for (size_t Lines = DataRead / BytesPerLine, LineIndex = 0; LineIndex != Lines; ++LineIndex)
			{
				for (uint8_t* ReadEnd = Read + BytesPerLine; Read != ReadEnd; ++Read)
				{
					*Write++ = AsciiHexDigits[*Read >> 4];
					*Write++ = AsciiHexDigits[*Read & 0xF];
				}
				*Write++ = 0x0A;
			}
			if (Read != InputBuffer + DataRead)
			{
				for (uint8_t* ReadEnd = InputBuffer + DataRead; Read != ReadEnd; ++Read)
				{
					*Write++ = AsciiHexDigits[*Read >> 4];
					*Write++ = AsciiHexDigits[*Read & 0xF];
				}
				*Write++ = 0x0A;
			}
			for (size_t WriteSize = (size_t)((uintptr_t)Write - (uintptr_t)OutputBuffer), DataWritten = 0; DataWritten != WriteSize;)
			{
				size_t WriteResult = fwrite((void*)(OutputBuffer + DataWritten), 1, WriteSize - DataWritten, Output);
				if (!WriteResult && ferror(Output))
				{
					printf("Error writing output file \"%s\" failed\n", OutputFileName);
					free(InputBuffer);
					fclose(Input);
					fclose(Output);
					if (DefaultOutputFileName)
					{
						remove(OutputFileName);
						free(OutputFileName);
					}
					return -1;
				}
				DataWritten += WriteResult;
			}
		}
		free(InputBuffer);
	}
	int Result = fflush(Output);
	fclose(Input);
	fclose(Output);
	if (DefaultOutputFileName)
		free(OutputFileName);
	if (!Result)
		printf("Operation ended successfully\n");
	else
		printf("Warning some thing went wrong writing output file\n");
	return Result;
};