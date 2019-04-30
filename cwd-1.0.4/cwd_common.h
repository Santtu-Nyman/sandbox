/*
	Cool Water Dispenser common version 1.0.4 2019-04-29 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Cool Water Dispenser shared linux source.
		
	Version history
		version 1.0.4 2019-04-29
			Server API chance.
		version 1.0.3 2019-04-23
			Added new measurements features for Jarno.
		version 1.0.2 2019-04-16
			Bug fix.
		version 1.0.1 2019-04-12
			Added more reasonable initialization and simplefiad some code.
		version 1.0.0 2019-04-11
			First fully functioning version.
		version 0.0.7 2019-04-04
			Added more device control features.
		version 0.0.6 2019-03-31
			Added device control features.
		version 0.0.5 2019-03-25
			Added more measurement stuff and some device initialization things.
		version 0.0.4 2019-03-20
			Added support for DS18B20 temperature sensor.
		version 0.0.3 2019-03-17
			Preparing for new server API.
		version 0.0.2 2019-03-16
			Default device configuration added.
		version 0.0.1 2019-03-08
			First version.
*/

#ifndef CWD_COMMON_H
#define CWD_COMMON_H

#define CWD_VERSION_NXX 1
#define CWD_VERSION_XNX 0
#define CWD_VERSION_XXN 3

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define CWD_TEMPERATUTE_SENSOR_ANY -1
#define CWD_TEMPERATUTE_SENSOR_PRINTED 0
#define CWD_TEMPERATUTE_SENSOR_DALLAS 1
#define CWD_ULTRASONIC_0_TRIG_PIN 2
#define CWD_ULTRASONIC_0_ECHO_PIN 3
#define CWD_ULTRASONIC_1_TRIG_PIN 17
#define CWD_ULTRASONIC_1_ECHO_PIN 27
#define CWD_ULTRASONIC_2_TRIG_PIN 10
#define CWD_ULTRASONIC_2_ECHO_PIN 9
#define CWD_MOTOR_A_PIN 26
#define CWD_MOTOR_B_PIN 19
#define CWD_MOTOR_ENABLE_PIN 6
#define CWD_POWER_PLUGGED_PIN 13
#define CWD_POWER_ENABLE_PIN 5
#define CWD_COOLER_ENABLE_PIN 12
#define CWD_TEMPERATURE_SPI_CS_PIN 7
#define CWD_TEMPERATURE_SPI_CLK_PIN 25
#define CWD_TEMPERATURE_SPI_DOUT_PIN 8
#define CWD_PERSSURE_SPI_CS_PIN 21
#define CWD_PERSSURE_SPI_CLK_PIN 16
#define CWD_PERSSURE_SPI_DOUT_PIN 20
#define CWD_SR_CLK_PIN 24
#define CWD_SR_IN_PIN 23
#define CWD_TANK_CAPSITANCE_SENSOR_TRIGGER 18
#define CWD_TANK_CAPSITANCE_SENSOR_ECHO 22
#define CWD_UI_EXECUTABLE "python"
#define CWD_UI_ARGUMENTS { "./cwd_ui.py" }
#define CWD_DEAFULT_SERVER_API 2
#define CWD_DEFAULT_SERVER "www.students.oamk.fi/~t7nysa00"
#define CWD_URL_PROTOCOL "http://"
#define CWD_URL_DEVICE_CONFIGURATION_PATH "/~t7nysa00/device_configuration.php"
#define CWD_URL_DATA_INPUT_PATH "/~t7nysa00/device_data.php"
#define CWD_POST_STARTUP "0"
#define CWD_POST_PERIODIC_MEASUREMENTS "1"
#define CWD_POST_BYPASSING "2"
#define CWD_POST_ORDER "3"
#define CWD_URL_PROTOCOL_2 "http://"
#define CWD_URL_DATA_PATH_2 "/api/dispenser"
#define CWD_URL_EXTENDE_DATA_PATH_2 "/api/dispenser.php"

