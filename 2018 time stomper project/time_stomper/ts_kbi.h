// Time Stomper keyboard interface
/*

#ifndef TS_KBI_H
#define TS_KBI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void ts_initialize_keyboard();
uint8_t ts_query_keyboard_state();
size_t ts_get_keyboard_input(size_t buffer_length, char* buffer, uint8_t abort_on_not_number_input, uint32_t time_out_ms);

void ts_initialize_keyboard()
{
	pinMode(PB12, INPUT);
	pinMode(PB13, INPUT);
	pinMode(PB14, INPUT);
	pinMode(PB15, INPUT);
}

uint8_t ts_query_keyboard_state()
{
	return (uint8_t)digitalRead(PB12) | ((uint8_t)digitalRead(PB13) << 1) | ((uint8_t)digitalRead(PB14) << 2) | ((uint8_t)digitalRead(PB15) << 3);
}

size_t ts_get_keyboard_input(size_t buffer_length, char* buffer, uint8_t abort_on_not_number_input, uint32_t time_out_ms)
{
	const char keyboard_layout[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	if (!buffer_length)
		return 0;
	*buffer = 0;
	size_t length = 0;
	if (time_out_ms)
	{
		time_out_ms += millis();
		while (length + 1 != buffer_length && time_out_ms > millis())
		{
			uint8_t input = ts_query_keyboard_state();
			if (input)
			{
				buffer[length] = keyboard_layout[ts_query_keyboard_state()];
				buffer[++length] = 0;
				while (time_out_ms > millis() && ts_query_keyboard_state())
					delay(16);
			}
			else
				delay(16);
		}
		return length;
	}
	else
	{
		while (length + 1 != buffer_length)
		{
			uint8_t input = ts_query_keyboard_state();
			if (input)
			{
				buffer[length] = keyboard_layout[ts_query_keyboard_state()];
				buffer[++length] = 0;
				while (ts_query_keyboard_state())
					delay(16);
			}
			else
				delay(16);
		}
		return length;
	}
}

#ifdef __cplusplus
}
#endif

#endif
*/
