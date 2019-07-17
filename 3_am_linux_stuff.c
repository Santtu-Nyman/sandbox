#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

int get_mapping_size(const void* mapping, size_t* size)
{
	size_t page_size = (size_t)sysconf(_SC_PAGE_SIZE);
	if (page_size == (size_t)-1)
		return errno;
	size_t buffer_allocation_granularity = (((size_t)0x10000 * sizeof(char)) + (page_size - 1)) & ~(page_size - 1);
	size_t buffer_size = buffer_allocation_granularity;
	char* buffer = (char*)mmap(0, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buffer == (char*)MAP_FAILED)
		return errno;
	int file_descriptor;
	size_t file_size;
	int error = EAGAIN;
	while (error == EAGAIN)
	{
		file_descriptor = -1;
		while (file_descriptor == -1)
		{
			file_descriptor = open("/proc/self/maps", O_RDONLY);
			if (file_descriptor == -1)
			{
				error = errno;
				if (error != EINTR)
				{
					munmap(buffer, buffer_size);
					return error;
				}
			}
		}
		file_size = 0;
		for (;;)
		{
			if (file_size == buffer_size)
			{
				close(file_descriptor);
				if (mmap((void*)((uintptr_t)buffer + buffer_size), buffer_allocation_granularity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == MAP_FAILED)
				{
					error = errno;
					munmap(buffer, buffer_size);
					return error;
				}
				buffer_size += buffer_allocation_granularity;
				error = EAGAIN;
				break;
			}
			ssize_t read_size = read(file_descriptor, (void*)((uintptr_t)buffer + file_size), ((buffer_size - file_size) < (size_t)0x40000000) ? (buffer_size - file_size) : (size_t)0x40000000);
			if (read_size == -1)
			{
				error = errno;
				if (error != EINTR)
				{
					close(file_descriptor);
					munmap(buffer, buffer_size);
					return error;
				}
				else
					read_size = 0;
			}
			else if (read_size)
				file_size += (size_t)read_size;
			else
			{
				error = 0;
				break;
			}
		}
	}
	close(file_descriptor);
	for (const char* entry_read = (const char*)buffer, * file_end = (const char*)((uintptr_t)buffer + file_size); entry_read != file_end;)
	{
		const char* entry_end = 0;
		uintptr_t mapping_begin = 0;
		uintptr_t mapping_end = 0;
		int valid_format = 1;
		char character;
		for (const char* file_read = entry_read, * file_end = (const char*)((uintptr_t)buffer + file_size);; ++file_read)
		{
			printf("%c", *file_read);
			if (file_read == file_end || *file_read == '\n')
			{
				entry_end = file_read;
				break;
			}
		}
		//printf("\n");
		if (entry_end == entry_read)
			valid_format = 0;
		else
		{
			character = *entry_read;
			if (!(character >= '0' && character <= '9') && !(character >= 'a' && character <= 'f'))
				valid_format = 0;
		}
		if (valid_format)
		{
			while (entry_read != entry_end && valid_format)
			{
				char character = *entry_read++;
				if (character >= '0' && character <= '9')
				{
					uintptr_t overflow_check = mapping_begin;
					mapping_begin <<= (uintptr_t)4;
					if (mapping_begin >> (uintptr_t)4 == overflow_check)
						mapping_begin |= (uintptr_t)(character - '0');
					else
						valid_format = 0;
				}
				else if (character >= 'a' && character <= 'f')
				{
					uintptr_t overflow_check = mapping_begin;
					mapping_begin <<= (uintptr_t)4;
					if (mapping_begin >> (uintptr_t)4 == overflow_check)
						mapping_begin |= (uintptr_t)(character - ('a' - 10));
					else
						valid_format = 0;
				}
				else if (character == '-')
					break;
				else
					valid_format = 0;
			}
		}
		if (valid_format)
		{
			if (!((uintptr_t)entry_end - (uintptr_t)entry_read))
				valid_format = 0;
			else
			{
				character = *entry_read;
				if (!(character >= '0' && character <= '9') && !(character >= 'a' && character <= 'f'))
					valid_format = 0;
			}
			while (entry_read != entry_end && valid_format)
			{
				char character = *entry_read++;
				if (character >= '0' && character <= '9')
				{
					uintptr_t overflow_check = mapping_end;
					mapping_end <<= (uintptr_t)4;
					if (mapping_end >> (uintptr_t)4 == overflow_check)
						mapping_end |= (uintptr_t)(character - '0');
					else
						valid_format = 0;
					
				}
				else if (character >= 'a' && character <= 'f')
				{
					uintptr_t overflow_check = mapping_end;
					mapping_end <<= (uintptr_t)4;
					if (mapping_end >> (uintptr_t)4 == overflow_check)
						mapping_end |= (uintptr_t)(character - ('a' - 10));
					else
						valid_format = 0;
				}
				else if (character == ' ')
					break;
				else
					valid_format = 0;
			}
		}

		printf("%i %p-%p\n", (uintptr_t)mapping_begin <= (uintptr_t)mapping && (uintptr_t)mapping_end > (uintptr_t)mapping, (void*)mapping_begin, (void*)mapping_end);

		if ((entry_end + 1) != file_end)
			entry_read = entry_end + 1;
		else
			entry_read = file_end;
	}
	munmap(buffer, buffer_size);
	*size = 0;
	return ENOENT;
}

int linux_process_arguments(size_t* argument_count, char*** arguments)
{
	const size_t allocation_granularity = 256 * sizeof(char);
	int error = 0;
	size_t buffer_size = allocation_granularity;
	char* buffer = malloc(buffer_size);
	if (!buffer)
		return ENOMEM;
	size_t file_size = 0;
	int file_descriptor = -1;
	while (file_descriptor == -1)
	{
		file_descriptor = open("/proc/self/cmdline", O_RDONLY);
		if (file_descriptor == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				free(buffer);
				return error;
			}
		}
	}
	for (;;)
	{
		if (file_size == buffer_size)
		{
			buffer_size += allocation_granularity;
			char* buffer_tmp = (char*)realloc(buffer, buffer_size);
			if (!buffer_tmp)
			{
				close(file_descriptor);
				free(buffer);
				return ENOMEM;
			}
		}
		ssize_t read_size = read(file_descriptor, (void*)((uintptr_t)buffer + file_size), ((buffer_size - file_size) < (size_t)0x40000000) ? (buffer_size - file_size) : (size_t)0x40000000);
		if (read_size == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				close(file_descriptor);
				free(buffer);
				return error;
			}
			else
				read_size = 0;
		}
		else if (read_size)
			file_size += (size_t)read_size;
		else
			break;
	}
	close(file_descriptor);
	size_t count = 0;
	for (const char* command_read = (const char*)buffer, * command_end = (const char*)((uintptr_t)buffer + file_size); command_read != command_end; ++command_read)
		if (!*command_read)
			++count;
	if (!count)
	{
		free(buffer);
		return EBADMSG;
	}
	if (buffer_size < (count * sizeof(char*)) + file_size)
	{
		buffer_size = (count * sizeof(char*)) + file_size;
		char* buffer_tmp = (char*)realloc(buffer, buffer_size);
		if (!buffer_tmp)
		{
			free(buffer);
			return ENOMEM;
		}
	}
	char** argument_table = (char**)buffer;
	char* argument_read = (char*)((uintptr_t)buffer + (count * sizeof(char*)));
	memmove(argument_read, buffer, count * sizeof(char*));
	for (size_t i = 0; i != count; ++i)
	{
		argument_table[i] = argument_read;
		while (*argument_read)
			++argument_read;
		++argument_read;
	}
	*argument_count = count;
	*arguments = argument_table;
	return 0;
}

