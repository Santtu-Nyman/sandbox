/*
	Graph Drawing Tool 1.1.0 2019-07-22 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#include "gdt_make_bitmap.h"
#include <string.h>
#include <stdlib.h>

static void gdt_unaligned_store_le32(void* memory_address, uint32_t value)
{
	*(uint8_t*)((uintptr_t)memory_address + 0) = (uint8_t)(value >> 0);
	*(uint8_t*)((uintptr_t)memory_address + 1) = (uint8_t)(value >> 8);
	*(uint8_t*)((uintptr_t)memory_address + 2) = (uint8_t)(value >> 16);
	*(uint8_t*)((uintptr_t)memory_address + 3) = (uint8_t)(value >> 24);
}

int gdt_make_bitmap(int width, int height, size_t stride, const uint32_t* pixels, size_t* bitmap_file_size, void** bitmap_file_data)
{
	static const uint8_t bmp_header_template[] = {
		0x42, 0x4D, /* bmp signature */
		0, 0, 0, 0, /* file size */
		0, 0, 0, 0, /* reserved */
		128, 0, 0, 0, /* pixel data offset */
		108, 0, 0, 0, /* DIB header size */
		0, 0, 0, 0, /* image width */
		0, 0, 0, 0, /* image height */
		1, 0, /* number of color planes */
		32, 0, /* bits per pixel */
		3, 0, 0, 0, /* BI_BITFIELDS */
		0, 0, 0, 0, /* bitmap data size */
		0x13, 0x0B, 0, 0, /* horizontal pixels per meter */
		0x13, 0x0B, 0, 0, /* vertical pixels per meter */
		0, 0, 0, 0, /* number of colors in palette */
		0, 0, 0, 0, /* number of important colors */
		0, 0, 0xFF, 0, /* red channel mask */
		0, 0xFF, 0, 0, /* green channel mask */
		0xFF, 0, 0, 0, /* blue channel mask */
		0, 0, 0, 0xFF, /* alpha channel mask */
		0x20, 0x6E, 0x69, 0x57, /* LCS_WINDOWS_COLOR_SPACE */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0x47, 0x44, 0x54, 0 };
	if (sizeof(size_t) < sizeof(uint32_t))
		return ENOSYS;
	if (width < 0 || height < 0 || ((sizeof(int) > 4) && (width > 0x7FFFFFFF || height > 0x7FFFFFFF)))
		return EINVAL;
	if (((((uint32_t)width * (uint32_t)sizeof(uint32_t)) / (uint32_t)sizeof(uint32_t)) != (uint32_t)width) ||
		((((uint32_t)height * ((uint32_t)width * (uint32_t)sizeof(uint32_t))) / ((uint32_t)width * (uint32_t)sizeof(uint32_t))) != (uint32_t)height) ||
		((((uint32_t)height * ((uint32_t)width * (uint32_t)sizeof(uint32_t))) + (uint32_t)sizeof(bmp_header_template)) < ((uint32_t)height * ((uint32_t)width * (uint32_t)sizeof(uint32_t)))))
		return EFBIG;
	size_t file_stride = (size_t)width * sizeof(uint32_t);
	size_t file_size = sizeof(bmp_header_template) + ((size_t)height * file_stride);
	uintptr_t file_data = (uintptr_t)malloc(file_size);
	if (!file_data)
		return ENOMEM;
	memcpy((void*)file_data, bmp_header_template, sizeof(bmp_header_template));
	gdt_unaligned_store_le32((void*)(file_data + 2), (uint32_t)file_size);
	gdt_unaligned_store_le32((void*)(file_data + 18), (uint32_t)width);
	gdt_unaligned_store_le32((void*)(file_data + 22), (uint32_t)height);
	gdt_unaligned_store_le32((void*)(file_data + 34), (uint32_t)height * (uint32_t)file_stride);
	size_t source_row_skip = stride - ((size_t)width * sizeof(uint32_t));
	for (uint32_t* source = (uint32_t*)pixels, * source_end = (uint32_t*)((uintptr_t)pixels + ((size_t)height * stride)),
		* destination = (uint32_t*)(file_data + 128); source != source_end; source = (uint32_t*)((uintptr_t)source + source_row_skip))
		for (uint32_t* source_row_end = source + width; source != source_row_end; ++source, ++destination)
			*destination = *source;
	*bitmap_file_size = file_size;
	*bitmap_file_data = (void*)file_data;
	return 0;
}