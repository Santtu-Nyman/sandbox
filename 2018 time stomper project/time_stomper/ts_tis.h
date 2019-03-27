// Time Stomper time information storage

#ifndef TS_TIS_H
#define TS_TIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "ts_24aa1025.h"

void ts_clean_time_information();
void ts_save_wall_clock_time(uint32_t time);
uint32_t ts_load_wall_clock_time();
uint8_t ts_add_time_begin(uint16_t identity, uint32_t time);
uint8_t ts_add_time_end(uint16_t identity, uint32_t time);
uint8_t ts_query_info_by_identitys(size_t identity_count, uint16_t* identitys, uint32_t query_from, uint32_t query_to, uint32_t* stamp_count, uint32_t* total_time);

static uint8_t ts_tis_buffer[136];

void ts_clean_time_information()
{
	for (uint8_t* clean = ts_tis_buffer, * end_clean = clean + 0x80; clean != end_clean; ++clean)
		*clean = 0;
	for (uint16_t offset = 0; (uint16_t)(offset + 0x80); offset += 0x80)
		ts_write_eeprom_page(ts_tis_buffer, offset);
}

static uint8_t ts_eeprom_page_buffer[0x80];

void ts_save_wall_clock_time(uint32_t time)
{
  ts_read_eeprom_page(ts_eeprom_page_buffer, 0xFF80);
  ts_eeprom_page_buffer[0x7C] = (uint8_t)time;
  ts_eeprom_page_buffer[0x7D] = (uint8_t)(time >> 8);
  ts_eeprom_page_buffer[0x7E] = (uint8_t)(time >> 16);
  ts_eeprom_page_buffer[0x7F] = (uint8_t)(time >> 24);
  ts_write_eeprom_page(ts_eeprom_page_buffer, 0xFF80);
}

uint32_t ts_load_wall_clock_time()
{
  ts_read_eeprom_page(ts_eeprom_page_buffer, 0xFF80);
  return (uint32_t)ts_eeprom_page_buffer[0x7C] | ((uint32_t)ts_eeprom_page_buffer[0x7D] << 8) | ((uint32_t)ts_eeprom_page_buffer[0x7E] << 16) | ((uint32_t)ts_eeprom_page_buffer[0x7F] << 24);
}

void ts_write_eeprom(size_t size, const void* buffer, uint16_t offset)
{
	while (size)
	{
		uint16_t page_offset = offset & 0x7F;
		size_t block_size = 0x80 - (size_t)page_offset;
		if (block_size > size)
			block_size = size;
		if (block_size != 0x80)
			ts_read_eeprom_page(ts_eeprom_page_buffer, offset & 0xFF80);
		for (uint8_t* copy_destination = ts_eeprom_page_buffer + (offset & 0x7F), *copy_source_end = (uint8_t*)buffer + block_size; buffer != (const void*)copy_source_end; buffer = (const void*)((uintptr_t)buffer + 1), ++copy_destination)
			*copy_destination = *(uint8_t*)buffer;
		ts_write_eeprom_page(ts_eeprom_page_buffer, offset & 0xFF80);
		offset += (uint16_t)block_size;
		size -= block_size;
	}
}

uint8_t ts_add_time_begin(uint16_t identity, uint32_t time)
{
	size_t data_in_buffer = 0;
	for (uint16_t offset = 0; (uint16_t)(offset + 0x80); offset += 0x80)
	{
		ts_read_eeprom_page(ts_tis_buffer + data_in_buffer, offset);
		data_in_buffer += 0x80;
		for (size_t buffer_iterator = 0, sample_data_end = data_in_buffer - (data_in_buffer % 10); buffer_iterator != sample_data_end; buffer_iterator += 10)
		{
			uint16_t sample_identity = (uint16_t)ts_tis_buffer[buffer_iterator] | ((uint16_t)ts_tis_buffer[buffer_iterator + 1] << 8);
			if (!sample_identity)
			{
				ts_tis_buffer[0] = (uint8_t)identity;
				ts_tis_buffer[1] = (uint8_t)(identity >> 8);
				ts_tis_buffer[2] = (uint8_t)time;
				ts_tis_buffer[3] = (uint8_t)(time >> 8);
				ts_tis_buffer[4] = (uint8_t)(time >> 16);
				ts_tis_buffer[5] = (uint8_t)(time >> 24);
				ts_tis_buffer[6] = 0;
				ts_tis_buffer[7] = 0;
				ts_tis_buffer[8] = 0;
				ts_tis_buffer[9] = 0;
				ts_write_eeprom(10, ts_tis_buffer, offset - (uint16_t)(data_in_buffer - 0x80) + (uint16_t)buffer_iterator);
				return 0;//function successful
			}
		}
		for (uint8_t* move_destination = ts_tis_buffer, * move_source = ts_tis_buffer + (data_in_buffer - (data_in_buffer % 10)), * move_source_end = move_source + (data_in_buffer % 10); move_source != move_source_end; ++move_source, ++move_destination)
			*move_destination = *move_source;
		data_in_buffer = data_in_buffer % 10;
	}
	return 1;//error not enough space to store time information
}