int main(int argc, char** argv)
{
	size_t argument_count;
	char** argument_table;
	int error = linux_process_arguments(&argument_count, &argument_table);
		
	for (size_t i = 0; i != argument_count; ++i)
		printf("%i \"%s\"\n", error, argument_table[i]);

	/*
	int file_descriptor = open("/proc/self/maps", O_RDONLY);
	int error = 0;
	size_t file_size = 0;
	while (!error)
	{
		ssize_t read_size = read(file_descriptor, (void*)((uintptr_t)buffer + file_size), ((sizeof(buffer) - file_size) < (size_t)0x40000000) ? (sizeof(buffer) - file_size) : (size_t)0x40000000);
		if (read_size == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				close(file_descriptor);
				return error;
			}
			else
				read_size = 0;
		}
		else if (read_size)
			file_size += (size_t)read_size;
		else
			break;
	}


	for (const char* entry_read = (const char*)buffer, * file_end = (const char*)((uintptr_t)buffer + file_size); entry_read != file_end;)
	{
		const char* entry_end = 0;
		uintptr_t mapping_begin = 0;
		uintptr_t mapping_end = 0;
		int valid_format = 1;
		char character;
		for (const char* file_read = entry_read, * file_end = (const char*)((uintptr_t)buffer + file_size);; ++file_read)
			if (file_read == file_end || *file_read == '\n')
			{
				entry_end = file_read;
				break;
			}
		if (entry_end == entry_read)
			valid_format = 0;
		else
		{
			character = *entry_read;
			if (!(character >= '0' && character <= '9') && !(character >= 'a' && character <= 'f'))
				valid_format = 0;
		}
		if (valid_format)
		{
			while (entry_read != entry_end && valid_format)
			{
				char character = *entry_read++;
				if (character >= '0' && character <= '9')
				{
					uintptr_t overflow_check = mapping_begin;
					mapping_begin <<= (uintptr_t)4;
					if (mapping_begin >> (uintptr_t)4 == overflow_check)
						mapping_begin |= (uintptr_t)(character - '0');
					else
						valid_format = 0;
				}
				else if (character >= 'a' && character <= 'f')
				{
					uintptr_t overflow_check = mapping_begin;
					mapping_begin <<= (uintptr_t)4;
					if (mapping_begin >> (uintptr_t)4 == overflow_check)
						mapping_begin |= (uintptr_t)(character - ('a' - 10));
					else
						valid_format = 0;
				}
				else if (character == '-')
					break;
				else
					valid_format = 0;
			}
		}
		if (valid_format)
		{
			if (!((uintptr_t)entry_end - (uintptr_t)entry_read))
				valid_format = 0;
			else
			{
				character = *entry_read;
				if (!(character >= '0' && character <= '9') && !(character >= 'a' && character <= 'f'))
					valid_format = 0;
			}
			while (entry_read != entry_end && valid_format)
			{
				char character = *entry_read++;
				if (character >= '0' && character <= '9')
				{
					uintptr_t overflow_check = mapping_end;
					mapping_end <<= (uintptr_t)4;
					if (mapping_end >> (uintptr_t)4 == overflow_check)
						mapping_end |= (uintptr_t)(character - '0');
					else
						valid_format = 0;
					
				}
				else if (character >= 'a' && character <= 'f')
				{
					uintptr_t overflow_check = mapping_end;
					mapping_end <<= (uintptr_t)4;
					if (mapping_end >> (uintptr_t)4 == overflow_check)
						mapping_end |= (uintptr_t)(character - ('a' - 10));
					else
						valid_format = 0;
				}
				else if (character == ' ')
					break;
				else
					valid_format = 0;
			}
		}
		printf("%i %i %p-%p\n", 0, valid_format, (void*)mapping_begin, (void*)mapping_end);

		if ((entry_end + 1) != file_end)
			entry_read = entry_end + 1;
		else
			entry_read = file_end;
	}
	*/

	return 0;
}
