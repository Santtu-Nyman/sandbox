/*
	Graph Drawing Tool version 1.4.0 2019-09-16 by Santtu Nyman.
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
