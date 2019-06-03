/*
	Cool Water Dispenser shared interface for bcm2835 library version 1.0.0 2019-03-02 written by Santtu Nyman.
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

static volatile int futex;
static volatile int initialized;

static void acquire_futex(volatile int* futex)
{
	while (!__sync_bool_compare_and_swap(futex, 0, 1))
		syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

static void release_futex(volatile int* futex)
{
	__sync_bool_compare_and_swap(futex, 1, 0);
	syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

int cwd_sbcm2835_initialize()
{
	acquire_futex(&futex);
	int local_initialized = initialized;
	if (!local_initialized && !bcm2835_init())
	{
		release_futex(&futex);
		return EIO;
	}
	initialized = local_initialized + 1;
	release_futex(&futex);
	return 0;
}

void cwd_sbcm2835_close()
{
	acquire_futex(&futex);
	int local_initialized = initialized;
	if (local_initialized > 1)
		initialized = local_initialized - 1;
	else
	{
		initialized = 0;
		if (local_initialized == 1)
			bcm2835_close();
	}
	release_futex(&futex);
}

#endif