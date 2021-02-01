#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "ssn_display_window.h"

void vector_3d(float alpha, float beta, float length, float* x, float* z, float* y)
{
	float cos_alpha = cos(alpha);
	float cos_beta = cos(beta);
	float sin_alpha = sin(alpha);
	float sin_beta = sin(beta);

	*x = length * cos_alpha * cos_beta;
	*z = length * sin_alpha * cos_beta;
	*y = length * sin_beta;
}

/*
void map_process(int width, int height, size_t stride, float* map, float radius)
{
	const float tau = 6.28318530718f;
	const float half_circle = tau / 2.0f;
	const float quarter_circle = tau / 4.0f;
	const float minus_half_circle = -half_circle;
	const float minus_quarter_circle = -quarter_circle;

	float i_scale = half_circle / (float)(height - 1);
	float j_scale = tau / (float)(width - 1);
	float* data = map;
	for (int i = 0; i != height; ++i)
	{
		float altitude = i * i_scale + minus_quarter_circle;
		float sin_altitude = sinf(altitude);
		float cos_altitude = cosf(altitude);
		float y = radius * sin_altitude;
		for (int j = 0; j != width; ++j)
		{
			float azimuth = j * j_scale + minus_half_circle;
			float sin_azimuth = sinf(altitude);
			float cos_azimuth = cosf(altitude);
			float x = radius * cos_azimuth * cos_altitude;
			float z = radius * sin_azimuth * cos_altitude;

			do something
		}
		data = (float*)((uintptr_t)data + stride);
	}
}
*/

#include <emmintrin.h>
#include <xmmintrin.h>
#include <tmmintrin.h>
#include <immintrin.h>
void test_color_map(int width, int height, size_t intensity_stride, const float* intensity_data, size_t image_stride, uint32_t* image_data)
{
	/* maps 0 -> 1 values to colors blue -> black -> red (SSE2 required) not too bad implementation */
	__m128i multiplier = _mm_cvtsi32_si128((int)0xC3FF0000);
	multiplier = _mm_shuffle_epi32(multiplier, 0);
	__m128i increment = _mm_cvtsi32_si128((int)0x437F0000);
	increment = _mm_shuffle_epi32(increment, 0);
	__m128i absolute_mask = _mm_cvtsi32_si128((int)0x7FFFFFFF);
	absolute_mask = _mm_shuffle_epi32(absolute_mask, 0);
	__m128i opaque_alpha = _mm_cvtsi32_si128((int)0xFF000000);
	opaque_alpha = _mm_shuffle_epi32(opaque_alpha, 0);
	size_t intensity_row_jump = ((size_t)width * sizeof(float)) - intensity_stride;
	size_t image_row_jump = ((size_t)width * sizeof(uint32_t)) - image_stride;
	for (int y = 0; y != height; ++y)
	{
		const float* intensity_data_end = intensity_data + (width & ~0x3);
		while (intensity_data != intensity_data_end)
		{
			__m128i select = _mm_castps_si128(_mm_loadu_ps(intensity_data));
			intensity_data += 4;
			__m128i blue = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(select), _mm_castsi128_ps(multiplier)));
			blue = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(blue), _mm_castsi128_ps(increment)));
			select = _mm_srai_epi32(blue, 31);
			blue = _mm_and_si128(blue, absolute_mask);
			blue = _mm_cvttps_epi32(_mm_castsi128_ps(blue));
			__m128i red = _mm_slli_epi32(blue, 16);
			blue = _mm_andnot_si128(select, blue);
			red = _mm_and_si128(select, red);
			select = _mm_or_si128(blue, red);
			select = _mm_or_si128(select, opaque_alpha);
			_mm_storeu_si128((__m128i*)image_data, select);
			image_data += 4;
		}
		intensity_data_end = intensity_data + (width & 0x3);
		while (intensity_data != intensity_data_end)
		{
			__m128i select = _mm_castps_si128(_mm_load_ss(intensity_data));
			++intensity_data;
			__m128i blue = _mm_castps_si128(_mm_mul_ss(_mm_castsi128_ps(select), _mm_castsi128_ps(multiplier)));
			blue = _mm_castps_si128(_mm_add_ss(_mm_castsi128_ps(blue), _mm_castsi128_ps(increment)));
			select = _mm_srai_epi32(blue, 31);
			blue = _mm_and_si128(blue, absolute_mask);
			blue = _mm_cvttps_epi32(_mm_castsi128_ps(blue));
			__m128i red = _mm_slli_epi32(blue, 16);
			blue = _mm_andnot_si128(select, blue);
			red = _mm_and_si128(select, red);
			select = _mm_or_si128(blue, red);
			select = _mm_or_si128(select, opaque_alpha);
			_mm_storeu_si32((__m128i*)image_data, select);
			++image_data;
		}
		intensity_data = (const float*)((uintptr_t)intensity_data + intensity_row_jump);
		image_data = (uint32_t*)((uintptr_t)image_data + image_row_jump);
	}
	/*
	float x = 0;
	float b = fabsf(x * -510.0f + 255.0f);
	float r = b;//x * 510.0f + -255.0f;
	uint32_t color;
	if (x < 0.5)
		color = 0xFF000000 | ((uint32_t)(b) << 0);
	else
		color = 0xFF000000 | ((uint32_t)(r) << 16);
	return color;
	*/
}

