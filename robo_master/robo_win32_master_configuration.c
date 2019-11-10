/*
	VarastoRobo common code 2019-11-10 by Santtu Nyman.
*/

#include "robo_win32_master_configuration.h"

jsonpl_value_t* find_child_by_name(jsonpl_value_t* value_tree, const char* child_name)
{
	if (value_tree->type == JSONPL_TYPE_OBJECT)
		for (size_t i = 0; i != value_tree->object.value_count; ++i)
			if (!strcmp(child_name, value_tree->object.table[i].name))
				return value_tree->object.table[i].value;
	return 0;
}

uint8_t* read_static_map_from_json(const jsonpl_value_t* configuration, uint8_t* map_height, uint8_t* map_width)
{
	jsonpl_value_t* static_map = find_child_by_name(configuration, "static_map");
	if (!static_map || static_map->type != JSONPL_TYPE_ARRAY || !static_map->array.value_count || static_map->array.table[0]->type != JSONPL_TYPE_ARRAY || !static_map->array.table[0]->array.value_count)
		return 0;
	uint8_t height = (uint8_t)static_map->array.value_count;
	uint8_t width = (uint8_t)static_map->array.table[0]->array.value_count;
	for (uint8_t y = 1; y != height; ++y)
		if (static_map->array.table[y]->type != JSONPL_TYPE_ARRAY || static_map->array.table[y]->array.value_count != (size_t)width)
			return 0;
	uint8_t* map = (uint8_t*)malloc((((size_t)height * (size_t)width) + 7) / 8);
	if (!map)
		return 0;
	memset(map, 0, (((size_t)height * (size_t)width) + 7) / 8);
	for (uint8_t y = 0; y != height; ++y)
		for (uint8_t x = 0; x != width; ++x)
		{
			int in_memory_bit_index = ((int)y * (int)width) + (int)x;
			int byte_index = in_memory_bit_index / 8;
			int bit_index = in_memory_bit_index % 8;
			map[byte_index] |= (static_map->array.table[(height - 1) - y]->array.table[x]->type == JSONPL_TYPE_BOOLEAN && static_map->array.table[(height - 1) - y]->array.table[x]->boolean_value) ? (uint8_t)(1 << bit_index) : 0;
		}
	*map_height = height;
	*map_width = width;
	return map;
}

uint32_t read_broadcast_delay_from_json(const jsonpl_value_t* configuration)
{
	uint32_t delay = 5000;
	jsonpl_value_t* broadcast_ms_delay = find_child_by_name(configuration, "broadcast_ms_delay");
	if (broadcast_ms_delay && broadcast_ms_delay->type == JSONPL_TYPE_NUMBER)
		delay = (uint32_t)broadcast_ms_delay->number_value;
	return delay;
}

uint8_t read_master_id_from_json(const jsonpl_value_t* configuration)
{
	uint8_t id = 0;
	jsonpl_value_t* master_id = find_child_by_name(configuration, "master_device_id");
	if (master_id && master_id->type == JSONPL_TYPE_NUMBER)
		id = (uint8_t)master_id->number_value;
	return id;
}

uint8_t read_system_status_from_json(const jsonpl_value_t* configuration)
{
	uint8_t status = 1;
	jsonpl_value_t* initial_system_status = find_child_by_name(configuration, "initial_system_status");
	if (initial_system_status && initial_system_status->type == JSONPL_TYPE_NUMBER)
		status = (uint8_t)initial_system_status->number_value;
	return status;
}

