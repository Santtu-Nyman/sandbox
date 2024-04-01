/*
	VarastoRobo master server version 0.1.0 2019-11-10 by Santtu Nyman.
*/

#include "robo_win32_broadcast.h"
#include "robo_win32_file.h"
#include "robo_win32_master_configuration.h"
#include "jsonpl.h"
#include "stdio.h"

void CALLBACK ctr_c_close_process_routine(ULONG_PTR parameter) { ExitProcess(0); }

int is_block_at_location(size_t block_count, uint8_t* block_table, uint8_t x, uint8_t y)
{
	for (size_t i = 0; i != block_count; ++i)
		if (block_table[i * 2 + 0] == x && block_table[i * 2 + 1] == y)
			return 1;
	return 0;
}

int is_gopigo_at_location(size_t device_count, robo_win32_broadcast_device_t* device_table, uint8_t x, uint8_t y)
{
	for (size_t i = 0; i != device_count; ++i)
		if (device_table[i].type == 2 && device_table[i].x == x && device_table[i].y == y)
			return 1;
	return 0;
}

void print_ascii_map(uint8_t map_height, uint8_t map_width, uint8_t* map, size_t block_count, uint8_t* block_table, size_t device_count, robo_win32_broadcast_device_t* device_table)
{
	printf("     +");
	for (uint8_t x = 0; x != map_width; ++x)
		printf("-");
	printf("+\n");
	for (uint8_t y = 0; y != map_height; ++y)
	{
		printf("    %i", ((map_height - 1) - y) % 10);
		printf("|");
		for (uint8_t x = 0; x != map_width; ++x)
		{
			if (is_gopigo_at_location(device_count, device_table, x, ((map_height - 1) - y)))
				printf("G");
			else if(is_block_at_location(block_count, block_table, x, ((map_height - 1) - y)))
				printf("B");
			else
			{
				int in_memory_bit_index = ((int)((map_height - 1) - y) * (int)map_width) + (int)x;
				int byte_index = in_memory_bit_index / 8;
				int bit_index = in_memory_bit_index % 8;
				printf((map[byte_index] & (uint8_t)(1 << bit_index)) ? "#" : " ");
			}
		}
		printf("|\n");
	}
	printf("     +");
	for (uint8_t x = 0; x != map_width; ++x)
		printf("-");
	printf("+\n");
	printf("      ");
	for (uint8_t x = 0; x != map_width; ++x)
		printf("%i", x % 10);
	printf("\n");
}

void print_broadcast_info(robo_win32_broadcast_info_t* info)
{
	printf("Master %lu device in state %lu at address %lu.%lu.%lu.%lu\n", (unsigned long)info->master_id, (unsigned long)info->system_status, (info->master_ip_address >> 24), (info->master_ip_address >> 16) & 0xFF, (info->master_ip_address >> 8) & 0xFF, info->master_ip_address & 0xFF);

	printf("map\n");
	print_ascii_map(info->map_height, info->map_width, info->valid_map_locations, info->block_count, info->block_table, info->device_count, info->device_table);

	printf("%lu blocks\n", (unsigned long)info->block_count);
	for (size_t i = 0; i != (size_t)info->block_count; ++i)
		printf("    Block found at map x=%lu,y=%lu\n", (unsigned long)info->block_table[i * 2 + 0], (unsigned long)info->block_table[i * 2 + 1]);

	printf("%lu other devices\n", (unsigned long)info->device_count);
	for (size_t i = 0; i != (size_t)info->device_count; ++i)
	{
		printf("    Device %lu of type %lu found at address %lu.%lu.%lu.%lu and map x=%lu,y=%lu\n",
			(unsigned long)info->device_table[i].id,
			(unsigned long)info->device_table[i].type,
			info->device_table[i].ip_address >> 24, (info->device_table[i].ip_address >> 16) & 0xFF, (info->device_table[i].ip_address >> 8) & 0xFF, info->device_table[i].ip_address & 0xFF,
			(unsigned long)info->device_table[i].x, (unsigned long)info->device_table[i].y);
	}
}

int main(int argc, char* argv)
{
	SetConsoleCtrlHandler(ctr_c_close_process_routine, TRUE);
	printf("Test master server version 0.1.0\n");

	printf("Loading configuration...\n");
	robo_win32_master_configuration_t* configuration = robo_win32_load_master_configuration();
	if (!configuration)
	{
		printf("Loading configuration failed. Check if \"test_robo_master.json\" file is missing.\n");
		return 0;
	}
	printf("Configuration loaded\n");

	printf("Creating broadcast...\n");
	volatile robo_win32_broadcast_t master_broadcast;
	DWORD broadcast_create_error = robo_win32_create_broadcast_thread(&master_broadcast, configuration->broadcast_ms_delay, configuration->system_status, configuration->master_id, configuration->map_height, configuration->map_width, configuration->map);
	if (broadcast_create_error)
	{
		printf("Failed to create broadcast\n");
		return 0;
	}
	printf("Broadcast created\n");

	for (size_t i = 0; i != configuration->block_count; ++i)
		robo_win32_add_broadcast_block(&master_broadcast, configuration->block_table[i * 2 + 0], configuration->block_table[i * 2 + 1]);
	for (size_t i = 0; i != configuration->device_count; ++i)
		robo_win32_add_broadcast_device(&master_broadcast, configuration->device_table[i].type, configuration->device_table[i].id, configuration->device_table[i].x, configuration->device_table[i].y, configuration->device_table[i].ip_address);
	printf("Configuration information added to broadcast\n");

	printf("Receiving broadcast...\n");
	robo_win32_broadcast_info_t* broadcast_info;
	DWORD find_master_error = robo_win32_find_master(&broadcast_info);
	if (find_master_error)
	{
		printf("Failed to receive broadcast\n");
		return 0;
	}
	printf("Broadcast received\n");

	print_broadcast_info(broadcast_info);
	robo_win32_free_broadcast_info(broadcast_info);
	robo_win32_free_master_configuration(configuration);

	printf("Press CTRL+C to end broadcast and to close this process\n");
	Sleep(INFINITE);
	return 0;
}