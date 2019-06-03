/*
	Cool Water Dispenser common version 1.0.8 2019-04-29 written by Santtu Nyman.
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
#include <dirent.h>
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
#ifdef CWD_BCM2835
#include "cwd_sbcm2835.h"
#include "bcm2835.h"
#include "cwd_mcp3201.h"
#endif

static volatile int device_initialization_lock;
static volatile int device_is_initialized;
static volatile int gpio_lock;
static volatile int last_tank_refill_time_lock;
static volatile int last_tank_refill_time_loaded;
static volatile uint64_t last_tank_refill_time;

int cwd_is_device_initialized()
{
	cwd_acquire_futex(&device_initialization_lock);
	int local_device_is_initialized = device_is_initialized;
	cwd_release_futex(&device_initialization_lock);
	return local_device_is_initialized;
}

int cwd_read_ds18b20(float* temperature)
{
#ifdef NO_DS18B20
	return ENOSYS;
#else
	int error;
	char buffer[0x80];
	char path[45];
	size_t size = 0;
	int file;
	DIR* directory;
	memcpy(path, "/sys/bus/w1/devices", 20 * sizeof(char));	
	directory = opendir(path);
	if (!directory)
	{
		error = errno;
		return error;
	}
	while (directory)
	{
		errno = 0;
		struct dirent* entry = readdir(directory);
		if (entry)
		{
			if (strlen(entry->d_name) == 15 && !memcmp(entry->d_name, "28-", 3 * sizeof(char)))
			{
				path[19] = '/';
				memcpy(path + 20, entry->d_name, 15 * sizeof(char));
				memcpy(path + 35, "/w1_slave", 10 * sizeof(char));
				closedir(directory);
				directory = 0;
			}
		}
		else
		{
			error = errno;
			if (!error)
				error = ENOENT;
			closedir(directory);
			return error;
		}
	}
	file = open(path, O_RDONLY);
	if (file == -1)
		return errno;
	for (ssize_t read_result = 1; size != 0x80 && read_result;)
	{ 
		read_result = read(file, buffer + size, 0x80 - size);
		if (read_result == -1)
		{
			error = errno;
			close(file);
			return error;
		}
		else if (read_result)
			size += (size_t)read_result;
	}
	close(file);
	if (size < 27)
		return EILSEQ;
	*temperature = (float)(((int)((buffer[0] <= '9') ? (buffer[0] - '0') : (buffer[0] - 'a' + 10)) << 4) |
		((int)((buffer[1] <= '9') ? (buffer[1] - '0') : (buffer[1] - 'a' + 10)) << 0) |
		((int)((buffer[4] <= '9') ? (buffer[4] - '0') : (buffer[4] - 'a' + 10)) << 8)) * 0.0625f;
	return 0;
#endif
}

int cwd_get_device_uid(uint64_t* uid)
{
	int error;
	const size_t buffer_allocation_granularity = 0x10000 * sizeof(char);
	size_t buffer_size = buffer_allocation_granularity;
	char* buffer = (char*)malloc(buffer_size);
	if (!buffer)
		return ENOMEM;
	size_t size = 0;
	int file = open("/proc/cpuinfo", O_RDONLY);
	if (file == -1)
	{
		error = errno;
		close(file);
		free(buffer);
		return error;
	}
	for (ssize_t read_result = 1; read_result;)
	{
		if (size == buffer_size)
		{
			if (buffer_size < buffer_size + buffer_allocation_granularity)
			{
				close(file);
				free(buffer);
				return EFBIG;
			}
			buffer_size += buffer_allocation_granularity;
			char* buffer_tmp = (char*)realloc(buffer, buffer_size);
			if (!buffer_tmp)
			{
				close(file);
				free(buffer);
				return ENOMEM;
			}
			buffer = buffer_tmp;
		}
		read_result = read(file, buffer + size, buffer_size - size);
		if (read_result == -1)
		{
			error = errno;
			close(file);
			free(buffer);
			return error;
		}
		else if (read_result)
			size += (size_t)read_result;
	}
	close(file);
	const char* serial_string = 0;
	for (const char* line = buffer; !serial_string && line; line = cwd_next_line(line, size - (size_t)((uintptr_t)line - (uintptr_t)buffer)))
	{
		size_t line_length = cwd_line_length(line, size - (size_t)((uintptr_t)line - (uintptr_t)buffer));
		if (line_length > 6 && (!memcmp("Serial\t", line, 7) || !memcmp("Serial ", line, 7)))
		{
			size_t read_line_index = 7;
			while (read_line_index != line_length && line[read_line_index] != ':')
				++read_line_index;
			if (line_length - read_line_index == 18 && (line[read_line_index + 1] == '\t' || line[read_line_index + 1] == ' ') && line[read_line_index + 18] == '\n')
			{
				int serial_ok = 1;
				for (size_t serial_digit_index = 0; serial_ok && serial_digit_index != 16; ++serial_digit_index)
					if (!(line[read_line_index + 2 + serial_digit_index] >= '0' && line[read_line_index + 2 + serial_digit_index] <= '9') && !(line[read_line_index + 2 + serial_digit_index] >= 'a' && line[read_line_index + 2 + serial_digit_index] <= 'f'))
						serial_ok = 0;
				if (serial_ok)
					serial_string = line + read_line_index + 2;
			}
		}
	}
	if (!serial_string)
	{
		free(buffer);
		return EILSEQ;
	}
	uint64_t processor_serial = 0;
	for (size_t i = 0; i != 16; ++i)
		processor_serial = (processor_serial << 4) | (uint64_t)((serial_string[i] <= '9') ? (serial_string[i] - '0') : (serial_string[i] - 'a' + 10));
	*uid = processor_serial;
	free(buffer);
	return 0;
}

int cwd_get_executable_file_name(char** executable_file_name)
{
	char* buffer = (char*)malloc((PATH_MAX + 1) * sizeof(char));
	if (!buffer)
		return ENOMEM;
	ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX);
	if (length == -1)
	{
		int readlink_error = errno;
		free(buffer);
		return readlink_error;
	}
	buffer[length] = 0;
	char* buffer_tmp = (char*)realloc(buffer, (length + 1) * sizeof(char));
	if (buffer_tmp)
		buffer = buffer_tmp;
	*executable_file_name = buffer;
	return 0;
}

int cwd_get_executable_directory_path(char** directory_path)
{
	char* buffer = (char*)malloc((PATH_MAX + 1) * sizeof(char));
	if (!buffer)
		return ENOMEM;
	ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX);
	if (length == -1)
	{
		int readlink_error = errno;
		free(buffer);
		return readlink_error;
	}
	if (!length)
	{
		free(buffer);
		return EILSEQ;
	}
	--length;
	while (length && buffer[length] != '/')
		--length;
	if (!length)
	{
		free(buffer);
		return EILSEQ;
	}
	buffer[length] = 0;
	char* buffer_tmp = (char*)realloc(buffer, (length + 1) * sizeof(char));
	if (buffer_tmp)
		buffer = buffer_tmp;
	*directory_path = buffer;
	return 0;
}

int cwd_get_last_tank_refill_time(uint64_t* timestamp)
{
	cwd_acquire_futex(&last_tank_refill_time_lock);
	if (!last_tank_refill_time_loaded)
	{
		size_t file_size;
		void* file_data;
		int error = cwd_load_file("last_tank_refill_time.dat", &file_size, &file_data);
		if (!error)
		{
			if (file_size != sizeof(uint64_t))
			{
				memcpy((void*)&last_tank_refill_time, file_data, sizeof(uint64_t));
				last_tank_refill_time_loaded = 1;
			}
			else
				error = EILSEQ;
			free(file_data);
		}
		if (error)
		{
			cwd_release_futex(&last_tank_refill_time_lock);
			return error;
		}
		last_tank_refill_time_loaded = 1;
	}
	*timestamp = last_tank_refill_time;
	cwd_release_futex(&last_tank_refill_time_lock);
	return 0;
}

int cwd_set_last_tank_refill_time(uint64_t timestamp)
{
	cwd_acquire_futex(&last_tank_refill_time_lock);
	last_tank_refill_time = timestamp;
	last_tank_refill_time_loaded = 1;
	cwd_save_file("last_tank_refill_time.dat", sizeof(uint64_t), (void*)&last_tank_refill_time);
	cwd_release_futex(&last_tank_refill_time_lock);
	return 0;
}

int cwd_read_thermistor_adc(int* value)
{
#ifdef NO_PRIN_TEMP
	return ENOSYS;
#else
#ifdef CWD_BCM2835
	cwd_acquire_futex(&gpio_lock);
	int error = cwd_sbcm2835_initialize();
	if (!error)
	{
		int m[9];
		for (int i = 0; i != (sizeof(m) / sizeof(int)); ++i)
		{
			m[i] = cwd_mcp3201_read(CWD_TEMPERATURE_SPI_CS_PIN, CWD_TEMPERATURE_SPI_CLK_PIN, CWD_TEMPERATURE_SPI_DOUT_PIN);
			const struct timespec delay_time = { 0, 100000 };
			nanosleep(&delay_time, 0);
		}
		cwd_sbcm2835_close();
		cwd_release_futex(&gpio_lock);
		for (int i = 1; i != (sizeof(m) / sizeof(int));)
			if (m[i - 1] > m[i])
			{
				int tmp = m[i];
				m[i] = m[i - 1];
				m[i - 1] = tmp;
				if (i != 1)
					--i;
			}
			else
				++i;
		*value = m[(sizeof(m) / sizeof(int)) / 2];
		return 0;
	}
	cwd_release_futex(&gpio_lock);
	return error;
#else
	return ENOSYS;
#endif
#endif
}

int cwd_read_temperature(int sensor, float* temperature)
{
	if (sensor == CWD_TEMPERATUTE_SENSOR_ANY)
	{
		int last_sensor_error = cwd_read_temperature(CWD_TEMPERATUTE_SENSOR_PRINTED, temperature);
		if (!last_sensor_error)
			return 0;
		last_sensor_error = cwd_read_temperature(CWD_TEMPERATUTE_SENSOR_DALLAS, temperature);
		if (!last_sensor_error)
			return 0;
		return last_sensor_error;
	}
	else if (sensor == CWD_TEMPERATUTE_SENSOR_PRINTED)
	{
		int adc_value;
		int error = cwd_read_thermistor_adc(&adc_value);
		if (!error)
			*temperature = cwd_calculate_thermistor_temperature(adc_value);
		return error;
	}
	else if (sensor == CWD_TEMPERATUTE_SENSOR_DALLAS)
	{
		return cwd_read_ds18b20(temperature);
	}
	else
		return ENOSYS;
}

int cwd_motor_enable(int enable)
{
#ifdef CWD_BCM2835
	if (enable)
		enable = 1;
	int error = cwd_sbcm2835_initialize();
	if (error)
		return error;
	cwd_acquire_futex(&gpio_lock);
	bcm2835_gpio_fsel(CWD_MOTOR_ENABLE_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_MOTOR_ENABLE_PIN, (uint8_t)enable);
	cwd_release_futex(&gpio_lock);
	cwd_sbcm2835_close();
	return 0;
#else
	return ENOSYS;
#endif
}

int cwd_pump_water(int direction, uint64_t time_ns)
{
#ifdef CWD_BCM2835
	if (!time_ns && !cwd_is_device_initialized())
		return EAGAIN; 
	if (!direction)
	{
		cwd_wait_ns(time_ns);
		return 0;
	}
	else if (direction < 0)
		direction = 1;
	else
		direction = 2;
	if (time_ns)
	{
		int error = cwd_sbcm2835_initialize();
		if (error)
			return error;
		cwd_acquire_futex(&gpio_lock);
	}
	bcm2835_gpio_fsel(CWD_MOTOR_A_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_MOTOR_A_PIN, (uint8_t)(direction & 1));
	bcm2835_gpio_fsel(CWD_MOTOR_B_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_MOTOR_B_PIN, (uint8_t)((direction >> 1) & 1));
	bcm2835_gpio_fsel(CWD_MOTOR_ENABLE_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_MOTOR_ENABLE_PIN, 1);
    if (!time_ns)
		return 0;
	cwd_wait_ns(time_ns);
	bcm2835_gpio_write(CWD_MOTOR_ENABLE_PIN, 0);
	bcm2835_gpio_write(CWD_MOTOR_A_PIN, 0);
	bcm2835_gpio_write(CWD_MOTOR_B_PIN, 0);
	cwd_release_futex(&gpio_lock);
	cwd_sbcm2835_close();
	return 0;
#else
	return ENOSYS;
#endif
}

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

int cwd_get_processor_indices(size_t* processor_count, int** processor_indices)
{
	static volatile int lock;
	static volatile size_t count;
	static volatile int* indices;
	int error = 0;
	cwd_acquire_futex(&lock);
	if (!count)
	{
		error = cwd_get_affinity((size_t*)&count, (int**)&indices);
		if (error)
		{
			count = 0;
			indices = 0;
		}
	}
	*processor_count = (size_t)count;
	*processor_indices = (int*)indices;
	cwd_release_futex(&lock);
	return error;
}

int cwd_default_configuration(struct cwd_device_configuration_t* configuration)
{
	configuration->version = CWD_VERSION_NXX;
	configuration->version_extension = CWD_VERSION_XNX;
	configuration->version_patch = CWD_VERSION_XXN;
#ifdef CWD_BCM2835
	configuration->full_bcm2835_build = 1;
#else
	configuration->full_bcm2835_build = 0;
#endif
#ifndef NO_PRIN_TEMP
	configuration->prinlab_ptc_support = configuration->full_bcm2835_build;
#else
	configuration->prinlab_ptc_support = 0;
#endif
#ifndef NO_PRIN_PRES
	configuration->printed_fsr_support = configuration->full_bcm2835_build;
#else
	configuration->printed_fsr_support = 0;
#endif
#ifndef NO_DS18B20
	configuration->ds18b20_support = 1;
#else
	configuration->ds18b20_support = 0;
#endif
	int curl_error = cwd_is_curl_installed_with_http(&configuration->is_curl_installed);
	if (curl_error)
		configuration->is_curl_installed = 0;
	int id_error = cwd_get_device_uid(&configuration->device_id);
	if (id_error)
		configuration->device_id = (uint64_t)~0;
	configuration->periodic_mesurement_delay = 10;
	configuration->loop_delay = 0;
	configuration->server = CWD_DEFAULT_SERVER;
	int dir_error = cwd_get_executable_directory_path(&configuration->directory);
	if (dir_error)
		configuration->directory = "";
	int pro_error = cwd_get_processor_indices(&configuration->processor_count, &configuration->processor_indices);
	if (!pro_error)
		configuration->ui_process_processor_index = configuration->processor_indices[configuration->processor_count - 1];
	else
	{
		configuration->processor_count = 0;
		configuration->processor_indices = 0;
		configuration->ui_process_processor_index = 0;
	}
	configuration->ui_process_executable = CWD_UI_EXECUTABLE;
	static const char* ui_process_arguments[] = CWD_UI_ARGUMENTS;
	configuration->ui_process_argument_count = sizeof(ui_process_arguments) / sizeof(char*);
	configuration->ui_process_arguments = (char**)ui_process_arguments;
	configuration->target_water_temperature = 10.0f;
	configuration->server_api_version = CWD_DEAFULT_SERVER_API;
	configuration->device_operation_mode = 1;
	configuration->use_extended_url = 0;
	configuration->disable_ui = 0;
	configuration->disable_12v_power = 0;
	configuration->disable_cooler = 0;
	configuration->print_measurements = 0;
	configuration->skip_insert = 0;
	configuration->print_server_responses = 0;
	configuration->log_measurements = 0;
	configuration->disable_capasitance_sensor = 0;
	configuration->disable_thermistor = 0;
	configuration->offline = 0;
	configuration->disable_temperature = 0;
	if (curl_error)
		return curl_error;
	if (id_error)
		return id_error;
	else if (dir_error)
		return dir_error;
	else if (pro_error)
		return pro_error;
	else
		return 0;
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

int cwd_internal_save_file(const char* name, int append, size_t size, const void* data)
{
	int error;
	int file = open(name, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
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

int cwd_save_file(const char* name, size_t size, const void* data)
{
	return cwd_internal_save_file(name, 0, size, data);
}

int cwd_append_to_file(const char* name, size_t size, const void* data)
{
	return cwd_internal_save_file(name, 1, size, data);
}

int cwd_append_string_to_file(const char* name, const char* string)
{
	return cwd_append_to_file(name, strlen(string), string);
}

int cwd_append_float_to_file(const char* name, float value)
{
	char value_string[17];
	value_string[cwd_print_f32_n_dot3(16, value_string, value)] = 0;
	return cwd_append_string_to_file(name, value_string);
}

void cwd_print_time(uint64_t timestamp, char* buffer)
{
	// Output of the function is allways 19 characters long.
	time_t time = (time_t)timestamp;
	struct tm date;
	localtime_r(&time, &date);
	buffer[0] = '0' + (char)(((date.tm_year + 1900) / 1000) % 10);
	buffer[1] = '0' + (char)(((date.tm_year + 1900) / 100) % 10);
	buffer[2] = '0' + (char)(((date.tm_year + 1900) / 10) % 10);
	buffer[3] = '0' + (char)((date.tm_year + 1900) % 10);
	buffer[4] = '-';
	buffer[5] = '0' + (char)(((date.tm_mon + 1) / 10) % 10);
	buffer[6] = '0' + (char)((date.tm_mon + 1) % 10);
	buffer[7] = '-';
	buffer[8] = '0' + (char)((date.tm_mday / 10) % 10);
	buffer[9] = '0' + (char)(date.tm_mday % 10);
	buffer[10] = ' ';
	buffer[11] = '0' + (char)((date.tm_hour / 10) % 10);
	buffer[12] = '0' + (char)(date.tm_hour % 10);
	buffer[13] = ':';
	buffer[14] = '0' + (char)((date.tm_min / 10) % 10);
	buffer[15] = '0' + (char)(date.tm_min % 10);
	buffer[16] = ':';
	buffer[17] = '0' + (char)((date.tm_sec / 10) % 10);
	buffer[18] = '0' + (char)(date.tm_sec % 10);
}

size_t cwd_line_length(const char* string, size_t buffer_size)
{
	size_t length = 0;
	while (length != buffer_size && string[length] && string[length] != '\n' && string[length] != '\r')
		++length;
	return length;
}

const char* cwd_next_line(const char* string, size_t buffer_size)
{
	const char* line_end = string + cwd_line_length(string, buffer_size);
	if (!*line_end || (*line_end != '\n' && *line_end != '\r') || (size_t)((uintptr_t)line_end - (uintptr_t)string) == buffer_size)
		return 0;
	return line_end + 1;
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


int cwd_decode_integer_argument(int argc, char** argv, const char* argument, int* negative_interger, uint64_t* interger_value)
{
	char* string = cwd_search_for_argument(argc, argv, argument);
	if (string)
	{
		uint64_t value = 0;
		int is_negative = 0;
		if (*string == '-')
		{
			is_negative = 1;
			++string;
		}
		else if (*string == '+')
			++string;
		if (!*string)
			return 0;
		while (*string)
			if (*string >= '0' && *string <= '9')
				value = (uint64_t)10 * value + (uint64_t)(*string++ - '0');
			else
				return 0;
		*negative_interger = is_negative;
		*interger_value = value;
		return 1;
	}
	else
		return 0;
}

int cwd_check_flag_argument(int argc, char** argv, const char* argument)
{
	for (int i = 0; i != argc; ++i)
		if (!strcmp(argument, argv[i]))
			return 1;
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
		const char* curl_arguments[12] = { "curl", "--data", key_value_buffer, "-X", "PUT", url, "-o", http_data_file_name, "-m", "10", "-s", 0 };
		execvp(curl_arguments[0], (char**)curl_arguments);
		free(key_value_buffer);
		exit(EXIT_FAILURE);
		return -1;
	}
}

int cwd_get_device_configuration(int api_version, const char* server, uint64_t device_id, struct cwd_device_configuration_t* configuration, int use_extended_url, int print_response)
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
	char device_id_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	device_id_string[cwd_print_u64(20, device_id_string, device_id)] = 0;
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
	uint64_t configuration_device_id = 0;
	while (data_read != data_end && *data_read >= '0' && *data_read <= '9')
	{
		configuration_device_id = 10 * configuration_device_id + (uint64_t)(*data_read - '0');
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
	configuration->server_api_version = 1;
	configuration->device_id = configuration_device_id;
	configuration->device_operation_mode = (int)configuration_device_operation_mode;
	configuration->periodic_mesurement_delay = configuration_periodic_mesurement_delay;
	configuration->target_water_temperature = configuration_target_water_temperature;
	if (print_response)
	{
		printf("cwd_get_device_configuration: device configuration id = %u mode = %u periodic mesurement = %u temperature = %f\n",
			(unsigned int)configuration_device_id, (unsigned int)configuration_device_operation_mode, (unsigned int)configuration_periodic_mesurement_delay, configuration_target_water_temperature );
	}
	return 0;
}

int cwd_send_device_startup(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, int use_extended_url, int print_response)
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
	char device_id_string[21];
	char time_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(20, device_id_string, device_id)] = 0;
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
	if (print_response)
	{
		printf("cwd_send_device_startup: server responded ");
		cwd_debug_print(0, data_size, (const char*)data);
	}
	free(data);
	return 0;
};

int cwd_send_periodic_mesurements(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, float water_level, float water_temperature, uint64_t water_refill_timestamp, int use_extended_url, int print_response)
{
	if (api_version < 1)
		return EINVAL;
	if (api_version != 1 && api_version != 2)
		return ENOSYS;
	if (api_version == 2)
		return cwd_send_periodic_mesurements2(server, device_id, timestamp, water_level, water_temperature, water_refill_timestamp, use_extended_url, print_response);
	const char* url_protocol = CWD_URL_PROTOCOL;
	const char* url_server_path = CWD_URL_DATA_INPUT_PATH;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_server_path_length = strlen(url_server_path);
	size_t server_length = strlen(server);
	char device_id_string[21];
	char time_string[21];
	char water_string[21];
	char temperature_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(20, device_id_string, device_id)] = 0;
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
	if (print_response)
	{
		printf("cwd_send_periodic_mesurements: server responded ");
		cwd_debug_print(0, data_size, (const char*)data);
	}
	free(data);
	return 0;
};

int cwd_send_bypassing(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, int use_extended_url, int print_response)
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
	char device_id_string[21];
	char time_string[21];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(20, device_id_string, device_id)] = 0;
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
	if (print_response)
	{
		printf("cwd_send_bypassing: server responded ");
		cwd_debug_print(0, data_size, (const char*)data);
	}
	free(data);
	return 0;
};

int cwd_send_order(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, uint32_t order_type, int use_extended_url, int print_response)
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
	char device_id_string[21];
	char time_string[21];
	char order_string[11];
	char* url = (char*)malloc(url_protocol_length + server_length + url_server_path_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_server_path, url_server_path_length + 1);
	device_id_string[cwd_print_u64(20, device_id_string, device_id)] = 0;
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
	if (print_response)
	{
		printf("cwd_send_order: server responded ");
		cwd_debug_print(0, data_size, (const char*)data);
	}
	free(data);
	return 0;
};

int cwd_send_periodic_mesurements2(const char* server, uint64_t device_id, uint64_t timestamp, float water_level, float water_temperature, uint64_t water_refill_timestamp, int use_extended_url, int print_response)
{
	const char* url_protocol = CWD_URL_PROTOCOL_2;
	const char* url_path = use_extended_url ? CWD_URL_EXTENDE_DATA_PATH_2 : CWD_URL_DATA_PATH_2;
	size_t url_protocol_length = strlen(url_protocol);
	size_t url_path_length = strlen(url_path);
	size_t server_length = strlen(server);
	char device_id_string[21];
	char date_string[20];
	char temperature_string[17];
	char refill_time_string[20];
	char water_level_string[17];
	size_t device_id_length = cwd_print_u64(20, device_id_string, device_id);
	device_id_string[device_id_length] = 0;
	cwd_print_time(timestamp, date_string);
	date_string[19] = 0;
	temperature_string[cwd_print_f32_n_dot3(16, temperature_string, water_temperature)] = 0;
	cwd_print_time(water_refill_timestamp, refill_time_string);
	refill_time_string[19] = 0;
	if (water_level < 0.0f)
		water_level = 0.0f;
	else if (water_level > 100.0f)
		water_level = 100.0f;
	water_level_string[cwd_print_f32_n_dot3(16, water_level_string, water_level / 100.0f)] = 0;
	char* url = (char*)malloc(url_protocol_length + server_length + url_path_length + 1 + device_id_length + 1);
	if (!url)
		return ENOMEM;
	memcpy(url, url_protocol, url_protocol_length);
	memcpy(url + url_protocol_length, server, server_length);
	memcpy(url + url_protocol_length + server_length, url_path, url_path_length);
	url[url_protocol_length + server_length + url_path_length] = '/';
	memcpy(url + url_protocol_length + server_length + url_path_length + 1, device_id_string, device_id_length + 1);
	const char* key_value_pairs[14] = {
		"serialNumber", device_id_string,
		"mode", "on",
		"time", date_string,
		"waterlevel", water_level_string,
		"temperature", temperature_string,
		"lastChangedTime", refill_time_string,
		"area_id", CWD_AREA_ID };
	size_t data_size;
	void* data;
	cwd_save_file("cwd_last_mesuremt_url.txt", strlen(url), url);
	int error = cwd_http_post(url, 7, key_value_pairs, &data_size, &data);
	free(url);
	if (error == -1)
		error = ECOMM;
	if (error)
		return error;
	if (print_response)
	{
		printf("cwd_send_periodic_mesurements2: server responded ");
		cwd_debug_print(0, data_size, (const char*)data);
	}
	free(data);
	return 0;
}

int cwd_read_pressure_sensor(int* value)
{
#ifdef NO_PRIN_PRES
	*value = 0;
	return ENOSYS;
#else
#ifdef CWD_BCM2835
	cwd_acquire_futex(&gpio_lock);
	int error = cwd_sbcm2835_initialize();
	if (!error)
	{
		int m[9];
		for (int i = 0; i != (sizeof(m) / sizeof(int)); ++i)
		{
			m[i] = cwd_mcp3201_read(CWD_PERSSURE_SPI_CS_PIN, CWD_PERSSURE_SPI_CLK_PIN, CWD_PERSSURE_SPI_DOUT_PIN);
			const struct timespec delay_time = { 0, 100000 };
			nanosleep(&delay_time, 0);
		}
		cwd_sbcm2835_close();
		cwd_release_futex(&gpio_lock);
		for (int i = 1; i != (sizeof(m) / sizeof(int));)
			if (m[i - 1] > m[i])
			{
				int tmp = m[i];
				m[i] = m[i - 1];
				m[i - 1] = tmp;
				if (i != 1)
					--i;
			}
			else
				++i;
		*value = m[(sizeof(m) / sizeof(int)) / 2];
		return 0;
	}
	cwd_release_futex(&gpio_lock);
	return error;
#else
	*value = 0;
	return ENOSYS;
#endif
#endif
}

float cwd_calculate_water_level_from_pressure_value(int value)
{
	float x = (float)value;
	return (x - 528.0f) / 961.0f;
}

int cwd_get_tank_water_level(float* water_level)
{
	const float capasitance_pressure_split = 1.0f;
	float from_capasitance;
	int from_capasitance_error = cwd_measure_water_level_from_tank_capasitance(&from_capasitance);
	float from_pressure;
	int from_pressure_raw;
	int from_pressure_error = cwd_read_pressure_sensor(&from_pressure_raw);
	if (!from_capasitance_error && !from_pressure_error)
	{
		if (from_capasitance < 0.0f)
			from_capasitance = 0.0f;
		else if (from_capasitance > 1.0f)
			from_capasitance = 1.0f;
		from_pressure = cwd_calculate_water_level_from_pressure_value(from_pressure_raw);
		if (from_pressure < 0.0f)
			from_pressure = 0.0f;
		else if (from_pressure > 1.0f)
			from_pressure = 1.0f;
		*water_level = capasitance_pressure_split * from_capasitance + (1.0f - capasitance_pressure_split) * from_pressure;
		return 0;
	}
	else if (!from_capasitance_error)
	{
		if (from_capasitance < 0.0f)
			from_capasitance = 0.0f;
		else if (from_capasitance > 1.0f)
			from_capasitance = 1.0f;
		*water_level = from_capasitance;
		return 0;
	}
	else if (!from_pressure_error)
	{
		from_pressure = cwd_calculate_water_level_from_pressure_value(from_pressure_raw);
		if (from_pressure < 0.0f)
			from_pressure = 0.0f;
		else if (from_pressure > 1.0f)
			from_pressure = 1.0f;
		*water_level = from_pressure;
		return 0;
	}
	return from_capasitance_error;
}

int cwd_get_tank_water_level_in_percents(float* water_level)
{
	int error = cwd_get_tank_water_level(water_level);
	if (!error)
		*water_level = 100.0f * *water_level;
	return error;
}

int cwd_is_12v_power_plugged(int* plugged)
{
#ifdef CWD_BCM2835
	int error = cwd_sbcm2835_initialize();
	if (error)
		return error;
	cwd_acquire_futex(&gpio_lock);
	bcm2835_gpio_fsel(CWD_POWER_PLUGGED_PIN, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(CWD_POWER_PLUGGED_PIN, BCM2835_GPIO_PUD_OFF);
	cwd_release_futex(&gpio_lock);
	cwd_wait_ns(16000000);
	*plugged = (int)bcm2835_gpio_lev(CWD_POWER_PLUGGED_PIN);
	cwd_sbcm2835_close();
	return 0;
#else
	return ENOSYS;
#endif
}

int cwd_enable_12v_power(int enable)
{
#ifdef CWD_BCM2835
	if (enable)
		enable = 1;
	int error = cwd_sbcm2835_initialize();
	if (error)
		return error;
	cwd_acquire_futex(&gpio_lock);
	bcm2835_gpio_fsel(CWD_POWER_ENABLE_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_POWER_ENABLE_PIN, (uint8_t)enable);
	cwd_release_futex(&gpio_lock);
	cwd_sbcm2835_close();
	return 0;
#else
	return ENOSYS;
#endif
}

int cwd_initialize_cooler()
{
#ifdef CWD_BCM2835
	int error = cwd_sbcm2835_initialize();
	if (error)
		return error;
	cwd_acquire_futex(&gpio_lock);
	bcm2835_gpio_fsel(CWD_COOLER_ENABLE_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_COOLER_ENABLE_PIN, 1);
	cwd_release_futex(&gpio_lock);
	cwd_sbcm2835_close();
	return 0;
#else
	return ENOSYS;
#endif
}

int cwd_enable_cooler(int enable)
{
#ifdef CWD_BCM2835
	if (!cwd_is_device_initialized())
		return EAGAIN;
	if (enable)
		enable = 0;
	else
		enable = 1;
	cwd_acquire_futex(&gpio_lock);
	bcm2835_gpio_write(CWD_COOLER_ENABLE_PIN, (uint8_t)enable);
	cwd_release_futex(&gpio_lock);
	return 0;
#else
	return ENOSYS;
#endif
}

#ifdef CWD_BCM2835
static void cwd_shutdown_leds_wait_half_clock()
{
	const unsigned long long half_clock = 1000;
	struct timespec begin;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	now.tv_sec = begin.tv_sec;
	now.tv_nsec = begin.tv_nsec;
	while ((((unsigned long long)(now.tv_sec - begin.tv_sec) * 1000000000) + (unsigned long long)now.tv_nsec) - (unsigned long long)begin.tv_nsec < half_clock)
		clock_gettime(CLOCK_MONOTONIC, &now);
}
#endif

int cwd_shutdown_leds()
{
#ifdef CWD_BCM2835
	const uint8_t sr_data[12] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0 };
	int error = cwd_sbcm2835_initialize();
	if (error)
		return error;
	cwd_acquire_futex(&gpio_lock);
	bcm2835_gpio_fsel(CWD_SR_CLK_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CWD_SR_IN_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
	cwd_shutdown_leds_wait_half_clock();
	for (int i = 12; i--;)
	{
		bcm2835_gpio_write(CWD_SR_IN_PIN, sr_data[i]);
		cwd_shutdown_leds_wait_half_clock();
		bcm2835_gpio_write(CWD_SR_CLK_PIN, 1);
		cwd_shutdown_leds_wait_half_clock();
		bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
	}
	bcm2835_gpio_fsel(CWD_SR_CLK_PIN, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(CWD_SR_IN_PIN, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(CWD_SR_CLK_PIN, BCM2835_GPIO_PUD_OFF);
	bcm2835_gpio_set_pud(CWD_SR_IN_PIN, BCM2835_GPIO_PUD_OFF);
	cwd_release_futex(&gpio_lock);
	cwd_sbcm2835_close();
	return 0;
#else
	return ENOSYS;
#endif
}

int cwd_device_initialization()
{
#ifdef CWD_BCM2835
	cwd_acquire_futex(&device_initialization_lock);
	if (!device_is_initialized)
	{
		int error = cwd_sbcm2835_initialize();
		if (error)
		{
			cwd_release_futex(&device_initialization_lock);
			return error;
		}
		error = cwd_enable_12v_power(0);
		if (error)
		{
			cwd_sbcm2835_close();
			cwd_release_futex(&device_initialization_lock);
			return error;
		}
		error = cwd_motor_enable(0);
		if (error)
		{
			cwd_sbcm2835_close();
			cwd_release_futex(&device_initialization_lock);
			return error;
		}
		error = cwd_initialize_cooler();
		if (error)
		{
			cwd_sbcm2835_close();
			cwd_release_futex(&device_initialization_lock);
			return error;
		}
		int power_is_plugged;
		error = cwd_is_12v_power_plugged(&power_is_plugged);
		if (error)
		{
			cwd_sbcm2835_close();
			cwd_release_futex(&device_initialization_lock);
			return error;
		}
		if (!power_is_plugged)
		{
			cwd_sbcm2835_close();
			cwd_release_futex(&device_initialization_lock);
			return EAGAIN;
		}
		device_is_initialized = 1;
		cwd_release_futex(&device_initialization_lock);
		return 0;
	}
	cwd_shutdown_leds();
	cwd_release_futex(&device_initialization_lock);
	return 0;
#else
	return 0;
#endif
}

void cwd_device_close()
{
#ifdef CWD_BCM2835
	cwd_acquire_futex(&device_initialization_lock);
	cwd_enable_12v_power(0);
	cwd_enable_cooler(0);
	cwd_sbcm2835_close();
	device_is_initialized = 0;
	cwd_release_futex(&device_initialization_lock);
#endif
}

int cwd_ligth_demo(int length)
{
#ifdef CWD_BCM2835
	int error = cwd_sbcm2835_initialize();
	if (!error)
	{
		cwd_acquire_futex(&gpio_lock);
		bcm2835_gpio_fsel(CWD_SR_CLK_PIN, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(CWD_SR_IN_PIN, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
		cwd_wait_ns(1000);
		static const uint8_t sr_data[168] = {
			0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
			0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0,
			1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
			0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0,
			0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0,
			0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0,
			0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
			0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
			0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0 };
		for (int t = 0; t != length; t = (length != -1) ? (t + 1) : 0)
			for (int i = 0; i != 12; ++i)
			{
				for (int j = 12; j--;)
				{
					bcm2835_gpio_write(CWD_SR_IN_PIN, sr_data[12 * i + j]);
					cwd_wait_ns(1000);
					bcm2835_gpio_write(CWD_SR_CLK_PIN, 1);
					cwd_wait_ns(1000);
					bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
				}
				cwd_wait_ns(1000000000);            
			}
		for (int i = 0; i != 2; ++i)
		{
			for (int j = 12; j--;)
			{
				bcm2835_gpio_write(CWD_SR_IN_PIN, sr_data[12 * (12 + i) + j]);
				cwd_wait_ns(1000);
				bcm2835_gpio_write(CWD_SR_CLK_PIN, 1);
				cwd_wait_ns(1000);
				bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
			}
			cwd_wait_ns(1000000000);
		}
		bcm2835_gpio_fsel(CWD_SR_CLK_PIN, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_fsel(CWD_SR_IN_PIN, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(CWD_SR_CLK_PIN, BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_set_pud(CWD_SR_IN_PIN, BCM2835_GPIO_PUD_OFF);
		cwd_release_futex(&gpio_lock);
		cwd_sbcm2835_close();
		return 0;
	}
	return error;
#else
	return ENOSYS;
#endif
}

int cwd_set_leds_to_blue()
{
#ifdef CWD_BCM2835
	int error = cwd_sbcm2835_initialize();
	if (!error)
	{
		cwd_acquire_futex(&gpio_lock);
		bcm2835_gpio_fsel(CWD_SR_CLK_PIN, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(CWD_SR_IN_PIN, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
		cwd_wait_ns(1000);
		static const uint8_t sr_data[24] = {
			0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
			0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0 };
		for (int i = 0; i != 2; ++i)
		{
			for (int j = 12; j--;)
			{
				bcm2835_gpio_write(CWD_SR_IN_PIN, sr_data[12 * i + j]);
				cwd_wait_ns(1000);
				bcm2835_gpio_write(CWD_SR_CLK_PIN, 1);
				cwd_wait_ns(1000);
				bcm2835_gpio_write(CWD_SR_CLK_PIN, 0);
			}
			sleep(1);
		}
		bcm2835_gpio_fsel(CWD_SR_CLK_PIN, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_fsel(CWD_SR_IN_PIN, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(CWD_SR_CLK_PIN, BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_set_pud(CWD_SR_IN_PIN, BCM2835_GPIO_PUD_OFF);
		cwd_sbcm2835_close();
		cwd_release_futex(&gpio_lock);
		return 0;
	}
	return error;
#else
	return ENOSYS;
#endif	
}

void cwd_wait_ns(uint64_t time)
{
	struct timespec begin;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	now.tv_sec = begin.tv_sec;
	now.tv_nsec = begin.tv_nsec;
	while (((((uint64_t)(now.tv_sec - begin.tv_sec) * (uint64_t)1000000000) + (uint64_t)now.tv_nsec) - (uint64_t)begin.tv_nsec) < time)
		clock_gettime(CLOCK_MONOTONIC, &now);
}

void cwd_linear_calibration(float x0, float x1, float y0, float y1, float* k, float* p)
{
	float local_k = (y1 - y0) / (x1 - x0);
	float local_p = -local_k * x0 + y0;
	*k = local_k;
	*p = local_p;
}

int cwd_measure_water_level_from_tank_capasitance_raw(int* sensor_value)
{
#ifdef CWD_BCM2835
	int error = cwd_sbcm2835_initialize();
	if (!error)
	{
		cwd_acquire_futex(&gpio_lock);
		const uint64_t timeout = 1000000;
		struct timespec timer_begin;
		struct timespec timer_end;
		uint64_t time_delta = 0;
		bcm2835_gpio_fsel(CWD_TANK_CAPSITANCE_SENSOR_TRIGGER, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_fsel(CWD_TANK_CAPSITANCE_SENSOR_ECHO, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(CWD_TANK_CAPSITANCE_SENSOR_ECHO, BCM2835_GPIO_PUD_OFF);
		cwd_wait_ns(1000000000);
		for (int i = 0; i != 256; ++i)
		{
			bcm2835_gpio_write(CWD_TANK_CAPSITANCE_SENSOR_TRIGGER, 0);
			cwd_wait_ns(3333333);
			clock_gettime(CLOCK_MONOTONIC, &timer_begin);
			timer_end.tv_sec = timer_begin.tv_sec;
			timer_end.tv_nsec = timer_begin.tv_nsec;
			bcm2835_gpio_write(CWD_TANK_CAPSITANCE_SENSOR_TRIGGER, 1);
			while (!bcm2835_gpio_lev(CWD_TANK_CAPSITANCE_SENSOR_ECHO) && (((uint64_t)(timer_end.tv_sec - timer_begin.tv_sec) * 1000000000) + (uint64_t)timer_end.tv_nsec) - (uint64_t)timer_begin.tv_nsec < timeout)
				clock_gettime(CLOCK_MONOTONIC, &timer_end);
			time_delta += (((uint64_t)(timer_end.tv_sec - timer_begin.tv_sec) * 1000000000) + (uint64_t)timer_end.tv_nsec) - (uint64_t)timer_begin.tv_nsec;
		}
		bcm2835_gpio_fsel(CWD_TANK_CAPSITANCE_SENSOR_TRIGGER, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_fsel(CWD_TANK_CAPSITANCE_SENSOR_ECHO, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(CWD_TANK_CAPSITANCE_SENSOR_TRIGGER, BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_set_pud(CWD_TANK_CAPSITANCE_SENSOR_ECHO, BCM2835_GPIO_PUD_OFF);
		cwd_release_futex(&gpio_lock);
		cwd_sbcm2835_close();
		time_delta >>= 8;
		*sensor_value = (int)(time_delta / 1000);
		return 0;
	}
	return error;
#else
	return ENOSYS;
#endif
}

int cwd_measure_water_level_from_tank_capasitance(float* water_level)
{
	int error = 0;
	for (int c = 3; !error && c--;)
	{
		int sensor_value;
		error = cwd_measure_water_level_from_tank_capasitance_raw(&sensor_value);
		if (!error)
		{
			float k;
			float p;
			cwd_linear_calibration(46.0f, 93.0f, 0.0f, 1.0f, &k, &p);
			float y = (float)sensor_value * k + p;
			if (y < 0.0f)
				y = 0.0f;
			else if (y > 1.0f)
				y = 1.0f;
			*water_level = y;
			if (y > 0.0f && y < 1.0f)
				return 0;
		}
	}
	return error;
}

float cwd_calculate_thermistor_temperature(int measurement)
{
	return (float)measurement * 0.116153f + -348.931f;
}
