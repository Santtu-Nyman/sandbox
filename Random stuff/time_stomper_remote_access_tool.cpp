#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include <Shellapi.h>
#include <TlHelp32.h>
#include "ts_calendar.h"
#include <stddef.h>
#include <stdint.h>

#define IS_DECIMAL_W(w) ((WCHAR)(w) >= L'0' && (WCHAR)(w) <= L'9')

inline void db_assert(BOOL assertion)
{
#if (defined(_DEBUG) || defined(DEBUG))
	if (!assertion)
		DebugBreak();
#endif
}

DWORD get_last_error()
{
	DWORD error = GetLastError();
	SetLastError(0);
	return error ? error : ERROR_UNIDENTIFIED_ERROR;
}

BOOL search_argument(SIZE_T argument_count, const WCHAR** argument_values, const WCHAR* short_argument, const WCHAR* long_argument, const WCHAR** value)
{
	db_assert(!argument_count || (argument_count && argument_values));
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
	db_assert(heap && argument_count && argument_values);
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HMODULE shell32 = LoadLibraryW(L"Shell32.dll");
	if (!shell32)
		return get_last_error();
	SIZE_T local_argument_count = 0;
	const WCHAR** local_argument_values = ((const WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(shell32, "CommandLineToArgvW"))(GetCommandLineW(), (int*)&local_argument_count);
	if (!local_argument_values)
	{
		error = get_last_error();
		FreeLibrary(shell32);
		return error;
	}
	SIZE_T argument_value_data_size = 0;
	for (SIZE_T i = 0; i != local_argument_count; ++i)
		argument_value_data_size += (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
	WCHAR** argument_buffer = (WCHAR**)HeapAlloc(heap, 0, local_argument_count * sizeof(WCHAR*) + argument_value_data_size);
	if (!argument_buffer)
	{
		error = get_last_error();
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
	db_assert(file_name && heap && file_size && file_data);
	DWORD error = 0;
	HANDLE file_handle = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return get_last_error();
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
						error = get_last_error();
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
		error = get_last_error();
	CloseHandle(file_handle);
	return error;
}

DWORD save_file(const WCHAR* file_name, SIZE_T file_size, LPVOID file_data)
{
	db_assert(file_name && (!file_size || (file_size && file_data)));
	DWORD error = 0;
	HANDLE file_handle = CreateFileW(file_name, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
	if (file_handle == INVALID_HANDLE_VALUE)
		return get_last_error();
	if (!SetFilePointer(file_handle, 0, 0, FILE_BEGIN) && SetEndOfFile(file_handle))
	{
		for (SIZE_T file_write_result = 0, file_saved = 0; !error && file_saved != file_size; file_saved += file_write_result)
			if (!WriteFile(file_handle, (LPVOID)((UINT_PTR)file_data + file_saved), (DWORD)((file_size - file_saved) < 0x80000000 ? (file_size - file_saved) : 0x80000000), (DWORD*)&file_write_result, 0))
				error = get_last_error();
		if (!error)
		{
			if (FlushFileBuffers(file_handle))
			{
				CloseHandle(file_handle);
				return 0;
			}
			else
				error = get_last_error();
		}
	}
	else
		error = get_last_error();
	CloseHandle(file_handle);
	DeleteFileW(file_name);
	return error;
}

DWORD print(HANDLE console, const WCHAR* string)
{
	DWORD console_write_lenght = 0;
	DWORD string_lenght = lstrlenW(string);
	return WriteConsoleW(console, string, string_lenght, &console_write_lenght, 0) && console_write_lenght == string_lenght ? 0 : get_last_error();
}

DWORD get_current_time()
{
	const SYSTEMTIME time_stomper_epoch_Greenwich_England = { 2016, 2, 0, 29, 22, 0, 0, 0 };
	ULONGLONG windows_file_time_stomper_epoch;
	db_assert(SystemTimeToFileTime(&time_stomper_epoch_Greenwich_England, (FILETIME*)&windows_file_time_stomper_epoch));
	ULONGLONG windows_file_time_current_time;
	GetSystemTimeAsFileTime((FILETIME*)&windows_file_time_current_time);
	const ULONGLONG file_time_to_seconds_divider = 10000000;
	return (DWORD)((windows_file_time_current_time - windows_file_time_stomper_epoch) / file_time_to_seconds_divider);
}

void print_full_format_date(DWORD raw_date, WCHAR* buffer)
{
	// date format L"YYYY.MM.DD DOW HH:MM:SS Week WN";
	db_assert(buffer != 0);
	Date date = getDate((uint32_t)raw_date);
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

DWORD decode_full_format_date(const WCHAR* date_string)
{
	//L"YYYY.MM.DD DOW HH:MM:SS Week WN";
	if (!IS_DECIMAL_W(date_string[0]) || !IS_DECIMAL_W(date_string[1]) || !IS_DECIMAL_W(date_string[2]) || !IS_DECIMAL_W(date_string[3]) ||
		date_string[4] != L'.' ||
		!IS_DECIMAL_W(date_string[5]) || !IS_DECIMAL_W(date_string[6]) ||
		date_string[7] != L'.' ||
		!IS_DECIMAL_W(date_string[8]) || !IS_DECIMAL_W(date_string[9]) ||
		date_string[10] != L' ' ||
		!((((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'M' | ((DWORD)L'o' << 8) | ((DWORD)L'n') << 16)) ||
		(((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'T' | ((DWORD)L'u' << 8) | ((DWORD)L'e') << 16)) ||
		(((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'W' | ((DWORD)L'e' << 8) | ((DWORD)L'd') << 16)) ||
		(((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'T' | ((DWORD)L'h' << 8) | ((DWORD)L'u') << 16)) ||
		(((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'F' | ((DWORD)L'r' << 8) | ((DWORD)L'i') << 16)) ||
		(((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'S' | ((DWORD)L'a' << 8) | ((DWORD)L't') << 16)) ||
		(((DWORD)date_string[11] | ((DWORD)date_string[12] << 8) | ((DWORD)date_string[13] << 16)) == ((DWORD)L'S' | ((DWORD)L'u' << 8) | ((DWORD)L'n') << 16))) ||
		date_string[14] != L' ' ||
		!IS_DECIMAL_W(date_string[15]) || !IS_DECIMAL_W(date_string[16]) ||
		date_string[17] != L':' ||
		!IS_DECIMAL_W(date_string[18]) || !IS_DECIMAL_W(date_string[19]) ||
		date_string[20] != L':' ||
		!IS_DECIMAL_W(date_string[21]) || !IS_DECIMAL_W(date_string[22]) ||
		date_string[23] != L' ' ||
		date_string[24] != L'W' || date_string[25] != L'e' || date_string[26] != L'e' || date_string[27] != L'k' ||
		date_string[28] != L' ' ||
		!IS_DECIMAL_W(date_string[29]) || !IS_DECIMAL_W(date_string[30]) ||
		date_string[31])
		return 0;
	return (DWORD)convertDateToSeconds(makeDate(
		(uint16_t)((((DWORD)date_string[0] - L'0') * 1000) + (((DWORD)date_string[1] - L'0') * 100) + (((DWORD)date_string[2] - L'0') * 10) + ((DWORD)date_string[3] - L'0')),
		(uint8_t)((((DWORD)date_string[5] - L'0') * 10) + ((DWORD)date_string[6] - L'0')),
		(uint8_t)((((DWORD)date_string[8] - L'0') * 10) + ((DWORD)date_string[9] - L'0')),
		(uint8_t)((((DWORD)date_string[15] - L'0') * 10) + ((DWORD)date_string[16] - L'0')),
		(uint8_t)((((DWORD)date_string[18] - L'0') * 10) + ((DWORD)date_string[19] - L'0')),
		(uint8_t)((((DWORD)date_string[21] - L'0') * 10) + ((DWORD)date_string[22] - L'0'))));
}

DWORD decode_generic_format_date(const WCHAR* date_string)
{
	DWORD from_full_format_date = decode_full_format_date(date_string);
	if (from_full_format_date)
		return from_full_format_date;
	DWORD part_length = 0;
	DWORD year = 0;
	DWORD month = 0;
	DWORD day = 0;
	DWORD hour = 0;
	DWORD minute = 0;
	DWORD second = 0;
	while (IS_DECIMAL_W(*date_string))
	{
		year = (year * 10) + ((DWORD)*date_string++ - (DWORD)L'0');
		++part_length;
	}
	WCHAR ymd_separator = *date_string++;
	if (!part_length || (ymd_separator != L'.' && ymd_separator != L'-' && ymd_separator != L'/'))
		return 0;
	part_length = 0;
	while (IS_DECIMAL_W(*date_string))
	{
		month = (month * 10) + ((DWORD)*date_string++ - (DWORD)L'0');
		++part_length;
	}
	if (!part_length || *date_string++ != ymd_separator)
		return 0;
	part_length = 0;
	while (IS_DECIMAL_W(*date_string))
	{
		day = (day * 10) + ((DWORD)*date_string++ - (DWORD)L'0');
		++part_length;
	}
	if (!part_length || (*date_string != L' ' && *date_string != 0))
		return 0;
	++date_string;
	while (*date_string == L' ')
		date_string++;
	if (IS_DECIMAL_W(*date_string))
	{
		part_length = 0;
		while (IS_DECIMAL_W(*date_string))
		{
			hour = (hour * 10) + ((DWORD)*date_string++ - (DWORD)L'0');
			++part_length;
		}
		if (!part_length || *date_string++ != L':')
			return 0;
		part_length = 0;
		while (IS_DECIMAL_W(*date_string))
		{
			minute = (minute * 10) + ((DWORD)*date_string++ - (DWORD)L'0');
			++part_length;
		}
		if (!part_length)
			return 0;
		if (*date_string == L':')
		{
			++date_string;
			part_length = 0;
			while (IS_DECIMAL_W(*date_string))
			{
				second = (second * 10) + ((DWORD)*date_string++ - (DWORD)L'0');
				++part_length;
			}
			if (!part_length || (*date_string != L' ' && *date_string != 0))
				return 0;
		}
		while (*date_string == L' ')
			date_string++;
	}
	if (*date_string)
		return 0;
	if (day > 99)
	{
		DWORD swap = year;
		year = day;
		day = swap;
	}
	return (DWORD)convertDateToSeconds(makeDate((uint16_t)year, (uint8_t)month, (uint8_t)day, (uint8_t)hour, (uint8_t)minute, (uint8_t)second));
}

SIZE_T append_text(BYTE* destination, const BYTE* source)
{
	db_assert(destination && source);
	const BYTE* read_source = source;
	while (*read_source)
		*destination++ = *read_source++;
	*destination++ = *read_source++;
	return (SIZE_T)((UINT_PTR)read_source - (UINT_PTR)source);
}

SIZE_T print_decimal_number(SIZE_T buffer_length, BYTE* buffer, DWORD number)
{
	db_assert(!number || (number && buffer));
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
	db_assert(ouput != INVALID_HANDLE_VALUE && transferred <= 0x10000);
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
	db_assert(com_port && com_port_handle);
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
		return get_last_error();
	WCHAR* port_name = (WCHAR*)com_port;
	if (port_name[0] != L'\\' || port_name[1] != L'\\' || port_name[2] != L'.' || port_name[3] != L'\\')
	{
		SIZE_T input_port_name_length = 0;
		while (com_port[input_port_name_length])
			++input_port_name_length;
		port_name = (WCHAR*)HeapAlloc(heap, 0, (5 + input_port_name_length) * sizeof(WCHAR));
		if (!port_name)
			return get_last_error();
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
		error = get_last_error();
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
					error = get_last_error();
					HeapFree(heap, 0, serial_configuration);
					if (port_name != com_port)
						HeapFree(heap, 0, port_name);
					return error;
				}
				serial_configuration = new_allocation;
			}
			else
			{
				error = get_last_error();
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
		error = get_last_error();
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
		error = get_last_error();
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

const WCHAR* search_time_stomper_port(DWORD time, BOOL ignore_stop_command)
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
		if (!ignore_stop_command)
		{
			HKEY time_stomper_registry_key;
			if (!RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Time Stomper", 0, 0, 0, KEY_QUERY_VALUE, 0, &time_stomper_registry_key, 0))
			{
				DWORD stop_port_search_registry_value_type;
				DWORD stop_port_search_registry_value;
				DWORD stop_port_search_registry_value_size = sizeof(DWORD);
				BOOL time_stomper_registry_key_is_open = TRUE;
				if (!RegQueryValueExW(time_stomper_registry_key, L"stop_port_search", 0, &stop_port_search_registry_value_type, (BYTE*)&stop_port_search_registry_value, &stop_port_search_registry_value_size) && stop_port_search_registry_value_type == REG_DWORD && stop_port_search_registry_value_size == sizeof(DWORD))
				{
					RegCloseKey(time_stomper_registry_key);
					time_stomper_registry_key_is_open = FALSE;
					if (stop_port_search_registry_value)
					{
						HANDLE current_process = GetCurrentProcess();
						BOOL priority_class_changed = SetPriorityClass(current_process, PROCESS_MODE_BACKGROUND_BEGIN);
						while (stop_port_search_registry_value)
						{
							Sleep(0x1000);
							if (!RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Time Stomper", 0, 0, 0, KEY_QUERY_VALUE, 0, &time_stomper_registry_key, 0))
							{
								if (RegQueryValueExW(time_stomper_registry_key, L"stop_port_search", 0, &stop_port_search_registry_value_type, (BYTE*)&stop_port_search_registry_value, &stop_port_search_registry_value_size) || stop_port_search_registry_value_type != REG_DWORD || stop_port_search_registry_value_size != sizeof(DWORD))
									stop_port_search_registry_value = FALSE;
								RegCloseKey(time_stomper_registry_key);
							}
							else
								stop_port_search_registry_value = FALSE;
						}
						if (priority_class_changed)
							SetPriorityClass(current_process, PROCESS_MODE_BACKGROUND_END);
					}
				}
				if (time_stomper_registry_key_is_open)
					RegCloseKey(time_stomper_registry_key);
			}
		}
		Sleep(0x1000);
	}
}

DWORD get_time_stomper_date(HANDLE time_stomper, DWORD* date)
{
	db_assert(time_stomper != INVALID_HANDLE_VALUE);
	DWORD io_result;
	BYTE remote_access_buffer[4];
	remote_access_buffer[0] = 0x2A;
	if (!WriteFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
		return get_last_error();
	if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
		return get_last_error();
	if (remote_access_buffer[0] != 0x4B)
		return ERROR_INVALID_HANDLE;
	for (SIZE_T remote_access_read = 0; remote_access_read != 4; ++remote_access_read)
		if (!ReadFile(time_stomper, (LPVOID)((UINT_PTR)remote_access_buffer + remote_access_read), 1, &io_result, 0) || io_result != 1)
			return get_last_error();
	*date = (DWORD)remote_access_buffer[0] | ((DWORD)remote_access_buffer[1] << 8) | ((DWORD)remote_access_buffer[2] << 16) | ((DWORD)remote_access_buffer[3] << 24);
	return 0;
}

DWORD synchronize_time_stomper_date(HANDLE time_stomper, DWORD* raw_date)
{
	db_assert(time_stomper != INVALID_HANDLE_VALUE);
	DWORD current_time = get_current_time();
	DWORD io_result;
	BYTE remote_access_buffer[5] = { 0x23, (BYTE)current_time, (BYTE)(current_time >> 8), (BYTE)(current_time >> 16), (BYTE)(current_time >> 24) };
	for (SIZE_T remote_access_write = 0; remote_access_write != 5; ++remote_access_write)
		if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
			return get_last_error();
	if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
		return get_last_error();
	if (remote_access_buffer[0] != 0x4B)
		return ERROR_INVALID_HANDLE;
	if (raw_date)
		*raw_date = current_time;
	return 0;
}

DWORD read_time_stomper_eeprom(HANDLE time_stomper, LPVOID eeprom_buffer, HANDLE console_to_print_progress)
{
	db_assert(time_stomper != INVALID_HANDLE_VALUE && eeprom_buffer);
	DWORD io_result;
	BYTE remote_access_buffer[3];
	for (SIZE_T read_offset = 0; read_offset != 0x10000; read_offset += 0x80)
	{
		remote_access_buffer[0] = 0x24;
		remote_access_buffer[1] = (BYTE)read_offset;
		remote_access_buffer[2] = (BYTE)(read_offset >> 8);
		for (SIZE_T remote_access_write = 0; remote_access_write != 3; ++remote_access_write)
			if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
				return get_last_error();
		if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
			return get_last_error();
		if (remote_access_buffer[0] != 0x4B)
			return ERROR_INVALID_HANDLE;
		for (SIZE_T remote_access_read = 0; remote_access_read != 0x80; ++remote_access_read)
			if (!ReadFile(time_stomper, (LPVOID)((UINT_PTR)eeprom_buffer + read_offset + remote_access_read), 1, &io_result, 0) || io_result != 1)
				return get_last_error();
		if (console_to_print_progress && console_to_print_progress != INVALID_HANDLE_VALUE && !((read_offset + 0x80) & 0x3FF))
			print_64KiB_progress(console_to_print_progress, read_offset + 0x80);
	}
	return 0;
}

DWORD write_time_stomper_eeprom(HANDLE time_stomper, LPCVOID eeprom_buffer, HANDLE console_to_print_progress)
{
	db_assert(time_stomper != INVALID_HANDLE_VALUE && eeprom_buffer);
	DWORD io_result;
	BYTE remote_access_buffer[3];
	for (SIZE_T write_offset = 0; write_offset != 0x10000; write_offset += 0x80)
	{
		remote_access_buffer[0] = 0x24;
		remote_access_buffer[1] = (BYTE)write_offset | 1;
		remote_access_buffer[2] = (BYTE)(write_offset >> 8);
		for (SIZE_T remote_access_write = 0; remote_access_write != 3; ++remote_access_write)
			if (!WriteFile(time_stomper, remote_access_buffer + remote_access_write, 1, &io_result, 0) || io_result != 1)
				return get_last_error();
		if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
			return get_last_error();
		if (remote_access_buffer[0] != 0x4B)
			return ERROR_INVALID_HANDLE;
		for (SIZE_T remote_access_write = 0; remote_access_write != 0x80; ++remote_access_write)
			if (!WriteFile(time_stomper, (LPCVOID)((UINT_PTR)eeprom_buffer + write_offset + remote_access_write), 1, &io_result, 0) || io_result != 1)
				return get_last_error();
		if (console_to_print_progress && console_to_print_progress != INVALID_HANDLE_VALUE && !((write_offset + 0x80) & 0x3FF))
			print_64KiB_progress(console_to_print_progress, write_offset + 0x80);
	}
	return 0;
}

DWORD erase_time_stomper_eeprom(HANDLE time_stomper, HANDLE console_to_print_progress)
{
	db_assert(time_stomper != INVALID_HANDLE_VALUE);
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
				return get_last_error();
		if (!ReadFile(time_stomper, remote_access_buffer, 1, &io_result, 0) || io_result != 1)
			return get_last_error();
		if (remote_access_buffer[0] != 0x4B)
			return ERROR_INVALID_HANDLE;
		for (SIZE_T remote_access_write = 0; remote_access_write != 0x80; ++remote_access_write)
			if (!WriteFile(time_stomper, (LPCVOID)&empty_byte, 1, &io_result, 0) || io_result != 1)
				return get_last_error();
		if (console_to_print_progress && console_to_print_progress != INVALID_HANDLE_VALUE && !((write_offset + 0x80) & 0x3FF))
			print_64KiB_progress(console_to_print_progress, write_offset + 0x80);
	}
	return 0;
}

DWORD print_time_stamp_information_to_file(const WCHAR* file_name, LPCVOID time_stamp_information)
{
	db_assert(file_name && time_stamp_information);
	DWORD error = 0;
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return get_last_error();
	BYTE* ouput_buffer = (BYTE*)HeapAlloc(heap, 0, 128);
	if (!ouput_buffer)
		return get_last_error();
	DWORD output_file_io_result;
	HANDLE output_file = CreateFileW(file_name, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
	if (output_file == INVALID_HANDLE_VALUE)
	{
		error = get_last_error();
		HeapFree(heap, 0, ouput_buffer);
		return error;
	}
	if (SetFilePointer(output_file, 0, 0, FILE_BEGIN) || !SetEndOfFile(output_file))
	{
		error = get_last_error();
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
			Date date = getDate((uint32_t)sample_start_time);
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
					error = get_last_error();
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
		error = get_last_error();
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
		ExitProcess((UINT)get_last_error());
	SIZE_T service_module_file_name_length = (SIZE_T)GetEnvironmentVariableW(L"APPDATA", 0, 0);
	if (!service_module_file_name_length)
		ExitProcess((UINT)get_last_error());
	service_module_file_name_length += 46;
	WCHAR* service_module_file_name = (WCHAR*)HeapAlloc(heap, 0, 2 * service_module_file_name_length * sizeof(WCHAR));
	if (!service_module_file_name)
		ExitProcess((UINT)get_last_error());
	if ((SIZE_T)GetEnvironmentVariableW(L"APPDATA", service_module_file_name, (DWORD)service_module_file_name_length) != service_module_file_name_length - 47)
	{
		error = get_last_error();
		HeapFree(heap, 0, service_module_file_name);
		ExitProcess((UINT)error);
	}
	for (WCHAR* d = service_module_file_name + service_module_file_name_length - 47, *s = (WCHAR*)L"\\Time Stomper\\time_synchronization_service.exe", *e = s + 47; s != e; ++s, ++d)
		*d = *s;
	WCHAR* current_module_file_name = service_module_file_name + service_module_file_name_length;
	HMODULE current_module;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const WCHAR*)time_synchronization_service, &current_module))
	{
		error = get_last_error();
		HeapFree(heap, 0, service_module_file_name);
		ExitProcess((UINT)error);
	}
	SIZE_T current_module_file_name_length = (SIZE_T)GetModuleFileNameW(current_module, current_module_file_name, (DWORD)service_module_file_name_length);
	if (!current_module_file_name_length || current_module_file_name_length > service_module_file_name_length)
	{
		error = get_last_error();
		HeapFree(heap, 0, service_module_file_name);
		ExitProcess((UINT)error);
	}
	++current_module_file_name_length;
	if (service_module_file_name_length != current_module_file_name_length || lstrcmpiW(service_module_file_name, current_module_file_name))
	{
		error = get_last_error();
		HeapFree(heap, 0, service_module_file_name);
		ExitProcess((UINT)error);
	}
	HeapFree(heap, 0, service_module_file_name);
	HMODULE User32 = LoadLibraryW(L"User32.dll");
	if (!User32)
		ExitProcess((UINT)get_last_error());
	int (WINAPI* MessageBoxW)(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) = (int (WINAPI*)(HWND, LPCTSTR, LPCTSTR, UINT))GetProcAddress(User32, "MessageBoxW");
	if (!MessageBoxW)
	{
		error = get_last_error();
		FreeLibrary(User32);
		ExitProcess((UINT)error);
	}
	DWORD serial_configuration_allocation_size = sizeof(COMMCONFIG);
	DWORD serial_configuration_size = serial_configuration_allocation_size;
	WCHAR* synchronized_message = (WCHAR*)HeapAlloc(heap, 0, (81 * sizeof(WCHAR)) + (SIZE_T)serial_configuration_allocation_size);
	if (!synchronized_message)
	{
		error = get_last_error();
		FreeLibrary(User32);
		ExitProcess((UINT)error);
	}
	for (WCHAR* read_empty_message = (WCHAR*)L"Time Stomper timer successfully synchronized to YYYY.MM.DD DOW HH:MM:SS Week WN.", *read_empty_message_end = read_empty_message + 81, * write_empty_message = synchronized_message; read_empty_message != read_empty_message_end; ++read_empty_message, ++write_empty_message)
		 *write_empty_message = *read_empty_message;
	COMMCONFIG* serial_configuration = (COMMCONFIG*)((UINT_PTR)synchronized_message + (81 * sizeof(WCHAR)));
	for (const WCHAR* serial_port_name = search_time_stomper_port(0, FALSE);; serial_port_name = search_time_stomper_port(0, FALSE))
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
						error = get_last_error();
						HeapFree(heap, 0, serial_configuration);
						FreeLibrary(User32);
						ExitProcess((UINT)error);
					}
					synchronized_message = (WCHAR*)synchronized_message;
					synchronized_message = (WCHAR*)((UINT_PTR)new_allocation + (81 * sizeof(WCHAR)));
				}
				else
				{
					error = get_last_error();
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
					else if (error == ERROR_FILE_NOT_FOUND)
						disable_time_synchronization_messages = 0;
					if (!disable_time_synchronization_messages)
					{
						print_full_format_date(raw_date, synchronized_message + 48);
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
	db_assert(is_installed != 0);
	HKEY run_registry_key;
	DWORD error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, 0, 0, KEY_QUERY_VALUE, 0, &run_registry_key, 0);
	if (error)
		return error;
	DWORD run_time_stomper_registry_value_type;
	error = (DWORD)RegQueryValueExW(run_registry_key, L"TimeStomper", 0, &run_time_stomper_registry_value_type, 0, 0);
	RegCloseKey(run_registry_key);
	if (error && error != ERROR_FILE_NOT_FOUND)
		return error;
	*is_installed = !error && run_time_stomper_registry_value_type == REG_SZ ? TRUE : FALSE;
	return 0;
}

DWORD stop_port_search()
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
		DWORD stop_port_search_value = 1;
		error = (DWORD)RegSetValueExW(time_stomper_registry_key, L"stop_port_search", 0, REG_DWORD, (BYTE*)&stop_port_search_value, sizeof(DWORD));
		RegCloseKey(time_stomper_registry_key);
	}
	return error;
}

DWORD continue_port_searh()
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
		DWORD stop_port_search_value = 0;
		error = (DWORD)RegSetValueExW(time_stomper_registry_key, L"stop_port_search", 0, REG_DWORD, (BYTE*)&stop_port_search_value, sizeof(DWORD));
		RegCloseKey(time_stomper_registry_key);
	}
	return error;
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
		return get_last_error();
	HMODULE current_module;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const WCHAR*)install_time_synchronization_service, &current_module))
		return get_last_error();
	SIZE_T current_module_file_name_length;
	WCHAR* current_module_file_name = (WCHAR*)HeapAlloc(heap, 0, (MAX_PATH + 1) * sizeof(WCHAR));
	if (!current_module_file_name)
		return get_last_error();
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
				error = get_last_error();
				HeapFree(heap, 0, current_module_file_name);
				return error;
			}
			current_module_file_name = new_current_module_file_name;
		}
		else
		{
			error = get_last_error();
			HeapFree(heap, 0, current_module_file_name);
			return error;
		}
	}
	SIZE_T app_data_directory_length = (SIZE_T)GetEnvironmentVariableW(L"APPDATA", 0, 0);
	if (!app_data_directory_length)
	{
		error = get_last_error();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	WCHAR* app_data_directory = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, ((current_module_file_name_length + 1) * sizeof(WCHAR)) + (app_data_directory_length * sizeof(WCHAR)) + ((app_data_directory_length + 13) * sizeof(WCHAR)) + ((app_data_directory_length + 46) * sizeof(WCHAR)) + ((app_data_directory_length + 79) * sizeof(WCHAR)));
	if (!app_data_directory)
	{
		error = get_last_error();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	current_module_file_name = app_data_directory;
	app_data_directory += (current_module_file_name_length + 1);
	if ((SIZE_T)GetEnvironmentVariableW(L"APPDATA", app_data_directory, (DWORD)app_data_directory_length) + 1 != app_data_directory_length)
	{
		error = get_last_error();
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
	error = CreateDirectoryW(ts_time_synchronization_service_directory, 0) ? 0 : get_last_error();
	if (error == ERROR_ALREADY_EXISTS)
		error = 0;
	if (!error)
	{
		if (CopyFileW(current_module_file_name, ts_time_synchronization_service_executable, FALSE))
		{
			HANDLE ts_time_synchronization_service_executable_handle = CreateFileW(ts_time_synchronization_service_executable, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
			if (ts_time_synchronization_service_executable_handle != INVALID_HANDLE_VALUE)
			{
				error = SetFilePointer(ts_time_synchronization_service_executable_handle, 0x3C, 0, FILE_BEGIN) == 0x3C ? 0 : get_last_error();
				DWORD image_nt_header_offset;
				for (DWORD file_read_result, file_read = 0; !error && file_read != 4;)
					if (ReadFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_nt_header_offset + file_read), 4 - file_read, &file_read_result, 0))
						file_read += file_read_result;
					else
						error = get_last_error();
				if (!error)
				{
					error = SetFilePointer(ts_time_synchronization_service_executable_handle, image_nt_header_offset, 0, FILE_BEGIN) == image_nt_header_offset ? 0 : get_last_error();
					DWORD image_nt_header_signature;
					for (DWORD file_read_result, file_read = 0; !error && file_read != 4;)
						if (ReadFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_nt_header_signature + file_read), 4 - file_read, &file_read_result, 0))
							file_read += file_read_result;
						else
							error = get_last_error();
					WORD image_file_header_machine_architecture;
					for (DWORD file_read_result, file_read = 0; !error && file_read != 2;)
						if (ReadFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_file_header_machine_architecture + file_read), 2 - file_read, &file_read_result, 0))
							file_read += file_read_result;
						else
							error = get_last_error();
					if (!error)
					{
						if (image_nt_header_signature == 0x00004550 && (image_file_header_machine_architecture == IMAGE_FILE_MACHINE_AMD64 || image_file_header_machine_architecture == IMAGE_FILE_MACHINE_I386))
						{
							error = SetFilePointer(ts_time_synchronization_service_executable_handle, image_nt_header_offset + ((image_file_header_machine_architecture == IMAGE_FILE_MACHINE_AMD64) ? 0x5C : 0x58), 0, FILE_BEGIN) == image_nt_header_offset + ((image_file_header_machine_architecture == IMAGE_FILE_MACHINE_AMD64) ? 0x5C : 0x58) ? 0 : get_last_error();
							const WORD image_optional_header_Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
							for (DWORD file_write_result, file_written = 0; !error && file_written != 2;)
								if (WriteFile(ts_time_synchronization_service_executable_handle, (LPVOID)((UINT_PTR)&image_optional_header_Subsystem + file_written), 2 - file_written, &file_write_result, 0))
									file_written += file_write_result;
								else
									error = get_last_error();
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
												error = get_last_error();
											RegDeleteValueW(run_registry_key, L"TimeStomper");
										}
										RegCloseKey(run_registry_key);
									}
								}
							}
							else
								error = get_last_error();
						}
						else
							error = ERROR_BAD_EXE_FORMAT;
					}
				}
				if (ts_time_synchronization_service_executable_handle != INVALID_HANDLE_VALUE)
					CloseHandle(ts_time_synchronization_service_executable_handle);
			}
			else
				error = get_last_error();
			DeleteFileW(ts_time_synchronization_service_executable);
		}
		else
			error = get_last_error();
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
		return get_last_error();
	HANDLE (WINAPI* CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID) = (HANDLE (WINAPI*)(DWORD, DWORD))GetProcAddress(Kernel32, "CreateToolhelp32Snapshot");
	if (!CreateToolhelp32Snapshot)
		return get_last_error();
	BOOL (WINAPI* Process32FirstW)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) = (BOOL (WINAPI*)(HANDLE, LPPROCESSENTRY32W))GetProcAddress(Kernel32, "Process32FirstW");
	if (!Process32FirstW)
		return get_last_error();
	BOOL (WINAPI* Process32NextW)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) = (BOOL (WINAPI*)(HANDLE, LPPROCESSENTRY32W))GetProcAddress(Kernel32, "Process32NextW");
	if (!Process32NextW)
		return get_last_error();
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return get_last_error();
	HMODULE current_module;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const WCHAR*)install_time_synchronization_service, &current_module))
		return get_last_error();
	SIZE_T current_module_file_name_length;
	WCHAR* current_module_file_name = (WCHAR*)HeapAlloc(heap, 0, (MAX_PATH + 1) * sizeof(WCHAR));
	if (!current_module_file_name)
		return get_last_error();
	for (SIZE_T current_module_file_name_capasity = MAX_PATH + 1; current_module_file_name_capasity;)
	{
		current_module_file_name_length = (SIZE_T)GetModuleFileNameW(current_module, current_module_file_name, (DWORD)current_module_file_name_capasity);
		if (current_module_file_name_length < current_module_file_name_capasity)
			current_module_file_name_capasity = 0;
		else if (current_module_file_name_length)
		{
			current_module_file_name_capasity += MAX_PATH;
			WCHAR* new_current_module_file_name = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, current_module_file_name_capasity * sizeof(WCHAR));
			if (!new_current_module_file_name)
			{
				error = get_last_error();
				HeapFree(heap, 0, current_module_file_name);
				return error;
			}
			current_module_file_name = new_current_module_file_name;
		}
		else
		{
			error = get_last_error();
			HeapFree(heap, 0, current_module_file_name);
			return error;
		}
	}
	SIZE_T app_data_directory_length = (SIZE_T)GetEnvironmentVariableW(L"APPDATA", 0, 0);
	if (!app_data_directory_length)
	{
		error = get_last_error();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	WCHAR* app_data_directory = (WCHAR*)HeapReAlloc(heap, 0, current_module_file_name, ((current_module_file_name_length + 1) * sizeof(WCHAR)) + (app_data_directory_length * sizeof(WCHAR)) + ((app_data_directory_length + 13) * sizeof(WCHAR)) + ((app_data_directory_length + 46) * sizeof(WCHAR)) + ((app_data_directory_length + 79) * sizeof(WCHAR)) + ((app_data_directory_length + 50) * sizeof(WCHAR)) + ((app_data_directory_length + 46) * sizeof(WCHAR)));
	if (!app_data_directory)
	{
		error = get_last_error();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	current_module_file_name = app_data_directory;
	app_data_directory += (current_module_file_name_length + 1);
	if ((SIZE_T)GetEnvironmentVariableW(L"APPDATA", app_data_directory, (DWORD)app_data_directory_length) + 1 != app_data_directory_length)
	{
		error = get_last_error();
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
	WCHAR* ts_time_synchronization_service_temporal_executable = ts_time_synchronization_service_command + (app_data_directory_length + 79);
	for (WCHAR* d = ts_time_synchronization_service_temporal_executable, *s = ts_time_synchronization_service_executable, *e = s + (app_data_directory_length + 45); s != e; ++s, ++d)
		*d = *s;
	for (WCHAR* d = ts_time_synchronization_service_temporal_executable + (app_data_directory_length + 45), *s = (WCHAR*)L".tmp", *e = s + 5; s != e; ++s, ++d)
		*d = *s;
	WCHAR* name_buffer = ts_time_synchronization_service_temporal_executable + (app_data_directory_length + 50);
	BOOL change_current_directory_up = FALSE;
	SIZE_T current_directory_length = (SIZE_T)GetCurrentDirectoryW((DWORD)(app_data_directory_length + 46), name_buffer);
	if (!current_directory_length)
	{
		error = get_last_error();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	else if (current_directory_length == (app_data_directory_length + 45) && !lstrcmpiW(name_buffer, ts_time_synchronization_service_directory))
	{
		if (!SetCurrentDirectoryW(app_data_directory))
		{
			error = get_last_error();
			HeapFree(heap, 0, current_module_file_name);
			return error;
		}
		change_current_directory_up = TRUE;
	}
	HANDLE process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!process_snapshot)
	{
		error = get_last_error();
		HeapFree(heap, 0, current_module_file_name);
		return error;
	}
	PROCESSENTRY32W process_entry;
	process_entry.dwSize = sizeof(PROCESSENTRY32W);
	for (BOOL loop = Process32FirstW(process_snapshot, &process_entry); loop; loop = Process32NextW(process_snapshot, &process_entry))
	{
		HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, process_entry.th32ProcessID);
		if (process_handle)
		{
			DWORD image_file_name_buffer_length = (DWORD)(app_data_directory_length + 46);
			if (QueryFullProcessImageNameW(process_handle, 0, name_buffer, &image_file_name_buffer_length))
			{
				if ((image_file_name_buffer_length == (DWORD)(app_data_directory_length + 45) && !lstrcmpiW(name_buffer, ts_time_synchronization_service_executable)) && (!TerminateProcess(process_handle, 0) || WAIT_OBJECT_0 != WaitForSingleObject(process_handle, INFINITE)))
				{
					error = get_last_error();
					CloseHandle(process_handle);
					CloseHandle(process_snapshot);
					HeapFree(heap, 0, current_module_file_name);
					return error;
				}
			}
			else
			{
				error = get_last_error();
				if (error != ERROR_INSUFFICIENT_BUFFER)
				{
					CloseHandle(process_handle);
					CloseHandle(process_snapshot);
					HeapFree(heap, 0, current_module_file_name);
					return error;
				}
				else
					error = 0;
			}
			CloseHandle(process_handle);
		}
	}
	CloseHandle(process_snapshot);
	if (GetFileAttributesW(ts_time_synchronization_service_temporal_executable) != INVALID_FILE_ATTRIBUTES)
		DeleteFileW(ts_time_synchronization_service_temporal_executable);
	if (MoveFileW(ts_time_synchronization_service_executable, ts_time_synchronization_service_temporal_executable))
	{
		HKEY run_registry_key;
		error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, 0, 0, KEY_SET_VALUE, 0, &run_registry_key, 0);
		if (!error)
		{
			error = (DWORD)RegDeleteValueW(run_registry_key, L"TimeStomper");
			RegCloseKey(run_registry_key);
			if (!error)
			{
				DeleteFileW(ts_time_synchronization_service_temporal_executable);
				if (!RemoveDirectoryW(ts_time_synchronization_service_directory))
				{
					HMODULE Shell32 = LoadLibraryW(L"Shell32.dll");
					if (Shell32)
					{
						int (WINAPI* SHFileOperationW)(LPSHFILEOPSTRUCT lpFileOp) = (int (WINAPI*)(LPSHFILEOPSTRUCT))GetProcAddress(Shell32, "SHFileOperationW");
						if (SHFileOperationW)
						{
							WCHAR* shell_file_operation_from = (WCHAR*)HeapAlloc(heap, 0, (app_data_directory_length + 14) * sizeof(WCHAR));
							if (shell_file_operation_from)
							{
								for (WCHAR* d = shell_file_operation_from, *s = ts_time_synchronization_service_directory, *e = s + (app_data_directory_length + 13); s != e; ++s, ++d)
									*d = *s;
								*(shell_file_operation_from + (app_data_directory_length + 13)) = 0;
								SHFILEOPSTRUCT shell_file_operation = { 0, FO_DELETE, shell_file_operation_from, 0, FOF_NO_UI, 0, 0, 0 };
								SHFileOperationW(&shell_file_operation);
								HeapFree(heap, 0, shell_file_operation_from);
							}
						}
						FreeLibrary(Shell32);
					}
				}
				RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Time Stomper");
				if (change_current_directory_up && GetFileAttributesW(ts_time_synchronization_service_directory) != INVALID_FILE_ATTRIBUTES)
					SetCurrentDirectoryW(ts_time_synchronization_service_directory);
				HeapFree(heap, 0, current_module_file_name);
				return 0;
			}
		}
		MoveFileW(ts_time_synchronization_service_temporal_executable, ts_time_synchronization_service_executable);
	}
	else
		error = get_last_error();
	STARTUPINFO time_synchronization_service_process_strtup_information = { sizeof(STARTUPINFO), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	PROCESS_INFORMATION time_synchronization_service_process_information = { 0, 0, 0, 0 };
	if (CreateProcessW(ts_time_synchronization_service_executable, ts_time_synchronization_service_command, 0, 0, 0, CREATE_DEFAULT_ERROR_MODE | CREATE_UNICODE_ENVIRONMENT, 0, ts_time_synchronization_service_directory, &time_synchronization_service_process_strtup_information, &time_synchronization_service_process_information))
	{
		CloseHandle(time_synchronization_service_process_information.hThread);
		CloseHandle(time_synchronization_service_process_information.hProcess);
	}
	if (change_current_directory_up && GetFileAttributesW(ts_time_synchronization_service_directory) != INVALID_FILE_ATTRIBUTES)
		SetCurrentDirectoryW(ts_time_synchronization_service_directory);
	HeapFree(heap, 0, current_module_file_name);
	return error;
}

DWORD show_current_date()
{
	WCHAR current_date[32];
	print_full_format_date(get_current_time(), current_date);
	current_date[31] = 0;
	HMODULE User32 = LoadLibraryW(L"User32.dll");
	if (!User32)
		return get_last_error();
	int (WINAPI* MessageBoxW)(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) = (int (WINAPI*)(HWND, LPCTSTR, LPCTSTR, UINT))GetProcAddress(User32, "MessageBoxW");
	if (!MessageBoxW)
	{
		DWORD error = get_last_error();
		FreeLibrary(User32);
		return error;
	}
	MessageBoxW(0, current_date, L"Current date", MB_OK | MB_ICONINFORMATION);
	FreeLibrary(User32);
	return 0;
}

void print_program_usage_info(HANDLE console_out)
{
	print(console_out, 
		L"Program description:\n"
		L"	This tool is used to install/uninstall Time Stomper time synchronization service, extract time stamps from Time Stomper device and debug the device.\n"
		L"	Give only one of following arguments -h, --install_time_synchronization_service, --uninstall_time_synchronization_service, -f, -r, -w, -t, -e and -d.\n"
		L"Parameter List:\n"
		L"	-h or --help Displays help message.\n"
		L"	--install_time_synchronization_service If this argument is given the program will install time synchronization service. The time synchronization service will automatically set the Time Stomper device's time when it is connected to the machine.\n"
		L"	--uninstall_time_synchronization_service If this argument is given the program will uninstall time synchronization service.\n"
		L"	-f or --find_port If this argument is given the program will search the port of Time Stomper device.\n"
		L"	-r or --read If this argument is given the program will read Time Stomper EEPROM to output file.\n"
		L"	-w or --write If this argument is given the program will write 64 KiB input file to Time Stomper EEPROM.\n"
		L"	-t or --test If this argument is given the program will test Time Stomper device EEPROM. This test will fill the EEPROM with random data.\n"
		L"	-e or --erase If this argument is given the program will erase all data from EEPROM.\n"
		L"	-d or --print_time_stamp_data If this argument is given the program will print time stamps to file from Time Stomper device EEPROM or raw time stamp file.\n"
		L"	-p or --port Specifies the Time Stomper port.\n"
		L"	-i or --input Specifies the input file.\n"
		L"	-o or --output Specifies the ouput file.\n"
	);
}

BOOL validate_arguments(SIZE_T argc, const WCHAR** argv, HANDLE console_out)
{
	BOOL read_operation = search_argument(argc, argv, L"-r", L"--read", 0);
	BOOL write_operation = search_argument(argc, argv, L"-w", L"--write", 0);
	BOOL test_operation = search_argument(argc, argv, L"-t", L"--test", 0);
	BOOL erase_operation = search_argument(argc, argv, L"-e", L"--erase", 0);
	BOOL print_time_stamp_data_operation = search_argument(argc, argv, L"-d", L"--print_time_stamp_data", 0);
	BOOL set_device_date_operation = search_argument(argc, argv, L"-s", L"--set_device_date", 0);
	BOOL get_device_date_operation = search_argument(argc, argv, L"-g", L"--get_device_date", 0);
	BOOL install_time_synchronization_service_operation = search_argument(argc, argv, 0, L"--install_time_synchronization_service", 0);
	BOOL uninstall_time_synchronization_service_operation = search_argument(argc, argv, 0, L"--uninstall_time_synchronization_service", 0);
	BOOL find_port_operation = search_argument(argc, argv, L"-f", L"--find_port", 0);
	BOOL help_operation = search_argument(argc, argv, L"-h", L"--help", 0);
	const WCHAR* com_port_name;
	search_argument(argc, argv, L"-p", L"--port", &com_port_name);
	const WCHAR* input_file_name;
	search_argument(argc, argv, L"-i", L"--input", &input_file_name);
	const WCHAR* output_file_name;
	search_argument(argc, argv, L"-o", L"--output", &output_file_name);
	SIZE_T operation_argument_count = (SIZE_T)((read_operation ? 1 : 0) + (write_operation ? 1 : 0) + (test_operation ? 1 : 0) + (erase_operation ? 1 : 0) + (print_time_stamp_data_operation ? 1 : 0) + (set_device_date_operation ? 1 : 0) + (get_device_date_operation ? 1 : 0) + (install_time_synchronization_service_operation ? 1 : 0) + (uninstall_time_synchronization_service_operation ? 1 : 0) + (find_port_operation ? 1 : 0) + (help_operation ? 1 : 0));
	if (!operation_argument_count)
	{
		if (console_out && console_out != INVALID_HANDLE_VALUE)
			print(console_out, L"No operation argument were given.\n");
		return FALSE;
	}
	else if (operation_argument_count != 1)
	{
		if (console_out && console_out != INVALID_HANDLE_VALUE)
			print(console_out, L"Multiple different operation arguments were given.\n");
		return FALSE;
	}
	else if (read_operation)
	{
		if (!com_port_name)
		{
			print(console_out, L"No COM port specified.\n");
			return FALSE;
		}
		else if (!output_file_name)
		{
			print(console_out, L"No output file specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (write_operation)
	{
		if (!com_port_name)
		{
			print(console_out, L"No COM port specified.\n");
			return FALSE;
		}
		else if (!input_file_name)
		{
			print(console_out, L"No input file specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (test_operation)
	{
		if (!com_port_name)
		{
			print(console_out, L"No COM port specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (erase_operation)
	{
		if (!com_port_name)
		{
			print(console_out, L"No COM port specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (print_time_stamp_data_operation)
	{
		if (!com_port_name && !input_file_name)
		{
			print(console_out, L"No COM port or input file specified.\n");
			return FALSE;
		}
		else if (!output_file_name)
		{
			print(console_out, L"No output file specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (set_device_date_operation)
	{
		if (!com_port_name)
		{
			print(console_out, L"No COM port specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (get_device_date_operation)
	{
		if (!com_port_name)
		{
			print(console_out, L"No COM port specified.\n");
			return FALSE;
		}
		else
			return TRUE;
	}
	else if (install_time_synchronization_service_operation || uninstall_time_synchronization_service_operation || find_port_operation || help_operation)
		return TRUE;
	else
		return FALSE;
}

#define OPERATION_READ 0x00000001
#define OPERATION_WRITE 0x00000002
#define OPERATION_TEST 0x00000003
#define OPERATION_ERASE 0x00000004
#define OPERATION_PRINT 0x00000005
#define OPERATION_SET_TIME 0x00000006
#define OPERATION_GET_TIME 0x00000007
#define OPERATION_INSTALL 0x00000008
#define OPERATION_UNINSTALL 0x00000009
#define OPERATION_FIND_PORT 0x0000000A
#define OPERATION_HELP 0x0000000B

DWORD main_process(HANDLE heap, SIZE_T argc, const WCHAR** argv)
{
	db_assert(heap && (!argc || (argc && argv)));
	if (search_argument(argc, argv, 0, L"--time_synchronization_service", 0))// do not start the program with argument
		time_synchronization_service();
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HANDLE console_out = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (console_out == INVALID_HANDLE_VALUE)
		return get_last_error();
	if (!validate_arguments(argc, argv, console_out))
	{
		print(console_out, L"The arguments passed to the program are invalid.\n");
		print_program_usage_info(console_out);
		CloseHandle(console_out);
		return ERROR_BAD_ARGUMENTS;
	}
	DWORD operation = search_argument(argc, argv, L"-r", L"--read", 0) ? OPERATION_READ : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-w", L"--write", 0) ? OPERATION_WRITE : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-t", L"--test", 0) ? OPERATION_TEST : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-e", L"--erase", 0) ? OPERATION_ERASE : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-d", L"--print_time_stamp_data", 0) ? OPERATION_PRINT : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-s", L"--set_device_date", 0) ? OPERATION_SET_TIME : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-g", L"--get_device_date", 0) ? OPERATION_GET_TIME : 0;
	if (!operation)
		operation = search_argument(argc, argv, 0, L"--install_time_synchronization_service", 0) ? OPERATION_INSTALL : 0;
	if (!operation)
		operation = search_argument(argc, argv, 0, L"--uninstall_time_synchronization_service", 0) ? OPERATION_UNINSTALL : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-f", L"--find_port", 0) ? OPERATION_FIND_PORT : 0;
	if (!operation)
		operation = search_argument(argc, argv, L"-h", L"--help", 0) ? OPERATION_HELP : 0;
	db_assert(operation);
	const WCHAR* com_port_name;
	search_argument(argc, argv, L"-p", L"--port", &com_port_name);
	const WCHAR* input_file_name;
	search_argument(argc, argv, L"-i", L"--input", &input_file_name);
	const WCHAR* output_file_name;
	search_argument(argc, argv, L"-o", L"--output", &output_file_name);
	if (operation == OPERATION_HELP)
	{
		print_program_usage_info(console_out);
		CloseHandle(console_out);
		return 0;
	}
	BOOL port_seach_stopped;
	error = is_time_synchronization_servece_installed(&port_seach_stopped);
	if (error)
	{
		print(console_out, L"Unable to determine installation state.\n");
		CloseHandle(console_out);
		return 0;
	}
	if (port_seach_stopped)
	{
		port_seach_stopped = !stop_port_search();
		if (port_seach_stopped)
			Sleep(0x400);
	}
	if (operation == OPERATION_FIND_PORT)
	{
		print(console_out, L"Searching for Time Stomper port...\n");
		const WCHAR* time_stomper_port = search_time_stomper_port(60000, TRUE);
		if (time_stomper_port)
		{
			print(console_out, L"Time Stomper found at port \"");
			if (time_stomper_port[0] == L'\\' && time_stomper_port[1] == L'\\' && time_stomper_port[2] == L'.' && time_stomper_port[3] == L'\\' && time_stomper_port[4] == L'C' && time_stomper_port[5] == L'O' && time_stomper_port[6] == L'M' && time_stomper_port[7])
				print(console_out, time_stomper_port + 4);
			else
				print(console_out, time_stomper_port);
			print(console_out, L"\".\n");
		}
		else
			print(console_out, L"Time Stomper port not found.\n");
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return 0;
	}
	if (operation == OPERATION_INSTALL)
	{
		error = install_time_synchronization_service();
		print(console_out, error ? L"Installing time synchronization service failed.\n" : L"Installing time synchronization service successful.\n");
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return error;
	}
	if (operation == OPERATION_UNINSTALL)
	{
		error = uninstall_time_synchronization_service();
		print(console_out, error ? L"Uninstalling time synchronization service failed.\n" : L"Uninstalling time synchronization service successful.\n");
		if (port_seach_stopped)
			continue_port_searh();
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
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return error;
	}
	if (operation == OPERATION_GET_TIME)
	{
		DWORD raw_date;
		error = get_time_stomper_date(time_stomper, &raw_date);
		if (error)
		{
			print(console_out, L"Reading device timer failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		WCHAR* output_buffer = (WCHAR*)HeapAlloc(heap, 0, 128 * sizeof(WCHAR));
		if (!output_buffer)
		{
			error = get_last_error();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		WCHAR* write_output = output_buffer;
		for (const WCHAR* i = L"Date in device "; *i; ++i)
			*write_output++ = *i;
		print_full_format_date(raw_date, write_output);
		write_output += 31;
		*write_output++ = L'\n';
		*write_output++ = 0;
		print(console_out, output_buffer);
		HeapFree(heap, 0, output_buffer);
		CloseHandle(time_stomper);
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return 0;
	}
	if (operation == OPERATION_SET_TIME)
	{
		DWORD raw_date;
		error = synchronize_time_stomper_date(time_stomper, &raw_date);
		if (error)
		{
			print(console_out, L"Device timer synchronization failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		WCHAR* output_buffer = (WCHAR*)HeapAlloc(heap, 0, 128 * sizeof(WCHAR));
		if (!output_buffer)
		{
			error = get_last_error();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		for (WCHAR* read_output = (WCHAR*)L"Device timer successfully synchronized to YYYY.MM.DD DOW HH:MM:SS Week WN.\n", *read_output_end = read_output + 76, *write_output = output_buffer; read_output != read_output_end; ++read_output, ++write_output)
			*write_output = *read_output;
		print_full_format_date(raw_date, output_buffer + 42);
		print(console_out, output_buffer);
		HeapFree(heap, 0, output_buffer);
		CloseHandle(time_stomper);
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return 0;
	}
	if (operation == OPERATION_ERASE)
	{
		print(console_out, L"Erasing EEPROM...\n");
		error = erase_time_stomper_eeprom(time_stomper, console_out);
		if (error)
		{
			print(console_out, L"Erasing EEPROM failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Erasing EEPROM successful\n");
		CloseHandle(time_stomper);
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return 0;
	}
	if (operation == OPERATION_PRINT)
	{
		BYTE* file_buffer;
		if (time_stomper != INVALID_HANDLE_VALUE)
		{
			file_buffer = (BYTE*)HeapAlloc(heap, 0, 0x10000);
			if (!file_buffer)
			{
				error = get_last_error();
				print(console_out, L"Memory allocation failed.\n");
				CloseHandle(time_stomper);
				if (port_seach_stopped)
					continue_port_searh();
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
				if (port_seach_stopped)
					continue_port_searh();
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
				if (error)
					print(console_out, L"Unable to load input file \"");
				else
				{
					HeapFree(heap, 0, file_buffer);
					error = ERROR_INVALID_DATA;
					print(console_out, L"Invalid input file \"");
				}
				print(console_out, input_file_name);
				print(console_out, L"\".\n");
				if (port_seach_stopped)
					continue_port_searh();
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
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Time stamp information written to \"");
		print(console_out, output_file_name);
		print(console_out, L"\".\n");
		if (port_seach_stopped)
			continue_port_searh();
		CloseHandle(console_out);
		return 0;
	}
	if (operation == OPERATION_WRITE)
	{
		LPVOID file_buffer;
		SIZE_T input_file_size;
		error = load_file(input_file_name, heap, &input_file_size, &file_buffer);
		if (error || input_file_size != 0x10000)
		{
			if (error)
				print(console_out, L"Unable to load input file \"");
			else
			{
				HeapFree(heap, 0, file_buffer);
				error = ERROR_INVALID_DATA;
				print(console_out, L"Invalid input file \"");
			}
			print(console_out, input_file_name);
			print(console_out, L"\".\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
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
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Writing EEPROM successful\n");
	}
	if (operation == OPERATION_READ)
	{
		LPVOID file_buffer = HeapAlloc(heap, 0, 0x10000);
		if (!file_buffer)
		{
			error = get_last_error();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
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
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Reading EEPROM successful.\n");
		error = save_file(output_file_name, 0x10000, file_buffer);
		HeapFree(heap, 0, file_buffer);
		if (error)
		{
			print(console_out, L"Error writing output file \"");
			print(console_out, output_file_name);
			print(console_out, L"\".\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"EEPROM data written to \"");
		print(console_out, output_file_name);
		print(console_out, L"\".\n");
	}
	if (operation == OPERATION_TEST)
	{
		BYTE* test_data = (BYTE*)HeapAlloc(heap, 0, 0x20000);
		if (!test_data)
		{
			error = get_last_error();
			print(console_out, L"Memory allocation failed.\n");
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		HMODULE Advapi32 = LoadLibraryW(L"Advapi32.dll");
		if (!Advapi32)
		{
			error = get_last_error();
			print(console_out, L"Loading library failed.\n");
			HeapFree(heap, 0, test_data);
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		BOOLEAN (WINAPI* SystemFunction036)(PVOID RandomBuffer, ULONG RandomBufferLength) = (BOOLEAN (WINAPI*)(PVOID, ULONG))GetProcAddress(Advapi32, "SystemFunction036");
		if (!SystemFunction036)
		{
			error = get_last_error();
			print(console_out, L"System function missing.\n");
			FreeLibrary(Advapi32);
			HeapFree(heap, 0, test_data);
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		if (!SystemFunction036(test_data, 0x10000))
		{
			error = get_last_error();
			print(console_out, L"Generating test data failed.\n");
			FreeLibrary(Advapi32);
			HeapFree(heap, 0, test_data);
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		FreeLibrary(Advapi32);
		BYTE* test_result_data = test_data + 0x10000;
		for (DWORD* i = (DWORD*)test_result_data, * e = i + 0x4000; i != e; ++i)
			*i = 0;
		print(console_out, L"Writing EEPROM...\n");
		error = write_time_stomper_eeprom(time_stomper, test_data, console_out);
		if (error)
		{
			print(console_out, L"Writing EEPROM failed.\n");
			HeapFree(heap, 0, test_data);
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Writing EEPROM successful\nReading EEPROM...\n");
		error = read_time_stomper_eeprom(time_stomper, test_result_data, console_out);
		if (error)
		{
			print(console_out, L"Reading EEPROM failed.\n");
			HeapFree(heap, 0, test_result_data);
			CloseHandle(time_stomper);
			if (port_seach_stopped)
				continue_port_searh();
			CloseHandle(console_out);
			return error;
		}
		print(console_out, L"Reading EEPROM successful.\n");
		for (DWORD* i = (DWORD*)test_result_data, * t = (DWORD*)test_result_data, *e = i + 0x4000; i != e; ++i, ++t)
			if (*i != *t)
			{
				print(console_out, L"EEPROM data corrupted.\n");
				HeapFree(heap, 0, test_result_data);
				CloseHandle(time_stomper);
				if (port_seach_stopped)
					continue_port_searh();
				CloseHandle(console_out);
				return 0;
			}
		HeapFree(heap, 0, test_result_data);
		print(console_out, L"EEPROM test OK.\n");
	}
	CloseHandle(time_stomper);
	if (port_seach_stopped)
		continue_port_searh();
	CloseHandle(console_out);
	return 0;
}

void entry_point()
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		ExitProcess((UINT)get_last_error());
	SIZE_T argc;
	const WCHAR** argv;
	DWORD error = get_arguments(heap, &argc, &argv);
	if (error)
		ExitProcess((UINT)error);
	SetLastError(0);
	error = main_process(heap, argc, argv);
	HeapFree(heap, 0, argv);
	ExitProcess((UINT)error);
}

#ifdef __cplusplus
}
#endif