size_t read_device_list_from_json(const jsonpl_value_t* configuration, robo_win32_broadcast_device_t** device_table)
{
	jsonpl_value_t* list = find_child_by_name(configuration, "initial_device_list");
	if (!list || list->type != JSONPL_TYPE_ARRAY)
		return 0;
	size_t n = 0;
	for (size_t i = 0; i != list->array.value_count; ++i)
		if (list->array.table[i]->type == JSONPL_TYPE_OBJECT)
			++n;
	robo_win32_broadcast_device_t* table = (robo_win32_broadcast_device_t*)malloc(n ? (n * sizeof(robo_win32_broadcast_device_t)) : 1);
	if (!table)
		return 0;
	for (size_t c = 0, i = 0; c != n; ++i)
		if (list->array.table[i]->type == JSONPL_TYPE_OBJECT)
		{
			jsonpl_value_t* component = find_child_by_name(list->array.table[i], "type");
			if (component && component->type == JSONPL_TYPE_NUMBER)
				table[c].type = (uint8_t)component->number_value;
			else
				table[c].type = 0xFF;
			component = find_child_by_name(list->array.table[i], "id");
			if (component && component->type == JSONPL_TYPE_NUMBER)
				table[c].id = (uint8_t)component->number_value;
			else
				table[c].id = 0xFF;
			component = find_child_by_name(list->array.table[i], "x");
			if (component && component->type == JSONPL_TYPE_NUMBER)
				table[c].x = (uint8_t)component->number_value;
			else
				table[c].x = 0xFF;
			component = find_child_by_name(list->array.table[i], "y");
			if (component && component->type == JSONPL_TYPE_NUMBER)
				table[c].y = (uint8_t)component->number_value;
			else
				table[c].y = 0xFF;
			component = find_child_by_name(list->array.table[i], "ip");
			if (component && component->type == JSONPL_TYPE_STRING)
				table[c].ip_address = (uint32_t)ntohl(inet_addr(component->string.value));
			else
				table[c].ip_address = 0;
			++c;
		}
	*device_table = table;
	return n;
}

size_t read_block_list_from_json(const jsonpl_value_t* configuration, uint8_t** block_list)
{
	jsonpl_value_t* list = find_child_by_name(configuration, "initial_block_list");
	if (!list || list->type != JSONPL_TYPE_ARRAY)
		return 0;
	size_t n = 0;
	for (size_t i = 0; i != list->array.value_count; ++i)
		if (list->array.table[i]->type == JSONPL_TYPE_OBJECT)
			++n;
	uint8_t* table = (uint8_t*)malloc(n ? (n * 2) : 1);
	if (!table)
		return 0;
	for (size_t c = 0, i = 0; c != n; ++i)
		if (list->array.table[i]->type == JSONPL_TYPE_OBJECT)
		{
			jsonpl_value_t* component = find_child_by_name(list->array.table[i], "x");
			if (component && component->type == JSONPL_TYPE_NUMBER)
				table[c * 2 + 0] = (uint8_t)component->number_value;
			else
				table[c * 2 + 0] = 0xFF;
			component = find_child_by_name(list->array.table[i], "y");
			if (component && component->type == JSONPL_TYPE_NUMBER)
				table[c * 2 + 1] = (uint8_t)component->number_value;
			else
				table[c * 2 + 1] = 0xFF;
			++c;
		}
	*block_list = table;
	return n;
}

robo_win32_master_configuration_t* robo_win32_load_master_configuration()
{
	jsonpl_value_t* json;
	if (robo_win32_load_json_from_program_directory_file(L"test_robo_master.json", &json))
		return 0;

	uint32_t broadcast_delay = read_broadcast_delay_from_json(json);
	uint8_t system_status = read_system_status_from_json(json);
	uint8_t master_id = read_master_id_from_json(json);

	uint8_t map_height;
	uint8_t map_width;
	uint8_t* map = read_static_map_from_json(json, &map_height, &map_width);
	if (!map)
	{
		robo_win32_free_file_data(json);
		return 0;
	}

	uint8_t* block_table = 0;
	size_t block_count = read_block_list_from_json(json, &block_table);

	robo_win32_broadcast_device_t* device_table = 0;
	size_t device_count = read_device_list_from_json(json, &device_table);

	robo_win32_free_file_data(json);

	robo_win32_master_configuration_t* configuration = (robo_win32_master_configuration_t*)malloc(sizeof(robo_win32_master_configuration_t));
	if (!map)
	{
		if (device_table)
			free(device_table);
		if (block_table)
			free(block_table);
		free(map);
		return 0;
	}
	
	configuration->broadcast_ms_delay = broadcast_delay;
	configuration->master_id = master_id;
	configuration->system_status = system_status;
	configuration->map_height = map_height;
	configuration->map_width = map_width;
	configuration->map = map;
	configuration->block_count = block_count;
	configuration->block_table = block_table;
	configuration->device_count = device_count;
	configuration->device_table = device_table;
	return configuration;
}

void robo_win32_free_master_configuration(robo_win32_master_configuration_t* master_configuration)
{
	if (master_configuration->device_table)
		free(master_configuration->device_table);
	if (master_configuration->block_table)
		free(master_configuration->block_table);
	free(master_configuration->map);
	free(master_configuration);
}