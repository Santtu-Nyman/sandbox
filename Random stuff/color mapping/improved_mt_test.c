#ifdef __cplusplus
extern "C" {
#endif

#include "improved_mt_test.h"
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

//void debug_assert(int x, int location) { if (!x) *(volatile int*)0 = location; }
//#define assert(x) debug_assert((int)((x) != 0), (int)(__LINE__))

static HMODULE Advapi32;
static BOOLEAN (WINAPI* RtlGenRandom)(PVOID RandomBuffer, ULONG RandomBufferLength);

void debug_random_delay()
{
	DWORD random;
	RtlGenRandom(&random, sizeof(DWORD));
	Sleep(random & 0xF);
}

#define TEST_THREAD_COUNT 8

typedef struct mt_test_t
{
	long thread_count;
	volatile long start_counter;
	volatile long completed_counter;
	HANDLE work_start_event;
	HANDLE work_completed_event;
	volatile void (WINAPI* work)(struct mt_test_t* test, size_t thread_index);
	DWORD thread_id_table[TEST_THREAD_COUNT];
	HANDLE thread_handle_table[TEST_THREAD_COUNT];
	volatile DWORD thread_result_table[TEST_THREAD_COUNT];
} mt_test_t;

void mt_execute_work_block(mt_test_t* test, size_t thread_index)
{
	if (!InterlockedDecrement(&test->start_counter))
	{
		test->start_counter = test->thread_count;
		ResetEvent(test->work_completed_event);
		SetEvent(test->work_start_event);
	}
	else
	{
		WaitForSingleObject(test->work_start_event, INFINITE);
	}

	test->work(test, thread_index);
	//printf("Thread %zu did something\n", thread_index);

	if (!InterlockedDecrement(&test->completed_counter))
	{
		test->completed_counter = test->thread_count;
		ResetEvent(test->work_start_event);
		SetEvent(test->work_completed_event);
	}
	else
	{
		WaitForSingleObject(test->work_completed_event, INFINITE);
	}
}

void mt_execute_work(mt_test_t* test, void (WINAPI* work)(mt_test_t* test, size_t thread_index))
{
	test->work = work;
	mt_execute_work_block(test, 0);
}

DWORD CALLBACK mt_test_procedure(mt_test_t* test)
{
	size_t thread_index = 1;
	for (DWORD thread_id = GetCurrentThreadId(); test->thread_id_table[thread_index] != thread_id;)
		++thread_index;
	printf("Thread %zu created\n", thread_index);

	for (;;)
		mt_execute_work_block(test, thread_index);

	return 1;
}

void WINAPI exit_work(mt_test_t* test, size_t thread_index)
{
	HANDLE work_completed_event = test->work_completed_event;
	if (!InterlockedDecrement(&test->completed_counter))
	{
		test->completed_counter = test->thread_count;
		SetEvent(work_completed_event);
	}
	else
	{
		WaitForSingleObject(work_completed_event, INFINITE); /* Note that event may be invalid and test may be freed */
	}

	printf("Thread %zu died\n", thread_index);
	ExitThread(0);
}

void WINAPI test_work(mt_test_t* test, size_t thread_index)
{
	test->thread_result_table[thread_index] = thread_index;
}

void end_test(mt_test_t* test)
{
	test->work = exit_work;

	if (!InterlockedDecrement(&test->start_counter))
	{
		test->start_counter = test->thread_count;
		ResetEvent(test->work_completed_event);
		SetEvent(test->work_start_event);
	}
	else
	{
		WaitForSingleObject(test->work_start_event, INFINITE);
	}

	if (!InterlockedDecrement(&test->completed_counter))
	{
		test->completed_counter = test->thread_count;
		SetEvent(test->work_completed_event);
	}
	else
	{
		WaitForSingleObject(test->work_completed_event, INFINITE);
	}

	for (size_t i = 1; i != test->thread_count; ++i)
		CloseHandle(test->thread_handle_table[i]);

	VirtualFree(test, 0, MEM_RELEASE);
}

mt_test_t* begin_test(long thread_count)
{
	assert(thread_count > 0 && thread_count <= TEST_THREAD_COUNT);

	mt_test_t* test = (mt_test_t*)VirtualAlloc(0, sizeof(mt_test_t), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assert(test);

	test->thread_count = thread_count;
	test->start_counter = thread_count;
	test->completed_counter = thread_count;
	test->work_start_event = CreateEventW(0, TRUE, FALSE, 0);
	test->work_completed_event = CreateEventW(0, TRUE, FALSE, 0);
	test->work = 0;

	test->thread_handle_table[0] = 0;
	test->thread_id_table[0] = 0xFFFFFFFF;
	for (size_t i = 1; i != test->thread_count; ++i)
	{
		test->thread_handle_table[i] = CreateThread(0, 0, mt_test_procedure, test, CREATE_SUSPENDED, test->thread_id_table + i);
		assert(test->thread_handle_table[i]);
	}

	for (size_t i = 1; i != test->thread_count; ++i)
		ResumeThread(test->thread_handle_table[i]);

	return test;
}

void run_improved_mt_test()
{
	Advapi32 = LoadLibraryW(L"Advapi32.dll");
	RtlGenRandom = (BOOLEAN (WINAPI*)(PVOID, ULONG))GetProcAddress(Advapi32, "SystemFunction036");

	for (long i = 0; i != 1000; ++i)
	{
		for (long n = TEST_THREAD_COUNT; n != 0; --n)
		{
			mt_test_t* test = begin_test(n);
			for (long c = 0; c != 100; ++c)
			{
				for (size_t i = 0; i != n; ++i)
					test->thread_result_table[i] = 0xFFFFFFFF;

				mt_execute_work(test, test_work);

				for (size_t i = 0; i != n; ++i)
					if (test->thread_result_table[i] != (DWORD)i)
						assert(0);
			}
			end_test(test);

			printf("%lu (%lu) test successful\n", i, n);
		}
	}


	int dummy = 0;
	Sleep(1000);
	FreeLibrary(Advapi32);
	Advapi32 = 0;
	RtlGenRandom = 0;
}

#ifdef __cplusplus
}
#endif