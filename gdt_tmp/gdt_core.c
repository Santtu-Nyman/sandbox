/*
	Graph Drawing Tool 1.1.0 2019-07-22 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#include "gdt_core.h"
#ifdef _MSC_VER
#pragma float_control(except, off, push)
#define GDT_CORE_ASSUME(x) __assume(x)
#endif
#ifdef __GNUC__
#define GDT_CORE_ASSUME(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#endif
#ifndef GDT_NO_SSSE3
#if (defined(_M_X64) || defined(_M_IX86) || defined(__i386__) || defined(__x86_64__))
#define GDT_SSSE3
#include <emmintrin.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <tmmintrin.h>
#endif
#endif
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct gdt_iv2_t
{
	int x;
	int y;
};

static void gdt_fill_bitmap(int width, int height, size_t stride, uint32_t* pixels, uint32_t color)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0);
#ifdef GDT_SSSE3
	__m128i color_block = _mm_cvtsi32_si128(color);
	color_block = _mm_shuffle_epi32(color_block, 0);
	size_t block_fill = (((size_t)width << 2) & (size_t)~15);
	int end_fill = (int)(((size_t)width << 2) - block_fill);
	GDT_CORE_ASSUME(end_fill < 16);
	size_t skip = stride - (block_fill + (size_t)end_fill);
	uintptr_t write = (uintptr_t)pixels;
	uintptr_t write_end = write + ((size_t)height * stride);
	if (!(write & (uintptr_t)~15) && !(stride & (uintptr_t)~15))
		while (write != write_end)
		{
			for (uintptr_t loop_end = write + block_fill; write != loop_end; write += 16)
				_mm_store_si128((__m128i*)write, color_block);
			for (uintptr_t loop_end = write + end_fill; write != loop_end; write += 4)
				_mm_store_ss((float*)write, _mm_castsi128_ps(color_block));
			write += skip;
		}
	else
		while (write != write_end)
		{
			for (uintptr_t loop_end = write + block_fill; write != loop_end; write += 16)
				_mm_storeu_si128((__m128i*)write, color_block);
			for (uintptr_t loop_end = write + end_fill; write != loop_end; write += 4)
				_mm_store_ss((float*)write, _mm_castsi128_ps(color_block));
			write += skip;
		}
#else
	int skip = (int)(stride / sizeof(uint32_t)) - width;
	for (uint32_t* i = pixels, *l = (uint32_t*)((uintptr_t)pixels + (height * stride)); i != l; i += skip)
		for (uint32_t* r = i + width; i != r; ++i)
			*i = color;
#endif
}

static void gdt_draw_dot_to_bitmap(int width, int height, size_t stride, uint32_t* pixels, uint32_t dot_color, int dot_radius, int dot_x, int dot_y)
{
	GDT_CORE_ASSUME(width > -1 && height > -1 && dot_radius > -1 && (uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0);
	if (width < 1 || height < 1 || dot_radius < 1 || dot_x + dot_radius <= 0 || dot_y + dot_radius <= 0 || dot_x >= width || dot_y >= height)
		return;
	int offset_x = (dot_x - dot_radius > 0) ? -dot_radius : -dot_x;
	int offset_y = (dot_y - dot_radius > 0) ? -dot_radius : -dot_y;
	int end_x = (dot_x + dot_radius < width) ? dot_radius : (dot_radius - ((dot_x + dot_radius) - width));
	int end_y = (dot_y + dot_radius < height) ? dot_radius : (dot_radius - ((dot_y + dot_radius) - height));
#ifdef GDT_SSSE3
	width = end_x - offset_x;
	height = end_y - offset_y;
	uint32_t* square = (uint32_t*)((uintptr_t)pixels + (size_t)(dot_y + offset_y) * stride + (size_t)(dot_x + offset_x) * sizeof(uint32_t));
	size_t block_fill = (((size_t)width << 2) & (size_t)~15);
	int end_fill = (int)(((size_t)width << 2) - block_fill);
	GDT_CORE_ASSUME(end_fill < 16);
	size_t skip = stride - (block_fill + (size_t)end_fill);
	uintptr_t iterator = (uintptr_t)square;
	uintptr_t end = iterator + ((size_t)height * stride);
	__m128 row_x = _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000));
	__m128 tmp = _mm_castsi128_ps(_mm_cvtsi32_si128(dot_radius));
	tmp = _mm_cvtepi32_ps(_mm_castps_si128(tmp));
	tmp = _mm_mul_ss(tmp, tmp);
	tmp = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(tmp), _mm_castps_si128(row_x)));
	int minus_r_squared = _mm_cvtsi128_si32(_mm_castps_si128(tmp));
	__m128i color_block = _mm_cvtsi32_si128(dot_color);
	color_block = _mm_shuffle_epi32(color_block, 0);
	__m128 block_increment = _mm_castsi128_ps(_mm_setzero_si128());
	block_increment = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(block_increment), 0x3F80, 3));
	block_increment = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(block_increment), 0x4000, 5));
	block_increment = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(block_increment), 0x4040, 7));
	__m128 row_offset_x = _mm_castsi128_ps(_mm_cvtsi32_si128(offset_x));
	row_offset_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(row_offset_x), 0));
	row_offset_x = _mm_cvtepi32_ps(_mm_castps_si128(row_offset_x));
	row_offset_x = _mm_add_ps(row_offset_x, block_increment);
	block_increment = _mm_castsi128_ps(_mm_cvtsi32_si128(0x40800000));
	block_increment = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(block_increment), 0));
	__m128 column_y = _mm_castsi128_ps(_mm_cvtsi32_si128(offset_y));
	column_y = _mm_cvtepi32_ps(_mm_castps_si128(column_y));
	column_y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(column_y), 0));
	__m128 y_squared_minus_r_squared;
	__m128i block_blend;
	if (!(iterator & (uintptr_t)~15) && !(stride & (uintptr_t)~15))
		while (iterator != end)
		{
			row_x = row_offset_x;
			tmp = _mm_castsi128_ps(_mm_cvtsi32_si128(minus_r_squared));
			tmp = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0));
			y_squared_minus_r_squared = _mm_mul_ps(column_y, column_y);
			y_squared_minus_r_squared = _mm_add_ps(y_squared_minus_r_squared, tmp);
			for (uintptr_t loop_end = iterator + block_fill; iterator != loop_end; iterator += 16)
			{
				block_blend = _mm_load_si128((const __m128i*)iterator);
				tmp = _mm_mul_ps(row_x, row_x);
				tmp = _mm_add_ps(y_squared_minus_r_squared, tmp);
				tmp = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(tmp), 31));
				block_blend = _mm_andnot_si128(_mm_castps_si128(tmp), block_blend);
				tmp = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(tmp), color_block));
				block_blend = _mm_or_si128(block_blend, _mm_castps_si128(tmp));
				_mm_store_si128((__m128i*)iterator, block_blend);
				row_x = _mm_add_ps(row_x, block_increment);
			}
			tmp = _mm_mul_ps(row_x, row_x);
			tmp = _mm_add_ps(y_squared_minus_r_squared, tmp);
			tmp = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(tmp), 31));
			for (uintptr_t loop_end = iterator + end_fill; iterator != loop_end; iterator += 4)
			{
				block_blend = _mm_castps_si128(_mm_load_ss((const float*)iterator));
				row_x = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(tmp), color_block));
				block_blend = _mm_andnot_si128(_mm_castps_si128(tmp), block_blend);
				block_blend = _mm_or_si128(block_blend, _mm_castps_si128(row_x));
				_mm_store_ss((float*)iterator, _mm_castsi128_ps(block_blend));
				tmp = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0x39));
			}
			iterator += skip;
			tmp = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F800000));
			tmp = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0));
			column_y = _mm_add_ps(column_y, tmp);
		}
	else
		while (iterator != end)
		{
			row_x = row_offset_x;
			tmp = _mm_castsi128_ps(_mm_cvtsi32_si128(minus_r_squared));
			tmp = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0));
			y_squared_minus_r_squared = _mm_mul_ps(column_y, column_y);
			y_squared_minus_r_squared = _mm_add_ps(y_squared_minus_r_squared, tmp);
			for (uintptr_t loop_end = iterator + block_fill; iterator != loop_end; iterator += 16)
			{
				block_blend = _mm_loadu_si128((const __m128i*)iterator);
				tmp = _mm_mul_ps(row_x, row_x);
				tmp = _mm_add_ps(y_squared_minus_r_squared, tmp);
				tmp = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(tmp), 31));
				block_blend = _mm_andnot_si128(_mm_castps_si128(tmp), block_blend);
				tmp = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(tmp), color_block));
				block_blend = _mm_or_si128(block_blend, _mm_castps_si128(tmp));
				_mm_storeu_si128((__m128i*)iterator, block_blend);
				row_x = _mm_add_ps(row_x, block_increment);
			}
			tmp = _mm_mul_ps(row_x, row_x);
			tmp = _mm_add_ps(y_squared_minus_r_squared, tmp);
			tmp = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(tmp), 31));
			for (uintptr_t loop_end = iterator + end_fill; iterator != loop_end; iterator += 4)
			{
				block_blend = _mm_castps_si128(_mm_load_ss((const float*)iterator));
				row_x = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(tmp), color_block));
				block_blend = _mm_andnot_si128(_mm_castps_si128(tmp), block_blend);
				block_blend = _mm_or_si128(block_blend, _mm_castps_si128(row_x));
				_mm_store_ss((float*)iterator, _mm_castsi128_ps(block_blend));
				tmp = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0x39));
			}
			tmp = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F800000));
			tmp = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp), 0));
			column_y = _mm_add_ps(column_y, tmp);
			iterator += skip;
		}
#else
	int minus_r_squared = -(dot_radius * dot_radius);
	uint32_t* dot_center = (uint32_t*)((uintptr_t)pixels + (size_t)dot_y * stride + (size_t)dot_x * sizeof(uint32_t));
	for (int y = offset_y; y != end_y; ++y)
	{
		uint32_t* row = (uint32_t*)((uintptr_t)dot_center + (size_t)y * stride);
		for (int y_squared_minus_r_squared = y * y + minus_r_squared, x = offset_x; x != end_x; ++x)
		{
			uint32_t blend = row[x];
			uint32_t mask = 0 - ((uint32_t)(x * x + y_squared_minus_r_squared) >> 31);
			GDT_CORE_ASSUME(mask == 0 || mask == 0xFFFFFFFF);
			row[x] = (mask & dot_color) | (blend & (mask ^ 0xFFFFFFFF));
		}
	}
#endif
}

static void gdt_draw_line_to_bitmap(int width, int height, size_t stride, uint32_t* pixels, uint32_t color, int line_thickness, struct gdt_iv2_t begin_point, struct gdt_iv2_t end_point)
{
	GDT_CORE_ASSUME(width > -1 && height > -1 && line_thickness > -1 && (uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0);
	if (width < 1 || height < 1 || line_thickness < 1)
		return;
	line_thickness /= 2;
	if (end_point.x < begin_point.x)
	{
		struct gdt_iv2_t swap = begin_point;
		begin_point = end_point;
		end_point = swap;
	}
	struct gdt_iv2_t left_bottom = { begin_point.x - line_thickness, (begin_point.y < end_point.y ? begin_point.y : end_point.y) - line_thickness };
	if (left_bottom.x < 0)
		left_bottom.x = 0;
	else if (left_bottom.x > width)
		left_bottom.x = width;
	if (left_bottom.y < 0)
		left_bottom.y = 0;
	else if (left_bottom.y > height)
		left_bottom.y = height;
	struct gdt_iv2_t rigth_top = { end_point.x + line_thickness, (begin_point.y > end_point.y ? begin_point.y : end_point.y) + line_thickness };
	if (rigth_top.x < 0)
		rigth_top.x = 0;
	else if (rigth_top.x > width)
		rigth_top.x = width;
	if (rigth_top.y < 0)
		rigth_top.y = 0;
	else if (rigth_top.y > height)
		rigth_top.y = height;
	float r = (float)line_thickness;
	float line_length = sqrtf((float)((end_point.x - begin_point.x) * (end_point.x - begin_point.x) + (end_point.y - begin_point.y) * (end_point.y - begin_point.y)));
	float line_angle = asinf((float)(end_point.y - begin_point.y) / line_length);
	float rotate_multipliers[2] = { cosf(line_angle), -sinf(line_angle) };
#ifdef GDT_SSSE3
	width = rigth_top.x - left_bottom.x;
	height = rigth_top.y - left_bottom.y;
	size_t block_fill = (((size_t)width << 2) & (size_t)~15);
	int end_fill = (int)(((size_t)width << 2) - block_fill);
	GDT_CORE_ASSUME(end_fill < 16);
	size_t skip = stride - (block_fill + (size_t)end_fill);
	uintptr_t iterator = (uintptr_t)pixels + ((size_t)left_bottom.y * stride) + ((size_t)left_bottom.x * sizeof(uint32_t));
	uintptr_t end = iterator + ((size_t)height * stride);
	int radius = *(const int*)&r;
	int length = *(const int*)&line_length;
	__m128 rotate_multiplier_x = _mm_load_ss(rotate_multipliers);
	__m128 rotate_multiplier_y = _mm_load_ss(rotate_multipliers + 1);
	rotate_multiplier_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(rotate_multiplier_x), 0));
	rotate_multiplier_y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(rotate_multiplier_y), 0));
	__m128 mask = _mm_castsi128_ps(_mm_setzero_si128());
	mask = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(mask), 0x3F80, 3));
	mask = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(mask), 0x4000, 5));
	mask = _mm_castsi128_ps(_mm_insert_epi16(_mm_castps_si128(mask), 0x4040, 7));
	__m128 relative_x_offsets = _mm_castsi128_ps(_mm_cvtsi32_si128(left_bottom.x - begin_point.x));
	relative_x_offsets = _mm_cvtepi32_ps(_mm_castps_si128(relative_x_offsets));
	relative_x_offsets = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(relative_x_offsets), 0));
	relative_x_offsets = _mm_add_ps(relative_x_offsets, mask);
	__m128 relative_y = _mm_castsi128_ps(_mm_cvtsi32_si128(left_bottom.y - begin_point.y));
	relative_y = _mm_cvtepi32_ps(_mm_castps_si128(relative_y));
	relative_y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(relative_y), 0));
	__m128 relative_x;
	__m128 tmp_x;
	__m128 tmp_y;
	while (iterator != end)
	{
		relative_x = relative_x_offsets;
		for (uintptr_t loop_end = iterator + block_fill; iterator != loop_end; iterator += 16)
		{
			tmp_x = _mm_mul_ps(rotate_multiplier_x, relative_x);
			tmp_y = _mm_mul_ps(rotate_multiplier_y, relative_y);
			tmp_x = _mm_sub_ps(tmp_x, tmp_y);
			tmp_y = _mm_castsi128_ps(_mm_cvtsi32_si128(length));
			tmp_y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_y), 0));
			mask = _mm_cmple_ps(tmp_x, tmp_y);
			tmp_y = _mm_castsi128_ps(_mm_setzero_si128());
			tmp_y = _mm_cmpge_ps(tmp_x, tmp_y);
			mask = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_y)));
			tmp_x = _mm_mul_ps(rotate_multiplier_x, relative_y);
			tmp_y = _mm_mul_ps(rotate_multiplier_y, relative_x);
			tmp_y = _mm_add_ps(tmp_x, tmp_y);
			tmp_x = _mm_castsi128_ps(_mm_cvtsi32_si128(0x7FFFFFFF));
			tmp_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_x), 0));
			tmp_y = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(tmp_y), _mm_castps_si128(tmp_x)));
			tmp_x = _mm_castsi128_ps(_mm_cvtsi32_si128(radius));
			tmp_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_x), 0));
			tmp_y = _mm_cmplt_ps(tmp_y, tmp_x);
			mask = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_y)));
			tmp_x = _mm_castsi128_ps(_mm_loadu_si128((const __m128i*)iterator));
			tmp_y = _mm_castsi128_ps(_mm_cvtsi32_si128(color));
			tmp_y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_y), 0));
			tmp_x = _mm_castsi128_ps(_mm_andnot_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_x)));
			tmp_y = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_y)));
			mask = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(tmp_x), _mm_castps_si128(tmp_y)));
			_mm_storeu_si128((__m128i*)iterator, _mm_castps_si128(mask));
			tmp_x = _mm_castsi128_ps(_mm_cvtsi32_si128(0x40800000));
			tmp_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_x), 0));
			relative_x = _mm_add_ps(relative_x, tmp_x);
		}
		tmp_x = _mm_mul_ps(rotate_multiplier_x, relative_x);
		tmp_y = _mm_mul_ps(rotate_multiplier_y, relative_y);
		tmp_x = _mm_sub_ps(tmp_x, tmp_y);
		tmp_y = _mm_castsi128_ps(_mm_cvtsi32_si128(length));
		tmp_y = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_y), 0));
		mask = _mm_cmple_ps(tmp_x, tmp_y);
		tmp_y = _mm_castsi128_ps(_mm_setzero_si128());
		tmp_y = _mm_cmpge_ps(tmp_x, tmp_y);
		mask = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_y)));
		tmp_x = _mm_mul_ps(rotate_multiplier_x, relative_y);
		tmp_y = _mm_mul_ps(rotate_multiplier_y, relative_x);
		tmp_y = _mm_add_ps(tmp_x, tmp_y);
		tmp_x = _mm_castsi128_ps(_mm_cvtsi32_si128(0x7FFFFFFF));
		tmp_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_x), 0));
		tmp_y = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(tmp_y), _mm_castps_si128(tmp_x)));
		tmp_x = _mm_castsi128_ps(_mm_cvtsi32_si128(radius));
		tmp_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_x), 0));
		tmp_y = _mm_cmplt_ps(tmp_y, tmp_x);
		mask = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_y)));
		for (uintptr_t loop_end = iterator + end_fill; iterator != loop_end; iterator += 4)
		{
			tmp_x = _mm_load_ss((const float*)iterator);
			tmp_y = _mm_castsi128_ps(_mm_cvtsi32_si128(color));
			tmp_x = _mm_castsi128_ps(_mm_andnot_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_x)));
			tmp_y = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(mask), _mm_castps_si128(tmp_y)));
			tmp_x = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(tmp_x), _mm_castps_si128(tmp_y)));
			_mm_store_ss((float*)iterator, tmp_x);
			mask = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(mask), 0x39));
		}
		tmp_x = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F800000));
		tmp_x = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp_x), 0));
		relative_y = _mm_add_ps(relative_y, tmp_x);
		iterator += skip;
	}
#else
	for (int y = left_bottom.y; y != rigth_top.y; ++y)
	{
		uint32_t* row = (uint32_t*)((uintptr_t)pixels + (size_t)y * stride);
		for (int x = left_bottom.x; x != rigth_top.x; ++x)
		{
			float r_x = (float)(x - begin_point.x);
			float r_y = (float)(y - begin_point.y);
			float rr_x = rotate_multipliers[0] * r_x - rotate_multipliers[1] * r_y;
			float rr_y = rotate_multipliers[0] * r_y + rotate_multipliers[1] * r_x;
#if (defined(_MSC_VER) && defined(_MT) && !defined(_DLL)) || defined(__GNUC__)
			rr_y = fabsf(rr_y);
#else
			if (rr_y < 0.0f)
				rr_y = -rr_y;
#endif
			if (rr_x >= 0.0f && rr_x <= line_length && rr_y < r)
				row[x] = color;
		}
	}
#endif
}

static float gdt_round(int direction, float granularity, float value)
{
	GDT_CORE_ASSUME((direction == 0 || direction == 1) && value != NAN && value != INFINITY && granularity != NAN && granularity != INFINITY);
	float flip = direction ? -1.0f : 1.0f;
	if (value < 0.0f)
	{
		float tmp = fmodf(-value, granularity);
		if (tmp > 0.0f)
			tmp = (-value - tmp) + (direction ? 0 : granularity);
		else
			tmp = -value;
		return -tmp;
	}
	else
	{
		float tmp = fmodf(value, granularity);
		if (tmp > 0.0f)
			tmp = (value - tmp) + (direction ? granularity : 0);
		else
			tmp = value;
		return tmp;
	}
}

static void gdt_fast_multilevel_blur_bitmap(int width, int height, size_t stride, uint32_t* pixels, uint32_t* temporal_buffer, int level)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && (uintptr_t)temporal_buffer % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1 && level > -1);
	size_t temporal_buffer_stride = (size_t)height * sizeof(uint32_t);
#ifdef GDT_SSSE3
	__m128i decode_shuffle = _mm_cvtsi32_si128(0x80808003);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8002, 2);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8080, 3);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8001, 4);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8080, 5);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8000, 6);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8080, 7);
	__m128i encode_shuffle = _mm_cvtsi32_si128(0x0004080C);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 2);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 3);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 4);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 5);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 6);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 7);
	__m128i magic = _mm_cvtsi32_si128(0x00002AA9);// ((((3 * 0xFF) * 0x2AA9) >> 14) + 1) >> 1 = 0xFF
	magic = _mm_shuffle_epi32(magic, 0);
	__m128i previous;
	__m128i current;
	__m128i next;
	__m128i tmp;
	__m128i buffer;
#endif
	for (int repeat_two_times = 2; repeat_two_times--;)
	{
#ifdef GDT_SSSE3
		if (width > 1)
			for (int y = 0; y != height; ++y)
			{
				uint32_t* row = (uint32_t*)((uintptr_t)pixels + (size_t)y * stride);
				for (int c = level; c--;)
				{
					current = _mm_castps_si128(_mm_load_ss((const float*)row));
					next = _mm_castps_si128(_mm_load_ss((const float*)(row + 1)));
					current = _mm_shuffle_epi8(current, decode_shuffle);
					next = _mm_shuffle_epi8(next, decode_shuffle);
					tmp = _mm_add_epi32(current, current);
					tmp = _mm_add_epi32(tmp, next);
					tmp = _mm_mulhrs_epi16(magic, tmp);
					buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
					_mm_store_ss((float*)row, _mm_castsi128_ps(buffer));
					for (int x = 1; x != width - 1; ++x)
					{
						previous = current;
						current = next;
						next = _mm_castps_si128(_mm_load_ss((const float*)(row + x + 1)));
						next = _mm_shuffle_epi8(next, decode_shuffle);
						tmp = _mm_add_epi32(previous, current);
						tmp = _mm_add_epi32(tmp, next);
						tmp = _mm_mulhrs_epi16(magic, tmp);
						buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
						_mm_store_ss((float*)(row + x), _mm_castsi128_ps(buffer));
					}
					tmp = _mm_add_epi32(next, next);
					tmp = _mm_add_epi32(tmp, current);
					tmp = _mm_mulhrs_epi16(magic, tmp);
					buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
					_mm_store_ss((float*)(row + width - 1), _mm_castsi128_ps(buffer));
				}
			}
#else
		float previous[4];
		float current[4];
		float next[4];
		float tmp[4];
		if (width > 1)
			for (int y = 0; y != height; ++y)
			{
				uint32_t* row = (uint32_t*)((uintptr_t)pixels + (size_t)y * stride);
				for (int c = level; c--;)
				{
					uint32_t row_current = row[0];
					uint32_t row_next = row[1];
					current[0] = (float)((row_current >> 0x18) & 0xff);
					current[1] = (float)((row_current >> 0x10) & 0xff);
					current[2] = (float)((row_current >> 0x08) & 0xff);
					current[3] = (float)((row_current >> 0x00) & 0xff);
					next[0] = (float)((row_next >> 0x18) & 0xff);
					next[1] = (float)((row_next >> 0x10) & 0xff);
					next[2] = (float)((row_next >> 0x08) & 0xff);
					next[3] = (float)((row_next >> 0x00) & 0xff);
					tmp[0] = 0.33333333333f * (current[0] + current[0] + next[0]);
					tmp[1] = 0.33333333333f * (current[1] + current[1] + next[1]);
					tmp[2] = 0.33333333333f * (current[2] + current[2] + next[2]);
					tmp[3] = 0.33333333333f * (current[3] + current[3] + next[3]);
					row[0] =
						((uint32_t)tmp[0] << 0x18) |
						((uint32_t)tmp[1] << 0x10) |
						((uint32_t)tmp[2] << 0x08) |
						((uint32_t)tmp[3] << 0x00);
					for (int x = 1; x != width - 1; ++x)
					{
						previous[0] = current[0];
						previous[1] = current[1];
						previous[2] = current[2];
						previous[3] = current[3];
						current[0] = next[0];
						current[1] = next[1];
						current[2] = next[2];
						current[3] = next[3];
						row_next = row[x + 1];
						next[0] = (float)((row_next >> 0x18) & 0xff);
						next[1] = (float)((row_next >> 0x10) & 0xff);
						next[2] = (float)((row_next >> 0x08) & 0xff);
						next[3] = (float)((row_next >> 0x00) & 0xff);
						tmp[0] = 0.33333333333f * (previous[0] + current[0] + next[0]);
						tmp[1] = 0.33333333333f * (previous[1] + current[1] + next[1]);
						tmp[2] = 0.33333333333f * (previous[2] + current[2] + next[2]);
						tmp[3] = 0.33333333333f * (previous[3] + current[3] + next[3]);
						row[x] =
							((uint32_t)tmp[0] << 0x18) |
							((uint32_t)tmp[1] << 0x10) |
							((uint32_t)tmp[2] << 0x08) |
							((uint32_t)tmp[3] << 0x00);
					}
					tmp[0] = 0.33333333333f * (current[0] + next[0] + next[0]);
					tmp[1] = 0.33333333333f * (current[1] + next[1] + next[1]);
					tmp[2] = 0.33333333333f * (current[2] + next[2] + next[2]);
					tmp[3] = 0.33333333333f * (current[3] + next[3] + next[3]);
					row[width - 1] =
						((uint32_t)tmp[0] << 0x18) |
						((uint32_t)tmp[1] << 0x10) |
						((uint32_t)tmp[2] << 0x08) |
						((uint32_t)tmp[3] << 0x00);
				}
			}
#endif
		uint32_t* rotate_source = pixels;
		uint32_t* rotate_destination = temporal_buffer;
		for (int x = 0; x != width; ++x)
		{
			for (int y = 0; y != height; ++y)
				rotate_destination[y] = *(uint32_t*)((uintptr_t)rotate_source + (size_t)y * stride);
			rotate_source = (uint32_t*)((uintptr_t)rotate_source + sizeof(uint32_t));
			rotate_destination = (uint32_t*)((uintptr_t)rotate_destination + temporal_buffer_stride);
		}
		size_t stride_swap = stride;
		stride = temporal_buffer_stride;
		temporal_buffer_stride = stride_swap;
		int dimension_swap = width;
		width = height;
		height = dimension_swap;
		uint32_t* buffer_swap = pixels;
		pixels = temporal_buffer;
		temporal_buffer = buffer_swap;
	}
}

static void gdt_blur_bitmap(int width, int height, size_t stride, uint32_t* pixels, int level)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1 && level > -1);
	if (level > 1)
	{
		uint32_t* temporal_buffer = (uint32_t*)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
		if (temporal_buffer)
		{
			gdt_fast_multilevel_blur_bitmap(width, height, stride, pixels, temporal_buffer, level);
			free(temporal_buffer);
			return;
		}
	}
#ifdef GDT_SSSE3
	__m128i decode_shuffle = _mm_cvtsi32_si128(0x80808003);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8002, 2);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8080, 3);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8001, 4);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8080, 5);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8000, 6);
	decode_shuffle = _mm_insert_epi16(decode_shuffle, 0x8080, 7);
	__m128i encode_shuffle = _mm_cvtsi32_si128(0x0004080C);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 2);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 3);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 4);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 5);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 6);
	encode_shuffle = _mm_insert_epi16(encode_shuffle, 0x8080, 7);
	__m128i magic = _mm_cvtsi32_si128(0x00002AA9);// ((((3 * 0xFF) * 0x2AA9) >> 14) + 1) >> 1 = 0xFF
	magic = _mm_shuffle_epi32(magic, 0);
	__m128i previous;
	__m128i current;
	__m128i next;
	__m128i tmp;
	__m128i buffer;
	while (level)
	{
		if (width > 1)
			for (int y = 0; y != height; ++y)
			{
				uint32_t* row = (uint32_t*)((uintptr_t)pixels + (y * stride));
				current = _mm_castps_si128(_mm_load_ss((const float*)row));
				next = _mm_castps_si128(_mm_load_ss((const float*)(row + 1)));
				current = _mm_shuffle_epi8(current, decode_shuffle);
				next = _mm_shuffle_epi8(next, decode_shuffle);
				tmp = _mm_add_epi32(current, current);
				tmp = _mm_add_epi32(tmp, next);
				tmp = _mm_mulhrs_epi16(magic, tmp);
				buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
				_mm_store_ss((float*)row, _mm_castsi128_ps(buffer));
				for (int x = 1; x != width - 1; ++x)
				{
					previous = current;
					current = next;
					next = _mm_castps_si128(_mm_load_ss((const float*)(row + x + 1)));
					next = _mm_shuffle_epi8(next, decode_shuffle);
					tmp = _mm_add_epi32(previous, current);
					tmp = _mm_add_epi32(tmp, next);
					tmp = _mm_mulhrs_epi16(magic, tmp);
					buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
					_mm_store_ss((float*)(row + x), _mm_castsi128_ps(buffer));
				}
				tmp = _mm_add_epi32(next, next);
				tmp = _mm_add_epi32(tmp, current);
				tmp = _mm_mulhrs_epi16(magic, tmp);
				buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
				_mm_store_ss((float*)(row + width - 1), _mm_castsi128_ps(buffer));
			}
		if (height > 1)
			for (int x = 0; x != width; ++x)
			{
				uintptr_t column = (uintptr_t)pixels + (x * sizeof(uint32_t));
				current = _mm_castps_si128(_mm_load_ss((const float*)column));
				next = _mm_castps_si128(_mm_load_ss((const float*)(column + stride)));
				current = _mm_shuffle_epi8(current, decode_shuffle);
				next = _mm_shuffle_epi8(next, decode_shuffle);
				tmp = _mm_add_epi32(current, current);
				tmp = _mm_add_epi32(tmp, next);
				tmp = _mm_mulhrs_epi16(magic, tmp);
				buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
				_mm_store_ss((float*)column, _mm_castsi128_ps(buffer));
				for (int y = 1; y != height - 1; ++y)
				{
					previous = current;
					current = next;
					next = _mm_castps_si128(_mm_load_ss((const float*)(column + ((y + 1) * stride))));
					next = _mm_shuffle_epi8(next, decode_shuffle);
					tmp = _mm_add_epi32(previous, current);
					tmp = _mm_add_epi32(tmp, next);
					tmp = _mm_mulhrs_epi16(magic, tmp);
					buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
					_mm_store_ss((float*)(column + (y * stride)), _mm_castsi128_ps(buffer));
				}
				tmp = _mm_add_epi32(next, next);
				tmp = _mm_add_epi32(tmp, current);
				tmp = _mm_mulhrs_epi16(magic, tmp);
				buffer = _mm_shuffle_epi8(tmp, encode_shuffle);
				_mm_store_ss((float*)(column + ((height - 1) * stride)), _mm_castsi128_ps(buffer));
			}
		--level;
	}
#else
	float previous[4];
	float current[4];
	float next[4];
	float tmp[4];
	while (level)
	{
		if (width > 1)
			for (int y = 0; y != height; ++y)
			{
				uint32_t* row = (uint32_t*)((uintptr_t)pixels + (size_t)y * stride);
				uint32_t row_current = row[0];
				uint32_t row_next = row[1];
				current[0] = (float)((row_current >> 0x18) & 0xff);
				current[1] = (float)((row_current >> 0x10) & 0xff);
				current[2] = (float)((row_current >> 0x08) & 0xff);
				current[3] = (float)((row_current >> 0x00) & 0xff);
				next[0] = (float)((row_next >> 0x18) & 0xff);
				next[1] = (float)((row_next >> 0x10) & 0xff);
				next[2] = (float)((row_next >> 0x08) & 0xff);
				next[3] = (float)((row_next >> 0x00) & 0xff);
				tmp[0] = 0.33333333333f * (current[0] + current[0] + next[0]);
				tmp[1] = 0.33333333333f * (current[1] + current[1] + next[1]);
				tmp[2] = 0.33333333333f * (current[2] + current[2] + next[2]);
				tmp[3] = 0.33333333333f * (current[3] + current[3] + next[3]);
				row[0] =
					((uint32_t)tmp[0] << 0x18) |
					((uint32_t)tmp[1] << 0x10) |
					((uint32_t)tmp[2] << 0x08) |
					((uint32_t)tmp[3] << 0x00);
				for (int x = 1; x != width - 1; ++x)
				{
					previous[0] = current[0];
					previous[1] = current[1];
					previous[2] = current[2];
					previous[3] = current[3];
					current[0] = next[0];
					current[1] = next[1];
					current[2] = next[2];
					current[3] = next[3];
					row_next = row[x + 1];
					next[0] = (float)((row_next >> 0x18) & 0xff);
					next[1] = (float)((row_next >> 0x10) & 0xff);
					next[2] = (float)((row_next >> 0x08) & 0xff);
					next[3] = (float)((row_next >> 0x00) & 0xff);
					tmp[0] = 0.33333333333f * (previous[0] + current[0] + next[0]);
					tmp[1] = 0.33333333333f * (previous[1] + current[1] + next[1]);
					tmp[2] = 0.33333333333f * (previous[2] + current[2] + next[2]);
					tmp[3] = 0.33333333333f * (previous[3] + current[3] + next[3]);
					row[x] =
						((uint32_t)tmp[0] << 0x18) |
						((uint32_t)tmp[1] << 0x10) |
						((uint32_t)tmp[2] << 0x08) |
						((uint32_t)tmp[3] << 0x00);
				}
				tmp[0] = 0.33333333333f * (current[0] + next[0] + next[0]);
				tmp[1] = 0.33333333333f * (current[1] + next[1] + next[1]);
				tmp[2] = 0.33333333333f * (current[2] + next[2] + next[2]);
				tmp[3] = 0.33333333333f * (current[3] + next[3] + next[3]);
				row[width - 1] =
					((uint32_t)tmp[0] << 0x18) |
					((uint32_t)tmp[1] << 0x10) |
					((uint32_t)tmp[2] << 0x08) |
					((uint32_t)tmp[3] << 0x00);
			}
		if (height > 1)
			for (int x = 0; x != width; ++x)
			{
				uintptr_t column = (uintptr_t)pixels + (x * sizeof(uint32_t));
				uint32_t column_current = *(uint32_t*)column;
				uint32_t column_next = *(uint32_t*)(column + stride);
				current[0] = (float)((column_current >> 0x18) & 0xff);
				current[1] = (float)((column_current >> 0x10) & 0xff);
				current[2] = (float)((column_current >> 0x08) & 0xff);
				current[3] = (float)((column_current >> 0x00) & 0xff);
				next[0] = (float)((column_next >> 0x18) & 0xff);
				next[1] = (float)((column_next >> 0x10) & 0xff);
				next[2] = (float)((column_next >> 0x08) & 0xff);
				next[3] = (float)((column_next >> 0x00) & 0xff);
				tmp[0] = 0.33333333333f * (current[0] + current[0] + next[0]);
				tmp[1] = 0.33333333333f * (current[1] + current[1] + next[1]);
				tmp[2] = 0.33333333333f * (current[2] + current[2] + next[2]);
				tmp[3] = 0.33333333333f * (current[3] + current[3] + next[3]);
				*(uint32_t*)column =
					((uint32_t)tmp[0] << 0x18) |
					((uint32_t)tmp[1] << 0x10) |
					((uint32_t)tmp[2] << 0x08) |
					((uint32_t)tmp[3] << 0x00);
				for (int y = 1; y != height - 1; ++y)
				{
					previous[0] = current[0];
					previous[1] = current[1];
					previous[2] = current[2];
					previous[3] = current[3];
					current[0] = next[0];
					current[1] = next[1];
					current[2] = next[2];
					current[3] = next[3];
					column_next = *(uint32_t*)(column + ((y + 1) * stride));
					next[0] = (float)((column_next >> 0x18) & 0xff);
					next[1] = (float)((column_next >> 0x10) & 0xff);
					next[2] = (float)((column_next >> 0x08) & 0xff);
					next[3] = (float)((column_next >> 0x00) & 0xff);
					tmp[0] = 0.33333333333f * (previous[0] + current[0] + next[0]);
					tmp[1] = 0.33333333333f * (previous[1] + current[1] + next[1]);
					tmp[2] = 0.33333333333f * (previous[2] + current[2] + next[2]);
					tmp[3] = 0.33333333333f * (previous[3] + current[3] + next[3]);
					*(uint32_t*)(column + (y * stride)) =
						((uint32_t)tmp[0] << 0x18) |
						((uint32_t)tmp[1] << 0x10) |
						((uint32_t)tmp[2] << 0x08) |
						((uint32_t)tmp[3] << 0x00);
				}
				tmp[0] = 0.33333333333f * (current[0] + next[0] + next[0]);
				tmp[1] = 0.33333333333f * (current[1] + next[1] + next[1]);
				tmp[2] = 0.33333333333f * (current[2] + next[2] + next[2]);
				tmp[3] = 0.33333333333f * (current[3] + next[3] + next[3]);
				*(uint32_t*)(column + ((height - 1) * stride)) =
					((uint32_t)tmp[0] << 0x18) |
					((uint32_t)tmp[1] << 0x10) |
					((uint32_t)tmp[2] << 0x08) |
					((uint32_t)tmp[3] << 0x00);
			}
		--level;
	}
#endif
}

struct gdt_grid_info_t
{
	int line_count_x;
	int line_count_y;
	float delta_x;
	float min_x;
	float max_x;
	float grid_min_x;
	float grid_max_x;
	float delta_y;
	float min_y;
	float max_y;
	float grid_min_y;
	float grid_max_y;
};

static void gdt_get_graph_grid_info(int flags, float delta_x, float delta_y, size_t line_count,
	size_t point_x_element, size_t point_count, const float* points, struct gdt_grid_info_t* grid_info)
{
	GDT_CORE_ASSUME(delta_x >= 0.0f && delta_y >= 0.0f);
	if (line_count < 2 || point_x_element > (line_count - 1) || !point_count)
		return;
	float min_x;
	float max_x;
	if (flags & GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION)
	{
		min_x = points[point_x_element * point_count];
		max_x = min_x;
		for (const float* i = points + point_x_element * point_count + 1, *l = i + point_count - 1; i != l; ++i)
		{
			float x = *i;
			if (x < min_x)
				min_x = x;
			if (x > max_x)
				max_x = x;
		}
	}
	else
	{
		min_x = points[point_x_element];
		max_x = min_x;
		for (size_t point_index = 1; point_index != point_count; ++point_index)
		{
			float x = points[point_index * line_count + point_x_element];
			if (x < min_x)
				min_x = x;
			if (x > max_x)
				max_x = x;
		}
	}
	float grid_min_x = (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? gdt_round(0, delta_x, min_x - (0.5f * delta_x)) : min_x;
	float grid_max_x = (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? gdt_round(1, delta_x, max_x + (0.5f * delta_x)) : max_x;
	float min_y;
	float max_y;
	if (flags & GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION)
	{
		min_y = point_x_element ? points[0] : points[point_count];
		max_y = min_y;
		for (size_t line_index = 0; line_index != line_count; ++line_index)
			if (line_index != point_x_element)
				for (const float* i = points + line_index * point_count, *l = i + point_count; i != l; ++i)
				{
					float y = *i;
					if (y < min_y)
						min_y = y;
					if (y > max_y)
						max_y = y;
				}
	}
	else
	{
		min_y = point_x_element ? points[0] : points[1];
		max_y = min_y;
		for (size_t point_index = 0; point_index != point_count; ++point_index)
			for (size_t line_index = 0; line_index != line_count; ++line_index)
				if (line_index != point_x_element)
				{
					float y = points[point_index * line_count + line_index];
					if (y < min_y)
						min_y = y;
					if (y > max_y)
						max_y = y;
				}
	}
	float grid_min_y = (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? gdt_round(0, delta_y, min_y - (0.5f * delta_y)) : min_y;
	float grid_max_y = (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? gdt_round(1, delta_y, max_y + (0.5f * delta_y)) : max_y;
	grid_info->line_count_x = (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? ((int)((grid_max_x - grid_min_x) / delta_x) + 1) : 0;
	grid_info->line_count_y = (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? ((int)((grid_max_y - grid_min_y) / delta_y) + 1) : 0;
	grid_info->delta_x = delta_x;
	grid_info->min_x = min_x;
	grid_info->max_x = max_x;
	grid_info->grid_min_x = grid_min_x;
	grid_info->grid_max_x = grid_max_x;
	grid_info->delta_y = delta_y;
	grid_info->min_y = min_y;
	grid_info->max_y = max_y;
	grid_info->grid_min_y = grid_min_y;
	grid_info->grid_max_y = grid_max_y;
}

static void gdt_internal_draw_graph_to_bitmap(int flags, uint32_t background_color, uint32_t grid_color, uint32_t x_axis_color,
	uint32_t y_axis_color, float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, int line_thickness, size_t line_count, size_t point_x_element, uint32_t* line_colors,
	size_t point_count, const float* points, const struct gdt_grid_info_t* grid_info)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1);
	if (line_count < 2 || line_count > INT_MAX || width < 1 || height < 1 ||
		grid_delta_x < 0.0f || grid_delta_y < 0.0f || line_thickness < 0 || !point_count)
		return;
	int radius = line_thickness / 2;
	float scaler_x = (float)(width - (2 * radius)) / ((flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? (grid_info->grid_max_x - grid_info->grid_min_x) : (grid_info->max_x - grid_info->min_x));
	float scaler_y = (float)(height - (2 * radius)) / ((flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID) ? (grid_info->grid_max_y - grid_info->grid_min_y) : (grid_info->max_y - grid_info->min_y));
	int offset_x = radius + (int)(scaler_x * -grid_info->grid_min_x);
	int offset_y = radius + (int)(scaler_y * -grid_info->grid_min_y);
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID)
	{
		for (int n = (int)((grid_info->grid_max_y - grid_info->grid_min_y) / grid_delta_y) + 1, i = 0; i != n; ++i)
		{
			int y = radius + (int)((float)i * ((float)(height - (2 * radius)) / (float)(n - 1)));
			gdt_draw_line_to_bitmap(width, height, stride, pixels, grid_color, 2 * radius, (struct gdt_iv2_t) { radius, y }, (struct gdt_iv2_t) { width - radius, y });
		}
		for (int n = (int)((grid_info->grid_max_x - grid_info->grid_min_x) / grid_delta_x) + 1, i = 0; i != n; ++i)
		{
			int x = radius + (int)((float)i * ((float)(width - (2 * radius)) / (float)(n - 1)));
			gdt_draw_line_to_bitmap(width, height, stride, pixels, grid_color, 2 * radius, (struct gdt_iv2_t) { x, radius }, (struct gdt_iv2_t) { x, height - radius });
		}
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, grid_color, radius, radius, radius);
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, grid_color, radius, width - radius, radius);
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, grid_color, radius, radius, height - radius);
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, grid_color, radius, width - radius, height - radius);
	}
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_X_AXIS)
	{
		int y = offset_y;
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, x_axis_color, radius, radius, y);
		gdt_draw_line_to_bitmap(width, height, stride, pixels, x_axis_color, 2 * radius, (struct gdt_iv2_t) { radius, y }, (struct gdt_iv2_t) { width - radius, y });
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, x_axis_color, radius, width - radius, y);
	}
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_Y_AXIS)
	{
		int x = offset_x;
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, y_axis_color, radius, x, radius);
		gdt_draw_line_to_bitmap(width, height, stride, pixels, y_axis_color, 2 * radius, (struct gdt_iv2_t) { x, radius }, (struct gdt_iv2_t) { x, height - radius });
		gdt_draw_dot_to_bitmap(width, height, stride, pixels, y_axis_color, radius, x, height - radius);
	}
	for (size_t line_index = 0; line_index != line_count; ++line_index)
		if (line_index != point_x_element)
		{
			const float* line_points = (flags & GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION) ? (points + line_index * point_count) : (points + line_index);
			uint32_t line_color = line_colors[line_index];
			int previous_point_y;
			int previous_point_x;
			for (size_t i = 0; i != point_count; ++i)
			{
				int point_y = offset_y + (int)(((flags & GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION) ? line_points[i] : line_points[i * line_count]) * scaler_y);
				int point_x = offset_x + (int)(((flags & GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION) ? points[point_x_element * point_count + i] : points[i * line_count + point_x_element]) * scaler_x);
				if (!i)
					gdt_draw_dot_to_bitmap(width, height, stride, pixels, line_color, radius, point_x, point_y);
				else if (point_x != previous_point_x || point_y != previous_point_y)
				{
					if (((previous_point_x < point_x) ? (point_x - previous_point_x) : (previous_point_x - point_x)) > 1 ||
						((previous_point_y < point_y) ? (point_y - previous_point_y) : (previous_point_y - point_y)) > 1)
						gdt_draw_line_to_bitmap(width, height, stride, pixels, line_color, 2 * radius, (struct gdt_iv2_t) { point_x, point_y }, (struct gdt_iv2_t) { previous_point_x, previous_point_y });
					gdt_draw_dot_to_bitmap(width, height, stride, pixels, line_color, radius, point_x, point_y);
				}
				previous_point_y = point_y;
				previous_point_x = point_x;
			}
		}
}

void gdt_draw_graph_to_bitmap(int flags, uint32_t background_color, uint32_t grid_color, uint32_t x_axis_color,
	uint32_t y_axis_color, float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, int line_thickness, size_t line_count, size_t point_x_element, uint32_t* line_colors,
	size_t point_count, const float* points)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1);
	if (line_count < 2 || line_count > INT_MAX || width < 1 || height < 1 ||
		grid_delta_x < 0.0f || grid_delta_y < 0.0f || line_thickness < 0 || !point_count)
		return;
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_BACKGROUND)
		gdt_fill_bitmap(width, height, stride, pixels, background_color);
	struct gdt_grid_info_t grid_info;
	gdt_get_graph_grid_info(flags, grid_delta_x, grid_delta_y, line_count, point_x_element, point_count, points, &grid_info);
	gdt_internal_draw_graph_to_bitmap(flags, background_color, grid_color, x_axis_color,
		y_axis_color, grid_delta_x, grid_delta_y, width, height, stride, pixels, line_thickness,
		line_count, point_x_element, line_colors, point_count, points, &grid_info);
	if (flags & GDT_DRAW_GRAPH_FLAG_SOFT_LINES)
		gdt_blur_bitmap(width, height, stride, pixels, line_thickness / 2);
}

static uint32_t gdt_make_line_color(int count, int index)
{
	GDT_CORE_ASSUME(count > -1 && index > -1);
	const float full_brightness = 246;
	float s = (float)index / (float)count;
	float r;
	float g;
	float b;
	if (s < 0.333333333f)
	{
		s = s * 3.0f;
		r = (1.0f - s) * full_brightness;
		g = s * full_brightness;
		b = 0.0f;
	}
	else if (s < 0.666666666f)
	{
		s = (s - 0.333333333f) * 3.0f;
		r = 0.0f;
		g = (1.0f - s) * full_brightness;
		b = s * full_brightness;
	}
	else
	{
		s = (s - 0.666666666f) * 3.0f;
		r = s * full_brightness;
		g = 0.0f;
		b = (1.0f - s) * full_brightness;
	}
	return ((uint32_t)0xff << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

int gdt_simplified_draw_graph_to_bitmap(float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, size_t line_count, size_t point_count, const float* points, int soa_representation)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1);
	if (line_count < 2 || line_count > INT_MAX || width < 1 || height < 1 ||
		grid_delta_x < 0.0f || grid_delta_y < 0.0f || !point_count)
		return EINVAL;
	const uint32_t background_color = 0xFF222222;
	const uint32_t grid_color = 0xFF444444;
	const uint32_t xy_line_colors[2] = { 0xFF000000, 0xFF0055EE };
	uint32_t* line_colors;
	if (line_count == 2)
		line_colors = (uint32_t*)xy_line_colors;
	else
	{
		line_colors = (uint32_t*)malloc(line_count * sizeof(uint32_t));
		if (!line_colors)
			return ENOMEM;;
		for (int i = 0; i != (int)line_count; ++i)
			line_colors[i] = gdt_make_line_color((int)line_count, i);
	}
	gdt_draw_graph_to_bitmap(
		(soa_representation ? GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION : 0) |
		GDT_DRAW_GRAPH_FLAG_DRAW_BACKGROUND |
		((grid_delta_x != 0.0f && grid_delta_y != 0.0f) ? GDT_DRAW_GRAPH_FLAG_DRAW_GRID : 0),
		background_color, grid_color, 0, 0,
		grid_delta_x, grid_delta_y, width, height, stride,
		pixels, (width < height ? width : height) / 180, line_count, 0, line_colors,
		point_count, points);
	if (line_colors != xy_line_colors)
		free(line_colors);
	return 0;
}

static int gdt_load_truetype_font(const char* truetype_file_name, void** font_buffer)
{
#ifdef _MSC_VER
	FILE* handle;
	int error = (int)fopen_s(&handle, truetype_file_name, "rb");
	if (error)
		return error;
#else
	int error = 0;
	FILE* handle = fopen(truetype_file_name, "rb");
	if (!handle)
	{
		error = errno;
		return error;
	}
#endif
	if (fseek(handle, 0, SEEK_END))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}
	size_t size = (size_t)ftell(handle);
	if (size == (size_t)-1)
	{
		error = errno;
		fclose(handle);
		return error;
	}
	if (fseek(handle, 0, SEEK_SET))
	{
		error = ferror(handle);
		fclose(handle);
		return error;
	}
	const size_t font_info_size = ((sizeof(stbtt_fontinfo) + 15) & ~15);
	stbtt_fontinfo* buffer = (stbtt_fontinfo*)malloc(font_info_size + ((size + 15) & ~15));
	void* file_buffer = (void*)((uintptr_t)buffer + font_info_size);
	if (!buffer)
	{
		error = errno;
		fclose(handle);
		return error;
	}
	for (size_t index = 0; index != size;)
	{
		size_t read = fread((void*)((uintptr_t)file_buffer + index), 1, size - index, handle);
		if (!read)
		{
			error = ferror(handle);
			if (!error)
				error = EIO;
			free(buffer);
			fclose(handle);
			return error;
		}
		index += read;
	}
	fclose(handle);
	error = stbtt_InitFont(buffer, (const unsigned char*)file_buffer, 0) ? 0 : EILSEQ;
	if (error)
	{
		free(buffer);
		return error;
	}
	*font_buffer = (void*)buffer;
	return error;
}

static size_t gdt_unicode_string_length(const uint32_t* string)
{
	const uint32_t* string_read = string;
	while (*string_read)
		++string_read;
	return (size_t)((uintptr_t)string_read - (uintptr_t)string) / sizeof(uint32_t);
}

static int gdt_calculate_string_rectangle(void* font, float font_height, const uint32_t* string, int* rectangle_left, int* rectangle_right, int* rectangle_bottom, int* rectangle_top)
{
	if (font_height < 0.0f)
		return EINVAL;
	stbtt_fontinfo* font_info = (stbtt_fontinfo*)font;
	int rectangle_set = 0;
	int x_l;
	int x_h;
	int y_l;
	int y_h;
	float x = 0.0f;
	float y = 0.0f;
	int ascent;
	stbtt_GetFontVMetrics(font_info, &ascent, 0, 0);
	float scale = stbtt_ScaleForPixelHeight(font_info, font_height);
	int baseline = (int)(scale * (float)ascent);
	for (size_t i = 0, l = gdt_unicode_string_length(string); i != l; ++i)
		if (string[i] != (uint32_t)'\n')
		{
			float x_shift = x - floorf(x);
			int advance;
			int lsb;
			int left;
			int right;
			int bottom;
			int top;
			stbtt_GetCodepointHMetrics(font_info, (int)string[i], &advance, &lsb);
			stbtt_GetCodepointBitmapBoxSubpixel(font_info, (int)string[i], scale, scale, x_shift, 0, &left, &bottom, &right, &top);
			int w = right - left;
			int h = top - bottom;
			if (rectangle_set)
			{
				if ((int)x + left < x_l)
					x_l = (int)x + left;
				if ((int)x + left + w > x_h)
					x_h = (int)x + left + w;
				if ((int)y - (baseline + bottom + h) < y_l)
					y_l = (int)y - (baseline + bottom + h);
				if ((int)y - (baseline + bottom) > y_h)
					y_h = (int)y - (baseline + bottom);
			}
			else
			{
				x_l = (int)x + left;
				x_h = (int)x + left + w;
				y_l = (int)y - (baseline + bottom + h);
				y_h = (int)y - (baseline + bottom);
				rectangle_set = 1;
			}
			x += (scale * (float)advance);
			if (i + 1 != l)
				x += scale * (float)stbtt_GetCodepointKernAdvance(font_info, (int)string[i], (int)string[i + 1]);
		}
		else
		{
			x = 0.0f;
			y -= font_height;
		}
	if (rectangle_set)
	{
		*rectangle_left = x_l;
		*rectangle_right = x_h;
		*rectangle_bottom = y_l;
		*rectangle_top = y_h;
	}
	else
	{
		*rectangle_left = 0;
		*rectangle_right = 0;
		*rectangle_bottom = 0;
		*rectangle_top = 0;
	}
	return 0;
}

static int gdt_draw_string(int width, int height, size_t stride, uint32_t* pixels, float x, float y, void* font, float font_height, const uint32_t* string, uint32_t color)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1);
	if (width < 1 || height < 1 || font_height < 0.0f)
		return EINVAL;
	stbtt_fontinfo* font_info = (stbtt_fontinfo*)font;
	float font_color[4] = {
		(float)((color >> 24) & 0xff) * 0.00392156862f,
		(float)((color >> 16) & 0xff) * 0.00392156862f,
		(float)((color >> 8) & 0xff) * 0.00392156862f,
		(float)((color >> 0) & 0xff) * 0.00392156862f };
	float string_x_offset = x;
	int glyph_bitmap_width = (int)font_height * 2 + 1;
	int glyph_bitmap_height = glyph_bitmap_width;
	uint8_t* glyph_bitmap = (uint8_t*)malloc(glyph_bitmap_width * glyph_bitmap_height);
	if (!glyph_bitmap)
		return ENOMEM;
	int ascent;
	stbtt_GetFontVMetrics(font_info, &ascent, 0, 0);
	float scale = stbtt_ScaleForPixelHeight(font_info, font_height);
	int baseline = (int)(scale * (float)ascent);
	for (size_t i = 0, l = gdt_unicode_string_length(string); i != l; ++i)
	{
		if (string[i] != (uint32_t)'\n')
		{
			float x_shift = x - floorf(x);
			int advance;
			int lsb;
			int left;
			int right;
			int bottom;
			int top;
			int w;
			int h;
			stbtt_GetCodepointHMetrics(font_info, (int)string[i], &advance, &lsb);
			stbtt_GetCodepointBitmapBoxSubpixel(font_info, (int)string[i], scale, scale, x_shift, 0, &left, &bottom, &right, &top);
			w = right - left;
			h = top - bottom;
			if (w > glyph_bitmap_width && h > glyph_bitmap_height)
			{
				if (w > glyph_bitmap_width)
					glyph_bitmap_width = w;
				if (h > glyph_bitmap_height)
					glyph_bitmap_height = h;
				uint8_t* new_glyph_bitmap = (uint8_t*)realloc(glyph_bitmap, glyph_bitmap_width * glyph_bitmap_height);
				if (!new_glyph_bitmap)
				{
					free(glyph_bitmap);
					return ENOMEM;
				}
				glyph_bitmap = new_glyph_bitmap;
			}
			stbtt_MakeCodepointBitmapSubpixel(font_info, glyph_bitmap, w, h, w, scale, scale, x_shift, 0, (int)string[i]);
			int blit_y = (int)y - (baseline + bottom);
			int blit_y_end = blit_y - h;
			int blit_x = (int)x + left;
			int blit_x_end = blit_x + w;
			if (blit_y > 0 && blit_y_end < height && blit_x < width && blit_x_end > 0)
			{
				int bounding_y = blit_y > (height - 1) ? (height - 1) : blit_y;
				int bounding_y_end = blit_y_end < -1 ? -1 : blit_y_end;
				int bounding_x = blit_x < 0 ? 0 : blit_x;
				int bounding_x_end = blit_x_end > width ? width : blit_x_end;
				for (int blend_y = bounding_y; blend_y != bounding_y_end; --blend_y)
				{
					int glyph_y = blit_y - blend_y;
					uint32_t* row = (uint32_t*)((uintptr_t)pixels + (size_t)blend_y * stride);
					for (int blend_x = bounding_x; blend_x != bounding_x_end; ++blend_x)
					{
						int glyph_x = blend_x - blit_x;
						float glyph_intensity_scaler = (float)glyph_bitmap[glyph_y * w + glyph_x] * 0.00392156862f;
						float glyph_intensity[4] = {
							glyph_intensity_scaler,
							glyph_intensity_scaler,
							glyph_intensity_scaler,
							glyph_intensity_scaler };
						float bitmap_intensity[4] = {
							1.0f - glyph_intensity[0],
							1.0f - glyph_intensity[1],
							1.0f - glyph_intensity[2],
							1.0f - glyph_intensity[3] };
						uint32_t bitmap_pixel_ARGB32 = row[blend_x];
						float bitmap_pixel[4] = {
							(float)((bitmap_pixel_ARGB32 >> 24) & 0xff) * 0.00392156862f,
							(float)((bitmap_pixel_ARGB32 >> 16) & 0xff) * 0.00392156862f,
							(float)((bitmap_pixel_ARGB32 >> 8) & 0xff) * 0.00392156862f,
							(float)((bitmap_pixel_ARGB32 >> 0) & 0xff) * 0.00392156862f };
						float final_pixel[4] = {
							(glyph_intensity[0] * font_color[0] + bitmap_intensity[0] * bitmap_pixel[0]) * 255.0f,
							(glyph_intensity[1] * font_color[1] + bitmap_intensity[1] * bitmap_pixel[1]) * 255.0f,
							(glyph_intensity[2] * font_color[2] + bitmap_intensity[2] * bitmap_pixel[2]) * 255.0f,
							(glyph_intensity[3] * font_color[3] + bitmap_intensity[3] * bitmap_pixel[3]) * 255.0f };
						int tmp_transform[4] = {
							(int)final_pixel[0],
							(int)final_pixel[1],
							(int)final_pixel[2],
							(int)final_pixel[3] };
						uint32_t pixel = ((uint32_t)tmp_transform[0] << 24) | ((uint32_t)tmp_transform[1] << 16) | ((uint32_t)tmp_transform[2] << 8) | (uint32_t)tmp_transform[3];
						row[blend_x] = pixel;
					}
				}
			}
			x += (scale * (float)advance);
			if (i + 1 != l)
				x += scale * (float)stbtt_GetCodepointKernAdvance(font_info, (int)string[i], (int)string[i + 1]);
		}
		else
		{
			x = string_x_offset;
			y -= font_height;
		}
	}
	free(glyph_bitmap);
	return 0;
}

#define GDT_FLOAT_STIRNG_BUFFER_LENGTH 13
static size_t gdt_print_grid_number(uint32_t* buffer, float value, int fraction_limit)
{
	GDT_CORE_ASSUME(value != NAN && value != INFINITY && fraction_limit > -1);
	int sign_bit = value < 0.0f;
	if (sign_bit)
		value = -value;
	if (value > 999999936.0f)
		value = 999999936.0f;
	int high_digit_count = 1;
	for (int digit_shift = 10; (int)(value / (float)digit_shift); digit_shift *= 10)
		++high_digit_count;
	int low_digit_count = 9 - high_digit_count;
	int digit_shift = 1;
	for (int c = low_digit_count; c--;)
		digit_shift *= 10;
	int digits = (int)(value * (float)digit_shift);
	int high_zero_count = 0;
	int low_zero_count = 0;
	if (digits)
	{
		for (int digit_shift = 100000000; !((digits / digit_shift) % 10); digit_shift /= 10)
			++high_zero_count;
		for (int digit_shift = 1; !((digits / digit_shift) % 10); digit_shift *= 10)
			++low_zero_count;
	}
	else
	{
		high_zero_count = 1;
		low_zero_count = 8;
	}
	int output_low_digit_count;
	if (fraction_limit)
	{
		int round_high_digit_count = ((high_digit_count > high_zero_count) ? high_digit_count : high_zero_count) + fraction_limit;
		int round_low_digit_count = 9 - round_high_digit_count;
		if (round_low_digit_count > 0)
		{
			int round_digit_shift = 1;
			for (int c = round_low_digit_count; c--;)
				round_digit_shift *= 10;
			int remainder = digits % round_digit_shift;
			int round_up = (remainder >= (round_digit_shift / 2));
			digits -= remainder;
			if (round_up)
			{
				int roud_up_over_flow = round_up;
				for (int digit_count = round_high_digit_count, digit_shift = 100000000; roud_up_over_flow && digit_count--; digit_shift /= 10)
					if (((digits / digit_shift) % 10) != 9)
						roud_up_over_flow = 0;
				if (roud_up_over_flow)
				{
					if (digits == (999999999 % round_digit_shift))
					{
						if (sign_bit)
							for (int copy_index = 0; copy_index != 13; ++copy_index)
								buffer[copy_index] = (uint32_t)"-1000000000.0"[copy_index];
						else
							for (int copy_index = 0; copy_index != 12; ++copy_index)
								buffer[copy_index] = (uint32_t)"1000000000.0"[copy_index];
						return 0;
					}
					digits = 100000000;
					++high_digit_count;
					--low_digit_count;
					high_zero_count = 0;
					low_zero_count = 8;
				}
				else
					digits += round_digit_shift;
			}
			if (digits)
			{
				high_zero_count = 0;
				for (int digit_shift = 100000000; !((digits / digit_shift) % 10); digit_shift /= 10)
					++high_zero_count;
				low_zero_count = 0;
				for (int digit_shift = 1; !((digits / digit_shift) % 10); digit_shift *= 10)
					++low_zero_count;
			}
			else
			{
				high_zero_count = 1;
				low_zero_count = 8;
			}
		}
		if (digits / 100000000)
		{
			output_low_digit_count = low_digit_count - low_zero_count;
			if (output_low_digit_count < 1)
				output_low_digit_count = 1;
			else if (output_low_digit_count > fraction_limit)
				output_low_digit_count = fraction_limit;
		}
		else if (digits)
		{
			output_low_digit_count = (high_zero_count - 1) + fraction_limit;
			if (output_low_digit_count > low_digit_count - low_zero_count)
				output_low_digit_count = low_digit_count - low_zero_count;
			if (!output_low_digit_count)
				output_low_digit_count = 1;
		}
		else
			output_low_digit_count = 1;
	}
	else
		output_low_digit_count = (low_digit_count - low_zero_count) ? (low_digit_count - low_zero_count) : 1;
	if (sign_bit)
		buffer[0] = (uint32_t)'-';
	int digit_select = 100000000;
	for (int i = 0; i != high_digit_count; ++i, digit_select /= 10)
		buffer[sign_bit + i] = (uint32_t)('0' + ((digits / digit_select) % 10));
	buffer[sign_bit + high_digit_count] = '.';
	for (int i = 0; i != output_low_digit_count; ++i, digit_select /= 10)
		buffer[sign_bit + high_digit_count + 1 + i] = (uint32_t)('0' + (char)((digits / digit_select) % 10));
	return (size_t)(sign_bit + high_digit_count + 1 + output_low_digit_count);
}

/*
static size_t gdt_unused_print_float(char* buffer, float value, int significant_fraction_limit)
{
	GDT_CORE_ASSUME(value != NAN && value != INFINITY);
	int sign_bit = value < 0.0f;
	if (sign_bit)
		value = -value;
	if (value > 999999936.0f)
		value = 999999936.0f;
	int decimals = 1;
	for (float divider = 10.0f; value / divider >= 1.0f; divider *= 10.0f)
		++decimals;
	int fractions = 9 - decimals;
	int decimal_shift = 1;
	for (int i = 0; i != fractions; ++i)
		decimal_shift *= 10;
	int shifted_value = (int)(value * (float)decimal_shift);
	if (sign_bit)
		buffer[0] = '-';
	int digit_select = 100000000;
	for (int i = 0; i != decimals; ++i, digit_select /= 10)
		buffer[sign_bit + i] = '0' + ((shifted_value / digit_select) % 10);
	buffer[sign_bit + decimals] = '.';
	if ((shifted_value / 100000000) % 10)
	{
		if (fractions > significant_fraction_limit)
			fractions = significant_fraction_limit;
	}
	else
	{
		int first_significant_digit_index = 0;
		for (int digit_find = digit_select; first_significant_digit_index != fractions && (shifted_value / digit_find) % 10 == 0; digit_find /= 10)
			++first_significant_digit_index;
		if (fractions > first_significant_digit_index + significant_fraction_limit)
			fractions = first_significant_digit_index + significant_fraction_limit;

	}
	if (fractions)
	{
		int ignored_zeroes = 0;
		for (int i = 0; i != fractions; ++i, digit_select /= 10)
		{
			int digit = (shifted_value / digit_select) % 10;
			if (digit)
				ignored_zeroes = 0;
			else
				++ignored_zeroes;
			buffer[sign_bit + decimals + 1 + i] = '0' + (char)digit;
		}
		fractions -= ignored_zeroes;
	}
	else
		buffer[sign_bit + decimals + 1] = '0';
	if (!fractions)
		fractions = 1;
	return (size_t)(sign_bit + decimals + 1 + fractions);
}
*/

