#include <stdio.h>
#include <stdint.h>
#include <Windows.h>

#define lo8(x) ((x)&0xff) 
#define hi8(x) ((x)>>8)
uint16_t RHcrc_ccitt_update(uint16_t crc, uint8_t data)
{
	data ^= lo8(crc);
	data ^= data << 4;
	return ((((uint16_t)data << 8) | hi8(crc)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

uint16_t RHcrc_ccitt_str(const char* data)
{
	uint16_t rh_crc = 0xFFFF;
	while (*data)
		rh_crc = RHcrc_ccitt_update(rh_crc, *data++);
	return rh_crc ^ 0xFFFF;
}

uint16_t crc_ccitt_update(uint16_t crc, uint8_t data)
{
	const uint16_t xor_mask = (1 << (16 - 5)) | (1 << (16 - 12));
	for (int bit_counter = 8; bit_counter--; data >>= 1)
	{
		uint16_t new_msb = ((uint16_t)data ^ crc) & 1;
		uint16_t xor_input_mask = 0 - new_msb;
		crc = (new_msb << 15) | ((crc ^ (xor_input_mask & xor_mask)) >> 1);
	}
	return crc;
};

uint16_t my_ccitt_str(const char* data)
{
	uint16_t my_crc = 0xFFFF;
	while (*data)
		my_crc = crc_ccitt_update(my_crc, *data++);
	return my_crc ^ 0xFFFF;
}

uint8_t crc8dvb_s2(size_t size, const void* data)
{
	const uint8_t xor_mask = (1 << (7 - 1)) | (1 << (6 - 1)) | (1 << (4 - 1)) | (1 << (2 - 1));
	uint8_t crc = 0;
	for (const uint8_t* i = (const uint8_t*)data, *e = i + size; i != e; ++i)
	{
		uint8_t input_byte = *i;
		for (int bit_counter = 8; bit_counter--; input_byte <<= 1)
		{
			uint8_t new_lsb = (input_byte ^ crc) >> 7;
			uint8_t xor_enable_mask = 0 - new_lsb;
			crc = (((xor_enable_mask & xor_mask) ^ crc) << 1) | new_lsb;
		}
	}
	return crc;
}

uint8_t crc8sae_j1850(size_t size, const void* data)
{
	const uint8_t xor_mask = (1 << (4 - 1)) | (1 << (3 - 1)) | (1 << (2 - 1));
	uint8_t crc = 0xFF;
	for (const uint8_t* i = (const uint8_t*)data, *e = i + size; i != e; ++i)
	{
		uint8_t input_byte = *i;
		for (int bit_counter = 8; bit_counter--; input_byte <<= 1)
		{
			uint8_t new_lsb = (input_byte ^ crc) >> 7;
			uint8_t xor_enable_mask = 0 - new_lsb;
			crc = ((crc ^ (xor_enable_mask & xor_mask)) << 1) | new_lsb;
		}
	}
	return ~crc;
}

uint8_t crc8darc(size_t size, const void* data)
{
	const uint8_t xor_mask = 0x39;
	uint8_t crc = 0;
	for (const uint8_t* i = (const uint8_t*)data, *e = i + size; i != e; ++i)
	{
		uint8_t input_byte = *i;
		for (int bit_counter = 8; bit_counter--; input_byte >>= 1)
		{
			uint8_t new_msb = (input_byte ^ crc) & 1;
			uint8_t xor_enable_mask = 0 - new_msb;
			crc = (((xor_enable_mask & xor_mask) ^ crc) >> 1) | (new_msb << 7);
		}
	}
	return crc;
}

void ask_write_symbol(void* buffer, int bit_index, uint8_t symbol)
{
	buffer = (void*)((uintptr_t)buffer + (bit_index >> 3));
	uint8_t shift = (uint8_t)bit_index & 7;
	uint8_t mask = (1 << shift) - 1;
	*(uint8_t*)buffer = (*(uint8_t*)buffer & mask) | (symbol << shift);
	if (shift > 2)
	{
		shift = 8 - shift;
		mask = ~((1 << (6 - shift)) - 1);
		*(uint8_t*)((uintptr_t)buffer + 1) = (*(uint8_t*)((uintptr_t)buffer + 1) & mask) | (symbol >> shift);
	}
}

void rotate_and_copy_array(size_t length, const int* source, int* destination, size_t distance)
{
	for (size_t i = 0; i != length; ++i)
		destination[(i + distance) % length] = source[i];
}

int xcorrelation(size_t length, const int* a, const int* b)
{
	int c = 0;
	for (const int* e = a + length; a != e;)
		c -= ((*a++ ^ *b++) << 1) - 1;
	return c;
}

typedef struct interface_t
{
	HMODULE kernel32;
	FARPROC (WINAPI* GetProcAddress)(HMODULE hmodule, LPCSTR lpprocname);
	CHAR create_file_name[12];
	WCHAR console_out_name[8];
	CHAR write_console_name[14];
	WCHAR message[256];
} interface_t;

DWORD CALLBACK test(interface_t* test_interface)
{
	DWORD ignored;
	HANDLE (WINAPI* CreateFileW)(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = (HANDLE(WINAPI*)(LPCTSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE))test_interface->GetProcAddress(test_interface->kernel32, test_interface->create_file_name);
	HANDLE console_out = CreateFileW(test_interface->console_out_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	BOOL (WINAPI* WriteConsoleW)(HANDLE hConsoleOutput, CONST VOID* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved) = (BOOL (WINAPI*)(HANDLE, CONST VOID*, DWORD, LPDWORD, LPVOID))test_interface->GetProcAddress(test_interface->kernel32, test_interface->write_console_name);
	DWORD i = 0;
	while (test_interface->message[i])
		++i;
	WriteConsoleW(console_out, test_interface->message, i, &ignored, 0);
	return 0;
}

void print_function_code(const BYTE* f)
{
	while (!(f[0] == 0xCC && f[1] == 0xCC && f[2] == 0xCC && f[3] == 0xCC))
	{
		printf("0x%c%c, ", "0123456789ABCDEF"[*f >> 4], "0123456789ABCDEF"[*f & 0xF]);
		++f;
	}
}

const BYTE test_code[] = { 0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x40, 0x48, 0x8B, 0xD9, 0x48, 0x8D, 0x51, 0x10, 0x48, 0x8B, 0x09, 0xFF, 0x53, 0x08, 0x45, 0x33, 0xC9, 0x48, 0xC7, 0x44, 0x24, 0x30, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x4B, 0x1C, 0xC7, 0x44, 0x24, 0x28, 0x00, 0x00, 0x00, 0x00, 0xBA, 0x00, 0x00, 0x00, 0xC0, 0xC7, 0x44, 0x24, 0x20, 0x03, 0x00, 0x00, 0x00, 0x45, 0x8D, 0x41, 0x02, 0xFF, 0xD0, 0x48, 0x8B, 0x0B, 0x48, 0x8D, 0x53, 0x2C, 0x48, 0x8B, 0xF8, 0xFF, 0x53, 0x08, 0x45, 0x33, 0xC0, 0x48, 0x8D, 0x53, 0x3A, 0x66, 0x44, 0x39, 0x02, 0x74, 0x10, 0x0F, 0x1F, 0x40, 0x00, 0x41, 0xFF, 0xC0, 0x66, 0x42, 0x83, 0x7C, 0x43, 0x3A, 0x00, 0x75, 0xF4, 0x4C, 0x8D, 0x4C, 0x24, 0x50, 0x48, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xCF, 0xFF, 0xD0, 0x48, 0x8B, 0x5C, 0x24, 0x58, 0x33, 0xC0, 0x48, 0x83, 0xC4, 0x40, 0x5F, 0xC3 };

BOOL enable_security_now()
{
	PROCESS_MITIGATION_DYNAMIC_CODE_POLICY process_dynamic_code_policy = {};
	process_dynamic_code_policy.ProhibitDynamicCode = 1;
	BOOL process_dynamic_code_policy_ok = SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &process_dynamic_code_policy, sizeof(PROCESS_MITIGATION_DYNAMIC_CODE_POLICY));
	if (!process_dynamic_code_policy_ok)
	{
		if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessDynamicCodePolicy, &process_dynamic_code_policy, sizeof(PROCESS_MITIGATION_DYNAMIC_CODE_POLICY)))
		{
			if (process_dynamic_code_policy.ProhibitDynamicCode)
				process_dynamic_code_policy_ok = TRUE;
		}
	}
	PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY process_binary_signature_policy = {};
	process_binary_signature_policy.MicrosoftSignedOnly = 1;
	BOOL process_binary_signature_policy_ok = SetProcessMitigationPolicy(ProcessSignaturePolicy, &process_binary_signature_policy, sizeof(PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY));
	if (!process_binary_signature_policy_ok)
	{
		if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessSignaturePolicy, &process_binary_signature_policy, sizeof(PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY)))
		{
			if (process_binary_signature_policy.MicrosoftSignedOnly)
				process_binary_signature_policy_ok = TRUE;
		}
	}
	PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY process_extension_point_disable_policy = {};
	process_extension_point_disable_policy.DisableExtensionPoints = 1;
	BOOL process_extension_point_disable_policy_ok = SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy, &process_extension_point_disable_policy, sizeof(PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY));
	if (!process_extension_point_disable_policy_ok)
	{
		if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessExtensionPointDisablePolicy, &process_extension_point_disable_policy, sizeof(PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY)))
		{
			if (process_extension_point_disable_policy.DisableExtensionPoints)
				process_extension_point_disable_policy_ok = TRUE;
		}
	}
	PROCESS_MITIGATION_IMAGE_LOAD_POLICY process_image_load_policy = {};
	process_image_load_policy.NoRemoteImages = 1;
	process_image_load_policy.NoLowMandatoryLabelImages = 1;
	process_image_load_policy.PreferSystem32Images = 1;
	BOOL process_image_load_policy_ok = SetProcessMitigationPolicy(ProcessImageLoadPolicy, &process_image_load_policy, sizeof(PROCESS_MITIGATION_IMAGE_LOAD_POLICY));
	if (!process_image_load_policy_ok)
	{
		if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessImageLoadPolicy, &process_image_load_policy, sizeof(PROCESS_MITIGATION_IMAGE_LOAD_POLICY)))
		{
			if (process_image_load_policy.NoRemoteImages && process_image_load_policy.NoLowMandatoryLabelImages && process_image_load_policy.PreferSystem32Images)
				process_image_load_policy_ok = TRUE;
		}
	}
	PROCESS_MITIGATION_DEP_POLICY process_dep_policy = {};
	process_dep_policy.Enable = 1;
	process_dep_policy.Permanent = TRUE;
	BOOL process_dep_policy_ok = SetProcessMitigationPolicy(ProcessDEPPolicy, &process_dep_policy, sizeof(PROCESS_MITIGATION_DEP_POLICY));
	if (!process_dep_policy_ok)
	{
		if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessDEPPolicy, &process_dep_policy, sizeof(PROCESS_MITIGATION_DEP_POLICY)))
		{
			if (process_dep_policy.Enable && process_dep_policy.Permanent)
				process_dep_policy_ok = TRUE;
		}
	}
	return process_dynamic_code_policy_ok && process_binary_signature_policy_ok && process_extension_point_disable_policy_ok && process_image_load_policy_ok && process_dep_policy_ok;
}

