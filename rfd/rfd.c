#include <Windows.h>
#define TMP_BUFFER_SIZE 0x10000
#define MAX_FILE_NAME_LENGTH 0x7FFF

void rfd_memcpy(void* dst, const void* src, size_t size)
{
	for (const void* end = (const void*)((uintptr_t)src + size); src != end; src = (const void*)((uintptr_t)src + 1), dst = (void*)((uintptr_t)dst + 1))
		*(BYTE*)dst = *(const BYTE*)src;
}

int rename_from_directory(size_t argument_count, const WCHAR** argument_table, HANDLE std_output, HANDLE std_input)
{
	DWORD io_transfer_size;

	if (argument_count < 2)
	{
		WriteFile(std_output, "Error: No directory parameter given\n", 36, &io_transfer_size, 0);
		return EINVAL;
	}

	CHAR* buffer = (CHAR*)LocalAlloc(0, TMP_BUFFER_SIZE + (2 * ((MAX_FILE_NAME_LENGTH + 1) * sizeof(WCHAR))));
	WCHAR* file_name_buffer = (WCHAR*)((uintptr_t)buffer + TMP_BUFFER_SIZE);
	WCHAR* new_file_name_buffer = (WCHAR*)((uintptr_t)buffer + TMP_BUFFER_SIZE + ((MAX_FILE_NAME_LENGTH + 1) * sizeof(WCHAR)));
	if (!buffer)
	{
		WriteFile(std_output, "Error: Memory allocation failed\n", 32, &io_transfer_size, 0);
		return ENOMEM;
	}

	size_t directory_path_length = (size_t)lstrlenW(argument_table[1]);
	if (directory_path_length && argument_table[1][directory_path_length - 1] == L'\\' && argument_table[1][directory_path_length - 1] == L'/')
		--directory_path_length;

	if (directory_path_length + 2 > MAX_FILE_NAME_LENGTH)
	{
		WriteFile(std_output, "Error: Directory path is too long\n", 34, &io_transfer_size, 0);
		LocalFree(buffer);
		return ENAMETOOLONG;
	}
	rfd_memcpy(file_name_buffer, argument_table[1], directory_path_length * sizeof(WCHAR));
	rfd_memcpy(file_name_buffer + directory_path_length, L"\\*", 3 * sizeof(WCHAR));
	rfd_memcpy(new_file_name_buffer, file_name_buffer, (directory_path_length + 1) * sizeof(WCHAR));

	WIN32_FIND_DATAW file_data;
	HANDLE search_handle = FindFirstFileW(file_name_buffer, &file_data);
	DWORD search_error = search_handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
	while (!search_error)
	{
		size_t file_name_length = (size_t)lstrlenW(file_data.cFileName);
		if (!(file_name_length == 1 && file_data.cFileName[0] == L'.') && !(file_name_length == 2 && file_data.cFileName[0] == L'.' && file_data.cFileName[1] == L'.'))
		{
			if (file_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES && (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				WriteFile(std_output, "Rename directory \"", 18, &io_transfer_size, 0);
			else
				WriteFile(std_output, "Rename file \"", 13, &io_transfer_size, 0);
			size_t utf8_file_name_length = WideCharToMultiByte(CP_UTF8, 0, file_data.cFileName, (int)file_name_length, buffer, TMP_BUFFER_SIZE, 0, 0);
			if (utf8_file_name_length)
				WriteFile(std_output, buffer, (DWORD)utf8_file_name_length, &io_transfer_size, 0);
			else
				WriteFile(std_output, "?", 1, &io_transfer_size, 0);
			WriteFile(std_output, "\" Y/N?\n", 7, &io_transfer_size, 0);

			BOOL rename_file = (BOOL)-1;
			while (rename_file == (BOOL)-1)
			{
				if (!ReadFile(std_input, buffer, TMP_BUFFER_SIZE, &io_transfer_size, 0))
				{
					WriteFile(std_output, "Error: Reading std input failed\n", 32, &io_transfer_size, 0);
					if (search_handle != INVALID_HANDLE_VALUE)
						FindClose(search_handle);
					LocalFree(buffer);
					return EIO;
				}
				if ((io_transfer_size == 2 && buffer[0] == 'N' && buffer[1] == '\n') || (io_transfer_size == 3 && buffer[0] == 'N' && buffer[1] == '\r' && buffer[2] == '\n'))
					rename_file = FALSE;
				else if ((io_transfer_size == 2 && buffer[0] == 'Y' && buffer[1] == '\n') || (io_transfer_size == 3 && buffer[0] == 'Y' && buffer[1] == '\r' && buffer[2] == '\n'))
					rename_file = TRUE;
				else
					WriteFile(std_output, "Enter Y or N\n", 13, &io_transfer_size, 0);
			}
			if (rename_file)
			{
				WriteFile(std_output, "Enter new file name\n", 20, &io_transfer_size, 0);
				if (!ReadFile(std_input, buffer, TMP_BUFFER_SIZE, &io_transfer_size, 0))
				{
					WriteFile(std_output, "Error: Reading std input failed\n", 32, &io_transfer_size, 0);
					if (search_handle != INVALID_HANDLE_VALUE)
						FindClose(search_handle);
					LocalFree(buffer);
					return EIO;
				}
				size_t ut8_new_file_name_length = (size_t)io_transfer_size;
				if (ut8_new_file_name_length >= 2 && buffer[ut8_new_file_name_length - 2] == '\r' && buffer[ut8_new_file_name_length - 1] == '\n')
					ut8_new_file_name_length -= 2;
				else if (ut8_new_file_name_length >= 1 && buffer[ut8_new_file_name_length - 1] == '\n')
					ut8_new_file_name_length -= 1;

				size_t new_file_name_length = (size_t)MultiByteToWideChar(CP_UTF8, 0, buffer, (int)ut8_new_file_name_length, file_name_buffer, MAX_FILE_NAME_LENGTH);
				if (!new_file_name_length)
				{
					WriteFile(std_output, "Error: Invalid file name inputed\n", 33, &io_transfer_size, 0);
					if (search_handle != INVALID_HANDLE_VALUE)
						FindClose(search_handle);
					LocalFree(buffer);
					return EBADMSG;
				}

				if (directory_path_length + 1 + new_file_name_length > MAX_FILE_NAME_LENGTH)
				{
					WriteFile(std_output, "Error: New file path is too long\n", 33, &io_transfer_size, 0);
					if (search_handle != INVALID_HANDLE_VALUE)
						FindClose(search_handle);
					LocalFree(buffer);
					return EIO;
				}
				rfd_memcpy(new_file_name_buffer + directory_path_length + 1, file_name_buffer, new_file_name_length * sizeof(WCHAR));
				*(new_file_name_buffer + directory_path_length + 1 + new_file_name_length) = 0;

				if (directory_path_length + 1 + file_name_length > MAX_FILE_NAME_LENGTH)
				{
					WriteFile(std_output, "Error: Old file path is too long\n", 33, &io_transfer_size, 0);
					if (search_handle != INVALID_HANDLE_VALUE)
						FindClose(search_handle);
					LocalFree(buffer);
					return EIO;
				}
				rfd_memcpy(file_name_buffer, argument_table[1], directory_path_length * sizeof(WCHAR));
				*(file_name_buffer + directory_path_length) = L'\\';
				rfd_memcpy(file_name_buffer + directory_path_length + 1, file_data.cFileName, (file_name_length + 1) * sizeof(WCHAR));

				/*
				WriteFile(std_output, "Renamed \"", 9, &io_transfer_size, 0);
				size_t utf8_old_name_length = WideCharToMultiByte(CP_UTF8, 0, file_name_buffer, (int)(directory_path_length + 1 + file_name_length), buffer, TMP_BUFFER_SIZE, 0, 0);
				if (utf8_old_name_length)
					WriteFile(std_output, buffer, (DWORD)utf8_old_name_length, &io_transfer_size, 0);
				else
					WriteFile(std_output, "?", 1, &io_transfer_size, 0);
				WriteFile(std_output, "\" to \"", 6, &io_transfer_size, 0);
				size_t utf8_new_name_length = WideCharToMultiByte(CP_UTF8, 0, new_file_name_buffer, (int)(directory_path_length + 1 + new_file_name_length), buffer, TMP_BUFFER_SIZE, 0, 0);
				if (utf8_new_name_length)
					WriteFile(std_output, buffer, (DWORD)utf8_new_name_length, &io_transfer_size, 0);
				else
					WriteFile(std_output, "?", 1, &io_transfer_size, 0);
				WriteFile(std_output, "\"\n", 2, &io_transfer_size, 0);
				*/

				if (!MoveFileW(file_name_buffer, new_file_name_buffer))
				{
					WriteFile(std_output, "Error: Failed to rename file\n", 29, &io_transfer_size, 0);
					if (search_handle != INVALID_HANDLE_VALUE)
						FindClose(search_handle);
					LocalFree(buffer);
					return EIO;
				}
			}
		}
		search_error = FindNextFileW(search_handle, &file_data) ? 0 : GetLastError();
	}
	if (search_handle != INVALID_HANDLE_VALUE)
		FindClose(search_handle);

	if (search_error != ERROR_FILE_NOT_FOUND && search_error != ERROR_NO_MORE_FILES)
	{
		WriteFile(std_output, "Error: Directory search failed\n", 31, &io_transfer_size, 0);
		LocalFree(buffer);
		return EIO;
	}

	WriteFile(std_output, "No more files found. Operation ended successfully\n", 29, &io_transfer_size, 0);
	LocalFree(buffer);
	return 0;
}

void main()
{
	HANDLE std_output = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE std_input = GetStdHandle(STD_INPUT_HANDLE);
	int argument_count;
	WCHAR** argument_table = (WCHAR**)CommandLineToArgvW(GetCommandLineW(), &argument_count);
	if (!argument_table)
		ExitProcess(EXIT_FAILURE);
	rename_from_directory((size_t)argument_count, (const WCHAR**)argument_table, std_output, std_input);
	LocalFree(argument_table);
	ExitProcess(EXIT_SUCCESS);
}