static int gdt_internal_draw_graph_with_titles_to_bitmap(int flags, uint32_t background_color, uint32_t grid_color, uint32_t x_axis_color,
	uint32_t y_axis_color, float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, int line_thickness, size_t line_count, size_t point_x_element, uint32_t* line_colors,
	size_t point_count, const float* points, const char* truetype_file_name, float gird_number_font_height,
	float graph_title_font_height, uint32_t graph_title_color, const uint32_t* graph_title, float axis_title_font_height,
	const uint32_t* axis_title_colors, const uint32_t** axis_titles)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1 && point_x_element < line_count);
	if (line_count < 2 || line_count > INT_MAX || width < 1 || height < 1 ||
		grid_delta_x < 0.0f || grid_delta_y < 0.0f || line_thickness < 0 || !point_count)
		return EINVAL;
	if ((flags & GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH) && line_count != 2)
		return EINVAL;
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_AUTO_SCALE_FONT)
	{
		gird_number_font_height = (float)height / 42.f;
		graph_title_font_height = (float)height / 24.f;
		axis_title_font_height = (float)height / 32.f;
	}
	if (!graph_title)
		graph_title_font_height = 0.0f;
	if (!axis_titles)
		axis_title_font_height = 0.0f;
	if (!(flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID))
		gird_number_font_height = 0.0f;
	struct gdt_grid_info_t grid_info;
	gdt_get_graph_grid_info(flags, grid_delta_x, grid_delta_y, line_count, point_x_element, point_count, points, &grid_info);
	int whole_number_grid = floorf(grid_delta_x) == grid_delta_x && floorf(grid_delta_y) == grid_delta_y;
	int axis_title_spacing = (int)(axis_title_font_height + 0.5f);
	int axis_color_line_length = (int)((axis_title_font_height * 1.5f) + 0.5f);
	void* font;
	int error = gdt_load_truetype_font(truetype_file_name, &font);
	if (error)
		return error;
	uint32_t gird_number_string[GDT_FLOAT_STIRNG_BUFFER_LENGTH + 1];
	int horizontal_space;
	int grid_y_numbers_yl;
	int grid_y_numbers_yh;
	int grid_y_numbers_width;
	int grid_y_number_bar_width;
	int grid_x_numbers_yl;
	int grid_x_numbers_yh;
	int grid_x_numbers_height;
	int grid_x_number_bar_height;
	int before_grid_space_required;
	int after_grid_space_required;
	int y_axis_title_x;
	int y_axis_title_y;
	int y_axis_title_width;
	int y_axis_title_height;
	int left_bar_width;
	int right_bar_width;
	int graph_title_x;
	int graph_title_y;
	int graph_title_width;
	int graph_title_height;
	int top_bar_height;
	int x_axis_title_x;
	int x_axis_title_y;
	int x_axis_title_width;
	int x_axis_title_height;
	int middle_bar_height;
	int axis_titles_yl;
	int axis_titles_yh;
	int axis_title_bar_width;
	int axis_title_bar_height;
	int bottom_bar_height;
	int graph_width;
	int graph_height;
	int string_xl;
	int string_xh;
	int string_yl;
	int string_yh;
	if (graph_title_font_height > 0.0f)
	{
		error = gdt_calculate_string_rectangle(font, graph_title_font_height, graph_title, &string_xl, &string_xh, &string_yl, &string_yh);
		if (error)
		{
			free(font);
			return error;
		}
		graph_title_x = string_xl;
		graph_title_y = string_yh;
		graph_title_width = string_xh - string_xl;
		graph_title_height = string_yh - string_yl;
		top_bar_height = 2 * graph_title_height;
	}
	else
	{
		graph_title_x = 0;
		graph_title_y = 0;
		graph_title_width = 0;
		graph_title_height = 0;
		top_bar_height = 0;
	}
	if (axis_title_font_height > 0.0f && (flags & GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH) && axis_titles[(int)point_x_element ^ 1])
	{
		error = gdt_calculate_string_rectangle(font, axis_title_font_height, axis_titles[(int)point_x_element ^ 1], &string_xl, &string_xh, &string_yl, &string_yh);
		if (error)
		{
			free(font);
			return error;
		}
		y_axis_title_x = string_xl;
		y_axis_title_y = string_yh;
		y_axis_title_width = string_xh - string_xl;
		y_axis_title_height = string_yh - string_yl;
		left_bar_width = 2 * y_axis_title_height;
	}
	else
		left_bar_width = 0;
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_GRID)
	{
		if (gird_number_font_height > 0.0f)
		{
			horizontal_space = (int)graph_title_font_height;
			if (horizontal_space < (int)axis_title_font_height)
				horizontal_space = (int)axis_title_font_height;
			if (horizontal_space < (int)gird_number_font_height)
				horizontal_space = (int)gird_number_font_height;
		}
		else
			horizontal_space = 0;
		for (int i = 0; i != grid_info.line_count_y; ++i)
		{
			gird_number_string[gdt_print_grid_number(gird_number_string, grid_info.grid_min_y + (float)i * grid_info.delta_y, 3) - (whole_number_grid ? 2 : 0)] = 0;
			error = gdt_calculate_string_rectangle(font, gird_number_font_height, gird_number_string, &string_xl, &string_xh, &string_yl, &string_yh);
			if (error)
			{
				free(font);
				return error;
			}
			if (i)
			{
				if (string_yl < grid_y_numbers_yl)
					grid_y_numbers_yl = string_yl;
				if (string_yh > grid_y_numbers_yh)
					grid_y_numbers_yh = string_yh;
				if (string_xh - string_xl > grid_y_numbers_width)
					grid_y_numbers_width = string_xh - string_xl;
			}
			else
			{
				grid_y_numbers_yl = string_yl;
				grid_y_numbers_yh = string_yh;
				grid_y_numbers_width = string_xh - string_xl;
			}
		}
		if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH)
			grid_y_number_bar_width = grid_y_numbers_width + 1 + horizontal_space;
		else
			grid_y_number_bar_width = grid_y_numbers_width + 2 * horizontal_space;
		for (int i = 0; i != grid_info.line_count_x; ++i)
		{
			gird_number_string[gdt_print_grid_number(gird_number_string, grid_info.grid_min_x + (float)i * grid_info.delta_x, 3) - (whole_number_grid ? 2 : 0)] = 0;
			error = gdt_calculate_string_rectangle(font, gird_number_font_height, gird_number_string, &string_xl, &string_xh, &string_yl, &string_yh);
			if (error)
			{
				free(font);
				return error;
			}
			if (i)
			{
				if (string_yl < grid_x_numbers_yl)
					grid_x_numbers_yl = string_yl;
				if (string_yh > grid_x_numbers_yh)
					grid_x_numbers_yh = string_yh;
				if (i + 1 == grid_info.line_count_x)
					before_grid_space_required = ((string_xh - string_xl) / 2);
			}
			else
			{
				grid_x_numbers_yl = string_yl;
				grid_x_numbers_yh = string_yh;
				after_grid_space_required = ((string_xh - string_xl) / 2);
			}
		}
		grid_x_numbers_height = grid_x_numbers_yh - grid_x_numbers_yl;
		grid_x_number_bar_height = 2 * grid_x_numbers_height;
	}
	else
	{
		before_grid_space_required = 0;
		after_grid_space_required = 0;
		horizontal_space = 0;
		grid_y_number_bar_width = 0;
		grid_x_numbers_height = 0;
		grid_x_number_bar_height = 0;
	}
	if (top_bar_height < horizontal_space)
		top_bar_height = horizontal_space;
	if (axis_title_font_height > 0.0f)
	{
		if (axis_titles[point_x_element])
		{
			error = gdt_calculate_string_rectangle(font, axis_title_font_height, axis_titles[point_x_element], &string_xl, &string_xh, &string_yl, &string_yh);
			if (error)
			{
				free(font);
				return error;
			}
			x_axis_title_x = string_xl;
			x_axis_title_y = string_yh;
			x_axis_title_width = string_xh - string_xl;
			x_axis_title_height = string_yh - string_yl;
			middle_bar_height = 2 * x_axis_title_height;
		}
		else
			middle_bar_height = 0;
		if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH)
			bottom_bar_height = 0;
		else
		{
			axis_title_bar_width = 0;
			for (size_t xy_set = 0, c = 0, i = 0; i != line_count; ++i)
				if (i != point_x_element)
				{
					error = gdt_calculate_string_rectangle(font, axis_title_font_height, axis_titles[i], &string_xl, &string_xh, &string_yl, &string_yh);
					if (error)
					{
						free(font);
						return error;
					}
					if (xy_set)
					{
						if (string_yl < axis_titles_yl)
							axis_titles_yl = string_yl;
						if (string_yh > axis_titles_yh)
							axis_titles_yh = string_yh;
					}
					else
					{
						xy_set = 1;
						axis_titles_yl = string_yl;
						axis_titles_yh = string_yh;
					}
					axis_title_bar_width += axis_color_line_length + axis_title_spacing + (string_xh - string_xl) + (c ? axis_title_spacing : 0);
					++c;
				}
			axis_title_bar_height = axis_titles_yh - axis_titles_yl;
			bottom_bar_height = 2 * axis_title_bar_height;
		}
	}
	else
	{
		middle_bar_height = 0;
		axis_title_bar_height = 0;
		bottom_bar_height = 0;
	}
	if (left_bar_width + grid_y_number_bar_width < after_grid_space_required)
		left_bar_width = after_grid_space_required - grid_y_number_bar_width;
	if (horizontal_space < before_grid_space_required)
		right_bar_width = before_grid_space_required;
	else
		right_bar_width = horizontal_space;
	graph_width = width - (left_bar_width + grid_y_number_bar_width + right_bar_width);
	graph_height = height - (top_bar_height + grid_x_number_bar_height + middle_bar_height + bottom_bar_height);
	if (graph_width < 0 || graph_height < 0)
	{
		free(font);
		return EINVAL;
	}
	if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_BACKGROUND)
		gdt_fill_bitmap(width, height, stride, pixels, background_color);
	gdt_internal_draw_graph_to_bitmap(flags,
		background_color, grid_color, x_axis_color, y_axis_color, grid_delta_x, grid_delta_y, graph_width, graph_height, stride,
		(uint32_t*)((uintptr_t)pixels + ((bottom_bar_height + middle_bar_height + grid_x_number_bar_height) * stride) + ((left_bar_width + grid_y_number_bar_width) * sizeof(uint32_t))),
		line_thickness, line_count, point_x_element, line_colors, point_count, points, &grid_info);
	if (flags & GDT_DRAW_GRAPH_FLAG_SOFT_LINES)
		gdt_blur_bitmap(width, height, stride, pixels, line_thickness / 2);
	//gdt_fill_bitmap(left_bar_width, height - top_bar_height, stride, ((uintptr_t)pixels + 0), 0xFF00FF00);
	//gdt_fill_bitmap(width, top_bar_height, stride, ((uintptr_t)pixels + ((height - top_bar_height) * stride)), 0xFFFF0000);
	if (graph_title_font_height > 0.0f)
		gdt_draw_string(width, height, stride, pixels,
			(float)((width / 2 - graph_title_width / 2) - graph_title_x), (float)(height - (top_bar_height / 4) - graph_title_y),
			font, graph_title_font_height, graph_title, graph_title_color);
	if (gird_number_font_height > 0.0f)
	{
		for (int i = 0; i != grid_info.line_count_y; ++i)
		{
			gird_number_string[gdt_print_grid_number(gird_number_string, grid_info.grid_min_y + (float)i * grid_info.delta_y, 3) - (whole_number_grid ? 2 : 0)] = 0;
			error = gdt_calculate_string_rectangle(font, gird_number_font_height, gird_number_string, &string_xl, &string_xh, &string_yl, &string_yh);
			if (error)
			{
				free(font);
				return error;
			}
			gdt_draw_string(width, height, stride, pixels,
				(float)(((left_bar_width + grid_y_number_bar_width) - right_bar_width - (string_xh - string_xl)) - string_xl),
				(float)(((bottom_bar_height + middle_bar_height + grid_x_number_bar_height) + ((string_yh - string_yl) / 2) + (float)i * ((float)graph_height / (float)(grid_info.line_count_y - 1))) - string_yh),
				font, gird_number_font_height, gird_number_string, grid_color);
		}
		for (int y_offset = bottom_bar_height + middle_bar_height + grid_x_number_bar_height - (grid_x_number_bar_height / 4), i = 0; i != grid_info.line_count_x; ++i)
		{
			gird_number_string[gdt_print_grid_number(gird_number_string, grid_info.grid_min_x + (float)i * grid_info.delta_x, 3) - (whole_number_grid ? 2 : 0)] = 0;
			error = gdt_calculate_string_rectangle(font, gird_number_font_height, gird_number_string, &string_xl, &string_xh, &string_yl, &string_yh);
			if (error)
			{
				free(font);
				return error;
			}
			gdt_draw_string(width, height, stride, pixels,
				(float)(((left_bar_width + grid_y_number_bar_width) + (float)i * ((float)graph_width / (float)(grid_info.line_count_x - 1))) - ((string_xh - string_xl) / 2) - string_xl),
				(float)(y_offset - string_yh), font, gird_number_font_height, gird_number_string, grid_color);
		}
	}
	//gdt_fill_bitmap(width - left_bar_width, middle_bar_height, stride, ((uintptr_t)pixels + (bottom_bar_height * stride) + (left_bar_width * sizeof(uint32_t))), 0xFF0000FF);
	//gdt_fill_bitmap(width - left_bar_width, bottom_bar_height, stride, ((uintptr_t)pixels + (left_bar_width * sizeof(uint32_t))), 0xFFFF0000);
	if (axis_title_font_height > 0.0f)
	{
		if (axis_titles[point_x_element])
			gdt_draw_string(width, height, stride, pixels,
				(float)(left_bar_width + ((width - left_bar_width) / 2 - x_axis_title_width / 2) - x_axis_title_x), (float)((bottom_bar_height + (middle_bar_height - (middle_bar_height / 4))) - x_axis_title_y),
				font, axis_title_font_height, axis_titles[point_x_element], axis_title_colors[point_x_element]);
		if (flags & GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH)
		{
			if (axis_titles[(int)point_x_element ^ 1])
			{
				if (y_axis_title_width > graph_height)
					return EINVAL;
				if ((((size_t)y_axis_title_width * sizeof(uint32_t)) / sizeof(uint32_t)) != y_axis_title_width ||
					((y_axis_title_height * ((size_t)y_axis_title_width * sizeof(uint32_t))) / ((size_t)y_axis_title_width * sizeof(uint32_t))) != y_axis_title_height)
					return ENOMEM;
				uint32_t* y_axis_title_buffer = (uint32_t*)malloc((size_t)y_axis_title_height * (size_t)y_axis_title_width * sizeof(uint32_t));
				if (!y_axis_title_buffer)
					return ENOMEM;
				int y_axis_title_offset_x = left_bar_width / 4;
				int y_axis_title_offset_y = (bottom_bar_height + middle_bar_height + grid_x_number_bar_height) + ((graph_height / 2) - (y_axis_title_width / 2));
				for (int x = 0; x != y_axis_title_width; ++x)
				{
					uint32_t* copy_row = (uint32_t*)((uintptr_t)pixels + ((y_axis_title_offset_y + x) * stride) + ((y_axis_title_offset_x + (y_axis_title_height - 1)) * sizeof(uint32_t)));
					for (int y = 0; y != y_axis_title_height; ++y)
						y_axis_title_buffer[y * y_axis_title_width + x] = copy_row[-y];
				}
				gdt_draw_string(y_axis_title_width, y_axis_title_height, (size_t)y_axis_title_width * sizeof(uint32_t), y_axis_title_buffer,
					(float)-y_axis_title_x, (float)(y_axis_title_height - y_axis_title_y), font, axis_title_font_height, axis_titles[(int)point_x_element ^ 1], axis_title_colors[(int)point_x_element ^ 1]);
				for (int x = 0; x != y_axis_title_width; ++x)
				{
					uint32_t* paste_row = (uint32_t*)((uintptr_t)pixels + ((y_axis_title_offset_y + x) * stride) + ((y_axis_title_offset_x + (y_axis_title_height - 1)) * sizeof(uint32_t)));
					for (int y = 0; y != y_axis_title_height; ++y)
						paste_row[-y] = y_axis_title_buffer[y * y_axis_title_width + x];
				}
				free(y_axis_title_buffer);
			}
		}
		else
			for (int x_offset = left_bar_width + ((width - left_bar_width) / 2) - (axis_title_bar_width / 2), y_offset = bottom_bar_height - (bottom_bar_height / 4), i = 0; i != (int)line_count; ++i)
				if (i != point_x_element)
				{
					gdt_draw_dot_to_bitmap(width, height, stride, pixels, line_colors[i], line_thickness / 2, x_offset, bottom_bar_height / 2);
					gdt_draw_line_to_bitmap(width, height, stride, pixels, line_colors[i], line_thickness, (struct gdt_iv2_t) { x_offset, bottom_bar_height / 2 }, (struct gdt_iv2_t) { x_offset + axis_color_line_length, bottom_bar_height / 2 });
					gdt_draw_dot_to_bitmap(width, height, stride, pixels, line_colors[i], line_thickness / 2, x_offset + axis_color_line_length, bottom_bar_height / 2);
					x_offset += axis_color_line_length + axis_title_spacing;
					error = gdt_calculate_string_rectangle(font, axis_title_font_height, axis_titles[i], &string_xl, &string_xh, &string_yl, &string_yh);
					if (error)
					{
						free(font);
						return error;
					}
					gdt_draw_string(width, height, stride, pixels,
						(float)(x_offset - string_xl), (float)(y_offset - string_yh),
						font, axis_title_font_height, axis_titles[i], axis_title_colors[i]);
					x_offset += (string_xh - string_xl) + axis_title_spacing;
				}
	}
	free(font);
	return 0;
}

