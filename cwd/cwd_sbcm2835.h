/*
	Cool Water Dispenser shared interface for bcm2835 library version 0.0.0 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Shared interface to bcm2835 library for the Cool Water Dispenser project.
*/

#ifndef CWD_SBCM2835_H
#define CWD_SBCM2835_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include "bcm2835.h"

int cwd_sbcm2835_initialize();
/*
	Description
		This function increments bcm2835 library usage counter and
		initalizes the library with a call to bcm2835_init if the counter was zero.
	Parameters
		Function has no parameters.
	Return
		Function returns zero on success and errno value on failure.
*/

void cwd_sbcm2835_close();
/*
	Description
		This function decrements bcm2835 library usage counter and
		frees resources used by the bcm2835 library with a call to bcm2835_close
		when the counter reaches zero.
	Parameters
		Function has no parameters.
	Return
		Function has no return value.
*/

#endif