struct cwd_device_configuration_t
{
	int version;
	int version_extension;
	int version_patch;
	int full_bcm2835_build;
	int prinlab_ptc_support;
	int printed_fsr_support;
	int ds18b20_support;
	int is_curl_installed;
	uint64_t device_id;
	uint64_t periodic_mesurement_delay;
	uint64_t loop_delay;
	char* server;
	char* directory;
	size_t processor_count;
	int* processor_indices;
	int ui_process_processor_index;
	char* ui_process_executable;
	size_t ui_process_argument_count;
	char** ui_process_arguments;
	float target_water_temperature;
	int server_api_version;
	int device_operation_mode;
	int use_extended_url;
	int disable_ui;
	int disable_12v_power;
	int disable_cooler;
	int print_measurements;
	int skip_insert;
	int print_server_responses;
	int log_measurements;
	int disable_capasitance_sensor;
	int disable_thermistor;
	int offline;
	int disable_temperature;
};

int cwd_is_device_initialized();

int cwd_read_ds18b20(float* temperature);

int cwd_get_device_uid(uint64_t* uid);

int cwd_get_executable_file_name(char** executable_file_name);

int cwd_get_executable_directory_path(char** directory_path);

int cwd_get_last_tank_refill_time(uint64_t* timestamp);

int cwd_set_last_tank_refill_time(uint64_t timestamp);

int cwd_read_thermistor_adc(int* value);

int cwd_read_temperature(int sensor, float* temperature);

int cwd_motor_enable(int enable);

int cwd_pump_water(int direction, uint64_t time_ns);

void cwd_debug_print(int hex, size_t size, const void* buffer);

int cwd_get_processor_indices(size_t* processor_count, int** processor_indices);

int cwd_default_configuration(struct cwd_device_configuration_t* configuration);

int cwd_load_file(const char* name, size_t* size, void** data);

int cwd_save_file(const char* name, size_t size, const void* data);

int cwd_append_to_file(const char* name, size_t size, const void* data);

int cwd_append_string_to_file(const char* name, const char* string);

int cwd_append_float_to_file(const char* name, float value);

void cwd_print_time(uint64_t timestamp, char* buffer);

size_t cwd_line_length(const char* string, size_t buffer_size);

const char* cwd_next_line(const char* string, size_t buffer_size);

char* cwd_search_for_argument(int argc, char** argv, const char* argument);

int cwd_decode_integer_argument(int argc, char** argv, const char* argument, int* negative_interger, uint64_t* interger_value);

int cwd_check_flag_argument(int argc, char** argv, const char* argument);

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

int cwd_get_device_configuration(int api_version, const char* server, uint64_t device_id, struct cwd_device_configuration_t* configuration, int use_extended_url, int print_response);

int cwd_send_device_startup(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, int use_extended_url, int print_response);

int cwd_send_periodic_mesurements(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, float water_level, float water_temperature, uint64_t water_refill_timestamp, int use_extended_url, int print_response);

int cwd_send_bypassing(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, int use_extended_url, int print_response);

int cwd_send_order(int api_version, const char* server, uint64_t device_id, uint64_t timestamp, uint32_t order_type, int use_extended_url, int print_response);

int cwd_send_periodic_mesurements2(const char* server, uint64_t device_id, uint64_t timestamp, float water_level, float water_temperature, uint64_t water_refill_timestamp, int use_extended_url, int print_response);

int cwd_read_pressure_sensor(int* value);

float cwd_calculate_water_level_from_pressure_value(int value);

int cwd_get_tank_water_level_in_percents(float* water_level);

int cwd_is_12v_power_plugged(int* plugged);

int cwd_enable_12v_power(int enable);

int cwd_initialize_cooler();

int cwd_enable_cooler(int enable);

int cwd_shutdown_leds();

int cwd_device_initialization();

void cwd_device_close();

int cwd_ligth_demo(int length);

int cwd_set_leds_to_blue();

void cwd_wait_ns(uint64_t time);

void cwd_linear_calibration(float x0, float x1, float y0, float y1, float* k, float* p);

int cwd_measure_water_level_from_tank_capasitance_raw(int* sensor_value);

int cwd_measure_water_level_from_tank_capasitance(float* water_level);

float cwd_calculate_thermistor_temperature(int measurement);

#endif
