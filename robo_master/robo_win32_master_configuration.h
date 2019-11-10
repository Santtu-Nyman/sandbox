/*
	VarastoRobo common code 2019-11-10 by Santtu Nyman.
*/

#ifndef ROBO_WIN32_MASTER_CONFIGURATION_H
#define ROBO_WIN32_MASTER_CONFIGURATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <windows.h>
#include "robo_win32_file.h"
#include "robo_win32_broadcast.h"
#include "jsonpl.h"

typedef struct robo_win32_master_configuration_t
{
	uint32_t broadcast_ms_delay;
	uint8_t master_id;
	uint8_t system_status;
	uint8_t map_height;
	uint8_t map_width;
	uint8_t* map;
	size_t block_count;
	uint8_t* block_table;
	size_t device_count;
	robo_win32_broadcast_device_t* device_table;
} robo_win32_master_configuration_t;

robo_win32_master_configuration_t* robo_win32_load_master_configuration();

void robo_win32_free_master_configuration(robo_win32_master_configuration_t* master_configuration);

#ifdef __cplusplus
}
#endif

#endif