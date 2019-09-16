/*
	Graph Drawing Tool version 1.4.0 2019-09-16 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#ifndef GDT_FILE_H
#define GDT_FILE_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

int gdt_load_file(const char* name, size_t* size, void** data);

void gdt_free_file_data(void* data, size_t size);

int gdt_store_file(const char* name, size_t size, const void* data);

int gdt_read_map_file(const char* name, size_t* size, void** mapping);

void gdt_unmap_file(void* mapping, size_t size);

int gdt_delete_file(const char* name);

int gdt_move_file(const char* current_name, const char* new_name);

#ifdef _WIN32

#include <windows.h> 

int gdt_load_file_win32(const WCHAR* name, size_t* size, void** data);

int gdt_store_file_win32(const WCHAR* name, size_t size, const void* data);

int gdt_read_map_file_win32(const WCHAR* name, size_t* size, void** mapping);

int gdt_delete_file_win32(const WCHAR* name);

int gdt_move_file_win32(const WCHAR* current_name, const WCHAR* new_name);

#endif

#endif
