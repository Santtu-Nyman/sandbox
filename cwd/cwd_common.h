/*
	Cool Water Dispenser common version 0.0.3 2019-03-17 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Cool Water Dispenser shared linux source.
		
	Version history
		version 0.0.3 2019-03-17
			Preparing for new server API.
		version 0.0.2 2019-03-16
			Default device configuration added.
		version 0.0.1 2019-03-08
			First version.
*/

#ifndef CWD_COMMON_H
#define CWD_COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define CWD_URL_PROTOCOL "http://"
#define CWD_URL_DEVICE_CONFIGURATION_PATH "/~t7nysa00/device_configuration.php"
#define CWD_URL_DATA_INPUT_PATH "/~t7nysa00/device_data.php"
#define CWD_POST_STARTUP "0"
#define CWD_POST_PERIODIC_MEASUREMENTS "1"
#define CWD_POST_BYPASSING "2"
#define CWD_POST_ORDER "3"

struct cwd_device_configuration_t
{
	uint32_t device_id;
	uint32_t device_operation_mode;
	uint64_t periodic_mesurement_delay;
	float target_water_temperature;
};

void cwd_debug_print(int hex, size_t size, const void* buffer);

void cwd_default_configuration(uint32_t device_id, struct cwd_device_configuration_t* configuration);

int cwd_load_file(const char* name, size_t* size, void** data);

int cwd_save_file(const char* name, size_t size, const void* data);

size_t cwd_line_length(const char* string, size_t buffer_size);

char* cwd_search_for_argument(int argc, char** argv, const char* argument);

size_t cwd_print_u64(size_t buffer_length, char* buffer, uint64_t value);

size_t cwd_print_f32_n_dot3(size_t buffer_length, char* buffer, float value);

int cwd_get_processor_count(size_t* processor_count);

void cwd_acquire_futex(volatile int* futex);

void cwd_release_futex(volatile int* futex);

int cwd_create_thread(int (*entry)(void*), void* parameter, size_t stack_size);

int cwd_get_scheduling(int* policy, struct sched_param* parameters);

int cwd_set_scheduling(int policy, const struct sched_param* parameters);

int cwd_get_affinity(size_t* processor_count, int** processor_indices);

int cwd_set_affinity(size_t processor_count, int* processor_indices);

void cwd_wait_for_process(pid_t process_id);

int cwd_is_curl_installed_with_http(int* is_isntalled);

int cwd_http_post(const char* url, size_t key_value_pair_count, const char** key_value_pairs, size_t* data_size, void** data);

int cwd_get_device_configuration(int api_version, const char* server, uint32_t device_id, struct cwd_device_configuration_t* configuration);

int cwd_send_device_startup(int api_version, const char* server, uint32_t device_id, uint64_t timestamp);

int cwd_send_periodic_mesurements(int api_version, const char* server, uint32_t device_id, uint64_t timestamp, float water_level, float water_temperature);

int cwd_send_bypassing(int api_version, const char* server, uint32_t device_id, uint64_t timestamp);

int cwd_send_order(int api_version, const char* server, uint32_t device_id, uint64_t timestamp, uint32_t order_type);

float cwd_calculate_thermistor_temperature(int measurement);

#endif
