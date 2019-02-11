/*
	Cool Water Dispenser shared interface for bcm2835 library version 0.0.0 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
*/

#ifndef CWD_SBCM2835_H
#define CWD_SBCM2835_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <linux/futex.h>
#include "cwd_sbcm2835.h"
#include "bcm2835.h"

static int futex;
static int usage_counter;

static void acquire_futex(int* futex)
{
	while (!__sync_bool_compare_and_swap(futex, 0, 1))
		syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

static void release_futex(int* futex)
{
	__sync_bool_compare_and_swap(futex, 1, 0);
	syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

int cwd_sbcm2835_initialize()
{
	acquire_futex(&futex);
	if (usage_counter++)
	{
		if (!bcm2835_init())
		{
			--usage_counter;
			release_futex(&futex);
			return EIO;
		}
	}
	release_futex(&futex);
	return 0;
}

void cwd_sbcm2835_close()
{
	acquire_futex(&futex);
	if (!--usage_counter)
	{
		bcm2835_close();
	}
	if (usage_counter < 0)
	{
		usage_counter = 0;
	}
	release_futex(&futex);
}

#endif