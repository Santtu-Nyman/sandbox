/*
	Cool Water Dispenser test data generator version 0.2.3 2019-03-17 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Simple command line tool for generating test data for cold water dispenser server.
		Read function print_instructions to see the instructions how to use the program.
		
	Version history
		version 0.2.3 2019-03-17
			Server API version argument added.
		version 0.2.2 2019-03-17
			Preparing for new server API.
		version 0.2.1 2019-03-11
			Fixed simulation operation mode bug when failed to load configuration from server.
		version 0.2.0 2019-03-08
			simulation quality increased and libcurl replaced by curl.
		version 0.1.1 2019-03-02
			Specified the fact that device id is a decimal number.
		version 0.1.0 2019-02-22
			Instructions how to use the program added.
		version 0.0.1 2019-02-22
			First version.
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "cwd_common.h"

static void print_instructions();

static int random_chance(float chance);

int main(int argc, char** argv)
{
	printf("cwd test data generating program\n");

	for (int i = 0; i != argc; ++i)
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
		{
			print_instructions();
			return EXIT_SUCCESS;
		}

	int curl_installed_with_http = 0;
	if (cwd_is_curl_installed_with_http(&curl_installed_with_http) || !curl_installed_with_http)
	{
		printf("Failed to find curl installation with http support. Install curl to use this program\n");
		return EXIT_FAILURE;
	}

	char* server_api_version_string = cwd_search_for_argument(argc, argv, "--server_api");
	if (!server_api_version_string)
	{
		printf("No server api version specified initialization failed use --server_api 1 or 2\n");
		return EXIT_FAILURE;
	}
	int server_api_version = 0;
	while (*server_api_version_string)
	{
		if (*server_api_version_string >= '0' && *server_api_version_string <= '9')
			server_api_version = 10 * server_api_version + (int)(*server_api_version_string++ - '0');
		else
		{
			printf("Specified server API version is not desimal number initialization failed use --server_api 1 or some other desimal number\n");
			return EXIT_FAILURE;
		}
	}

	char* server = cwd_search_for_argument(argc, argv, "--server");
	if (!server)
	{
		printf("No server specified initialization failed use --server \"www.students.oamk.fi\" or some other server\n");
		return EXIT_FAILURE;
	}

	char* device_id_string = cwd_search_for_argument(argc, argv, "--device_id");
	if (!device_id_string)
	{
		printf("No device id specified initialization failed use --device_id 9 or some other id\n");
		return EXIT_FAILURE;
	}
	uint32_t device_id = 0;
	while (*device_id_string)
	{
		if (*device_id_string >= '0' && *device_id_string <= '9')
			device_id = 10 * device_id + (uint32_t)(*device_id_string++ - '0');
		else
		{
			printf("Specified device id is not device id initialization failed use --device_id 9 or some other valid id\n");
			return EXIT_FAILURE;
		}
	}

	printf("Server API version %i\n", server_api_version);
	if (server_api_version != 1)
	{
		printf("Server API version %i is not supported\n", server_api_version);
		return EXIT_FAILURE;
	}
	printf("Server device configuration is at \"%s%s%s\" and data goes to \"%s%s%s\"\nSending data to server...\n", CWD_URL_PROTOCOL, server, CWD_URL_DEVICE_CONFIGURATION_PATH, CWD_URL_PROTOCOL, server, CWD_URL_DATA_INPUT_PATH);
	
	srand((unsigned int)time(0));
	struct cwd_device_configuration_t configuration;
	cwd_send_device_startup(server_api_version, server, device_id, (uint64_t)time(0));
	if (cwd_get_device_configuration(server_api_version, server, device_id, &configuration))
	{
		printf("Program failed to load configuration from server this error is ignored device_operation_mode = 1 periodic_mesurement_delay = 60\n");
		configuration.device_operation_mode = 1;
		configuration.periodic_mesurement_delay = 60;
	}
	sleep(1);
	for (uint64_t water_level = 90, last_measurements = 0, current_time = 0;;)
	{
		time_t raw_time = time(0);
		struct tm* date = localtime(&raw_time);
		uint64_t current_time = (uint64_t)raw_time;
		if (current_time - last_measurements > configuration.periodic_mesurement_delay)
		{
			cwd_send_periodic_mesurements(server_api_version, server, device_id, current_time, (float)water_level + (float)(rand() & 1), (float)(15 + (int)(4.0f * cos((float)current_time * 0.01))) + ((float)(rand() % 31) / 10.0f));
			if (cwd_get_device_configuration(server_api_version, server, device_id, &configuration))
			{
				printf("Program failed to load configuration from server this error is ignored device_operation_mode = 1 periodic_mesurement_delay = 60\n");
				configuration.device_operation_mode = 1;
				configuration.periodic_mesurement_delay = 60;
			}
			last_measurements = current_time;
		}
		if (configuration.device_operation_mode == 1)
		{
			if ((date->tm_wday > 1 && date->tm_wday < 6) || random_chance(0.33f))
			{
				if ((!(date->tm_hour > 6 && date->tm_hour < 21) && random_chance(0.001f)) || ((date->tm_hour > 6 && date->tm_hour < 21) && random_chance(0.02f)))
					cwd_send_bypassing(server_api_version, server, device_id, current_time);
				else if ((date->tm_hour > 6 && date->tm_hour < 21) && water_level && random_chance(0.01f))
				{
					cwd_send_bypassing(server_api_version, server, device_id, current_time);
					if (!(rand() % 4))
					{
						sleep(2 + (rand() % 9));
						cwd_send_bypassing(server_api_version, server, device_id, current_time);
					}
					sleep(2 + (rand() % 9));
					uint32_t order_type = (uint32_t)(rand() % 3);
					if (order_type + 1 > (uint32_t)water_level)
						order_type = (uint32_t)water_level - 1;
					cwd_send_order(server_api_version, server, device_id, current_time, order_type);
					water_level -= (uint64_t)(order_type + 1);
				}
				else if ((date->tm_hour > 6 && date->tm_hour < 16) && !water_level && random_chance(0.009f))
					water_level = 99;
			}
		}
		sleep(1);
	}
	
	return EXIT_FAILURE;
}

static void print_instructions()
{
	printf(
		"Program description:\n"
		"	Simple command line tool for generating test data for cold water dispenser server.\n"
		"	curl is required to be installed to use this program.\n"
		"	Stop the streaming by pressing CTRL+C.\n"
		"Parameter List:\n"
		"	-h or --help Displays help message.\n"
		"	--server Specifies the name of the server where data is send.\n"
		"	--device_id Desimal number that specifies id of the simulated device.\n"
		"	--server_api Desimal number that specifies server's web API version.");
}

static int random_chance(float chance)
{
	return (int)(((float)rand() / (float)RAND_MAX) <= chance);
}
