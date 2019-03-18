/*
	Cool Water Dispenser common version 0.0.3 2019-03-17 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <linux/futex.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cwd_common.h"

#define DEBUG_PRINT_SERVER_RESPONSE

void cwd_debug_print(int hex, size_t size, const void* buffer)
{
	if (hex)
	{
		for (const unsigned char* i = (const unsigned char*)buffer, * l = i + size; i != l; ++i)
			printf("%02X", *i);
		printf("\n");
	}
	else
	{
		printf("\"");
		for (const char* i = (const char*)buffer, * l = i + size; i != l; ++i)
			printf("%c", *i);
		printf("\"\n");
	}
}

void cwd_default_configuration(uint32_t device_id, struct cwd_device_configuration_t* configuration)
{
	configuration->device_id = device_id;
	configuration->device_operation_mode = 1;
	configuration->periodic_mesurement_delay = 120;
	configuration->target_water_temperature = 11;
}

int cwd_load_file(const char* name, size_t* size, void** data)
{
	int error;
	struct stat stats;
	int file = open(name, O_RDONLY);
	if (file == -1)
		return errno;
	if (fstat(file, &stats) == -1)
	{
		error = errno;
		close(file);
		return error;
	}
	if (sizeof(off_t) > sizeof(size_t) && stats.st_size > (off_t)SIZE_MAX)
	{
		error = EFBIG;
		close(file);
		return error;
	}
	size_t memory_size = (size_t)stats.st_size;
	uintptr_t buffer = (uintptr_t)malloc(memory_size);
	if (!buffer)
	{
		error = errno;
		close(file);
		return error;
	}
	for (size_t loaded = 0; loaded != memory_size;)
	{
		ssize_t read_result = read(file, (void*)(buffer + loaded), ((memory_size - loaded) < (size_t)SSIZE_MAX) ? (memory_size - loaded) : (size_t)SSIZE_MAX);
		if (read_result == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				close(file);
				return error;
			}
			read_result = 0;
		}
		loaded += (size_t)read_result;
	}
	close(file);
	*size = memory_size;
	*data = (void*)buffer;
	return 0;
}

int cwd_save_file(const char* name, size_t size, const void* data)
{
	int error;
	int file = open(name, O_WRONLY | O_TRUNC | O_CREAT);
	if (file == -1)
	{
		error = errno;
		return error;
	}
	for (size_t written = 0; written != size;)
	{
		ssize_t write_result = write(file, (const void*)((uintptr_t)data + written), ((size - written) < (size_t)SSIZE_MAX) ? (size - written) : (size_t)SSIZE_MAX);
		if (write_result == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				unlink(name);
				close(file);
				return error;
			}
			write_result = 0;
		}
		written += (size_t)write_result;
	}
	if (fsync(file) == -1)
	{
		error = errno;
		unlink(name);
		close(file);
		return error;
	}
	close(file);
	return 0;
}

size_t cwd_line_length(const char* string, size_t buffer_size)
{
	size_t length = 0;
	while (length != buffer_size && string[length] && string[length] != '\n' && string[length] != '\r')
		++length;
	return length;
}

char* cwd_search_for_argument(int argc, char** argv, const char* argument)
{
	if (!argc)
		return 0;
	for (int i = 0, l = argc - 1; i != l; ++i)
		if (!strcmp(argument, argv[i]))
			return argv[i + 1];
	return 0;
}

size_t cwd_print_u64(size_t buffer_length, char* buffer, uint64_t value)
{
	size_t lenght = 0;
	do
	{
		if (lenght == buffer_length)
			return 0;
		memmove(buffer + 1, buffer, lenght++);
		*buffer = '0' + (char)(value % 10);
		value /= 10;
	} while (value);
	return lenght;
}

size_t cwd_print_f32_n_dot3(size_t buffer_length, char* buffer, float value)
{
	int negative = value < 0.0f;
	uint32_t decimals = (uint32_t)(1000.f * (negative ? -value : value));
	size_t fraction_length = 3;
	uint32_t div = 1;
	size_t lenght = 0;
	uint32_t whole = decimals / 1000;
	while (fraction_length > 1 && !((decimals / div) % 10))
	{
		--fraction_length;
		div *= 10;
	}
	do
	{
		if (lenght == buffer_length)
			return 0;
		memmove(buffer + 1, buffer, lenght++);
		buffer[0] = '0' + (char)(whole % 10);
		whole /= 10;
	} while (whole);
	if (buffer_length - lenght < (size_t)negative + 1 + fraction_length)
		return 0;
	if (negative)
	{
		memmove(buffer + 1, buffer, lenght);
		buffer[0] = '-';
	}
	buffer[(size_t)negative + lenght] = '.';
	div = 100;
	for (size_t i = 0; i != fraction_length; ++i, div /= 10)
		buffer[(size_t)negative + lenght + 1 + i] = '0' + (char)((decimals / div) % 10);
	return (size_t)negative + lenght + (size_t)1 + fraction_length;
}

int cwd_get_processor_count(size_t* processor_count)
{
	int count = get_nprocs();
	if (count < SIZE_MAX)
		*processor_count = (size_t)count;
	else
		*processor_count = SIZE_MAX;
	return 0;
}

void cwd_acquire_futex(volatile int* futex)
{
	while (!__sync_bool_compare_and_swap(futex, 0, 1))
		syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

void cwd_release_futex(volatile int* futex)
{
	__sync_bool_compare_and_swap(futex, 1, 0);
	syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

int cwd_create_thread(int (*entry)(void*), void* parameter, size_t stack_size)
{
	// NOTE: Thread created with this function should not terminate before the process terminates beacause the stack is not freed before the virtual address space is removed.
	int error = 0;
	long page_size = sysconf(_SC_PAGESIZE);
	{
		error = errno;
		return error;
	}
	stack_size = (stack_size + ((size_t)page_size - 1)) & ~((size_t)page_size - 1); 
	void* stack = mmap(0, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED)
	{
		error = errno;
		return error;
	}
	int child_id = (int)clone(entry, (void*)((uintptr_t)stack + stack_size), CLONE_FILES | CLONE_FS | CLONE_SIGHAND | CLONE_THREAD | CLONE_VM, parameter, 0, 0, 0);
	if (child_id == -1)
	{
		error = errno; 
		munmap(stack, stack_size);
		return error;
	}
	return error;
}

int cwd_get_scheduling(int* policy, struct sched_param* parameters)
{
	int scheduling_policy = sched_getscheduler(0);
	if (scheduling_policy == -1)
		return errno;
	if (sched_getparam(0, parameters) == -1)
		return errno;
	*policy = scheduling_policy;
	return 0;
}

int cwd_set_scheduling(int policy, const struct sched_param* parameters)
{
	return sched_setscheduler(0, policy, parameters) == -1 ? errno : 0; 
}

int cwd_get_affinity(size_t* processor_count, int** processor_indices)
{
	cpu_set_t processor_set;
	CPU_ZERO(&processor_set);
	if (sched_getaffinity(0, sizeof(cpu_set_t), &processor_set) == -1)
		return errno;
	int* indices = (int*)malloc((size_t)CPU_SETSIZE * sizeof(int));
	if (!indices)
		return errno;
	size_t count = 0;
	for (int i = 0; i != (int)CPU_SETSIZE; i++)
		if (CPU_ISSET(i, &processor_set))
			indices[count++] = i;
	int* tmp = realloc(indices, count * sizeof(int));
	if (tmp)
		indices = tmp;
	*processor_count = count;
	*processor_indices = indices;
	return 0;
}

int cwd_set_affinity(size_t processor_count, int* processor_indices)
{
	cpu_set_t processor_set;
	CPU_ZERO(&processor_set);
	for (size_t i = 0; i != processor_count; ++i)
		if (processor_indices[i] < (int)CPU_SETSIZE)
			CPU_SET(processor_indices[i], &processor_set);
		else
			return EINVAL;
	if (sched_setaffinity (0, sizeof(cpu_set_t), &processor_set) == -1)
		return errno;
	return 0;
}

void cwd_wait_for_process(pid_t process_id)
{
	for (int status, wait_error = EINTR; wait_error == EINTR;)
		if (waitpid(process_id, &status, 0) == -1)
		{
			wait_error = errno;
			if (wait_error != EINTR)
				wait_error = 0;
		}
		else
			wait_error = 0;
}

int cwd_is_curl_installed_with_http(int* is_isntalled)
{
	const unsigned long long curl_execution_time_limit_ns = 16000000000;
	const size_t buffer_allocation_quantum = 0x10000;
	size_t buffer_size = buffer_allocation_quantum;
	int error;
	int pipe_out_in[2];
	char* buffer = (char*)malloc(buffer_size);
	if (!buffer)
		return ENOMEM;
	if (pipe(pipe_out_in) == -1)
	{
		error = errno;
		free(buffer);
		return error;
	}
	int pipe_read_access_mode = fcntl(pipe_out_in[0], F_GETFL);
	if (pipe_read_access_mode == -1 || fcntl(pipe_out_in[0], F_SETFL, pipe_read_access_mode | O_NONBLOCK) == -1)
	{
		error = errno;
		close(pipe_out_in[0]);
		close(pipe_out_in[1]);
		free(buffer);
		return error;
	}
	pid_t child_id = fork();
	if (child_id == -1)
	{
		error = errno;
		close(pipe_out_in[0]);
		close(pipe_out_in[1]);
		free(buffer);
		return error;
	}
	if (child_id)
	{
		close(pipe_out_in[1]);
		struct timespec time_begin;
		struct timespec time_now;
		clock_gettime(CLOCK_MONOTONIC, &time_begin);
		time_now.tv_sec = time_begin.tv_sec;
		time_now.tv_nsec = time_begin.tv_nsec;
		size_t output_size = 0;
		for (int wait_for_curl = 1; wait_for_curl && (((unsigned long long)(time_now.tv_sec - time_begin.tv_sec) * 1000000000) + (unsigned long long)time_now.tv_nsec) - (unsigned long long)time_begin.tv_nsec < curl_execution_time_limit_ns;)
		{
			if (output_size == buffer_size)
			{
				buffer_size += buffer_allocation_quantum;
				char* buffer_tmp = (char*)realloc(buffer, buffer_size);
				if (!buffer_tmp)
				{
					if (!kill(child_id, SIGTERM))
						kill(child_id, SIGKILL);
					cwd_wait_for_process(child_id);
					close(pipe_out_in[0]);
					free(buffer);
					return ENOMEM;
				}
				buffer = buffer_tmp;
			}
			ssize_t read_result = read(pipe_out_in[0], buffer + output_size, 1);
			if (read_result == -1)
			{
				int read_error = errno;
				if (read_error != EAGAIN && read_error != EINTR)
				{
					if (!kill(child_id, SIGTERM))
						kill(child_id, SIGKILL);
					cwd_wait_for_process(child_id);
					close(pipe_out_in[0]);
					free(buffer);
					return read_error;
				}
				if (read_error == EAGAIN)
					sleep(1);
			}
			else if (!read_result)
				wait_for_curl = 0;
			else
				++output_size;
			if (wait_for_curl)
				clock_gettime(CLOCK_MONOTONIC, &time_now);
		}
		close(pipe_out_in[0]);
		cwd_wait_for_process(child_id);
		char* curl_protocol_line = (char*)memmem(buffer, output_size, "\nProtocols:", 11);
		if (!curl_protocol_line)
		{
			free(buffer);
			*is_isntalled = 0;
			return 0;
		}
		curl_protocol_line += 11;
		size_t curl_protocol_line_length = cwd_line_length(curl_protocol_line, output_size - (size_t)((uintptr_t)curl_protocol_line - (uintptr_t)buffer));
		for (size_t i = 0; i != curl_protocol_line_length; ++i)
		{
			if (i && (curl_protocol_line[i - 1] == ' ' || curl_protocol_line[i - 1] == '	') && curl_protocol_line[i] != ' ' && curl_protocol_line[i] != '	' &&
			((i + 4 < curl_protocol_line_length && !memcmp(curl_protocol_line + i, "http ", 5)) || 
			(i + 4 < curl_protocol_line_length && !memcmp(curl_protocol_line + i, "http	", 5)) ||
			(i + 4 == curl_protocol_line_length && !memcmp(curl_protocol_line + i, "http", 4))))
			{
				free(buffer);
				*is_isntalled = 1;
				return 0;
			}
		}
		free(buffer);
		*is_isntalled = 0;
		return 0;
	}
	else
	{
		close(pipe_out_in[0]);
		free(buffer);
		close(STDOUT_FILENO);
		if (dup2(pipe_out_in[1], STDOUT_FILENO) == -1)
		{
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(pipe_out_in[1]);
		const char* curl_arguments[3] = { "curl", "-V", 0 };
		execvp(curl_arguments[0], (char**)curl_arguments);
		close(STDOUT_FILENO);
		exit(EXIT_FAILURE);
		return -1;
	}
}

int cwd_http_post(const char* url, size_t key_value_pair_count, const char** key_value_pairs, size_t* data_size, void** data)
{
	int error;
	size_t key_value_buffer_size = 0;
	char* key_value_buffer = 0;
	if (key_value_pair_count)
	{
		key_value_buffer_size = (key_value_pair_count * 2) * sizeof(char);
		for (size_t i = 0; i != key_value_pair_count; ++i)
			key_value_buffer_size += (strlen(key_value_pairs[i * 2]) + strlen(key_value_pairs[i * 2 + 1])) * sizeof(char);
		key_value_buffer = (char*)malloc(key_value_buffer_size);
		if (!key_value_buffer)
			return ENOMEM;
		char* key_value_pair_writer = key_value_buffer;
		for (size_t i = 0; i != key_value_pair_count; ++i)
		{
			size_t key_length = strlen(key_value_pairs[i * 2]);
			size_t value_length = strlen(key_value_pairs[i * 2 + 1]);
			memcpy(key_value_pair_writer, key_value_pairs[i * 2], key_length * sizeof(char));
			key_value_pair_writer[key_length] = '=';
			memcpy(key_value_pair_writer + key_length + 1, key_value_pairs[i * 2 + 1], value_length * sizeof(char));
			if (i + 1 != key_value_pair_count)
				key_value_pair_writer[key_length + value_length + 1] = '&';
			else
				key_value_pair_writer[key_length + value_length + 1] = 0;
			key_value_pair_writer += key_length + value_length + 2;
		}
	}
	else
	{
		key_value_buffer = (char*)calloc(1, sizeof(char));
		if (!key_value_buffer)
			return ENOMEM;
	}
	char http_data_file_name[20 + (2 * sizeof(pid_t))];
	memcpy(http_data_file_name, "/tmp/.cwd_http_", 15 * sizeof(char));
	pid_t thread_id = (pid_t)syscall(SYS_gettid);
	for (int i = 0; i != 2 * sizeof(pid_t); ++i)
		http_data_file_name[15 + i] = "0123456789ABCDEF"[(thread_id >> (pid_t)((sizeof(pid_t) * 8 - 4) - (4 * i))) & 0xF];
	memcpy(http_data_file_name + 15 + (2 * sizeof(pid_t)), ".dat", 5 * sizeof(char));
	pid_t child_id = fork();
	if (child_id == -1)
	{
		error = errno;
		free(key_value_buffer);
		return error;
	}
	if (child_id)
	{
		free(key_value_buffer);
		cwd_wait_for_process(child_id);
		error = cwd_load_file(http_data_file_name, data_size, data);
		unlink(http_data_file_name);
		return error;
	}
	else
	{
		const char* curl_arguments[10] = { "curl", "--data", key_value_buffer, "-X", "POST", url, "-o", http_data_file_name, "-s", 0 };
		execvp(curl_arguments[0], (char**)curl_arguments);
		free(key_value_buffer);
		exit(EXIT_FAILURE);
		return -1;
	}
}

int cwd_get_device_configuration(int api_version, const char* server, uint32_t device_id, struct cwd_device_configuration_t* configuration)
{
	if (api_version < 1)
		return EINVAL;
	if (api_version != 1)
		return ENOSYS;
	const char* url_protocol = CWD_URL_PROTOCOL;
	const char* url_server_path = CWD_URL_DEVICE_CONFIGURATION_PATH;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_server_path_length = strlen(url_server_path);
	size_t server_length = strlen(server);
	char device_id_string[11];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	device_id_string[cwd_print_u64(10, device_id_string, device_id)] = 0;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	const char* key_value_pairs[2] = { "id", device_id_string };
	size_t data_size;
	void* data;
	int error = cwd_http_post(url, 1, key_value_pairs, &data_size, &data);
	free(url);
	if (error == -1)
		error = ECOMM;
	if (error)
		return error;
	char* data_read = (char*)data;
	size_t data_length = 0;
	size_t data_comma_count = 0;
	while (data_length != data_size && data_read[data_length])
	{
		if (data_read[data_length] == ',')
			++data_comma_count;
		else if (data_read[data_length] < '0' || data_read[data_length] > '9')
		{
			free(data);
			return EILSEQ;
		}
		++data_length;
	}
	if (data_comma_count != 3)
	{
		free(data);
		return EILSEQ;
	}
	char* data_end = (char*)data + data_length;
	uint32_t configuration_device_id = 0;
	while (data_read != data_end && *data_read >= '0' && *data_read <= '9')
	{
		configuration_device_id = 10 * configuration_device_id + (uint32_t)(*data_read - '0');
		++data_read;
	}
	++data_read;
	uint32_t configuration_device_operation_mode = 0;
	while (data_read != data_end && *data_read >= '0' && *data_read <= '9')
	{
		configuration_device_operation_mode = 10 * configuration_device_operation_mode + (uint32_t)(*data_read - '0');
		++data_read;
	}
	++data_read;
	uint64_t configuration_periodic_mesurement_delay = 0;
	while (data_read != data_end && *data_read >= '0' && *data_read <= '9')
	{
		configuration_periodic_mesurement_delay = 10 * configuration_periodic_mesurement_delay + (uint64_t)(*data_read - '0');
		++data_read;
	}
	++data_read;
	float configuration_target_water_temperature = 0.0f;
	while (data_read != data_end && *data_read >= '0' && *data_read <= '9')
	{
		configuration_target_water_temperature = 10.0f * configuration_target_water_temperature + (float)(*data_read - '0');
		++data_read;
	}
	free(data);
	configuration->device_id = configuration_device_id;
	configuration->device_operation_mode = configuration_device_operation_mode;
	configuration->periodic_mesurement_delay = configuration_periodic_mesurement_delay;
	configuration->target_water_temperature = configuration_target_water_temperature;
#ifdef DEBUG_PRINT_SERVER_RESPONSE
	printf("cwd_get_device_configuration: device configuration id = %u mode = %u periodic mesurement = %u temperature = %f\n",
		(unsigned int)configuration_device_id, (unsigned int)configuration_device_operation_mode, (unsigned int)configuration_periodic_mesurement_delay, configuration_target_water_temperature );
#endif
	return 0;
}

int cwd_send_device_startup(int api_version, const char* server, uint32_t device_id, uint64_t timestamp)
{
	if (api_version < 1)
		return EINVAL;
	if (api_version != 1)
		return ENOSYS;
	const char* url_protocol = CWD_URL_PROTOCOL;
	const char* url_server_path = CWD_URL_DATA_INPUT_PATH;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_server_path_length = strlen(url_server_path);
	size_t server_length = strlen(server);
	char device_id_string[11];
	char time_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(10, device_id_string, device_id)] = 0;
	time_string[cwd_print_u64(20, time_string, timestamp)] = 0;
	const char* key_value_pairs[6] = { "id", device_id_string, "pt", time_string, "it", CWD_POST_STARTUP };
	size_t data_size;
	void* data;
	int error = cwd_http_post(url, 3, key_value_pairs, &data_size, &data);
	free(url);
	if (error == -1)
		error = ECOMM;
	if (error)
		return error;
#ifdef DEBUG_PRINT_SERVER_RESPONSE
	printf("cwd_send_device_startup: server responded ");
	cwd_debug_print(0, data_size, (const char*)data);
#endif
	free(data);
	return 0;
};

int cwd_send_periodic_mesurements(int api_version, const char* server, uint32_t device_id, uint64_t timestamp, float water_level, float water_temperature)
{
	if (api_version < 1)
		return EINVAL;
	if (api_version != 1)
		return ENOSYS;
	const char* url_protocol = CWD_URL_PROTOCOL;
	const char* url_server_path = CWD_URL_DATA_INPUT_PATH;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_server_path_length = strlen(url_server_path);
	size_t server_length = strlen(server);
	char device_id_string[11];
	char time_string[21];
	char water_string[21];
	char temperature_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(10, device_id_string, device_id)] = 0;
	time_string[cwd_print_u64(20, time_string, timestamp)] = 0;
	water_string[cwd_print_f32_n_dot3(20, water_string, water_level)] = 0;
	temperature_string[cwd_print_f32_n_dot3(20, temperature_string, water_temperature)] = 0;
	const char* key_value_pairs[10] = { "id", device_id_string, "pt", time_string, "it", CWD_POST_PERIODIC_MEASUREMENTS, "wl", water_string, "wt", temperature_string };
	size_t data_size;
	void* data;
	int error = cwd_http_post(url, 5, key_value_pairs, &data_size, &data);
	free(url);
	if (error == -1)
		error = ECOMM;
	if (error)
		return error;
#ifdef DEBUG_PRINT_SERVER_RESPONSE
	printf("cwd_send_periodic_mesurements: server responded ");
	cwd_debug_print(0, data_size, (const char*)data);
#endif
	free(data);
	return 0;
};

int cwd_send_bypassing(int api_version, const char* server, uint32_t device_id, uint64_t timestamp)
{
	if (api_version < 1)
		return EINVAL;
	if (api_version != 1)
		return ENOSYS;
	const char* url_protocol = CWD_URL_PROTOCOL;
	const char* url_server_path = CWD_URL_DATA_INPUT_PATH;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_server_path_length = strlen(url_server_path);
	size_t server_length = strlen(server);
	char device_id_string[11];
	char time_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(10, device_id_string, device_id)] = 0;
	time_string[cwd_print_u64(20, time_string, timestamp)] = 0;
	const char* key_value_pairs[6] = { "id", device_id_string, "pt", time_string, "it", CWD_POST_BYPASSING };
	size_t data_size;
	void* data;
	int error = cwd_http_post(url, 3, key_value_pairs, &data_size, &data);
	free(url);
	if (error == -1)
		error = ECOMM;
	if (error)
		return error;
#ifdef DEBUG_PRINT_SERVER_RESPONSE
	printf("cwd_send_bypassing: server responded ");
	cwd_debug_print(0, data_size, (const char*)data);
#endif
	free(data);
	return 0;
};

int cwd_send_order(int api_version, const char* server, uint32_t device_id, uint64_t timestamp, uint32_t order_type)
{
	if (api_version < 1)
		return EINVAL;
	if (api_version != 1)
		return ENOSYS;
	const char* url_protocol = CWD_URL_PROTOCOL;
	const char* url_server_path = CWD_URL_DATA_INPUT_PATH;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_server_path_length = strlen(url_server_path);
	size_t server_length = strlen(server);
	char device_id_string[11];
	char time_string[21];
	char order_string[11];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(10, device_id_string, device_id)] = 0;
	time_string[cwd_print_u64(20, time_string, timestamp)] = 0;
	order_string[cwd_print_u64(10, order_string, order_type)] = 0;
	const char* key_value_pairs[8] = { "id", device_id_string, "pt", time_string, "it", CWD_POST_ORDER, "ot", order_string };
	size_t data_size;
	void* data;
	int error = cwd_http_post(url, 4, key_value_pairs, &data_size, &data);
	free(url);
	if (error == -1)
		error = ECOMM;
	if (error)
		return error;
#ifdef DEBUG_PRINT_SERVER_RESPONSE
	printf("cwd_send_order: server responded ");
	cwd_debug_print(0, data_size, (const char*)data);
#endif
	free(data);
	return 0;
};

float cwd_calculate_thermistor_temperature(int measurement)
{
	const float series_resistor_resistance = 1005000.0f;
	float ptc_thermistor_resistance = (((float)measurement * 5.0f) / 1023.0f) / ((5.0f - (((float)measurement * 5.0f) / 1023.0f)) / series_resistor_resistance);
	return 0.0000579556f * ptc_thermistor_resistance + -125.622f;
}
