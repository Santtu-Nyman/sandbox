#define _GNU_SOURCE
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

int create_test_process(const char* executable_name, size_t argument_count, const char** arguments, pid_t* process_id, int* process_output)
{
	int error;
	int pipe_out_in[2];
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
		*process_id = child_id;
		*process_output = pipe_out_in[0];
		return 0;
	}
	else
	{
		close(pipe_out_in[0]);
		char** execute_arguments = (char**)malloc((argument_count + 2) * sizeof(char*));
		if (!arguments)
		{
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(STDOUT_FILENO);
		if (dup2(pipe_out_in[1], STDOUT_FILENO) == -1)
		{
			free(execute_arguments);
			close(pipe_out_in[1]);
			exit(EXIT_FAILURE);
		}
		close(pipe_out_in[1]);
		execute_arguments[0] = (char*)executable_name;
		memcpy(execute_arguments + 1, arguments, argument_count * sizeof(char*));
		execute_arguments[argument_count + 1] = 0;
		execvp(executable_name, execute_arguments);
		close(STDOUT_FILENO);
		free(execute_arguments);
		exit(EXIT_FAILURE);
		return -1;
	}
}

int main(int argc, char** argv)
{
	pid_t process_id;
	int output_pipe;
	int error = create_test_process("/home/pi/Desktop/cwd/ui", 0, 0, &process_id, &output_pipe);
	if (errno)
	{
		printf("Failed to start test process\n");
		return EXIT_FAILURE;
	}
	printf("test process printed \"");
	for (int read_pipe = 1; read_pipe;)
	{
		char tmp;
		ssize_t read_result = read(ui_output_pipe, &tmp, 1);
		if (read_result == -1)
		{
			int read_error = errno;
			if (read_error != EAGAIN && read_error != EINTR)
				read_pipe = 0;
		}
		else if (!read_result)
			read_pipe = 0;
		else
			printf("%c", tmp);
	}
	printf("\"\n");
	close(output_pipe);
	int child_status;
	for (int wait_error = EINTR; wait_error == EINTR;)
		if (waitpid(child_id, &child_status, 0) == -1)
		{
			wait_error = errno;
			if (wait_error != EINTR)
				wait_error = 0;
		}
		else
			wait_error = 0;
	return EXIT_SUCCESS;
}