static int gdt_utf8_to_unicode(size_t utf8_string_size, const char* utf8_string, size_t unicode_buffer_length, size_t* unicode_character_count, uint32_t* unicode_buffer)
{
	size_t character_count = 0;
	for (size_t character_offset = 0; character_offset != utf8_string_size;)
	{
		int invalid_data = 0;
		size_t character_size;
		uint32_t character_code;
		uint32_t utf8_data;
		int high_set_bit_count;
		size_t character_size_limit = utf8_string_size - character_offset;
		if (character_size_limit > 3)
			utf8_data =
			(uint32_t)*(((const uint8_t*)utf8_string) + character_offset) |
			((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + 1) << 8) |
			((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + 2) << 16) |
			((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + 3) << 24);
		else
		{
			utf8_data = 0;
			for (size_t byte_index = 0; byte_index != character_size_limit; ++byte_index)
				utf8_data |= ((uint32_t)*(((const uint8_t*)utf8_string) + character_offset + byte_index) << (byte_index << 3));
		}
		high_set_bit_count = 0;
		while (high_set_bit_count != 5 && (utf8_data & (uint32_t)(0x80 >> high_set_bit_count)))
			++high_set_bit_count;
		switch (high_set_bit_count)
		{
			case 0:
			{
				character_size = 1;
				character_code = utf8_data & 0x7F;
				break;
			}
			case 2:
			{
				if (((utf8_data >> 14) & 0x03) == 0x02)
				{
					character_size = 2;
					character_code = ((utf8_data & 0x1F) << 6) | ((utf8_data >> 8) & 0x3F);
				}
				else
					invalid_data = 1;
				break;
			}
			case 3:
			{
				if (((utf8_data >> 14) & 0x03) == 0x02 && ((utf8_data >> 22) & 0x03) == 0x02)
				{
					character_size = 3;
					character_code = ((utf8_data & 0x0F) << 12) | (((utf8_data >> 8) & 0x3F) << 6) | ((utf8_data >> 16) & 0x3F);
				}
				else
					invalid_data = 1;
				break;
			}
			case 4:
			{
				if (((utf8_data >> 14) & 0x03) == 0x02 && ((utf8_data >> 22) & 0x03) == 0x02 && ((utf8_data >> 30) & 0x03) == 0x02)
				{
					character_size = 4;
					character_code = ((utf8_data & 0x0F) << 18) | (((utf8_data >> 8) & 0x3F) << 12) | (((utf8_data >> 16) & 0x3F) << 6) | ((utf8_data >> 24) & 0x3F);
				}
				else
					invalid_data = 1;
				break;
			}
			default:
			{
				invalid_data = 1;
				break;
			}
		}
		if (invalid_data)
		{
			character_size = 1;
			while (character_size != character_size_limit && invalid_data)
			{
				utf8_data = (uint32_t)*(((const uint8_t*)utf8_string) + character_offset + character_size);
				high_set_bit_count = 0;
				while (high_set_bit_count != 5 && (utf8_data & (uint32_t)(0x80 >> high_set_bit_count)))
					++high_set_bit_count;
				switch (high_set_bit_count)
				{
					case 0:
					{
						invalid_data = 0;
						break;
					}
					case 2:
					{
						if (character_size_limit > (character_size + 1) &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 1) >> 6) == 0x02)
							invalid_data = 0;
						break;
					}
					case 3:
					{
						if (character_size_limit > (character_size + 2) &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 1) >> 6) == 0x02 &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 2) >> 6) == 0x02)
							invalid_data = 0;
						break;
					}
					case 5:
					{
						if (character_size_limit > (character_size + 2) &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 1) >> 6) == 0x02 &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 2) >> 6) == 0x02 &&
							(*(((const uint8_t*)utf8_string) + character_offset + character_size + 3) >> 6) == 0x02)
							invalid_data = 0;
						break;
					}
					default:
						break;
				}
				if (invalid_data)
					++character_size;
			}
			character_code = 0xFFFD;
		}
		if (character_count < unicode_buffer_length)
			unicode_buffer[character_count] = character_code;
		++character_count;
		character_offset += character_size;
	}
	*unicode_character_count = character_count;
	if (character_count > unicode_buffer_length)
		return ENOBUFS;
	else
		return 0;
}

