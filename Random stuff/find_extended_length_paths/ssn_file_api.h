#ifndef SSN_FILE_API_H
#define SSN_FILE_API_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
#include <Windows.h>
typedef HANDLE ssn_handle_t;
#else
typedef int ssn_handle_t;
#endif // _WIN32

#define SSN_READ_PERMISION 0x1
#define SSN_WRITE_PERMISION 0x2
#define SSN_EXECUTE_PERMISION 0x4

#define SSN_CREATE 0x8
#define SSN_TRUNCATE 0x10
#define SSN_CREATE_PATH 0x20

typedef void* (*ssn_allocator_callback_t)(void* context, size_t size);
typedef void (*ssn_deallocator_callback_t)(void* context, size_t size, void* allocation);

typedef struct ssn_file_entry_t
{
	size_t name_length;
	char* name;
} ssn_file_entry_t;

int ssn_open_file(size_t name_length, const char* name, int permissions_and_flags, ssn_handle_t* handle_address);

int ssn_close_file(ssn_handle_t handle);

int ssn_get_file_size(ssn_handle_t handle, uint64_t* file_size_address);

int ssn_truncate_file(ssn_handle_t handle, uint64_t file_size);

int ssn_flush_file_buffers(ssn_handle_t handle);

int ssn_read_file(ssn_handle_t handle, uint64_t file_offset, size_t io_size, void* buffer);

int ssn_write_file(ssn_handle_t handle, uint64_t file_offset, size_t io_size, const void* buffer);

int ssn_load_file(size_t name_length, const char* name, size_t buffer_size, void* buffer, size_t* file_size_address);

int ssn_allocate_and_load_file(size_t name_length, const char* name, void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t* file_size_address, void** file_data_address);

int ssn_store_file(size_t name_length, const char* name, size_t size, const void* data);

int ssn_delete_file(size_t name_length, const char* name);

int ssn_delete_directory(size_t name_length, const char* name);

int ssn_move_file(size_t old_name_length, const char* old_name, size_t new_name_length, const char* new_name);

int ssn_move_directory(size_t old_name_length, const char* old_name, size_t new_name_length, const char* new_name);

int ssn_create_directory(size_t name_length, const char* name);

int ssn_list_directory(size_t name_length, const char* name, size_t buffer_size, ssn_file_entry_t* buffer, size_t* required_buffer_size_address, size_t* directory_count_address, size_t* file_count_address);

int ssn_allocate_and_list_directory(size_t name_length, const char* name, void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t* entry_table_size_address, ssn_file_entry_t** entry_table_address, size_t* directory_count_address, size_t* file_count_address);

int ssn_get_executable_file_name(size_t buffer_size, char* buffer, size_t* file_name_length);

int ssn_allocate_and_get_executable_file_name(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, char** file_name_address, size_t* file_name_length_address);

int ssn_get_program_directory(size_t buffer_size, char* buffer, size_t* directory_name_length_address);

int ssn_allocate_and_get_program_directory(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, char** directory_name_address, size_t* directory_name_length_address);

int ssn_get_data_directory(size_t buffer_size, size_t sub_directory_name_length, const char* sub_directory_name, char* buffer, size_t* directory_name_length_address);

int ssn_allocate_and_get_data_directory(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t sub_directory_name_length, const char* sub_directory_name, char** directory_name_address, size_t* directory_name_length_address);

int ssn_make_path(size_t parent_directory_length, const char* parent_directory, size_t sub_directory_length, const char* sub_directory, size_t buffer_size, char* buffer, size_t* directory_name_length_address);

int ssn_allocate_and_make_path(void* allocator_context, ssn_allocator_callback_t allocator_procedure, ssn_deallocator_callback_t deallocator_procedure, size_t parent_directory_length, const char* parent_directory, size_t sub_directory_length, const char* sub_directory, char** directory_name_address, size_t* directory_name_length_address);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SSN_FILE_API_H