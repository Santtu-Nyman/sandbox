#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include <TlHelp32.h>
#include "calendar.h"
#include <stddef.h>
#include <stdint.h>

BOOL search_argument(SIZE_T argument_count, const WCHAR** argument_values, const WCHAR* short_argument, const WCHAR* long_argument, const WCHAR** value)
{
	for (SIZE_T index = 0; index != argument_count; ++index)
		if ((short_argument && !lstrcmpW(short_argument, argument_values[index])) || (long_argument && !lstrcmpW(long_argument, argument_values[index])))
		{
			if (value)
				*value = index + 1 != argument_count ? argument_values[index + 1] : 0;
			return TRUE;
		}
	if (value)
		*value = 0;
	return FALSE;
}

DWORD get_arguments(HANDLE heap, SIZE_T* argument_count, const WCHAR*** argument_values)
{
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HMODULE shell32 = LoadLibraryW(L"Shell32.dll");
	if (!shell32)
		return GetLastError();
	SIZE_T local_argument_count = 0;
	const WCHAR** local_argument_values = ((const WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(shell32, "CommandLineToArgvW"))(GetCommandLineW(), (int*)&local_argument_count);
	if (!local_argument_values)
	{
		error = GetLastError();
		FreeLibrary(shell32);
		return error;
	}
	SIZE_T argument_value_data_size = 0;
	for (SIZE_T i = 0; i != local_argument_count; ++i)
		argument_value_data_size += (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
	WCHAR** argument_buffer = (WCHAR**)HeapAlloc(heap, 0, local_argument_count * sizeof(WCHAR*) + argument_value_data_size);
	if (!argument_buffer)
	{
		error = GetLastError();
		LocalFree(local_argument_values);
		FreeLibrary(shell32);
		return error;
	}
	for (SIZE_T w = local_argument_count * sizeof(WCHAR*), i = 0; i != local_argument_count; ++i)
	{
		WCHAR* p = (WCHAR*)((UINT_PTR)argument_buffer + w);
		SIZE_T s = (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
		argument_buffer[i] = p;
		for (WCHAR* copy_source = (WCHAR*)local_argument_values[i], *copy_source_end = (WCHAR*)((UINT_PTR)copy_source + s), *copy_destination = argument_buffer[i]; copy_source != copy_source_end; ++copy_source, ++copy_destination)
			*copy_destination = *copy_source;
		w += s;
	}
	LocalFree(local_argument_values);
	FreeLibrary(shell32);
	*argument_count = local_argument_count;
	*argument_values = (const WCHAR**)argument_buffer;
	return 0;
}

DWORD load_file(const WCHAR* file_name, HANDLE heap, SIZE_T* file_size, LPVOID* file_data)
{
	DWORD error = 0;
	HANDLE file_handle = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return GetLastError();
	ULONGLONG native_file_size;
	if (GetFileSizeEx(file_handle, (LARGE_INTEGER*)&native_file_size))
	{
		if (native_file_size <= SIZE_MAX)
		{
			SIZE_T memory_file_size = (SIZE_T)native_file_size;
			LPVOID file_data_buffer = HeapAlloc(heap, 0, memory_file_size);
			if (file_data_buffer)
			{
				for (SIZE_T file_read_result = 0, file_loaded = 0; !error && file_loaded != memory_file_size; file_loaded += file_read_result)
					if (!ReadFile(file_handle, (LPVOID)((UINT_PTR)file_data_buffer + file_loaded), (DWORD)((memory_file_size - file_loaded) < 0x80000000 ? (memory_file_size - file_loaded) : 0x80000000), (DWORD*)&file_read_result, 0))
						error = GetLastError();
				if (!error)
				{
					*file_size = memory_file_size;
					*file_data = file_data_buffer;
					CloseHandle(file_handle);
					return 0;
				}
				HeapFree(heap, 0, file_data_buffer);
			}
		}
		else
			error = ERROR_FILE_TOO_LARGE;
	}
	else
		error = GetLastError();
	CloseHandle(file_handle);
	return error;
}

DWORD save_file(const WCHAR* file_name, SIZE_T file_size, LPVOID file_data)
{
	DWORD error = 0;
	HANDLE file_handle = CreateFileW(file_name, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return GetLastError();
	if (!SetFilePointer(file_handle, 0, 0, FILE_BEGIN) && SetEndOfFile(file_handle))
	{
		for (SIZE_T file_write_result = 0, file_saved = 0; !error && file_saved != file_size; file_saved += file_write_result)
			if (!WriteFile(file_handle, (LPVOID)((UINT_PTR)file_data + file_saved), (DWORD)((file_size - file_saved) < 0x80000000 ? (file_size - file_saved) : 0x80000000), (DWORD*)&file_write_result, 0))
				error = GetLastError();
		if (!error)
		{
			if (FlushFileBuffers(file_handle))
			{
				CloseHandle(file_handle);
				return 0;
			}
			else
				error = GetLastError();
		}
	}
	else
		error = GetLastError();
	CloseHandle(file_handle);
	DeleteFileW(file_name);
	return error;
}

DWORD print(HANDLE console, const WCHAR* string)
{
	DWORD console_write_lenght = 0;
	DWORD string_lenght = lstrlenW(string);
	return WriteConsoleW(console, string, string_lenght, &console_write_lenght, 0) && console_write_lenght == string_lenght ? 0 : GetLastError();
}

void print_date(DWORD raw_date, WCHAR* buffer)
{
	// date format L"YYYY.MM.DD DOW HH:MM:SS Week WN";
	Date date = getDate((unsigned int)raw_date);
	if (date.ymd.year > 9999)
		date.ymd.year = 9999;
	*buffer++ = (WCHAR)(0x30 + ((date.ymd.year / 1000) % 10));
	*buffer++ = (WCHAR)(0x30 + ((date.ymd.year / 100) % 10));
	*buffer++ = (WCHAR)(0x30 + ((date.ymd.year / 10) % 10));
	*buffer++ = (WCHAR)(0x30 + (date.ymd.year % 10));
	*buffer++ = L'.';
	*buffer++ = (WCHAR)(0x30 + (date.ymd.month / 10));
	*buffer++ = (WCHAR)(0x30 + (date.ymd.month % 10));
	*buffer++ = L'.';
	*buffer++ = (WCHAR)(0x30 + (date.ymd.day / 10));
	*buffer++ = (WCHAR)(0x30 + (date.ymd.day % 10));
	*buffer++ = L' ';
	for (const WCHAR* days_of_week[7] = { L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat", L"Sun" }, *read_day_of_week = days_of_week[date.week.day]; *read_day_of_week; ++read_day_of_week)
		*buffer++ = *read_day_of_week;
	*buffer++ = L' ';
	*buffer++ = (WCHAR)(0x30 + (date.hms.hour / 10));
	*buffer++ = (WCHAR)(0x30 + (date.hms.hour % 10));
	*buffer++ = L':';
	*buffer++ = (WCHAR)(0x30 + (date.hms.minute / 10));
	*buffer++ = (WCHAR)(0x30 + (date.hms.minute % 10));
	*buffer++ = L':';
	*buffer++ = (WCHAR)(0x30 + (date.hms.second / 10));
	*buffer++ = (WCHAR)(0x30 + (date.hms.second % 10));
	*buffer++ = L' ';
	*buffer++ = L'W';
	*buffer++ = L'e';
	*buffer++ = L'e';
	*buffer++ = L'k';
	*buffer++ = L' ';
	*buffer++ = (WCHAR)(0x30 + (date.week.number / 10));
	*buffer++ = (WCHAR)(0x30 + (date.week.number % 10));
}

SIZE_T append_text(BYTE* destination, const BYTE* source)
{
	const BYTE* read_source = source;
	while (*read_source)
		*destination++ = *read_source++;
	*destination++ = *read_source++;
	return (SIZE_T)((UINT_PTR)read_source - (UINT_PTR)source);
}

SIZE_T print_decimal_number(SIZE_T buffer_length, BYTE* buffer, DWORD number)
{
	SIZE_T length = 0;
	while (length != buffer_length && number)
	{
		for (BYTE* move = buffer + length++; move != buffer; --move)
			*move = *(move - 1);
		*buffer = (BYTE)(0x30 + (number % 10));
		number /= 10;
	}
	if (length != buffer_length && !length)
	{
		++length;
		*buffer = 0x30;
	}
	if (length != buffer_length)
	{
		*(buffer + length) = 0;
		return length + 1;
	}
	return 0;
}

void print_64KiB_progress(HANDLE ouput, SIZE_T transferred)
{
	WCHAR output[24];
	WCHAR* write = output;
	transferred >>= 10;
	if (transferred > 9)
		*write++ = (WCHAR)(0x30 + (transferred / 10) % 10);
	*write++ = (WCHAR)(0x30 + transferred % 10);
	for (const WCHAR* copy_source = L"/64 KiB transferred.\n", *copy_source_end = copy_source + 22; copy_source != copy_source_end; ++copy_source)
		*write++ = *copy_source;
	print(ouput, output);
}

DWORD open_com_port_to_time_stomper(const WCHAR* com_port, HANDLE* com_port_handle)
{
	if (com_port[0] == L'\\' && com_port[1] == L'\\' && com_port[2] == L'.' && com_port[3] == L'\\' && com_port[4] == L'C' && com_port[5] == L'O' && com_port[6] == L'M' && com_port[7] >= L'0' && com_port[7] <= L'9')
	{
		for (WCHAR* name_test = (WCHAR*)com_port + 8; *name_test; ++name_test)
			if (*name_test < L'0' || *name_test > L'9')
				return ERROR_INVALID_PARAMETER;
	}
	else if (com_port[0] == L'C' && com_port[1] == L'O' && com_port[2] == L'M' && com_port[3] >= L'0' && com_port[3] <= L'9')
	{
		for (WCHAR* name_test = (WCHAR*)com_port + 4; *name_test; ++name_test)
			if (*name_test < L'0' || *name_test > L'9')
				return ERROR_INVALID_PARAMETER;
	}
	else
		return ERROR_INVALID_PARAMETER;
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return GetLastError();
	WCHAR* port_name = (WCHAR*)com_port;
	if (port_name[0] != L'\\' || port_name[1] != L'\\' || port_name[2] != L'.' || port_name[3] != L'\\')
	{
		SIZE_T input_port_name_length = 0;
		while (com_port[input_port_name_length])
			++input_port_name_length;
		port_name = (WCHAR*)HeapAlloc(heap, 0, (5 + input_port_name_length) * sizeof(WCHAR));
		if (!port_name)
			return GetLastError();
		WCHAR* write_port_name = port_name;
		*write_port_name++ = L'\\';
		*write_port_name++ = L'\\';
		*write_port_name++ = L'.';
		*write_port_name++ = L'\\';
		for (WCHAR* input_port_name = (WCHAR*)com_port; *input_port_name; ++input_port_name, ++write_port_name)
			*write_port_name = *input_port_name;
		*write_port_name = 0;
	}
	DWORD serial_configuration_size = sizeof(COMMCONFIG);
	COMMCONFIG* serial_configuration = (COMMCONFIG*)HeapAlloc(heap, 0, (SIZE_T)serial_configuration_size);
	if (!serial_configuration)
	{
		error = GetLastError();
		if (port_name != com_port)
			HeapFree(heap, 0, port_name);
		return error;
	}
	for (BOOL get_serial_configuration = TRUE; get_serial_configuration;)
	{
		DWORD get_serial_configuration_size = serial_configuration_size;
		if (GetDefaultCommConfigW(port_name + 4, serial_configuration, &get_serial_configuration_size))
		{
			serial_configuration_size = get_serial_configuration_size;
			get_serial_configuration = FALSE;
		}
		else
		{
			if (get_serial_configuration_size > serial_configuration_size)
			{
				serial_configuration_size = get_serial_configuration_size;
				COMMCONFIG* new_allocation = (COMMCONFIG*)HeapReAlloc(heap, 0, serial_configuration, (SIZE_T)serial_configuration_size);
				if (!new_allocation)
				{
					error = GetLastError();
					HeapFree(heap, 0, serial_configuration);
					if (port_name != com_port)
						HeapFree(heap, 0, port_name);
					return error;
				}
				serial_configuration = new_allocation;
			}
			else
			{
				error = GetLastError();
				HeapFree(heap, 0, serial_configuration);
				if (port_name != com_port)
					HeapFree(heap, 0, port_name);
				return error;
			}
		}
	}
	serial_configuration->dcb.BaudRate = CBR_9600;
	serial_configuration->dcb.ByteSize = 8;
	serial_configuration->dcb.StopBits = ONESTOPBIT;
	serial_configuration->dcb.Parity = NOPARITY;
	serial_configuration->dcb.fDtrControl = DTR_CONTROL_ENABLE;
	COMMTIMEOUTS serial_timeouts = { 0x800, 0x800, 0x800, 0x800, 0x800 };
	HANDLE handle = CreateFileW(port_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (handle == INVALID_HANDLE_VALUE)
	{
		error = GetLastError();
		HeapFree(heap, 0, serial_configuration);
		if (port_name != com_port)
			HeapFree(heap, 0, port_name);
		return error;
	}
	if (!SetupComm(handle, systemInfo.dwPageSize < 0x10000 ? systemInfo.dwPageSize : 0x10000, systemInfo.dwPageSize < 0x10000 ? systemInfo.dwPageSize : 0x10000) ||
		!SetCommConfig(handle, serial_configuration, serial_configuration_size) ||
		!SetCommTimeouts(handle, &serial_timeouts) ||
		!PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR))
	{
		error = GetLastError();
		CloseHandle(handle);
		HeapFree(heap, 0, serial_configuration);
		if (port_name != com_port)
			HeapFree(heap, 0, port_name);
		return error;
	}
	Sleep(0x800);
	COMSTAT serial_status;
	DWORD serial_errors;
	ClearCommError(handle, &serial_errors, &serial_status);
	HeapFree(heap, 0, serial_configuration);
	*com_port_handle = handle;
	if (port_name != com_port)
		HeapFree(heap, 0, port_name);
	return 0;
}

const WCHAR* search_time_stomper_port(DWORD time)
{
	time = time ? time + GetTickCount() : 0;
	const WCHAR* constant_port_list[] = {
		L"\\\\.\\COM0", L"\\\\.\\COM1", L"\\\\.\\COM2", L"\\\\.\\COM3", L"\\\\.\\COM4", L"\\\\.\\COM5", L"\\\\.\\COM6", L"\\\\.\\COM7", L"\\\\.\\COM8", L"\\\\.\\COM9",
		L"\\\\.\\COM10", L"\\\\.\\COM11", L"\\\\.\\COM12", L"\\\\.\\COM13", L"\\\\.\\COM14", L"\\\\.\\COM15", L"\\\\.\\COM16", L"\\\\.\\COM17", L"\\\\.\\COM18", L"\\\\.\\COM19",
		L"\\\\.\\COM20", L"\\\\.\\COM21", L"\\\\.\\COM22", L"\\\\.\\COM23", L"\\\\.\\COM24", L"\\\\.\\COM25", L"\\\\.\\COM26", L"\\\\.\\COM27", L"\\\\.\\COM28", L"\\\\.\\COM29",
		L"\\\\.\\COM30", L"\\\\.\\COM31", L"\\\\.\\COM32", L"\\\\.\\COM33", L"\\\\.\\COM34", L"\\\\.\\COM35", L"\\\\.\\COM36", L"\\\\.\\COM37", L"\\\\.\\COM38", L"\\\\.\\COM39",
		L"\\\\.\\COM40", L"\\\\.\\COM41", L"\\\\.\\COM42", L"\\\\.\\COM43", L"\\\\.\\COM44", L"\\\\.\\COM45", L"\\\\.\\COM46", L"\\\\.\\COM47", L"\\\\.\\COM48", L"\\\\.\\COM49",
		L"\\\\.\\COM50", L"\\\\.\\COM51", L"\\\\.\\COM52", L"\\\\.\\COM53", L"\\\\.\\COM54", L"\\\\.\\COM55", L"\\\\.\\COM56", L"\\\\.\\COM57", L"\\\\.\\COM58", L"\\\\.\\COM59",
		L"\\\\.\\COM60", L"\\\\.\\COM61", L"\\\\.\\COM62", L"\\\\.\\COM63", L"\\\\.\\COM64", L"\\\\.\\COM65", L"\\\\.\\COM66", L"\\\\.\\COM67", L"\\\\.\\COM68", L"\\\\.\\COM69",
		L"\\\\.\\COM70", L"\\\\.\\COM71", L"\\\\.\\COM72", L"\\\\.\\COM73", L"\\\\.\\COM74", L"\\\\.\\COM75", L"\\\\.\\COM76", L"\\\\.\\COM77", L"\\\\.\\COM78", L"\\\\.\\COM79",
		L"\\\\.\\COM80", L"\\\\.\\COM81", L"\\\\.\\COM82", L"\\\\.\\COM83", L"\\\\.\\COM84", L"\\\\.\\COM85", L"\\\\.\\COM86", L"\\\\.\\COM87", L"\\\\.\\COM88", L"\\\\.\\COM89",
		L"\\\\.\\COM90", L"\\\\.\\COM91", L"\\\\.\\COM92", L"\\\\.\\COM93", L"\\\\.\\COM94", L"\\\\.\\COM95", L"\\\\.\\COM96", L"\\\\.\\COM97", L"\\\\.\\COM98", L"\\\\.\\COM99" };
	HANDLE handle;
	DWORD io_result;
	BYTE remote_access_buffer;
	for (const WCHAR** e = constant_port_list + (sizeof(constant_port_list) / sizeof(WCHAR*));;)
	{
		for (const WCHAR** i = constant_port_list; i != e; ++i)
		{
			if (time && time < GetTickCount())
				return 0;
			if (!open_com_port_to_time_stomper(*i, &handle))
			{
				remote_access_buffer = 0x3F;
				BOOL success = WriteFile(handle, &remote_access_buffer, 1, &io_result, 0) && io_result == 1 && ReadFile(handle, &remote_access_buffer, 1, &io_result, 0) && io_result == 1 && remote_access_buffer == 0x4B;
				CloseHandle(handle);
				if (success)
					return *i;
			}
		}
		Sleep(0x800);
	}
}

DWORD get_time_stomper_date(HANDLE time_stomper, DWORD* date)
{
	DWORD io_result;
	BYTE remote_access_buffer[4];
	remote_access_buffer[0] = 0x2A;
	if (!WriteFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
		return GetLastError();
	if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1 || remote_access_buffer[0] != 0x4B)
		return GetLastError();
	for (SIZE_T remote_access_read = 0; remote_access_read != 4; ++remote_access_read)
		if (!ReadFile(time_stomper, (LPVOID)((UINT_PTR)remote_access_buffer + remote_access_read), 1, &io_result, 0) || io_result != 1)
			return GetLastError();
	*date = (DWORD)remote_access_buffer[0] | ((DWORD)remote_access_buffer[1] << 8) | ((DWORD)remote_access_buffer[2] << 16) | ((DWORD)remote_access_buffer[3] << 24);
	return 0;
}

DWORD synchronize_time_stomper_date(HANDLE time_stomper, DWORD* raw_date)
{
	ULONGLONG windows_system_time;
	GetSystemTimeAsFileTime((FILETIME*)&windows_system_time);
	DWORD translated_time = (DWORD)((windows_system_time - (ULONGLONG)131012640000000000) / (ULONGLONG)10000000);// { 2016, 3, 0, 1, 0, 0, 0, 0 } SYSTEMTIME is 131012640000000000 FILETIME
	DWORD io_result;
	BYTE remote_access_buffer[5] = { 0x23, (BYTE)translated_time, (BYTE)(translated_time >> 8), (BYTE)(translated_time >> 16), (BYTE)(translated_time >> 24) };
	for (SIZE_T remote_access_write = 0; remote_access_write != 5; ++remote_access_write)
		if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
			return GetLastError();
	if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1 || remote_access_buffer[0] != 0x4B)
		return GetLastError();
	if (raw_date)
		*raw_date = translated_time;
	return 0;
}

DWORD read_time_stomper_eeprom(HANDLE time_stomper, LPVOID eeprom_buffer, HANDLE console_to_print_progress)
{
	DWORD io_result;
	BYTE remote_access_buffer[3];
	for (SIZE_T read_offset = 0; read_offset != 0x10000; read_offset += 0x80)
	{
		remote_access_buffer[0] = 0x24;
		remote_access_buffer[1] = (BYTE)read_offset;
		remote_access_buffer[2] = (BYTE)(read_offset >> 8);
		for (SIZE_T remote_access_write = 0; remote_access_write != 3; ++remote_access_write)
			if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
				return GetLastError();
		if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1 || remote_access_buffer[0] != 0x4B)
			return GetLastError();
		for (SIZE_T remote_access_read = 0; remote_access_read != 0x80; ++remote_access_read)
			if (!ReadFile(time_stomper, (LPVOID)((UINT_PTR)eeprom_buffer + read_offset + remote_access_read), 1, &io_result, 0) || io_result != 1)
				return GetLastError();
		if (console_to_print_progress && !((read_offset + 0x80) & 0x3FF))
			print_64KiB_progress(console_to_print_progress, read_offset + 0x80);
	}
	return 0;
}

DWORD write_time_stomper_eeprom(HANDLE time_stomper, LPCVOID eeprom_buffer, HANDLE console_to_print_progress)
{
	DWORD io_result;
	BYTE remote_access_buffer[3];
	for (SIZE_T write_offset = 0; write_offset != 0x10000; write_offset += 0x80)
	{
		remote_access_buffer[0] = 0x24;
		remote_access_buffer[1] = (BYTE)write_offset | 1;
		remote_access_buffer[2] = (BYTE)(write_offset >> 8);
		for (SIZE_T remote_access_write = 0; remote_access_write != 3; ++remote_access_write)
			if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
				return GetLastError();
		if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1 || remote_access_buffer[0] != 0x4B)
			return GetLastError();
		for (SIZE_T remote_access_write = 0; remote_access_write != 0x80; ++remote_access_write)
			if (!WriteFile(time_stomper, (LPCVOID)((UINT_PTR)eeprom_buffer + write_offset + remote_access_write), 1, &io_result, 0) || io_result != 1)
				return GetLastError();
		if (console_to_print_progress && !((write_offset + 0x80) & 0x3FF))
			print_64KiB_progress(console_to_print_progress, write_offset + 0x80);
	}
	return 0;
}

DWORD erase_time_stomper_eeprom(HANDLE time_stomper, HANDLE console_to_print_progress)
{
	DWORD io_result;
	const BYTE empty_byte = 0;
	BYTE remote_access_buffer[3];
	for (SIZE_T write_offset = 0; write_offset != 0x10000; write_offset += 0x80)
	{
		remote_access_buffer[0] = 0x24;
		remote_access_buffer[1] = (BYTE)write_offset | 1;
		remote_access_buffer[2] = (BYTE)(write_offset >> 8);
		for (SIZE_T remote_access_write = 0; remote_access_write != 3; ++remote_access_write)
			if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
				return GetLastError();
		if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1 || remote_access_buffer[0] != 0x4B)
			return GetLastError();
		for (SIZE_T remote_access_write = 0; remote_access_write != 0x80; ++remote_access_write)
			if (!WriteFile(time_stomper, (LPCVOID)&empty_byte, 1, &io_result, 0) || io_result != 1)
				return GetLastError();
		if (console_to_print_progress && !((write_offset + 0x80) & 0x3FF))
			print_64KiB_progress(console_to_print_progress, write_offset + 0x80);
	}
	return 0;
}

DWORD print_time_stamp_information_to_file(const WCHAR* file_name, LPCVOID time_stamp_information)
{
	DWORD error = 0;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return GetLastError();
	BYTE* ouput_buffer = (BYTE*)HeapAlloc(heap, 0, 128);
	if (!ouput_buffer)
		return GetLastError();
	DWORD output_file_io_result;
	HANDLE output_file = CreateFileW(file_name, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
	if (output_file == INVALID_HANDLE_VALUE)
	{
		error = GetLastError();
		HeapFree(heap, 0, ouput_buffer);
		return error;
	}
	if (SetFilePointer(output_file, 0, 0, FILE_BEGIN) || !SetEndOfFile(output_file))
	{
		error = GetLastError();
		CloseHandle(output_file);
		DeleteFileW(file_name);
		HeapFree(heap, 0, ouput_buffer);
		return error;
	}
	const BYTE* days_of_week[7] = { (const BYTE*)"Mon", (const BYTE*)"Tue", (const BYTE*)"Wed", (const BYTE*)"Thu", (const BYTE*)"Fri", (const BYTE*)"Sat", (const BYTE*)"Sun" };
	for (SIZE_T stamp_count = 0, offset = 0; offset != 65530; offset += 10)
	{
		WORD sample_identity = (WORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset) | ((WORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 1) << 8);
		DWORD sample_start_time = (DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 2) | ((DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 3) << 8) | ((DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 4) << 16) | ((DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 5) << 24);
		DWORD sample_end_time = (DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 6) | ((DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 7) << 8) | ((DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 8) << 16) | ((DWORD)*(const BYTE*)((UINT_PTR)time_stamp_information + offset + 9) << 24);
		if (sample_identity)
		{
			Date date = getDate((unsigned int)sample_start_time);
			BYTE* write_output = ouput_buffer;
			*write_output++ = '#';
			write_output += print_decimal_number(~0, write_output, (DWORD)stamp_count++) - 1;
			*write_output++ = ' ';
			write_output += print_decimal_number(~0, write_output, (DWORD)sample_identity) - 1;
			*write_output++ = ' ';
			if (sample_end_time)
			{
				DWORD sample_total_time = sample_end_time - sample_start_time;
				DWORD sample_total_hours = sample_total_time / 3600;
				DWORD sample_total_minutes = (sample_total_time / 60) % 60;
				DWORD sample_total_seconds = sample_total_time % 60;
				if (sample_total_hours < 10)
				{
					*write_output++ = 0x30;
					*write_output++ = (BYTE)(0x30 + sample_total_hours);
				}
				else
					write_output += print_decimal_number(~0, write_output, sample_total_hours) - 1;
				*write_output++ = ':';
				*write_output++ = (BYTE)(0x30 + (sample_total_minutes / 10));
				*write_output++ = (BYTE)(0x30 + (sample_total_minutes % 10));
				*write_output++ = ':';
				*write_output++ = (BYTE)(0x30 + (sample_total_seconds / 10));
				*write_output++ = (BYTE)(0x30 + (sample_total_seconds % 10));
			}
			else
				write_output += append_text(write_output, (const BYTE*)"??:??:??") - 1;
			write_output += append_text(write_output, (const BYTE*)" @ ") - 1;
			write_output += print_decimal_number(~0, write_output, (DWORD)date.ymd.year) - 1;
			*write_output++ = '.';
			*write_output++ = (BYTE)(0x30 + (date.ymd.month / 10));
			*write_output++ = (BYTE)(0x30 + (date.ymd.month % 10));
			*write_output++ = '.';
			*write_output++ = (BYTE)(0x30 + (date.ymd.day / 10));
			*write_output++ = (BYTE)(0x30 + (date.ymd.day % 10));
			*write_output++ = ' ';
			write_output += append_text(write_output, days_of_week[date.week.day]) - 1;
			*write_output++ = ' ';
			*write_output++ = (BYTE)(0x30 + (date.hms.hour / 10));
			*write_output++ = (BYTE)(0x30 + (date.hms.hour % 10));
			*write_output++ = ':';
			*write_output++ = (BYTE)(0x30 + (date.hms.minute / 10));
			*write_output++ = (BYTE)(0x30 + (date.hms.minute % 10));
			*write_output++ = ':';
			*write_output++ = (BYTE)(0x30 + (date.hms.second / 10));
			*write_output++ = (BYTE)(0x30 + (date.hms.second % 10));
			write_output += append_text(write_output, (const BYTE*)" Week ") - 1;
			*write_output++ = (BYTE)(0x30 + (date.week.number / 10));
			*write_output++ = (BYTE)(0x30 + (date.week.number % 10));
			*write_output++ = '\n';
			for (SIZE_T output_size = (SIZE_T)((UINT_PTR)write_output - (UINT_PTR)ouput_buffer), file_written = 0; file_written != output_size; file_written += (SIZE_T)output_file_io_result)
				if (!WriteFile(output_file, ouput_buffer + (SIZE_T)file_written, (DWORD)(output_size - file_written), &output_file_io_result, 0))
				{
					error = GetLastError();
					CloseHandle(output_file);
					DeleteFileW(file_name);
					HeapFree(heap, 0, ouput_buffer);
					return error;
				}
		}
		else
			offset = 65520;
	}
	if (!FlushFileBuffers(output_file))
	{
		error = GetLastError();
		CloseHandle(output_file);
		DeleteFileW(file_name);
		HeapFree(heap, 0, ouput_buffer);
		return error;
	}
	CloseHandle(output_file);
	return 0;
}

void time_synchronization_service()
{
	SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);
	DWORD error = 0;
	DWORD disable_time_synchronization_messages = 0;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		ExitProcess((UINT)GetLastError());
	HMODULE User32 = LoadLibraryW(L"User32.dll");
	if (!User32)
		ExitProcess((UINT)GetLastError());
	int (WINAPI* MessageBoxW)(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) = (int (WINAPI*)(HWND, LPCTSTR, LPCTSTR, UINT))GetProcAddress(User32, "MessageBoxW");
	if (!MessageBoxW)
	{
		error = GetLastError();
		FreeLibrary(User32);
		ExitProcess((UINT)error);
	}
	DWORD serial_configuration_allocation_size = sizeof(COMMCONFIG);
	DWORD serial_configuration_size = serial_configuration_allocation_size;
	WCHAR* synchronized_message = (WCHAR*)HeapAlloc(heap, 0, (81 * sizeof(WCHAR)) + (SIZE_T)serial_configuration_allocation_size);
	if (!synchronized_message)
	{
		error = GetLastError();
		FreeLibrary(User32);
		ExitProcess((UINT)error);
	}
	for (WCHAR* read_empty_message = (WCHAR*)L"Time Stomper timer successfully synchronized to YYYY.MM.DD DOW HH:MM:SS Week WN.", *read_empty_message_end = read_empty_message + 81, * write_empty_message = synchronized_message; read_empty_message != read_empty_message_end; ++read_empty_message, ++write_empty_message)
		 *write_empty_message = *read_empty_message;
	COMMCONFIG* serial_configuration = (COMMCONFIG*)((UINT_PTR)synchronized_message + (81 * sizeof(WCHAR)));
	for (const WCHAR* serial_port_name = search_time_stomper_port(0);; serial_port_name = search_time_stomper_port(0))
	{
		for (BOOL get_serial_configuration = TRUE; get_serial_configuration;)
		{
			serial_configuration_size = serial_configuration_allocation_size;
			if (GetDefaultCommConfigW(serial_port_name + 4, serial_configuration, &serial_configuration_size))
				get_serial_configuration = FALSE;
			else
			{
				if (serial_configuration_size > serial_configuration_allocation_size)
				{
					serial_configuration_allocation_size = serial_configuration_size;
					LPVOID new_allocation = HeapReAlloc(heap, 0, serial_configuration, (81 * sizeof(WCHAR)) + (SIZE_T)serial_configuration_allocation_size);
					if (!new_allocation)
					{
						error = GetLastError();
						HeapFree(heap, 0, serial_configuration);
						FreeLibrary(User32);
						ExitProcess((UINT)error);
					}
					synchronized_message = (WCHAR*)synchronized_message;
					synchronized_message = (WCHAR*)((UINT_PTR)new_allocation + (81 * sizeof(WCHAR)));
				}
				else
				{
					error = GetLastError();
					get_serial_configuration = FALSE;
				}
			}
		}
		if (!error)
		{
			HANDLE time_stomper;
			if (!open_com_port_to_time_stomper(serial_port_name, &time_stomper))
			{
				DWORD raw_date;
				error = synchronize_time_stomper_date(time_stomper, &raw_date);
				CloseHandle(time_stomper);
				if (!error)
				{
					HKEY time_stomper_registry_key;
					error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Time Stomper", 0, 0, 0, KEY_QUERY_VALUE, 0, &time_stomper_registry_key, 0);
					if (!error)
					{
						DWORD disable_time_synchronization_messages_registry_value_type;
						DWORD disable_time_synchronization_messages_registry_value;
						DWORD disable_time_synchronization_messages_registry_value_size = sizeof(DWORD);
						error = (DWORD)RegQueryValueExW(time_stomper_registry_key, L"disable_time_synchronization_messages", 0, &disable_time_synchronization_messages_registry_value_type, (BYTE*)&disable_time_synchronization_messages_registry_value, &disable_time_synchronization_messages_registry_value_size);
						if (!error && disable_time_synchronization_messages_registry_value_type == REG_DWORD && disable_time_synchronization_messages_registry_value_size == sizeof(DWORD))
							disable_time_synchronization_messages = disable_time_synchronization_messages_registry_value ? 1 : 0;
						RegCloseKey(time_stomper_registry_key);
					}
					if (!disable_time_synchronization_messages)
					{
						print_date(raw_date, synchronized_message + 48);
						MessageBoxW(0, synchronized_message, L"Time Stomper", MB_OK | MB_ICONINFORMATION);
					}
					for (serial_configuration_size = serial_configuration_allocation_size; GetDefaultCommConfigW(serial_port_name + 4, serial_configuration, &serial_configuration_size); serial_configuration_size = serial_configuration_allocation_size)
						Sleep(0x1000);
				}
			}
		}
		error = 0;
		Sleep(0x1000);
	}
}

DWORD is_time_synchronization_servece_installed(BOOL* is_installed)
{
	HKEY run_registry_key;
	DWORD error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, 0, 0, KEY_QUERY_VALUE, 0, &run_registry_key, 0);
	if (error)
		return error;
	DWORD run_time_stomper_registry_value_type;
	error = (DWORD)RegQueryValueExW(run_registry_key, L"TimeStomper", 0, &run_time_stomper_registry_value_type, 0, 0);
	RegCloseKey(run_registry_key);
	if (error && error != ERROR_FILE_NOT_FOUND)
		return error;
	*is_installed = error != ERROR_FILE_NOT_FOUND && run_time_stomper_registry_value_type == REG_SZ ? TRUE : FALSE;
	return 0;
}

DWORD disable_time_synchronization_messages()
{
	BOOL is_installed;
	DWORD error = is_time_synchronization_servece_installed(&is_installed);
	if (error)
		return error;
	if (!is_installed)
		return ERROR_INSTALL_NOTUSED;
	HKEY time_stomper_registry_key;
	error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Time Stomper", 0, 0, 0, KEY_SET_VALUE, 0, &time_stomper_registry_key, 0);
	if (!error)
	{
		DWORD disable_time_synchronization_messages_registry_value = 1;
		error = (DWORD)RegSetValueExW(time_stomper_registry_key, L"disable_time_synchronization_messages", 0, REG_DWORD, (BYTE*)&disable_time_synchronization_messages_registry_value, sizeof(DWORD));
		RegCloseKey(time_stomper_registry_key);
	}
	return error;
}

DWORD enable_time_synchronization_messages()
{
	BOOL is_installed;
	DWORD error = is_time_synchronization_servece_installed(&is_installed);
	if (error)
		return error;
	if (!is_installed)
		return ERROR_INSTALL_NOTUSED;
	HKEY time_stomper_registry_key;
	error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Time Stomper", 0, 0, 0, KEY_SET_VALUE, 0, &time_stomper_registry_key, 0);
	if (!error)
	{
		DWORD disable_time_synchronization_messages_registry_value = 0;
		error = (DWORD)RegSetValueExW(time_stomper_registry_key, L"disable_time_synchronization_messages", 0, REG_DWORD, (BYTE*)&disable_time_synchronization_messages_registry_value, sizeof(DWORD));
		RegCloseKey(time_stomper_registry_key);
	}
	return error;
}

DWORD install_time_synchronization_service()
{
	BOOL is_already_installed;
	DWORD error = is_time_synchronization_servece_installed(&is_already_installed);
	if (error)
		return error;
	if (is_already_installed)
		return 0;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return GetLastError();
	HMODULE current_module;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const WCHAR*)install_time_synchronization_service, &current_module))
		return GetLastError();
	SIZE_T current_module_file_name_length;
	WCHAR* current_module_file_name = (WCHAR*)HeapAlloc(heap, 0, (MAX_PATH + 1) * sizeof(WCHAR));
	if (!current_module_file_name)
		return GetLastError();
	for (SIZE_T current_module_file_name_capasity = MAX_PATH + 1; current_module_file_name_capasity;)
	{
		current_module_file_name_length = (SIZE_T)GetModuleFileNameW(current_module, current_module_file_name, (DWORD)current_module_file_name_capasity) * sizeof(WCHAR);
		if (current_module_file_name_length < current_module_file_name_capasity)
			current_module_file_name_capasity = 0;
		else if (current_module_file_name_length)
		{
			current_module_file_name_capasity += MAX_PATH;
			WCHAR* new_current_module_file_name = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, current_module_file_name_capasity * sizeof(WCHAR));
			if (!new_current_module_file_name)
			{
				error = GetLastError();
				HeapFree(heap, 0, current_module_file_name);
				return error;
			}
			current_module_file_name = new_current_module_file_name;
		}
		else
		{
			error = GetLastError();
			HeapFree(heap, 0, current_module_file_name);
			return error;
		}
	}
	SIZE_T app_data_directory_length = (SIZE_T)GetEnvironmentVariableW(L"APPDATA", 0, 0);
	if (!app_data_directory_length)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	WCHAR* app_data_directory = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, ((current_module_file_name_length + 1) * sizeof(WCHAR)) + (app_data_directory_length * sizeof(WCHAR)) + ((app_data_directory_length + 13) * sizeof(WCHAR)) + ((app_data_directory_length + 46) * sizeof(WCHAR)) + ((app_data_directory_length + 79) * sizeof(WCHAR)));
	if (!app_data_directory)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	current_module_file_name = app_data_directory;
	app_data_directory += (current_module_file_name_length + 1);
	if ((SIZE_T)GetEnvironmentVariableW(L"APPDATA", app_data_directory, (DWORD)app_data_directory_length) + 1 != app_data_directory_length)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	WCHAR* ts_time_synchronization_service_directory = app_data_directory + app_data_directory_length;
	for (WCHAR* d = ts_time_synchronization_service_directory, *s = app_data_directory, *e = s + (app_data_directory_length - 1); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_directory + (app_data_directory_length - 1), *s = (WCHAR*)L"\\Time Stomper", *e = s + 14; s != e; ++s, ++d)
		*d = *s;
	WCHAR* ts_time_synchronization_service_executable = ts_time_synchronization_service_directory + (app_data_directory_length + 13);
	for (WCHAR* d = ts_time_synchronization_service_executable, *s = ts_time_synchronization_service_directory, *e = s + (app_data_directory_length + 12); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_executable + (app_data_directory_length + 12), *s = (WCHAR*)L"\\time_synchronization_service.exe", *e = s + 34; s != e; ++s, ++d)
		*d = *s;
	WCHAR* ts_time_synchronization_service_command = ts_time_synchronization_service_executable + (app_data_directory_length + 46);
	*ts_time_synchronization_service_command = L'"';
	for (WCHAR* d = ts_time_synchronization_service_command + 1, *s = ts_time_synchronization_service_executable, *e = s + (app_data_directory_length + 45); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_command + (app_data_directory_length + 46), *s = (WCHAR*)L"\" --time_synchronization_service", *e = s + 33; s != e; ++s, ++d)
		*d = *s;
	error = CreateDirectoryW(ts_time_synchronization_service_directory, 0) ? 0 : GetLastError();
	if (error == ERROR_ALREADY_EXISTS)
		error = 0;
	if (!error)
	{
		if (CopyFileW(current_module_file_name, ts_time_synchronization_service_executable, FALSE))
		{
			HANDLE ts_time_synchronization_service_executable_handle = CreateFileW(ts_time_synchronization_service_executable, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
			if (ts_time_synchronization_service_executable_handle != INVALID_HANDLE_VALUE)
			{
				error = SetFilePointer(ts_time_synchronization_service_executable_handle, 0x3C, 0, FILE_BEGIN) == 0x3C ? 0 : GetLastError();
				DWORD image_nt_header_offset;
				for (DWORD file_read_result, file_read = 0; !error && file_read != 4;)
					if (ReadFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_nt_header_offset + file_read), 4 - file_read, &file_read_result, 0))
						file_read += file_read_result;
					else
						error = GetLastError();
				if (!error)
				{
					error = SetFilePointer(ts_time_synchronization_service_executable_handle, image_nt_header_offset, 0, FILE_BEGIN) == image_nt_header_offset ? 0 : GetLastError();
					DWORD image_nt_header_signature;
					for (DWORD file_read_result, file_read = 0; !error && file_read != 4;)
						if (ReadFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_nt_header_signature + file_read), 4 - file_read, &file_read_result, 0))
							file_read += file_read_result;
						else
							error = GetLastError();
					WORD image_file_header_machine_architecture;
					for (DWORD file_read_result, file_read = 0; !error && file_read != 2;)
						if (ReadFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_file_header_machine_architecture + file_read), 2 - file_read, &file_read_result, 0))
							file_read += file_read_result;
						else
							error = GetLastError();
					if (!error)
					{
						if (image_nt_header_signature == 0x00004550 && (image_file_header_machine_architecture == IMAGE_FILE_MACHINE_AMD64 || image_file_header_machine_architecture == IMAGE_FILE_MACHINE_I386))
						{
							error = SetFilePointer(ts_time_synchronization_service_executable_handle, image_nt_header_offset + ((image_file_header_machine_architecture == IMAGE_FILE_MACHINE_AMD64) ? 0x5C : 0x58), 0, FILE_BEGIN) == image_nt_header_offset + ((image_file_header_machine_architecture == IMAGE_FILE_MACHINE_AMD64) ? 0x5C : 0x58) ? 0 : GetLastError();
							const WORD image_optional_header_Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
							for (DWORD file_write_result, file_written = 0; !error && file_written != 2;)
								if (WriteFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_optional_header_Subsystem + file_written), 2 - file_written, &file_write_result, 0))
									file_written += file_write_result;
								else
									error = GetLastError();
							if (FlushFileBuffers(ts_time_synchronization_service_executable_handle))
							{
								CloseHandle(ts_time_synchronization_service_executable_handle);
								ts_time_synchronization_service_executable_handle = INVALID_HANDLE_VALUE;
								if (!error)
								{
									HKEY run_registry_key;
									error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, 0, 0, KEY_SET_VALUE, 0, &run_registry_key, 0);
									if (!error)
									{
										error = (DWORD)RegSetValueExW(run_registry_key, L"TimeStomper", 0, REG_SZ, (const BYTE*)ts_time_synchronization_service_command, (DWORD)((app_data_directory_length + 79) * sizeof(WCHAR)));
										if (!error)
										{
											STARTUPINFO time_synchronization_service_process_strtup_information = { sizeof(STARTUPINFO), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
											PROCESS_INFORMATION time_synchronization_service_process_information = { 0, 0, 0, 0 };
											if (CreateProcessW(ts_time_synchronization_service_executable, ts_time_synchronization_service_command, 0, 0, 0, CREATE_DEFAULT_ERROR_MODE | CREATE_UNICODE_ENVIRONMENT, 0, ts_time_synchronization_service_directory, &time_synchronization_service_process_strtup_information, &time_synchronization_service_process_information))
											{
												CloseHandle(time_synchronization_service_process_information.hThread);
												CloseHandle(time_synchronization_service_process_information.hProcess);
												RegCloseKey(run_registry_key);
												HeapFree(heap, 0, current_module_file_name);
												return 0;
											}
											else
												error = GetLastError();
											RegDeleteValueW(run_registry_key, L"TimeStomper");
										}
										RegCloseKey(run_registry_key);
									}
								}
							}
							else
								error = GetLastError();
						}
						else
							error = ERROR_BAD_EXE_FORMAT;
					}
				}
				if (ts_time_synchronization_service_executable_handle != INVALID_HANDLE_VALUE)
					CloseHandle(ts_time_synchronization_service_executable_handle);
			}
			else
				error = GetLastError();
			DeleteFileW(ts_time_synchronization_service_executable);
		}
		else
			error = GetLastError();
		RemoveDirectoryW(ts_time_synchronization_service_directory);
	}
	HeapFree(heap, 0, current_module_file_name);
	return error;
}

