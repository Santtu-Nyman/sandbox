/*
	Cool Water Dispenser test UI output generator version 0.0.0 2019-03-14 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Test program for generating fake UI output.
		
	Version history
		version 0.0.0 2019-03-14
			First version.
*/

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	srand((unsigned int)time(0));
	for (;;sleep(1 + (rand() % 7)))
	{
		time_t current_time = time(0);
		printf("cwd_ui_output_generator.c %jd!\n", (intmax_t)current_time);
		int c = rand() % 10;
		if (c)
		{
			for (int i = 0; i != c; ++i)
			{
				if (i)
					printf(" ");
				printf("TEST");
			}
			if (c == 9)
				printf(" 9");
			printf("\n");
		}
		if (rand() % 1)
			printf("!\n!!\n!!!\n");
		c = rand() % 31;
		if (c)
		{
			for (int i = 0; i != c; ++i)
				printf("?");
			printf("\n");
		}
		printf("!X\n");
		printf("printf(\"\");\n");
		printf("!%i\n", 1 + (rand() % 3));
		printf("!XYZW\n");
		printf("\n\n%i\n\n", rand());
		fflush(stdout);
	}
	return EXIT_FAILURE;
}