static int gdt_utf8_to_new_unicode_buffer(const char* utf8_string, uint32_t** unicode_buffer)
{
	size_t length = strlen(utf8_string);
	if (!(length + 1) || (((length + 1) * sizeof(uint32_t)) / sizeof(uint32_t)) != (length + 1))
		return ENOMEM;
	uint32_t* new_buffer = (uint32_t*)malloc((length + 1) * sizeof(uint32_t));
	if (!new_buffer)
		return ENOMEM;
	int error = gdt_utf8_to_unicode(length, utf8_string, length, &length, new_buffer);
	if (error)
	{
		free(new_buffer);
		return error;
	}
	new_buffer[length] = 0;
	*unicode_buffer = new_buffer;
	return 0;
}

int gdt_draw_graph_with_titles_to_bitmap(int flags, uint32_t background_color, uint32_t grid_color, uint32_t x_axis_color,
	uint32_t y_axis_color, float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, int line_thickness, size_t line_count, size_t point_x_element, uint32_t* line_colors,
	size_t point_count, const float* points, const char* truetype_file_name, float gird_number_font_height,
	float graph_title_font_height, uint32_t graph_title_color, const char* graph_title, float axis_title_font_height,
	const uint32_t* axis_title_colors, const char** axis_titles)
{
	int error;
	uint32_t* unicode_graph_title = 0;
	uint32_t** unicode_axis_titles = 0;
	if (graph_title)
	{
		error = gdt_utf8_to_new_unicode_buffer(graph_title, &unicode_graph_title);
		if (error)
			return error;
	}
	if (axis_titles)
	{
		unicode_axis_titles = (uint32_t**)malloc(line_count * sizeof(uint32_t*));
		if (!unicode_axis_titles)
		{
			if (unicode_graph_title)
				free(unicode_graph_title);
			return ENOMEM;
		}
		for (size_t i = 0; i != line_count; ++i)
			if (axis_titles[i])
			{
				error = gdt_utf8_to_new_unicode_buffer(axis_titles[i], &unicode_axis_titles[i]);
				if (error)
				{
					for (size_t j = 0; j != i; ++j)
						free(unicode_axis_titles[j]);
					free(unicode_axis_titles);
					if (unicode_graph_title)
						free(unicode_graph_title);
					return error;
				}
			}
			else
				unicode_axis_titles[i] = 0;
	}
	error = gdt_internal_draw_graph_with_titles_to_bitmap(flags, background_color, grid_color, x_axis_color,
		y_axis_color, grid_delta_x, grid_delta_y, width, height, stride,
		pixels, line_thickness, line_count, point_x_element, line_colors,
		point_count, points, truetype_file_name, gird_number_font_height,
		graph_title_font_height, graph_title_color, (const uint32_t*)unicode_graph_title, axis_title_font_height,
		axis_title_colors, (const uint32_t**)unicode_axis_titles);
	if (unicode_axis_titles)
	{
		for (size_t i = 0; i != line_count; ++i)
			free(unicode_axis_titles[i]);
		free(unicode_axis_titles);
	}
	if (unicode_graph_title)
		free(unicode_graph_title);
	return error;
}

