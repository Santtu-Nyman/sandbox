/*
	Graph Drawing Tool 1.1.0 2019-07-22 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#ifndef GDT_CORE_H
#define GDT_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define GDT_DRAW_GRAPH_FLAG_LINE_SOA_REPRESENTATION 0x1
#define GDT_DRAW_GRAPH_FLAG_DRAW_BACKGROUND 0x2
#define GDT_DRAW_GRAPH_FLAG_SOFT_LINES 0x4
#define GDT_DRAW_GRAPH_FLAG_DRAW_X_AXIS 0x8
#define GDT_DRAW_GRAPH_FLAG_DRAW_Y_AXIS 0x10
#define GDT_DRAW_GRAPH_FLAG_DRAW_GRID 0x20
#define GDT_DRAW_GRAPH_FLAG_DRAW_AUTO_SCALE_FONT 0x40
#define GDT_DRAW_GRAPH_FLAG_DRAW_X_TO_Y_GRAPH 0x80

void gdt_draw_graph_to_bitmap(int flags, uint32_t background_color, uint32_t grid_color, uint32_t x_axis_color,
	uint32_t y_axis_color, float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, int line_thickness, size_t line_count, size_t point_x_element, uint32_t* line_colors,
	size_t point_count, const float* points);

int gdt_simplified_draw_graph_to_bitmap(float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, size_t line_count, size_t point_count, const float* points, int soa_representation);

int gdt_draw_graph_with_titles_to_bitmap(int flags, uint32_t background_color, uint32_t grid_color, uint32_t x_axis_color,
	uint32_t y_axis_color, float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, int line_thickness, size_t line_count, size_t point_x_element, uint32_t* line_colors,
	size_t point_count, const float* points, const char* truetype_file_name, float gird_number_font_height,
	float graph_title_font_height, uint32_t graph_title_color, const char* graph_title, float axis_title_font_height,
	const uint32_t* axis_title_colors, const char** axis_titles);

int gdt_simplified_draw_graph_with_titles_to_bitmap(float grid_delta_x, float grid_delta_y, int width, int height, size_t stride,
	uint32_t* pixels, size_t line_count, size_t point_count, const float* points, const char* truetype_file_name, const char* graph_title,
	const char** axis_titles, int soa_representation);

#endif
