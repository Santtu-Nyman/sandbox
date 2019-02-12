#define _GNU_SOURCE
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int get_processor_count(size_t* processor_count)
{
	int count = get_nprocs();
	if (count < SIZE_MAX)
		*processor_count = (size_t)count;
	else
		*processor_count = SIZE_MAX;
	return 0;
}

void acquire_futex(int* futex)
{
	while (!__sync_bool_compare_and_swap(futex, 0, 1))
		syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

void release_futex(int* futex)
{
	__sync_bool_compare_and_swap(futex, 1, 0);
	syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

int create_thread(int (*entry)(void*), void* parameter, size_t stack_size)
{
	int error = 0;
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1)
	{
		error = errno;
		return error;
	}
	stack_size = (stack_size + ((size_t)page_size - 1)) & ~((size_t)page_size - 1); 
	void* stack = mmap(0, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (stack == MAP_FAILED)
	{
		error = errno;
		return error;
	}
	int child_id = (int)clone(entry, (void*)((uintptr_t)stack + stack_size), CLONE_FILES | CLONE_FS | CLONE_SIGHAND | CLONE_THREAD | CLONE_VM, parameter, 0, 0, 0);
	if (child_id == -1)
	{
		error = errno; 
		munmap(stack, stack_size);
		return error;
	}
	return error;
}

int get_scheduling(int* policy, struct sched_param* parameters)
{
	int scheduling_policy = sched_getscheduler(0);
	if (scheduling_policy == -1)
		return errno;
	if (sched_getparam (0, parameters) == -1)
		return errno;
	policy* = scheduling_policy;
	return 0;
}

int set_scheduling(int policy, struct sched_param* parameters)
{
	return sched_setschedule(0, policy, parameters) == -1 ? errno : 0; 
}

void test_print_stuff()
{
	pid_t pid = (pid_t)syscall(SYS_getpid);
	pid_t tid = (pid_t)syscall(SYS_gettid);
	printf("PID %i TID %i scheduling policy ", pid, tid);
	int policy = sched_getscheduler(0);
	struct sched_param parameters;
	switch (policy)
	{
		case SCHED_OTHER:
			sched_getparam(0, parameters);
			printf("normal with nice %i\n", nice(0));
			break;
		case SCHED_RR:
			sched_getparam(0, parameters);
			printf("round-robin with static priority %i\n", parameters.sched_priority);
			break;
		case SCHED_FIFO:
		
			printf("first-in, first-out with static priority %i\n", parameters.sched_priority);
			break;
		default:
			printf("unknown\n");
	}
}

void print_pid_and_tid()
{
	pid_t pid = (pid_t)syscall(SYS_getpid);
	pid_t tid = (pid_t)syscall(SYS_gettid);
	printf("PID %d TID %d\n", pid, tid);
}

static int hello_futex;
int hello(void* parameter)
{
	print_pid_and_tid();
	printf("waiting...\n");
	acquire_futex(&hello_futex);
	printf("hello from ");
	print_pid_and_tid();
	sleep(1);
	release_futex(&hello_futex);
	print_pid_and_tid();
	printf("done\n");
	return 0;
}

int main(int argc, char** argv)
{
	printf("begin\n");
	print_pid_and_tid();
	for (int c = 8; c--;)
	{
		errno = create_thread(hello, 0, 0x10000);
		perror("crete_thread");
	}
	sleep(16);
	printf("end\n");
	return 0;
}
