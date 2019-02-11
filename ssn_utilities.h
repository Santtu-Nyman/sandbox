#ifndef SSN_UTILITIES_LIBRARY_H
#define SSN_UTILITIES_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

int ssn_is_empty_space(char character);

int ssn_is_decimal(char character);

int ssn_is_hexadecimal(char character);

int ssn_is_base36(char character);

int ssn_is_letter(char character);

int ssn_decode_integer(int* value, size_t buffer_size, const char* buffer);

int ssn_encode_integer(int value, size_t buffer_size, char* buffer);

int ssn_decode_float(float* value, size_t buffer_size, const char* buffer);

int ssn_encode_float(float value, size_t decimals, size_t buffer_size, char* buffer);

int ssn_load_file(const char* file_name, size_t* file_size, void** file_data);

int ssn_store_file(const char* file_name, size_t file_size, const void* file_data);

#ifdef __cplusplus
}
#endif

#endif