void map_color_of_map(int width, int height, size_t value_stride, float* value_map, size_t color_stride, uint32_t* color_map)
{
	test_color_map(width, height, value_stride, value_map, color_stride, color_map);
	/*
	float* value_row = value_map;
	uint32_t* color_row = color_map;
	for (int i = 0; i != height; ++i)
	{
		for (int j = 0; j != width; ++j)
		{
			color_row[j] = test_color_map(value_row[j]);
		}
		value_row = (float*)((uintptr_t)value_row + value_stride);
		color_row = (uint32_t*)((uintptr_t)color_row + color_stride);
	}
	*/
}

void vd_map_process(int width, int height, size_t stride, float* map, float radius, float vector_x, float vector_y, float vector_z)
{
	const float tau = 6.28318530718f;
	const float half_circle = tau / 2.0f;
	const float quarter_circle = tau / 4.0f;
	const float minus_half_circle = -half_circle;
	const float minus_quarter_circle = -quarter_circle;

	float i_scale = half_circle / (float)(height - 1);
	float j_scale = tau / (float)(width - 1);
	float* row = map;
	for (int i = 0; i != height; ++i)
	{
		float altitude = i * i_scale + minus_quarter_circle;
		float sin_altitude = sinf(altitude);
		float cos_altitude = cosf(altitude);
		float y = radius * sin_altitude;
		for (int j = 0; j != width; ++j)
		{
			float azimuth = j * j_scale + minus_half_circle;
			float sin_azimuth = sinf(azimuth);
			float cos_azimuth = cosf(azimuth);
			float x = radius * cos_azimuth * cos_altitude;
			float z = radius * sin_azimuth * cos_altitude;

			float distance = sqrtf((vector_x - x) * (vector_x - x) + (vector_y - y) * (vector_y - y) + (vector_z - z) * (vector_z - z));

			row[j] = -distance;
		}
		row = (float*)((uintptr_t)row + stride);
	}

	float min_value = *map;
	float max_value = *map;
	row = map;
	for (int i = 0; i != height; ++i)
	{
		for (int j = 0; j != width; ++j)
		{
			float value = row[j];
			if (value < min_value)
				min_value = value;
			if (value > max_value)
				max_value = value;
		}
		row = (float*)((uintptr_t)row + stride);
	}

	float scaling_multiplier = 1.0f / (max_value - min_value);
	float scaling_increment = scaling_multiplier * -min_value;
	row = map;
	for (int i = 0; i != height; ++i)
	{
		for (int j = 0; j != width; ++j)
		{
			float value = row[j];
			row[j] = value * scaling_multiplier + scaling_increment;
		}
		row = (float*)((uintptr_t)row + stride);
	}
}

int main(int argv, char** argc)
{
	const float tau = 6.28318530718f;

	for (;;)
	{
		float input_alpha;
		float input_beta;
		float input_length;
		scanf("%f %f %f", &input_alpha, &input_beta, &input_length);

		float alpha = ((double)input_alpha * tau) / 360.0f;
		float beta = ((double)input_beta * tau) / 360.0f;
		float length = (double)input_length;
		float x;
		float y;
		float z;
		vector_3d(alpha, beta, length, &x, &y, &z);

		printf("a:%f, b:%f, l:%f = x:%f, y:%f, z:%f\n", (alpha * 360.0f) / tau, (beta * 360.0f) / tau, length, x, y, z);

		int map_w = 640;
		int map_h = 480;
		float* test_map = (float*)malloc(map_h * map_w * sizeof(float));
		uint32_t* map_image = (uint32_t*)malloc(map_h * map_w * sizeof(uint32_t));

		vd_map_process(map_w, map_h, map_w * sizeof(float), test_map, 1.0f, x, y, z);
		map_color_of_map(map_w, map_h, map_w * sizeof(float), test_map, map_w * sizeof(uint32_t), map_image);

		ssn_display_window("color map projection", map_w, map_h, map_image);

		free(map_image);
		free(test_map);
	}

	return 0;
}