#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#ifdef _WIN32
#include <Windows.h>
#endif

int main()
{
#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
#endif
	printf(
		"\n"
		"  CCCC\n"
		" C    CC\n"
		"C\n"
		"C       - C Ohjelmointi\n"
		"C\n"
		" C    CC\n"
		"  CCCC\n"
		"\n");
	return 0;
};