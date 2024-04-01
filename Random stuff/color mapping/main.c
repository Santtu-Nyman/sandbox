#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
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
	size_t intensity_row_jump = intensity_stride - ((size_t)width * sizeof(float));
	size_t image_row_jump = image_stride - ((size_t)width * sizeof(uint32_t));
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

void jet_color_map(int width, int height, size_t intensity_stride, const float* intensity, size_t image_stride, uint32_t* image)
{
	/*
		x >= 0.0
		x <= 1.0
		r >= 0.0
		r <= 255.0
		g >= 0.0
		g <= 255.0
		b >= 0.0
		b <= 255.0
		a = 255.0
		r = max(0.0, min(255.0, abs(x + -0.75) * -1020.0 + 382.5))
		g = max(0.0, min(255.0, abs(x +  -0.5) * -1020.0 + 382.5))
		b = max(0.0, min(255.0, abs(x + -0.25) * -1020.0 + 382.5))
		a = max(0.0, min(255.0, abs(x +   0.0) *     0.0 + 382.5)) = 255.0
	*/
	__m128i shuffle_control = _mm_cvtsi32_si128((int)0x0C000408);
	shuffle_control = _mm_insert_epi16(shuffle_control, (int)0x8080, (int)2);
	shuffle_control = _mm_insert_epi16(shuffle_control, (int)0x8080, (int)3);
	shuffle_control = _mm_shuffle_epi32(shuffle_control, (int)0x54); // { 8, 4, 0, 12, null, null, null, null, null, null, null, null, null, null, null, null }
	__m128 absolute_mask = _mm_castsi128_ps(_mm_cvtsi32_si128((int)0x7FFFFFFF)); // everything except sing bit set
	absolute_mask = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(absolute_mask), (int)0x00));
	__m128 offsets = _mm_castsi128_ps(_mm_cvtsi32_si128((int)0xBF400000));
	offsets = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(offsets), (int)0xBF00, (int)3));
	offsets = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(offsets), (int)0xBE80, (int)5)); // { -0.75f, -0.5f, -0.25f, 0.0f }
	__m128 multipliers = _mm_castsi128_ps(_mm_cvtsi32_si128((int)0xC47F0000));
	multipliers = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(multipliers), (int)0x40)); // { -1020.0f, -1020.0f, -1020.0f, 0.0f }
	__m128 increments = _mm_castsi128_ps(_mm_cvtsi32_si128((int)0x43BF4000));
	increments = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(increments), (int)0x00)); // { 382.5f, 382.5f, 382.5f, 382.5f }
	__m128 clamp_max = _mm_castsi128_ps(_mm_cvtsi32_si128((int)0x437F0000));
	clamp_max = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(clamp_max), (int)0x00)); // { 255.0f, 255.0f, 255.0f, 255.0f }
	__m128 clamp_min = _mm_setzero_ps();
	__m128 data = _mm_undefined_ps();
	size_t intensity_row_size = (size_t)width * sizeof(float);
	size_t intensity_row_jump = intensity_stride - intensity_row_size;
	size_t image_row_jump = image_stride - ((size_t)width * sizeof(uint32_t));
	for (const float* intensity_end = (const float*)((uintptr_t)intensity + ((size_t)height * intensity_stride));
		intensity != intensity_end;
		intensity = (const float*)((uintptr_t)intensity + intensity_row_jump), image = (uint32_t*)((uintptr_t)image + image_row_jump))
	{
		for (const float* intensity_row_end = (const float*)((uintptr_t)intensity + intensity_row_size); intensity != intensity_row_end; ++intensity, ++image)
		{
			data = _mm_load_ps1(intensity);
			data = _mm_add_ps(data, offsets);
			data = _mm_and_ps(data, absolute_mask);
			//data = _mm_mul_ps(data, multipliers);
			//data = _mm_add_ps(data, increments);
			data = _mm_fmadd_ps(data, multipliers, increments); /* FMA is needed for this */
			data = _mm_min_ps(data, clamp_max);
			data = _mm_max_ps(data, clamp_min);
			data = _mm_castsi128_ps(_mm_cvttps_epi32(data));
			data = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(data), shuffle_control)); /* SSSE3 is needed for this */
			_mm_storeu_si32((void*)image, _mm_castps_si128(data));
		}
	}
}

void map_color_of_map(int width, int height, size_t value_stride, float* value_map, size_t color_stride, uint32_t* color_map)
{
	uint64_t performance_frequency;
	uint64_t start_time;
	uint64_t end_time;
	QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
	for (int c = 100; c--;)
	{
		jet_color_map(width, height, value_stride, value_map, color_stride, color_map);
	}
	QueryPerformanceCounter((LARGE_INTEGER*)&end_time);
	printf("jet_color_map AVG: %f\n", ((double)(end_time - start_time) / (double)performance_frequency) / 100.0);

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

void squeeze_2d(int width, int height, size_t stride, float* map)
{
	float min_value = *map;
	float max_value = *map;
	float* row = map;
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
			//assert(row[j] >= -0.01f && row[j] <= 1.01f);
		}
		row = (float*)((uintptr_t)row + stride);
	}
}

int main(int argv, char** argc)
{
	const float tau = 6.28318530718f;

	
	return 0;
	
	/*
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
	*/
}