uint8_t ts_add_time_end(uint16_t identity, uint32_t time)
{
	size_t data_in_buffer = 0;
	for (uint16_t offset = 0; (uint16_t)(offset + 0x80); offset += 0x80)
	{
		ts_read_eeprom_page(ts_tis_buffer + data_in_buffer, offset);
		data_in_buffer += 0x80;
		for (size_t buffer_iterator = 0, sample_data_end = data_in_buffer - (data_in_buffer % 10); buffer_iterator != sample_data_end; buffer_iterator += 10)
		{
			uint16_t sample_identity = (uint16_t)ts_tis_buffer[buffer_iterator] | ((uint16_t)ts_tis_buffer[buffer_iterator + 1] << 8);
			uint32_t sample_start_time = (uint32_t)ts_tis_buffer[buffer_iterator + 2] | ((uint32_t)ts_tis_buffer[buffer_iterator + 3] << 8) | ((uint32_t)ts_tis_buffer[buffer_iterator + 4] << 16) | ((uint32_t)ts_tis_buffer[buffer_iterator + 5] << 24);
			uint32_t sample_end_time = (uint32_t)ts_tis_buffer[buffer_iterator + 6] | ((uint32_t)ts_tis_buffer[buffer_iterator + 7] << 8) | ((uint32_t)ts_tis_buffer[buffer_iterator + 8] << 16) | ((uint32_t)ts_tis_buffer[buffer_iterator + 9] << 24);
			if (!sample_identity)
				return 1;//error trying to end time that does not have beginning
			if (sample_identity == identity && sample_start_time <= time && !sample_end_time)
			{
				ts_tis_buffer[0] = (uint8_t)time;
				ts_tis_buffer[1] = (uint8_t)(time >> 8);
				ts_tis_buffer[2] = (uint8_t)(time >> 16);
				ts_tis_buffer[3] = (uint8_t)(time >> 24);
				ts_write_eeprom(4, ts_tis_buffer, offset - (uint16_t)(data_in_buffer - 0x80) + (uint16_t)buffer_iterator + 6);
				return 0;//function successful
			}
		}
		for (uint8_t* move_destination = ts_tis_buffer, * move_source = ts_tis_buffer + (data_in_buffer - (data_in_buffer % 10)), * move_source_end = move_source + (data_in_buffer % 10); move_source != move_source_end; ++move_source, ++move_destination)
			*move_destination = *move_source;
		data_in_buffer = data_in_buffer % 10;
	}
	return 1;//error trying to end time that does not have beginning
}

uint8_t ts_query_info_by_identitys(size_t identity_count, uint16_t* identitys, uint32_t query_from, uint32_t query_to, uint32_t* stamp_count, uint32_t* total_time)
{
	uint32_t total_stamp_count = 0;
	uint32_t total_time_count = 0;
	size_t data_in_buffer = 0;
	for (uint16_t offset = 0; (uint16_t)(offset + 0x80); offset += 0x80)
	{
		ts_read_eeprom_page(ts_tis_buffer + data_in_buffer, offset);
		data_in_buffer += 0x80;
		for (size_t buffer_iterator = 0, sample_data_end = data_in_buffer - (data_in_buffer % 10); buffer_iterator != sample_data_end; buffer_iterator += 10)
		{
			uint16_t sample_identity = (uint16_t)ts_tis_buffer[buffer_iterator] | ((uint16_t)ts_tis_buffer[buffer_iterator + 1] << 8);
			uint32_t sample_start_time = (uint32_t)ts_tis_buffer[buffer_iterator + 2] | ((uint32_t)ts_tis_buffer[buffer_iterator + 3] << 8) | ((uint32_t)ts_tis_buffer[buffer_iterator + 4] << 16) | ((uint32_t)ts_tis_buffer[buffer_iterator + 5] << 24);
			uint32_t sample_end_time = (uint32_t)ts_tis_buffer[buffer_iterator + 6] | ((uint32_t)ts_tis_buffer[buffer_iterator + 7] << 8) | ((uint32_t)ts_tis_buffer[buffer_iterator + 8] << 16) | ((uint32_t)ts_tis_buffer[buffer_iterator + 9] << 24);
			if (!sample_identity)
			{
				*stamp_count = total_stamp_count;
				*total_time = total_time_count;
				return 0;//function successful
			}
			for (size_t identity_number = 0; identity_number != identity_count; ++identity_number)
				if (identitys[identity_number] == sample_identity && sample_start_time >= query_from && sample_end_time && sample_end_time <= query_to)
				{
					++total_stamp_count;
					total_time_count += sample_end_time - sample_start_time;
					identity_number = identity_count - 1;
				}
		}
		for (uint8_t* move_destination = ts_tis_buffer, *move_source = ts_tis_buffer + (data_in_buffer - (data_in_buffer % 10)), *move_source_end = move_source + (data_in_buffer % 10); move_source != move_source_end; ++move_source, ++move_destination)
			*move_destination = *move_source;
		data_in_buffer %= 10;
	}
	*stamp_count = total_stamp_count;
	*total_time = total_time_count;
	return 0;//function successful
}

#ifdef __cplusplus
}
#endif

#endif