DWORD uninstall_time_synchronization_service()
{
	BOOL is_installed;
	DWORD error = is_time_synchronization_servece_installed(&is_installed);
	if (error)
		return error;
	if (!is_installed)
		return ERROR_INSTALL_NOTUSED;
	HMODULE Kernel32 = GetModuleHandleW(L"Kernel32.dll");
	if (!Kernel32)
		return GetLastError();
	HANDLE (WINAPI* CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID) = (HANDLE (WINAPI*)(DWORD, DWORD))GetProcAddress(Kernel32, "CreateToolhelp32Snapshot");
	if (!CreateToolhelp32Snapshot)
		return GetLastError();
	BOOL (WINAPI* Process32FirstW)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) = (BOOL (WINAPI*)(HANDLE, LPPROCESSENTRY32W))GetProcAddress(Kernel32, "Process32FirstW");
	if (!Process32FirstW)
		return GetLastError();
	BOOL (WINAPI* Process32NextW)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) = (BOOL (WINAPI*)(HANDLE, LPPROCESSENTRY32W))GetProcAddress(Kernel32, "Process32NextW");
	if (!Process32NextW)
		return GetLastError();
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return GetLastError();
	HMODULE current_module;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const WCHAR*)install_time_synchronization_service, &current_module))
		return GetLastError();
	SIZE_T current_module_file_name_length;
	WCHAR* current_module_file_name = (WCHAR*)HeapAlloc(heap, 0, (MAX_PATH + 1) * sizeof(WCHAR));
	if (!current_module_file_name)
		return GetLastError();
	for (SIZE_T current_module_file_name_capasity = MAX_PATH + 1; current_module_file_name_capasity;)
	{
		current_module_file_name_length = (SIZE_T)GetModuleFileNameW(current_module, current_module_file_name, (DWORD)current_module_file_name_capasity) * sizeof(WCHAR);
		if (current_module_file_name_length < current_module_file_name_capasity)
			current_module_file_name_capasity = 0;
		else if (current_module_file_name_length)
		{
			current_module_file_name_capasity += MAX_PATH;
			WCHAR* new_current_module_file_name = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, current_module_file_name_capasity * sizeof(WCHAR));
			if (!new_current_module_file_name)
			{
				error = GetLastError();
				HeapFree(heap, 0, current_module_file_name);
				return error;
			}
			current_module_file_name = new_current_module_file_name;
		}
		else
		{
			error = GetLastError();
			HeapFree(heap, 0, current_module_file_name);
			return error;
		}
	}
	SIZE_T app_data_directory_length = (SIZE_T)GetEnvironmentVariableW(L"APPDATA", 0, 0);
	if (!app_data_directory_length)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	WCHAR* app_data_directory = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, ((current_module_file_name_length + 1) * sizeof(WCHAR)) + (app_data_directory_length * sizeof(WCHAR)) + ((app_data_directory_length + 13) * sizeof(WCHAR)) + ((app_data_directory_length + 46) * sizeof(WCHAR)) + ((app_data_directory_length + 79) * sizeof(WCHAR)) + ((app_data_directory_length + 46) * sizeof(WCHAR)));
	if (!app_data_directory)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	current_module_file_name = app_data_directory;
	app_data_directory += (current_module_file_name_length + 1);
	if ((SIZE_T)GetEnvironmentVariableW(L"APPDATA", app_data_directory, (DWORD)app_data_directory_length) + 1 != app_data_directory_length)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	WCHAR* ts_time_synchronization_service_directory = app_data_directory + app_data_directory_length;
	for (WCHAR* d = ts_time_synchronization_service_directory, *s = app_data_directory, *e = s + (app_data_directory_length - 1); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_directory + (app_data_directory_length - 1), *s = (WCHAR*)L"\\Time Stomper", *e = s + 14; s != e; ++s, ++d)
		*d = *s;
	WCHAR* ts_time_synchronization_service_executable = ts_time_synchronization_service_directory + (app_data_directory_length + 13);
	for (WCHAR* d = ts_time_synchronization_service_executable, *s = ts_time_synchronization_service_directory, *e = s + (app_data_directory_length + 12); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_executable + (app_data_directory_length + 12), *s = (WCHAR*)L"\\time_synchronization_service.exe", *e = s + 34; s != e; ++s, ++d)
		*d = *s;
	WCHAR* ts_time_synchronization_service_command = ts_time_synchronization_service_executable + (app_data_directory_length + 46);
	*ts_time_synchronization_service_command = L'"';
	for (WCHAR* d = ts_time_synchronization_service_command + 1, *s = ts_time_synchronization_service_executable, *e = s + (app_data_directory_length + 45); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_command + (app_data_directory_length + 46), *s = (WCHAR*)L"\" --time_synchronization_service", *e = s + 33; s != e; ++s, ++d)
		*d = *s;
	WCHAR* image_file_name_buffer = ts_time_synchronization_service_command + (app_data_directory_length + 79);
	HANDLE process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!process_snapshot)
	{
		error = GetLastError();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	PROCESSENTRY32W process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32W);
	for (BOOL loop = Process32FirstW(process_snapshot, &process_entry); loop; loop = Process32NextW(process_snapshot, &process_entry))
	{
		HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, process_entry.th32ProcessID);
		if (process_handle)
		{
			DWORD image_file_name_buffer_length = (DWORD)(app_data_directory_length + 46);
			if (QueryFullProcessImageNameW(process_handle, 0, image_file_name_buffer, &image_file_name_buffer_length) && !lstrcmpiW(image_file_name_buffer, ts_time_synchronization_service_executable))
				TerminateProcess(process_handle, 0);
			CloseHandle(process_handle);
		}
	}
	CloseHandle(process_snapshot);
	HKEY run_registry_key;
	error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, 0, 0, KEY_SET_VALUE, 0, &run_registry_key, 0);
	if (error)
	{
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	error = (DWORD)RegDeleteValueW(run_registry_key, L"TimeStomper");
	RegCloseKey(run_registry_key);
	if (error)
	{
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	DeleteFileW(ts_time_synchronization_service_executable);
	RemoveDirectoryW(ts_time_synchronization_service_directory);
	RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Time Stomper");
	HeapFree(heap, 0, current_module_file_name);
	return 0;
}

DWORD main_process(HANDLE heap, SIZE_T argc, const WCHAR** argv)
{
	if (search_argument(argc, argv, 0, L"--time_synchronization_service", 0))
		time_synchronization_service();
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HANDLE console_out = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (console_out == INVALID_HANDLE_VALUE)
		return GetLastError();
	BOOL read_mode = search_argument(argc, argv, L"-r", L"--read", 0);
	BOOL write_mode = search_argument(argc, argv, L"-w", L"--write", 0);
	BOOL test_mode = search_argument(argc, argv, L"-t", L"--test", 0);
	BOOL erase_mode = search_argument(argc, argv, L"-e", L"--erase", 0);
	BOOL print_time_stamp_data_mode = search_argument(argc, argv, L"-d", L"--print_time_stamp_data", 0);
	BOOL set_device_date_mode = search_argument(argc, argv, L"-s", L"--set_device_date", 0);
	BOOL get_device_date_mode = search_argument(argc, argv, L"-g", L"--get_device_date", 0);
	BOOL install_time_synchronization_service_mode = search_argument(argc, argv, 0, L"--install_time_synchronization_service", 0);
	BOOL uninstall_time_synchronization_service_mode = search_argument(argc, argv, 0, L"--uninstall_time_synchronization_service", 0);
	const WCHAR* com_port_name;
	search_argument(argc, argv, L"-p", L"--port", &com_port_name);
	const WCHAR* input_file_name;
	search_argument(argc, argv, L"-i", L"--input", &input_file_name);
	const WCHAR* output_file_name;
	search_argument(argc, argv, L"-o", L"--output", &output_file_name);
	if ((((read_mode ? 1 : 0) + (write_mode ? 1 : 0) + (test_mode ? 1 : 0) + (erase_mode ? 1 : 0) + (print_time_stamp_data_mode ? 1 : 0) + (set_device_date_mode ? 1 : 0) + (get_device_date_mode ? 1 : 0) + (install_time_synchronization_service_mode ? 1 : 0) + (uninstall_time_synchronization_service_mode ? 1 : 0)) != 1) || (!com_port_name && (!print_time_stamp_data_mode && !install_time_synchronization_service_mode && !uninstall_time_synchronization_service_mode)) || (read_mode && !output_file_name) || (write_mode && !input_file_name) || (test_mode && (!input_file_name || !output_file_name)) || (print_time_stamp_data_mode && (((com_port_name ? 1 : 0) + (input_file_name ? 1 : 0)) != 1 || !output_file_name)))
	{
		print(console_out, L"The arguments passed to the program are invalid.\n");
		CloseHandle(console_out);
		return ERROR_BAD_ARGUMENTS;
	}
	if (install_time_synchronization_service_mode)
	{
		error = install_time_synchronization_service();
		print(console_out, error ? L"Installing time synchronization service failed.\n" : L"Installing time synchronization service successful.\n");
		CloseHandle(console_out);
		return error;
	}
	if (uninstall_time_synchronization_service_mode)
	{
		error = uninstall_time_synchronization_service();
		print(console_out, error ? L"Uninstalling time synchronization service failed.\n" : L"Uninstalling time synchronization service successful.\n");
		CloseHandle(console_out);
		return error;
	}
	HANDLE time_stomper = INVALID_HANDLE_VALUE;
	error = com_port_name ? open_com_port_to_time_stomper(com_port_name, &time_stomper) : 0;
	if (error)
	{
		print(console_out, L"Unable to open com port \"");
		print(console_out, com_port_name);
		print(console_out, L"\".\n");
		CloseHandle(console_out);
		return error;
	}
	if (get_device_date_mode)
	{
		DWORD raw_date;
		error = get_time_stomper_date(time_stomper, &raw_date);
		if (error)
		{
			print(console_out, L"Reading device timer failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		WCHAR* output_buffer = (WCHAR*)HeapAlloc(heap, 0, 128 * sizeof(WCHAR));
		if (!output_buffer)
		{
			error = GetLastError();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		WCHAR* write_output = output_buffer;
		for (const WCHAR* i = L"Date in device "; *i; ++i)
			*write_output++ = *i;
		print_date(raw_date, write_output);
		write_output += 31;
		*write_output++ = L'\n';
		*write_output++ = 0;
		print(console_out, output_buffer);
		HeapFree(heap, 0, output_buffer);
		CloseHandle(time_stomper);
		CloseHandle(console_out);
		return 0;
	}
	if (set_device_date_mode)
	{
		DWORD raw_date;
		error = synchronize_time_stomper_date(time_stomper, &raw_date);
		if (error)
		{
			print(console_out, L"Device timer synchronization failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		WCHAR* output_buffer = (WCHAR*)HeapAlloc(heap, 0, 128 * sizeof(WCHAR));
		if (!output_buffer)
		{
			error = GetLastError();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		for (WCHAR* read_output = (WCHAR*)L"Device timer successfully synchronized to YYYY.MM.DD DOW HH:MM:SS Week WN.\n", *read_output_end = read_output + 76, *write_output = output_buffer; read_output != read_output_end; ++read_output, ++write_output)
			*write_output = *read_output;
		print_date(raw_date, output_buffer + 42);
		print(console_out, output_buffer);
		HeapFree(heap, 0, output_buffer);
		CloseHandle(time_stomper);
		CloseHandle(console_out);
		return 0;
	}
	if (erase_mode)
	{
		print(console_out, L"Erasing EEPROM...\n");
		error = erase_time_stomper_eeprom(time_stomper, console_out);
		if (error)
		{
			print(console_out, L"Erasing EEPROM failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Erasing EEPROM successful\n");
		CloseHandle(time_stomper);
		CloseHandle(console_out);
		return 0;
	}
	if (print_time_stamp_data_mode)
	{
		BYTE* file_buffer;
		if (time_stomper != INVALID_HANDLE_VALUE)
		{
			file_buffer = (BYTE*)HeapAlloc(heap, 0, 0x10000);
			if (!file_buffer)
			{
				error = GetLastError();
				print(console_out, L"Memory allocation failed.\n");
				CloseHandle(time_stomper);
				CloseHandle(console_out);
				return error;
			}
			print(console_out, L"Reading EEPROM...\n");
			error = read_time_stomper_eeprom(time_stomper, file_buffer, console_out);
			if (error)
			{
				print(console_out, L"Reading EEPROM failed.\n");
				HeapFree(heap, 0, file_buffer);
				CloseHandle(time_stomper);
				CloseHandle(console_out);
				return error;
			}
			print(console_out, L"Reading EEPROM successful.\n");
			CloseHandle(time_stomper);
		}
		else
		{
			SIZE_T input_file_size;
			error = load_file(input_file_name, heap, &input_file_size, (LPVOID*)&file_buffer);
			if (error || input_file_size != 0x10000)
			{
				if (input_file_size != 0x10000)
					error = ERROR_INVALID_DATA;
				print(console_out, L"Unable to load input file \"");
				print(console_out, input_file_name);
				print(console_out, L"\".\n");
				HeapFree(heap, 0, file_buffer);
				CloseHandle(time_stomper);
				CloseHandle(console_out);
				return error;
			}
			print(console_out, L"Input file loaded \"");
			print(console_out, input_file_name);
			print(console_out, L"\".\n");
		}
		error = print_time_stamp_information_to_file(output_file_name, file_buffer);
		HeapFree(heap, 0, file_buffer);
		if (error)
		{
			print(console_out, L"Unable to write output file \"");
			print(console_out, output_file_name);
			print(console_out, L"\".\n");
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Time stamp information written to \"");
		print(console_out, output_file_name);
		print(console_out, L"\".\n");
		CloseHandle(console_out);
		return 0;
	}
	if (write_mode || test_mode)
	{
		LPVOID file_buffer;
		SIZE_T input_file_size;
		error = load_file(input_file_name, heap, &input_file_size, &file_buffer);
		if (error || input_file_size != 0x10000)
		{
			if (input_file_size != 0x10000)
				error = ERROR_INVALID_DATA;
			print(console_out, L"Unable to load input file \"");
			print(console_out, input_file_name);
			print(console_out, L"\".\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Input file loaded \"");
		print(console_out, input_file_name);
		print(console_out, L"\".\n");
		print(console_out, L"Writing EEPROM...\n");
		error = write_time_stomper_eeprom(time_stomper, file_buffer, console_out);
		HeapFree(heap, 0, file_buffer);
		if (error)
		{
			print(console_out, L"Writing EEPROM failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Writing EEPROM successful\n");
	}
	if (read_mode || test_mode)
	{
		LPVOID file_buffer = HeapAlloc(heap, 0, 0x10000);
		if (!file_buffer)
		{
			error = GetLastError();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Reading EEPROM...\n");
		error = read_time_stomper_eeprom(time_stomper, file_buffer, console_out);
		if (error)
		{
			print(console_out, L"Writing EEPROM failed.\n");
			HeapFree(heap, 0, file_buffer);
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Writing EEPROM successful.\n");
		error = save_file(output_file_name, 0x10000, file_buffer);
		HeapFree(heap, 0, file_buffer);
		if (error)
		{
			print(console_out, L"Error writing output file \"");
			print(console_out, output_file_name);
			print(console_out, L"\".\n");
			CloseHandle(time_stomper);
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"EEPROM data written to \"");
		print(console_out, output_file_name);
		print(console_out, L"\".\n");
	}
	CloseHandle(time_stomper);
	CloseHandle(console_out);
	return 0;
}

void entry_point()
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		ExitProcess((UINT)GetLastError());
	SIZE_T argc;
	const WCHAR** argv;
	DWORD error = get_arguments(heap, &argc, &argv);
	if (error)
		ExitProcess((UINT)error);
	error = main_process(heap, argc, argv);
	HeapFree(heap, 0, argv);
	ExitProcess((UINT)error);
}

#ifdef __cplusplus
}
#endif