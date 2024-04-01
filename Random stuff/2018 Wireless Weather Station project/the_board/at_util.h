#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define INP_POST_SERVER_STUDENTS ((uintptr_t)0)
#define INP_POST_ASK_INTERVAL ((uintptr_t)1)
#define INP_POST_PATH_ADD_MEASUREMENT ((uintptr_t)2)

bool at_wait_at_module_wakeup(unsigned long wait_for);

bool at_generic_command(const char* command, size_t result_buffer_length, char* result_buffer, size_t* result_length, unsigned long wait_for_result);

bool at_get_cfun(int* value);

bool at_set_cfun(int value);

bool at_get_cpin(bool* value);

bool at_set_cpin(const char* value);

bool at_get_creg(int* value);

bool at_set_creg(int value);

bool at_get_cgatt(int* value);

bool at_set_cgatt(int value);

bool at_enable_cmnet();

bool at_ciicr();

bool at_cifsr(size_t* ip_address_length, char** ip_address);

bool at_http_post(const char* host, uint16_t port, const char* path, const char* key_value_pairs, int* response_status, size_t* response_size, char** response);

bool at_udp_send(const char* host, uint16_t port, size_t size, void* data);

bool inp_http_ask_measurement_interval(const char* host, const char* path, uint32_t station, uint32_t* interval);

bool inp_http_post_measurements(const char* host, const char* path, unsigned long station, float temperature, float humididy, float pressure, float illuminance);
