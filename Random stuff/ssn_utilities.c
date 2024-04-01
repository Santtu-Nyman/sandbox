#include <stddef.h>
#include <stdint.h>
#include <errno.h>


#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int ssn_is_empty_space(char character)
{
	return character == ' ' || character == '\t' || character == '\n' || character == '\r';
}

int ssn_is_decimal(char character)
{
	return (character >= '0' && character <= '9');
}

int ssn_is_hexadecimal(char character)
{
	return (character >= '0' && character <= '9') || (character >= 'A' && character <= 'F') || (character >= 'a' && character <= 'f');
}

int ssn_is_base36(char character)
{
	return (character >= '0' && character <= '9') || (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

int ssn_is_letter(char character)
{
	return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
}

int ssn_decode_integer(int* value, size_t buffer_size, const char* buffer)
{
	if (!buffer_size)
		return EINVAL;
	int negative = 0;
	if (*buffer == '-')
	{
		negative = 1;
		--buffer_size;
		++buffer;
	}
	if (*buffer == '+')
	{
		--buffer_size;
		++buffer;
	}
	if (!buffer_size)
		return EINVAL;
	unsigned int n = 0;
	for (char c = *buffer, * l = (char*)buffer + buffer_size; buffer != (const char*)l && c >= '0' && c <= '9'; ++buffer, c = *buffer)
	{
		if (n != ((n * (unsigned int)10) / (unsigned int)10))
			return ERANGE;
		n *= (unsigned int)10;
		unsigned int i = (unsigned int)(c - '0');
		if ((n + i < n) || (!negative && (unsigned int)INT_MAX < n + i) || (negative && ((unsigned int)INT_MAX + (unsigned int)1) < n + i))
			return ERANGE;
		n += i;
	}
	if (negative == 1)
		*value = (int)n;
	else
	{
		if (n == ((unsigned int)INT_MAX + (unsigned int)1))
			*value = INT_MIN;
		else
			*value = -(int)n;
	}
	return 0;
}

int ssn_encode_integer(int value, size_t buffer_size, char* buffer)
{
	int negative = value < 0;
	unsigned int n;
	if (negative)
	{
		if (value == INT_MIN)
			n = ((unsigned int)INT_MAX + (unsigned int)1);
		else
			n = (unsigned int)-value;
	}
	else
		n = (unsigned int)value;
	while (value)
	{

	}
}

int ssn_decode_float(float* value, size_t buffer_size, const char* buffer);

int ssn_encode_float(float value, size_t decimals, size_t buffer_size, char* buffer);

int ssn_load_file(const char* file_name, size_t* file_size, void** file_data)
{
	int error;
	FILE* file = fopen(file_name, "rb");
	if (!file)
	{
		error = errno;
		return error;
	}
	if (fseek(file, 0, SEEK_END))
	{
		error = errno;
		fclose(file);
		return error;
	}
	long end = ftell(file);
	if (end == EOF)
	{
		error = errno;
		fclose(file);
		return error;
	}
	if (fseek(file, 0, SEEK_SET))
	{
		error = errno;
		fclose(file);
		return error;
	}
	if ((sizeof(size_t) < sizeof(long)) && (end > (long)((size_t)~0)))
	{
		error = EFBIG;
		fclose(file);
		return error;
	}
	size_t size = (size_t)end;
	void* data = malloc(size);
	if (!data)
	{
		error = ENOMEM;
		fclose(file);
		return error;
	}
	for (size_t read = 0, result; read != size; read += result)
	{
		result = fread((void*)((uintptr_t)data + read), 1, size - read, file);
		if (!result)
		{
			error = ferror(file);
			free(data);
			fclose(file);
			return error;
		}
	}
	fclose(file);
	*file_size = size;
	*file_data = data;
	return 0;
}

int ssn_store_file(const char* file_name, size_t file_size, const void* file_data)
{
	int error;
	size_t file_name_length = strlen(file_name);
	char* temporal_file_name = (char*)malloc((file_name_length + 16) * sizeof(char));
	if (!temporal_file_name)
	{
		error = ENOMEM;
		return error;
	}
	memcpy(temporal_file_name, file_name, file_name_length);
	int temporal_file_name_index = 0;
	time_t current_time;
	struct tm* current_date = time(&current_time) != -1 ? localtime(&current_time) : 0;
	if (current_date)
	{
		temporal_file_name[file_name_length] = '.';
		temporal_file_name[file_name_length + 1] = '0' + (char)((current_date->tm_hour / 10) % 10);
		temporal_file_name[file_name_length + 2] = '0' + (char)(current_date->tm_hour % 10);
		temporal_file_name[file_name_length + 3] = '0' + (char)((current_date->tm_min / 10) % 10);
		temporal_file_name[file_name_length + 4] = '0' + (char)(current_date->tm_min % 10);
		temporal_file_name[file_name_length + 5] = '0' + (char)((current_date->tm_sec / 10) % 10);
		temporal_file_name[file_name_length + 6] = '0' + (char)(current_date->tm_sec % 10);
		memcpy(temporal_file_name + file_name_length + 7, "0000.tmp", 9);
	}
	else
		memcpy(temporal_file_name + file_name_length, ".0000000000.tmp", 16);
	FILE* file = 0;
	while (!file)
	{
		file = fopen(temporal_file_name, "rb");
		if (!file)
		{
			file = fopen(temporal_file_name, "wb");
			if (!file)
			{
				error = errno;
				free(temporal_file_name);
				return error;
			}
		}
		else
		{
			fclose(file);
			file = 0;
			if (temporal_file_name_index == 9999)
			{
				error = EEXIST;
				free(temporal_file_name);
				return error;
			}
			for (size_t i = 0, v = temporal_file_name_index++; i != 4; ++i, v /= 10)
				temporal_file_name[file_name_length + 7 + (3 - i)] = '0' + (char)(v % 10);
		}
	}
	for (size_t written = 0, write_result; written != file_size; written += write_result)
	{
		write_result = fwrite((const void*)((uintptr_t)file_data + written), 1, file_size - written, file);
		if (!write_result)
		{
			error = ferror(file);
			fclose(file);
			remove(temporal_file_name);
			free(temporal_file_name);
			return error;
		}
	}
	if (fflush(file))
	{
		error = ferror(file);
		fclose(file);
		remove(temporal_file_name);
		free(temporal_file_name);
		return error;
	}
	fclose(file);
	if (rename(temporal_file_name, file_name))
	{
		char* backup_file_name = (char*)malloc(file_name_length + 16);
		if (!backup_file_name)
		{
			remove(temporal_file_name);
			free(temporal_file_name);
			error = ENOMEM;
			return error;
		}
		memcpy(backup_file_name, temporal_file_name, file_name_length + 16);
		for (int backup_file_name_index = temporal_file_name_index + 1; backup_file_name_index < 10000; ++backup_file_name_index)
		{
			for (size_t i = 0, v = backup_file_name_index; i != 4; ++i, v /= 10)
				backup_file_name[file_name_length + 7 + (3 - i)] = '0' + (char)(v % 10);
			file = fopen(backup_file_name, "rb");
			if (!file)
			{
				if (rename(file_name, backup_file_name))
				{
					error = errno;
					remove(temporal_file_name);
					free(backup_file_name);
					free(temporal_file_name);
					return error;
				}
				if (rename(temporal_file_name, file_name))
				{
					error = errno;
					rename(backup_file_name, file_name);
					remove(temporal_file_name);
					free(backup_file_name);
					free(temporal_file_name);
					return error;
				}
				remove(backup_file_name);
				free(backup_file_name);
				free(temporal_file_name);
				return 0;
			}
			else
				fclose(file);
		}
		error = EEXIST;
		remove(temporal_file_name);
		free(backup_file_name);
		free(temporal_file_name);
		return error;
	}
	free(temporal_file_name);
	return 0;
}
