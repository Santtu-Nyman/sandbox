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

int save_file(const char* name, size_t size, const void* data)
{
	int file = open(name, O_WRONLY | O_TRUNC | O_CREAT);
	if (file == -1)
		return errno;
	for (size_t written = 0; written != size;)
	{
		ssize_t write_result = write(file, (const void*)((uintptr_t)data + written), ((size - written) < (size_t)SSIZE_MAX) ? (size - written) : (size_t)SSIZE_MAX);
		if (write_result == -1)
		{
			int write_error = errno;
			if (write_error != EINTR)
			{
				unlink(name);
				close(file);
				return write_error;
			}
			write_result = 0;
		}
		written += (size_t)write_result;
	}
	if (fsync(file) == -1)
	{
		int flush_error = errno;
		unlink(name);
		close(file);
		return flush_error;
	}
	close(file);
	return 0;
}

int get_processor_count(size_t* processor_count)
{
	int count = get_nprocs();
	if (count < SIZE_MAX)
		*processor_count = (size_t)count;
	else
		*processor_count = SIZE_MAX;
	return 0;
}

void acquire_futex(volatile int* futex)
{
	while (!__sync_bool_compare_and_swap(futex, 0, 1))
		syscall(SYS_futex, futex, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

void release_futex(volatile int* futex)
{
	__sync_bool_compare_and_swap(futex, 1, 0);
	syscall(SYS_futex, futex, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0);
}

int create_thread(int (*entry)(void*), void* parameter, size_t stack_size)
{
	// NOTE: Thread created with this function should not terminate before the process terminates
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
	if (sched_getparam(0, parameters) == -1)
		return errno;
	*policy = scheduling_policy;
	return 0;
}

int set_scheduling(int policy, const struct sched_param* parameters)
{
	return sched_setscheduler(0, policy, parameters) == -1 ? errno : 0; 
}

void test_print_stuff()
{
	pid_t pid = (pid_t)syscall(SYS_getpid);
	pid_t tid = (pid_t)syscall(SYS_gettid);
	printf("PID %jd TID %jd scheduling policy ", (intmax_t)pid, (intmax_t)tid);
	int policy = sched_getscheduler(0);
	struct sched_param parameters;
	switch (policy)
	{
		case SCHED_OTHER:
			printf("normal with nice %i\n", nice(0));
			break;
		case SCHED_RR:
			sched_getparam(0, &parameters);
			printf("round-robin with static priority %i\n", parameters.sched_priority);
			break;
		case SCHED_FIFO:
			sched_getparam(0, &parameters);
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
	printf("PID %jd TID %jd\n", (intmax_t)pid, (intmax_t)tid);
}

static volatile int hello_futex;
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

void child_process_test()
{
	pid_t child_id = fork();
	test_print_stuff();
	if (child_id)
	{
		int status;
		int error = EINTR;
		while (error == EINTR)
		{
			pid_t terminated_id = wait(&status);
			if (terminated_id != -1)
			{
				if (terminated_id == child_id)
					error = 0;
			}
			else
				error = errno;
		}
	}
	else
		exit(0);
}

int bcm2835_library_smt_up_test(void* parameter)
{
	uint8_t input_pin = (uint8_t)((uintptr_t)parameter & 0xFF);
	uint8_t output_pin = (uint8_t)(((uintptr_t)parameter >> 8) & 0xFF);
	for (int error;;)
	{
		if (!bcm2835_init())
		{
			error = EIO;
			errno = error;
			perror("bcm2835_init");
			return error;
		}
		bcm2835_gpio_fsel(input_pin, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(input_pin, BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_fsel(output_pin, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(output_pin, LOW);
		for (int c = 16; c--;)
		{
			struct timespec wait_time = { 0, 16000000 };
			nanosleep(&wait_time, 0);
			bcm2835_gpio_write(output_pin, bcm2835_gpio_lev(input_pin));
		}
		if (!bcm_close())
		{
			error = EIO;
			errno = error;
			perror("bcm_close");
			return error;
		}
	}
	return 0;
}

int bcm2835_library_smt_up_test2(void* parameter)
{
	static volatile int futex;
	static volatile int initialized;
	uint8_t input_pin = (uint8_t)((uintptr_t)parameter & 0xFF);
	uint8_t output_pin = (uint8_t)(((uintptr_t)parameter >> 8) & 0xFF);
	for (int error;;)
	{
		acquire_futex(&futex);
		if (initialized)
			++initialized;
		else
		{
			if (!bcm2835_init())
			{
				release_futex(&futex);
				error = EIO;
				errno = error;
				perror("bcm2835_init");
				return error;
			}
			++initialized;
		}
		release_futex(&futex);
		bcm2835_gpio_fsel(input_pin, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(input_pin, BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_fsel(output_pin, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(output_pin, LOW);
		for (int c = 16; c--;)
		{
			struct timespec wait_time = { 0, 16000000 };
			nanosleep(&wait_time, 0);
			bcm2835_gpio_write(output_pin, bcm2835_gpio_lev(input_pin));
		}
		acquire_futex(&futex);
		if (initialized > 1)
			--initialized;
		else
		{
			if (!bcm_close())
			{
				release_futex(&futex);
				error = EIO;
				errno = error;
				perror("bcm_close");
				return error;
			}
			initialized = 0;
		}
		release_futex(&futex);
	}
	return 0;
}

void bcm2835_library_smt_up()
{
	create_thread(bcm2835_library_smt_up_test, (void*)(2 | (3 << 8)), 0x10000);
	create_thread(bcm2835_library_smt_up_test, (void*)(4 | (5 << 8)), 0x10000);
	create_thread(bcm2835_library_smt_up_test, (void*)(6 | (7 << 8)), 0x10000);
	for (;;)
		sleep(1);
}

int bcm2835_library_smt_mp_main(int argc, char** argv)
{
	uint8_t input_pin = (uint8_t)atoi(argv[1]);
	uint8_t output_pin = (uint8_t)atoi(argv[2]);
	for (int error;;)
	{
		if (!bcm2835_init())
		{
			error = EIO;
			errno = error;
			perror("bcm2835_init");
			return error;
		}
		bcm2835_gpio_fsel(input_pin, BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(input_pin, BCM2835_GPIO_PUD_OFF);
		bcm2835_gpio_fsel(output_pin, BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(output_pin, LOW);
		for (int c = 16; c--;)
		{
			struct timespec wait_time = { 0, 16000000 };
			nanosleep(&wait_time, 0);
			bcm2835_gpio_write(output_pin, bcm2835_gpio_lev(input_pin));
		}
		if (!bcm_close())
		{
			error = EIO;
			errno = error;
			perror("bcm_close");
			return error;
		}
	}
	return 0;
}

void bcm2835_library_smt_mp()
{
	const char* arguments[2] = { "bcm2835_smt_mp", 0 };
	execvp("bcm2835_smt_mp", &arguments);

}

int main(int argc, char** argv)
{
	int original_scheduling_policy;
	struct sched_param original_scheduling_parameters;
	int error = get_scheduling(&original_scheduling_policy, &original_scheduling_parameters);
	if (errno)
	{
		errno = error;
		perror("get_scheduling");
	}
	test_print_stuff();
	struct sched_param temporal_cheduling_parameters;
	memset(&temporal_cheduling_parameters, 0, sizeof(struct sched_param));
	temporal_cheduling_parameters.sched_priority = 1;
	if (sched_setscheduler(0, SCHED_FIFO, &temporal_cheduling_parameters) == -1)
		perror("sched_setscheduler");
	child_process_test();
	if (sched_setscheduler(0, original_scheduling_policy, &original_scheduling_parameters) == -1)
		perror("sched_setscheduler");
	test_print_stuff();
	return 0;
}