int gdt_simplified_draw_graph_with_titles_to_bitmap(float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, size_t line_count, size_t point_count, const float* points, const char* truetype_file_name, const char* graph_title,
	const char** axis_titles, int soa_representation)
{
	GDT_CORE_ASSUME((uintptr_t)pixels % sizeof(uint32_t) == 0 && stride % sizeof(uint32_t) == 0 && width > -1 && height > -1);
	if (line_count < 2 || line_count > INT_MAX || width < 1 || height < 1 ||
		grid_delta_x < 0.0f || grid_delta_y < 0.0f || !point_count)
		return EINVAL;
	const uint32_t background_color = 0xFF222222;
	const uint32_t grid_color = 0xFF444444;
	const uint32_t font_color = 0xFFEEEEEE;
	const uint32_t xy_line_colors[2] = { 0xFF000000, 0xFF0055EE };
	const uint32_t xy_axis_title_colors[2] = { font_color, font_color };
	uint32_t* line_colors;
	uint32_t* axis_title_colors;
	if (line_count == 2)
	{
		line_colors = (uint32_t*)xy_line_colors;
		axis_title_colors = (uint32_t*)xy_axis_title_colors;
	}
	else
	{
		line_colors = (uint32_t*)malloc(2 * line_count * sizeof(uint32_t));
		if (!line_colors)
			return ENOMEM;
		for (int i = 0; i != (int)line_count; ++i)
			line_colors[i] = gdt_make_line_color((int)line_count, i);
		axis_title_colors = line_colors + line_count;
		for (int i = 0; i != (int)line_count; ++i)
			axis_title_colors[i] = font_color;
	}
	int error = gdt_draw_graph_with_titles_to_bitmap(
		(soa_representation ? GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION : 0) |
		GDT_DRAW_GRAPH_FLAG_DRAW_AUTO_SCALE_FONT | GDT_DRAW_GRAPH_FLAG_DRAW_BACKGROUND |
		((grid_delta_x != 0.0f && grid_delta_y != 0.0f) ? GDT_DRAW_GRAPH_FLAG_DRAW_GRID : 0) |
		((line_count == 2) ? GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH : 0),
		background_color, grid_color, 0, 0,
		grid_delta_x, grid_delta_y, width, height, stride,
		pixels, (width < height ? width : height) / 180, line_count, 0, line_colors,
		point_count, points, truetype_file_name, 0.0f,
		0.0f, font_color, graph_title, 0.0f,
		axis_title_colors, axis_titles);
	if (line_colors != xy_line_colors)
		free(line_colors);
	return error;
}

#ifdef _MSC_VER
#pragma float_control(pop)
#endif
