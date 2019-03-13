/*
	Cool Water Dispenser test data generator version 0.0.0 2019-03-08 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Cold Water Dispenser Raspberry Pi device controlling software.
		The UI part of the software is on separate source and executable.
		
	Version history
		version 0.0.1 2019-03-13
			Second incomplete version.
		version 0.0.0 2019-03-08
			First incomplete version.
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
#include <signal.h>
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
#include "cwd_common.h"

void print_scheduling_info();

int cwd_old_create_ui_process(const char* ui_executable_name, size_t ui_process_argument_count, const char** ui_process_arguments, int ui_process_processor_index, pid_t* ui_process_id, int* ui_process_output)
{
	int error;
	int pipe_out_in[2];
	size_t processor_count;
	int* processor_indices;
	error = cwd_get_affinity(&processor_count, &processor_indices);
	if (error)
		return error;
	if (processor_count < 2)
	{
		free(processor_indices);
		return EINVAL;
	}
	int* new_processor_set;
	error = EINVAL;
	for (size_t i = 0; error && i != processor_count; ++i)
		if (processor_indices[i] == ui_process_processor_index)
		{
			new_processor_set = (int*)malloc((processor_count - 1) * sizeof(int));
			if (!new_processor_set)
			{
				error = errno;
				free(processor_indices);
				return error;
			}
			memcpy(new_processor_set, processor_indices, (size_t)i * sizeof(int));
			memcpy(new_processor_set + (size_t)i, processor_indices + ((size_t)i + 1), (processor_count - ((size_t)i + 1)) * sizeof(int));
			error = 0;
		}
	free(processor_indices);
	if (error)
		return EINVAL;
	if (pipe(pipe_out_in) == -1)
	{
		error = errno;
		free(new_processor_set);
		return error;
	}
	int pipe_read_access_mode = fcntl(pipe_out_in[0], F_GETFL);
	if (pipe_read_access_mode == -1 || fcntl(pipe_out_in[0], F_SETFL, pipe_read_access_mode | O_NONBLOCK) == -1)
	{
		error = errno;
		close(pipe_out_in[0]);
		close(pipe_out_in[1]);
		free(new_processor_set);
		return error;
	}
	pid_t child_id = fork();
	if (child_id == -1)
	{
		error = errno;
		close(pipe_out_in[0]);
		close(pipe_out_in[1]);
		free(new_processor_set);
		return error;
	}
	if (child_id)
	{
		close(pipe_out_in[1]);
		int child_status;
		pid_t wait_result = waitpid(child_id, &child_status, WNOHANG);
		if (wait_result == -1)
		{
			error = errno;
			if (!kill(child_id, SIGTERM) || !kill(child_id, SIGKILL))
			{
				for (int wait_error = EINTR; wait_error == EINTR;)
					if (waitpid(child_id, &child_status, 0) == -1)
					{
						wait_error = errno;
						if (wait_error != EINTR)
							wait_error = 0;
					}
					else
						wait_error = 0;
			}
			close(pipe_out_in[0]);
			free(new_processor_set);
			return error;
		}
		else if (wait_result == child_id)
		{
			close(pipe_out_in[0]);
			free(new_processor_set);
			return ECHILD;
		}
		error = cwd_set_affinity(processor_count - 1, new_processor_set);
		free(new_processor_set);
		if (error)
		{
			if (!kill(child_id, SIGTERM) || !kill(child_id, SIGKILL))
			{
				for (int wait_error = EINTR; wait_error == EINTR;)
					if (waitpid(child_id, &child_status, 0) == -1)
					{
						wait_error = errno;
						if (wait_error != EINTR)
							wait_error = 0;
					}
					else
						wait_error = 0;
			}
			close(pipe_out_in[0]);
			return error;
		}
		*ui_process_id = child_id;
		*ui_process_output = pipe_out_in[0];
		return 0;
	}
	else
	{
		close(pipe_out_in[0]);
		free(new_processor_set);
		error = cwd_set_affinity(1, &ui_process_processor_index);
		if (error)
		{
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		char** arguments = (char**)malloc((ui_process_argument_count + 2) * sizeof(char*));
		if (!arguments)
		{
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(STDOUT_FILENO);
		if (dup2(pipe_out_in[1], STDOUT_FILENO) == -1)
		{
			free(arguments);
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(pipe_out_in[1]);
		arguments[0] = (char*)ui_executable_name;
		memcpy(arguments + 1, ui_process_arguments, ui_process_argument_count * sizeof(char*));
		arguments[ui_process_argument_count + 1] = 0;
		execvp(ui_executable_name, arguments);
		close(STDOUT_FILENO);
		free(arguments);
		exit(EXIT_FAILURE);
		return -1;
	}
}

int cwd_old_create_ui_process_with_affinity(const char* ui_executable_name, size_t ui_process_argument_count, const char** ui_process_arguments, pid_t* ui_process_id, int* ui_process_output)
{
	size_t processor_count;
	int* processor_indices;
	int error = cwd_get_affinity(&processor_count, &processor_indices);
	if (error)
		return error;
	if (!processor_count)
	{
		free(processor_indices);
		return EINVAL;
	}
	int ui_process_processor_index = processor_indices[processor_count - 1];
	free(processor_indices);
	return cwd_old_create_ui_process(ui_executable_name, ui_process_argument_count, ui_process_arguments, ui_process_processor_index, ui_process_id, ui_process_output);
}


int cwd_create_ui_process(const char* ui_executable_name, size_t ui_process_argument_count, const char** ui_process_arguments, int ui_process_processor_index, pid_t* ui_process_id, int* ui_process_output)
{
	int error;
	int pipe_out_in[2];
	if (pipe(pipe_out_in) == -1)
	{
		error = errno;
		return error;
	}
	pid_t child_id = fork();
	if (child_id == -1)
	{
		error = errno;
		close(pipe_out_in[0]);
		close(pipe_out_in[1]);
		return error;
	}
	if (child_id)
	{
		close(pipe_out_in[1]);
		int child_status;
		pid_t wait_result = waitpid(child_id, &child_status, WNOHANG);
		if (wait_result == -1)
		{
			error = errno;
			if (!kill(child_id, SIGTERM) || !kill(child_id, SIGKILL))
				cwd_wait_for_process(child_id);
			close(pipe_out_in[0]);
			return error;
		}
		else if (wait_result == child_id)
		{
			close(pipe_out_in[0]);
			return ECHILD;
		}
		for (char child_not_ready = 1; child_not_ready;)
		{
			ssize_t read_result = read(pipe_out_in[0], &child_not_ready, 1);
			if (read_result == -1)
			{
				error = errno;
				if (error != EINTR)
				{
					if (!kill(child_id, SIGTERM) || !kill(child_id, SIGKILL))
						cwd_wait_for_process(child_id);
					close(pipe_out_in[0]);
					return error;
				}
				child_not_ready = 1;
			}
			else if (read_result == 1 && !child_not_ready)
				continue;
			else
			{
				if (!kill(child_id, SIGTERM) || !kill(child_id, SIGKILL))
					cwd_wait_for_process(child_id);
				close(pipe_out_in[0]);
				return error;
			}
		}
		int pipe_read_access_mode = fcntl(pipe_out_in[0], F_GETFL);
		if (pipe_read_access_mode == -1 || fcntl(pipe_out_in[0], F_SETFL, pipe_read_access_mode | O_NONBLOCK) == -1)
		{
			error = errno;
			if (!kill(child_id, SIGTERM) || !kill(child_id, SIGKILL))
				cwd_wait_for_process(child_id);
			close(pipe_out_in[0]);
			return error;
		}
		*ui_process_id = child_id;
		*ui_process_output = pipe_out_in[0];
		return 0;
	}
	else
	{
		close(pipe_out_in[0]);
		error = cwd_set_affinity(1, &ui_process_processor_index);
		if (error)
		{
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		if (write(pipe_out_in[1], "", 1) != 1)
		{
			error = errno;
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		char** arguments = (char**)malloc((ui_process_argument_count + 2) * sizeof(char*));
		if (!arguments)
		{
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(STDOUT_FILENO);
		if (dup2(pipe_out_in[1], STDOUT_FILENO) == -1)
		{
			free(arguments);
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(pipe_out_in[1]);
		arguments[0] = (char*)ui_executable_name;
		memcpy(arguments + 1, ui_process_arguments, ui_process_argument_count * sizeof(char*));
		arguments[ui_process_argument_count + 1] = 0;
		execvp(ui_executable_name, arguments);
		close(STDOUT_FILENO);
		free(arguments);
		exit(EXIT_FAILURE);
		return -1;
	}
}

int set_fifo_scheduling(int priority)
{
	struct sched_param scheduling_parameters;
	memset(&scheduling_parameters, 0, sizeof(struct sched_param));
	scheduling_parameters.sched_priority = priority;
	if (sched_setscheduler(0, SCHED_FIFO, &scheduling_parameters) == -1)
		return errno;
	return 0;
}

void print_scheduling_info()
{
	pid_t pid = (pid_t)syscall(SYS_getpid);
	pid_t tid = (pid_t)syscall(SYS_gettid);
	printf("PID %jd TID %jd scheduling policy ", (intmax_t)pid, (intmax_t)tid);
	int policy = sched_getscheduler(0);
	struct sched_param parameters;
	switch (policy)
	{
		case SCHED_OTHER:
			printf("normal with nice %i ", nice(0));
			break;
		case SCHED_RR:
			sched_getparam(0, &parameters);
			printf("round-robin with static priority %i ", parameters.sched_priority);
			break;
		case SCHED_FIFO:
			sched_getparam(0, &parameters);
			printf("first-in, first-out with static priority %i ", parameters.sched_priority);
			break;
		default:
			printf("unknown ");
	}
	size_t processor_count;
	int* processor_indices;
	cwd_get_affinity(&processor_count, &processor_indices);
	printf("processor set");
	for (size_t i = 0; i != processor_count; ++i)
		printf(" %i", processor_indices[i]);
	printf("\n");
	free(processor_indices);
}

int cwd_init(int* ui_process_processer_index)
{
	int curl_installed_with_http = 0;
	int error = cwd_is_curl_installed_with_http(&curl_installed_with_http);
	if (error)
		return ENOPKG;
	if (!curl_installed_with_http)
		return ENOPKG;
	size_t processor_count;
	int* processor_indices;
	error = cwd_get_affinity(&processor_count, &processor_indices);
	if (error)
		return error;
	if (processor_count < 2)
	{
		free(processor_indices);
		return EINVAL;
	}
	*ui_process_processer_index = processor_indices[processor_count - 1];
	error = cwd_set_affinity(processor_count - 1, processor_indices);
	if (error)
	{
		free(processor_indices);
		return error;
	}	
	error = set_fifo_scheduling(1);
	if (error)
	{
		cwd_set_affinity(processor_count, processor_indices);
		return error;
	}
	free(processor_indices);
	return 0;
}

int read_byte_form_pipe(int pipe, char* byte)
{
	for (;;)
	{
		ssize_t read_result = read(pipe, byte, 1);
		if (read_result == -1)
		{
			int read_error = errno;
			if (read_error != EINTR)
				continue;
			else
				return read_error;
		}
		else if (!read_result)
			return ENOTCONN;
		else
			return 0;
	}
}

int main(int argc, char** argv)
{
	int ui_process_processor_index = -1;
	if (cwd_init(&ui_process_processor_index))
		return EXIT_FAILURE;
	printf("Initialization succesfull\n");

	
	int error = 0;
	pid_t ui_process = -1;
	int ui_output_pipe = -1;
	char ui_process_output_tmp;
	char ui_process_output[4];
	
	// Logic for controlling ui process, reading sensors and sending data to the server goes here
	while (!error)
	{
		if (ui_process != -1 && ui_output_pipe == -1)
		{
			cwd_wait_for_process(ui_process);
			ui_process = -1;
		}
		if (ui_process == -1)
		{
			sleep(1);
			const char* ui_process_executable = "cat";
			const char* ui_process_arguments[1] = { "cwd_ui_output_test.txt" };
			error = cwd_create_ui_process(ui_process_executable, 1, ui_process_arguments, ui_process_processor_index, &ui_process, &ui_output_pipe);
			if (!error)
				memset(ui_process_output, 0, sizeof(ui_process_output));
			else
			{
				printf("error %i\n", error);
				error = 0;
				ui_process = -1;
				ui_output_pipe = -1;
			}
		}
		if (ui_output_pipe != -1)
		{
			error = read_byte_form_pipe(ui_output_pipe, &ui_process_output_tmp);
			if (!error)
			{
				memmove(ui_process_output, ui_process_output + 1, sizeof(ui_process_output) - 1);
				ui_process_output[sizeof(ui_process_output) - 1] = ui_process_output_tmp;
				if (!memcmp(ui_process_output, "\n!X\n", 4))
					printf("Bypass happened\n");
				if (!memcmp(ui_process_output, "\n!1\n", 4))
					printf("Order 1 happened\n");
				if (!memcmp(ui_process_output, "\n!2\n", 4))
					printf("Order 2 happened\n");
				if (!memcmp(ui_process_output, "\n!3\n", 4))
					printf("Order 3 happened\n");
			}
			else if (error == EAGAIN)
				error = 0;
			else
			{
				error = 0;
				close(ui_output_pipe);
				ui_output_pipe = -1;
			}
		}
	}
	
	if (ui_output_pipe != -1)
		close(ui_output_pipe);
	if (ui_process != -1)
		cwd_wait_for_process(ui_process);
	
	return EXIT_FAILURE;
}
