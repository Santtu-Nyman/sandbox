/*
	Graph Drawing Tool 1.1.0 2019-07-22 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#ifndef GDT_MAKE_BITMAP_H
#define GDT_MAKE_BITMAP_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

int gdt_make_bitmap(int width, int height, size_t stride, const uint32_t* pixels, size_t* file_size, void** file_data);

#endif
