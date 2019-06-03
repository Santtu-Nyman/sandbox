#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>

int main()
{
	int Test;
	scanf("%i", &Test);
	printf("%i is %s\n", Test, Test ? Test < 0 ? "negative" : "positive" : "neutral");
	return 0;
};