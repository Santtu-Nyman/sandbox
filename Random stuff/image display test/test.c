#include <Windows.h>
#include <string.h>
#include "ssn_display_window.h"
#include "ssn_native_command_to_arguments.h"

void main()
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
	{
		MessageBoxW(0, L"Failed to get process heap handle", L"Error", MB_ICONERROR | MB_OK);
		ExitProcess(EXIT_FAILURE);
	}

	size_t argument_count;
	char** argument_table;
	if (ssn_native_command_to_arguments(GetCommandLineW(), &argument_count, &argument_table))
	{
		MessageBoxW(0, L"Failed to get process command line", L"Error", MB_ICONERROR | MB_OK);
		ExitProcess(EXIT_FAILURE);
	}

	if (argument_count < 2)
	{
		MessageBoxW(0, L"No image file name specified", L"Error", MB_ICONERROR | MB_OK);
		ExitProcess(EXIT_FAILURE);
	}
	char* file_name = argument_table[1];

	int image_width;
	int image_height;
	DWORD* image_pixels;
	DWORD error = ssn_load_image_file(file_name, 0, heap, &image_width, &image_height, &image_pixels);
	if (error)
	{
		MessageBoxW(0, L"Failed to load image file", L"Error", MB_ICONERROR | MB_OK);
		ExitProcess(EXIT_FAILURE);
	}

	error = ssn_display_window(file_name, image_width, image_height, image_pixels);
	if (error)
	{
		MessageBoxW(0, L"Failed to create window", L"Error", MB_ICONERROR | MB_OK);
		HeapFree(heap, 0, image_pixels);
		ExitProcess(EXIT_FAILURE);
	}

	memset(image_pixels, 0, image_height * image_width * sizeof(DWORD));

	HeapFree(heap, 0, image_pixels);
	VirtualFree(argument_table, 0, MEM_RELEASE);
	ExitProcess(EXIT_SUCCESS);
	__assume(0);
}
