#include <Windows.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

static const uint64_t pcg32_multiplier = 6364136223846793005u;// Or something seed - dependent
static const uint64_t pcg32_increment = 1442695040888963407u;// Or an arbitrary odd constant

uint32_t pcg32(uint64_t* state)
{
	uint64_t x = *state;
	uint32_t count = (uint32_t)(x >> 59);
	*state = x * pcg32_multiplier + pcg32_increment;
	x ^= x >> 18;
	uint32_t tmp = (uint32_t)(x >> 27);
	return tmp >> count | tmp << ((0 - count) & 31);
}

void pcg32_init(uint64_t* state, uint64_t seed)
{
	*state = seed + pcg32_increment;
	pcg32(state);
}

char* create_password(uint32_t year, uint32_t month, uint32_t day, uint32_t count)
{
	static const char base36_table[36] = { 
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z' };
	static const char special_symbol_table[] = { '!', '?', '-', '+', '%', '#', ':', '=' };

	uint64_t pcg32_state;
	pcg32_init(&pcg32_state, ((year * 10000) + (month * 100) + day) ^ count);

	int length = 8 + (int)(pcg32(&pcg32_state) & 0x3);
	char* password = (char*)malloc(length + 1);
	__assume(password != 0);
	for (int i = 0; i != length; ++i)
	{
		if ((pcg32(&pcg32_state) & 0xF) != 0xF)
			password[i] = base36_table[pcg32(&pcg32_state) % 36];
		else
			password[i] = special_symbol_table[pcg32(&pcg32_state) & 0x7];
	}
	password[length] = 0;
	return password;
}

int main(int argc, char** argv)
{
	const WCHAR* registry_key_name = L"Software\\Santtu Nyman\\Glow in the dark";
	const WCHAR* registry_value_name = L"Password Count";
	if (argc >= 2 && !strcmp(argv[1], "--solve"))
	{
		if ((argc < 3) ||
			(strlen(argv[2]) != 15) ||
			(argv[2][0] < '0') || (argv[2][0] > '9') ||
			(argv[2][1] < '0') || (argv[2][1] > '9') ||
			(argv[2][2] < '0') || (argv[2][2] > '9') ||
			(argv[2][3] < '0') || (argv[2][3] > '9') ||
			(argv[2][4] != '-') ||
			(argv[2][5] < '0') || (argv[2][5] > '9') ||
			(argv[2][6] < '0') || (argv[2][6] > '9') ||
			(argv[2][7] != '-') ||
			(argv[2][8] < '0') || (argv[2][8] > '9') ||
			(argv[2][9] < '0') || (argv[2][9] > '9') ||
			(argv[2][10] != '+') ||
			(argv[2][11] < '0') || (argv[2][11] > '9') ||
			(argv[2][12] < '0') || (argv[2][12] > '9') ||
			(argv[2][13] < '0') || (argv[2][13] > '9') ||
			(argv[2][14] < '0') || (argv[2][14] > '9'))
			printf("Failed to solve password. The parameters were invalid\n");
		else
		{
			uint32_t year = ((uint32_t)(argv[2][0] - '0') * 1000) + ((uint32_t)(argv[2][1] - '0') * 100) + ((uint32_t)(argv[2][2] - '0') * 10) + (uint32_t)(argv[2][3] - '0');
			uint32_t month = ((uint32_t)(argv[2][5] - '0') * 10) + (uint32_t)(argv[2][6] - '0');
			uint32_t day = ((uint32_t)(argv[2][8] - '0') * 10) + (uint32_t)(argv[2][9] - '0');
			uint32_t count = ((uint32_t)(argv[2][11] - '0') * 1000) + ((uint32_t)(argv[2][12] - '0') * 100) + ((uint32_t)(argv[2][13] - '0') * 10) + (uint32_t)(argv[2][14] - '0');
			char* password = create_password(year, month, day, count);
			printf("password %s\n", password);
			free(password);
		}
	}
	else if (argc >= 2 && !strcmp(argv[1], "--reset"))
	{
		HKEY key_handle;
		DWORD error = (DWORD)RegOpenKeyExW(HKEY_CURRENT_USER, registry_key_name, 0, KEY_SET_VALUE, &key_handle);
		if (!error)
		{
			error = (DWORD)RegDeleteValueW(key_handle, registry_value_name);
			if (!error)
			{
				RegFlushKey(key_handle);
				printf("Reset successful.\n");
			}
			else
				printf("Failed to reset configuration. RegDeleteValueW error %08X\n", error);

			RegCloseKey(key_handle);
		}
		else
			printf("Failed to reset configuration. RegOpenKeyExW error %08X\n", error);
	}
	else
	{
		HKEY key_handle;
		DWORD error = (DWORD)RegCreateKeyExW(HKEY_CURRENT_USER, registry_key_name, 0, 0, REG_OPTION_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, 0, &key_handle, 0);
		if (!error)
		{
			DWORD registry_value_type = REG_NONE;
			DWORD registry_value_data = 0;
			DWORD registry_value_size = sizeof(DWORD);
			error = (DWORD)RegQueryValueExW(key_handle, registry_value_name, 0, &registry_value_type, (BYTE*)&registry_value_data, &registry_value_size);
			if (error == ERROR_FILE_NOT_FOUND)
			{
				registry_value_type = REG_DWORD;
				registry_value_data = 0;
				registry_value_size = sizeof(DWORD);
				error = 0;
			}

			if (!error)
			{
				if (registry_value_data > 9999)
				{
					printf("Warning. Current configuration data is invalid and will be deleted.");
					registry_value_data = 0;
				}

				SYSTEMTIME date;
				GetSystemTime(&date);
				char* password = create_password(date.wYear, date.wMonth, date.wDay, registry_value_data);
				printf("password %s\n", password);
				free(password);

				registry_value_data = (registry_value_data + 1) % 10000;
				error = (DWORD)RegSetValueExW(key_handle, registry_value_name, 0, REG_DWORD, (BYTE*)&registry_value_data, sizeof(DWORD));
				if (!error)
					RegFlushKey(key_handle);
				else
					printf("Warning. Saving configuration info failed. RegSetValueExW error %08X\n", error);
			}
			else
				printf("Failed to create password. RegQueryValueExW error %08X\n", error);

			RegCloseKey(key_handle);
		}
		else
			printf("Failed to create password. RegCreateKeyExW error %08X\n", error);

	}

	return 0;
}