int main()
{
	volatile BOOL security_ok = enable_security_now();




	SYSTEM_INFO system_info;
	GetNativeSystemInfo(&system_info);
	SIZE_T memory_size = (sizeof(test_code) + ((SIZE_T)system_info.dwPageSize - 1)) & ~((SIZE_T)system_info.dwPageSize - 1);

	HANDLE file = CreateFileW(L"C:\\Users\\Santtu Nyman\\Desktop\\test.bin", GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
	SetFilePointer(file, system_info.dwPageSize, 0, FILE_BEGIN);
	SetEndOfFile(file);
	DWORD file_written;
	SetFilePointer(file, 0, 0, FILE_BEGIN);
	WriteFile(file, test_code, sizeof(test_code), &file_written, 0);
	CloseHandle(file);

	file = CreateFileW(L"C:\\Users\\Santtu Nyman\\Desktop\\test.bin", GENERIC_EXECUTE, 0, 0, OPEN_EXISTING, 0, 0);
	HANDLE file_mapping = CreateFileMappingW(file, 0, PAGE_EXECUTE, (DWORD)(memory_size >> 32), (DWORD)memory_size, 0);
	LPVOID test_memory = MapViewOfFile(file_mapping, FILE_MAP_EXECUTE, 0, 0, memory_size);


	/*
	LPVOID test_memory = VirtualAlloc(0, (sizeof(test_code) + ((SIZE_T)system_info.dwPageSize - 1)) & ~((SIZE_T)system_info.dwPageSize - 1), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	memcpy(test_memory, test_code, sizeof(test_code));
	DWORD old_page_protection;
	VirtualProtect(test_memory, (sizeof(test_code) + ((SIZE_T)system_info.dwPageSize - 1)) & ~((SIZE_T)system_info.dwPageSize - 1), PAGE_EXECUTE, &old_page_protection);
	*/

	DWORD (CALLBACK* test_ptr)(interface_t* test_interface) = (DWORD(CALLBACK*)(interface_t*))test_memory;

	interface_t test_interface;
	test_interface.kernel32 = GetModuleHandleW(L"Kernel32.dll");
	test_interface.GetProcAddress = GetProcAddress;
	memcpy(test_interface.create_file_name, "CreateFileW", 12 * sizeof(CHAR));
	memcpy(test_interface.console_out_name, L"CONOUT$", 8 * sizeof(WCHAR));
	memcpy(test_interface.write_console_name, "WriteConsoleW", 14 * sizeof(CHAR));
	memcpy(test_interface.message, L"hello world\n", 13 * sizeof(WCHAR));

	test_ptr(&test_interface);

	//printf("%p\n", test);

	//print_function_code((const BYTE*)test);

	







	UnmapViewOfFile(test_memory);
	CloseHandle(file_mapping);
	CloseHandle(file);

	int a[24] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 };
	int b[24];
	int c[9];
	for (size_t i = 0; i != 9; ++i)
	{
		rotate_and_copy_array(24, a, b, i);
		c[i] = xcorrelation(24, a, b);
	}



	uint8_t crc = crc8darc(11, "hello world");




	const uint8_t test_data[7][9] = {
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0xF2, 0x01, 0x83 },
		{ 0x0F, 0xAA, 0x00, 0x55 },
		{ 0x00, 0xFF, 0x55, 0x11 },
		{ 0x33, 0x22, 0x55, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
		{ 0x92, 0x6B, 0x55 },
		{ 0xFF, 0xFF, 0xFF, 0xFF } };
	uint8_t test_results[7] = {
		crc8sae_j1850(4, test_data[0]),
		crc8sae_j1850(3, test_data[1]),
		crc8sae_j1850(4, test_data[2]),
		crc8sae_j1850(4, test_data[3]),
		crc8sae_j1850(9, test_data[4]),
		crc8sae_j1850(3, test_data[5]),
		crc8sae_j1850(4, test_data[6]),
	};


}