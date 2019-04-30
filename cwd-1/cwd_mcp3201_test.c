/*
	MCP3201 test version 1.0.0 2019-03-02 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Simple program to test MCP3201 ADC using Raspberry Pi.
		
	Version history
		version 1.0.0 2019-03-02
			First version.
*/

#define MCP_SPI_CS 21
#define MCP_SPI_CLK 20
#define MCP_SPI_DOUT 16

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <linux/futex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bcm2835.h"
#include "cwd_sbcm2835.h"
#include "cwd_mcp3201.h"

int main(int argc, char** argv)
{
	int original_scheduling_policy = sched_getscheduler(0);
	if (original_scheduling_policy == -1)
	{
		printf("Error 1: Getting scheduling info failed\n");
		return -1;
	}
	struct sched_param original_scheduling_parameters;
	if (sched_getparam(0, &original_scheduling_parameters) == -1)
	{
		printf("Error 2: Getting scheduling info failed\n");
		return -1;
	}
	int scheduling_policy = SCHED_FIFO;
	struct sched_param scheduling_parameters;
	memset(&scheduling_parameters, 0, sizeof(struct sched_param));
	scheduling_parameters.sched_priority = 1;
	if (sched_setscheduler(0, scheduling_policy, &scheduling_parameters) == -1)
	{
		printf("Error 3: Setting now scheduling failed\n");
		return -1;
	}
	if (cwd_sbcm2835_initialize())
	{
		printf("Error 4: bcm2835 library initialization failed\n");
		return -1;
	}
	printf("Reading MCP3201\n");
	for (;;)
	{
		printf("%i\n", cwd_mcp3201_read(MCP_SPI_CS, MCP_SPI_CLK, MCP_SPI_DOUT));
		sleep(1);
	}
	return -1;
}
