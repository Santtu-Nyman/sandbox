/*
	Graph Drawing Tool 1.1.0 2019-07-22 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#ifndef GDT_ERROR_H
#define GDT_ERROR_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

int gdt_get_error_info(int error, const char** name, const char** description);

#endif
