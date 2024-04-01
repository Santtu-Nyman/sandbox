#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>

int main()
{
	for (;;)
	{
		int Test;
		printf("Enter secret number\n");
		scanf("%i", &Test);
		if (Test == 17)
		{
			printf("Number is ok\n");
			return 0;
		}
		else
			printf("Wrong number!\n");
	}
};