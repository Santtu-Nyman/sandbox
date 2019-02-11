/*
	Cool Water Dispenser shared interface for bcm2835 library version 0.0.0 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
*/

#ifndef CWD_SBCM2835_H
#define CWD_SBCM2835_H

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "cwd_sbcm2835.h"
#include "bcm2835.h"

static int usage_counter;

static void cwd_sbcm2835_acquire_lock();

static void cwd_sbcm2835_release_lock();

static void cwd_sbcm2835_acquire_lock()
{
	// TODO: Acquire cwd_sbcm2835 lock here!
}

static void cwd_sbcm2835_release_lock()
{
	// TODO: Release cwd_sbcm2835 lock here!
}

int cwd_sbcm2835_initialize()
{
	cwd_sbcm2835_acquire_lock();
	if (usage_counter++)
	{
		if (!bcm2835_init())
		{
			--usage_counter;
			cwd_sbcm2835_release_lock();
			return EIO;
		}
	}
	cwd_sbcm2835_release_lock();
	return 0;
}

void cwd_sbcm2835_close()
{
	cwd_sbcm2835_acquire_lock();
	if (!--usage_counter)
	{
		bcm2835_close();
	}
	if (usage_counter < 0)
	{
		usage_counter = 0;
	}
	cwd_sbcm2835_release_lock();
}

#endif