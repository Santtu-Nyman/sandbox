#define _POSIX_C_SOURCE 199309L
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include "bcm2835.h"

const uint8_t r3i16_input_pins[16] = {  2, 3, 4, 5,
										6, 7, 8, 9,
										10, 11, 12, 13,
										14, 15, 16, 17 };

typedef struct r3i16_context_t
{
	size_t measurement_length;
} r3i16_context_t;

#define R3I16_CONTEXT_HEADER_SIZE ((sizeof(r3i16_context_t) + (sizeof(uintptr_t) - 1)) & ~(sizeof(uintptr_t) - 1))
#define R3I16_CONTEXT_SIZE(l) (R3I16_CONTEXT_HEADER_SIZE + (((uintptr_t)(l)) * (sizeof(uint16_t) + sizeof(float))))
#define R3I16_CONTEXT_GET_SIZE(c) (R3I16_CONTEXT_HEADER_SIZE + (((r3i16_context_t*)(c))->measurement_length * (sizeof(uint16_t) + sizeof(float))))
#define R3I16_CONTEXT_GET_RAW_MEASUREMENT_BUFFER(c) ((uint16_t*)(((uintptr_t)(c)) + (uintptr_t)R3I16_CONTEXT_HEADER_SIZE))
#define R3I16_CONTEXT_GET_MEASUREMENT_BUFFER(c) ((float*)(((uintptr_t)(c)) + (uintptr_t)R3I16_CONTEXT_HEADER_SIZE + (uintptr_t)(((r3i16_context_t*)(c))->measurement_length * sizeof(uint16_t))))

int r3i16_open(r3i16_context_t** context, size_t measurement_length);

void r3i16_close(r3i16_context_t* context);

static int r3i16_measure_no_warmup;
static int r3i16_measure_print_time;
static int r3i16_measure_lsb;

void r3i16_measure(r3i16_context_t* context);

int r3i16_save_file(const char* name, size_t size, const void* data);

int r3i16_open(r3i16_context_t** context, size_t measurement_length)
{
	r3i16_context_t* new_context = (r3i16_context_t*)malloc(R3I16_CONTEXT_SIZE(measurement_length));
	if (!new_context)
		return ENOMEM;
	new_context->measurement_length = measurement_length;
	if (!bcm2835_init())
	{
		free((void*)new_context);
		return EIO;
	}
	for (int i = 0; i != 16; ++i)
	{
		bcm2835_gpio_set_pud(r3i16_input_pins[i], BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_fsel(r3i16_input_pins[i], BCM2835_GPIO_FSEL_INPT);
	}
	*context = new_context;
	return 0;
}

int main(int argc, char** argv)
{
	const size_t length = 100 * 1024;
	for (int i = 0; i != argc; ++i)
                if (!strcmp(argv[i], "-c"))
			r3i16_measure_no_warmup = 1;
	for (int i = 0; i != argc; ++i)
                if (!strcmp(argv[i], "-l"))
                        r3i16_measure_lsb = 1;
	r3i16_context_t* context;
	int error = r3i16_open(&context, length);
	if (error)
	{
		printf("r3i16_open error %i\n", error);
		return error;
	}
	if (!r3i16_measure_no_warmup)
		for (int c = 4; c--;)
			r3i16_measure(context);
	r3i16_measure_print_time = 1;
	r3i16_measure(context);
	for (int i = 0; i != argc; ++i)
		if (!strcmp(argv[i], "-o") && i + 1 != argc)
		{
			error = r3i16_save_file(argv[i + 1], length * sizeof(float), R3I16_CONTEXT_GET_MEASUREMENT_BUFFER(context));
			if (error)
				printf("r3i16_save_file error %i\n", error);
			break;
		}
	r3i16_close(context);
	return 0;
}

void r3i16_close(r3i16_context_t* context)
{
	free((void*)context);
	bcm2835_close();
}

void r3i16_measure(r3i16_context_t* context)
{
	size_t measurement_length = context->measurement_length;
	uint16_t* raw_measurement_buffer = R3I16_CONTEXT_GET_RAW_MEASUREMENT_BUFFER(context);
	memset(raw_measurement_buffer, 0, measurement_length * sizeof(uint16_t));

	struct timespec timer_begin;
	struct timespec timer_end;
	clock_gettime(CLOCK_MONOTONIC, &timer_begin);

	for (uint16_t* i = raw_measurement_buffer, *l = i + measurement_length; i != l; ++i)
	{
		uint16_t s = 0;
		for (int j = 0; j != 16; ++j)
			s |= ((uint16_t)bcm2835_gpio_lev(r3i16_input_pins[j]) << j);
		*i = s;
	}

	clock_gettime(CLOCK_MONOTONIC, &timer_end);

	if (r3i16_measure_print_time)
		printf("measurements %lu in seconds %f\n",
			(unsigned long)measurement_length,
			((float)timer_end.tv_sec + ((float)timer_end.tv_nsec / 1000000000.0f)) - ((float)timer_begin.tv_sec + ((float)timer_begin.tv_nsec / 1000000000.0f)));

	if (!r3i16_measure_lsb)
	{
		float* measurement_buffer = R3I16_CONTEXT_GET_MEASUREMENT_BUFFER(context);
		for (uint16_t* i = raw_measurement_buffer, *l = i + measurement_length; i != l; ++i, ++measurement_buffer)
		{
			int s = (int)*i;
			int j = 16;
			while (j)
				if (s & (1 << (j - 1)))
					break;
				else
					--j;
			*measurement_buffer = (float)j;
		}
	}
	else
	{
		float* measurement_buffer = R3I16_CONTEXT_GET_MEASUREMENT_BUFFER(context);
		for (uint16_t* i = raw_measurement_buffer, *l = i + measurement_length; i != l; ++i, ++measurement_buffer)
			*measurement_buffer = (float)(*i & (uint16_t)1); 
	}
}

int r3i16_save_file(const char* name, size_t size, const void* data)
{
	int file = open(name, O_WRONLY | O_TRUNC | O_CREAT);
	if (file == -1)
		return errno;
	for (size_t written = 0; written != size;)
	{
		ssize_t write_result = write(file, (const void*)((uintptr_t)data + written), ((size - written) < (size_t)SSIZE_MAX) ? (size - written) : (size_t)SSIZE_MAX);
		if (write_result == -1)
		{
			int write_error = errno;
			if (write_error != EINTR)
			{
				unlink(name);
				close(file);
				return write_error;
			}
			write_result = 0;
		}
		written += (size_t)write_result;
	}
	if (fsync(file) == -1)
	{
		int flush_error = errno;
		unlink(name);
		close(file);
		return flush_error;
	}
	close(file);
	return 0;
}

