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
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1)
		return errno;
	stack_size = (stack_size + ((size_t)page_size - 1)) & ~((size_t)page_size - 1); 
	void* stack = (void*)malloc(stack_size);
	if (!stack)
		return ENOMEM;
	int tmp = (int)syscall(SYS_clone, entry, (void*)((uintptr_t)stack + stack_size), CLONE_FILES | CLONE_FS | CLONE_IO | CLONE_PARENT | CLONE_SIGHAND | CLONE_THREAD | CLONE_VM, parameter);
}

void print_pid_and_tid()
{
	pid_t pid = (pid_t)syscall(SYS_getpid);
	pid_t tid = (pid_t)syscall(SYS_gettid);
	printf("PID %l TID %l\n", pid, tid);
}

int main(int argc, char** argv)
{
	print_pid_and_tid();
	sleep(1);
	return 0;
}
