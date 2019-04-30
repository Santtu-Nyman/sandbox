/*
	Cool Water Dispenser version 1.0.4 2019-04-29 written by Santtu Nyman.
	git repository https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Cold Water Dispenser Raspberry Pi device controlling software.
		The UI part of the software is on separate source and executable.
		
	Version history
		version 1.0.4 2019-04-29
			Server API chance.
		version 1.0.3 2019-04-23
			Added new measurements features for Jarno.
		version 1.0.1 2019-04-12
			Added more reasonable initialization and simplefiad some code.
		version 1.0.0 2019-04-11
			First complete version.
		version	0.0.6 2019-04-04
			Added debug features.
		version 0.0.5 2019-03-31
			Cooling logic added.
		version 0.0.4 2019-03-26
			UI start code added.
		version 0.0.3 2019-03-17
			Server API version argument added.
		version 0.0.2 2019-03-17
			Preparing for new server API.
		version 0.0.1 2019-03-16
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
#include <time.h>
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
		int child_status;
		pid_t wait_result = waitpid(child_id, &child_status, WNOHANG);
		close(pipe_out_in[1]);
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
		error = cwd_set_affinity(1, &ui_process_processor_index);
		close(pipe_out_in[0]);
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

int cwd_init_process(int argc, char** argv, struct cwd_device_configuration_t* configuration)
{
	cwd_default_configuration(configuration);
	if (!configuration->is_curl_installed)
		return ENOPKG;
	if (configuration->processor_count < 2)
		return EINVAL;
	char* server_api_version_argument = cwd_search_for_argument(argc, argv, "--server_api");
	if (server_api_version_argument)
	{
		int tmp_server_api_version = 0;
		while (*server_api_version_argument)
		{
			if (*server_api_version_argument >= '0' && *server_api_version_argument <= '9')
				tmp_server_api_version = 10 * tmp_server_api_version + (int)(*server_api_version_argument++ - '0');
			else
				return EINVAL;
		}
		configuration->server_api_version = tmp_server_api_version;
	}
	char* server_argument = cwd_search_for_argument(argc, argv, "--server");
	if (server_argument)
		configuration->server = server_argument;
	char* device_id_argument = cwd_search_for_argument(argc, argv, "--device_id");
	if (device_id_argument)
	{
		uint64_t tmp_device_id = 0;
		while (*device_id_argument)
		{
			if (*device_id_argument >= '0' && *device_id_argument <= '9')
				tmp_device_id = 10 * tmp_device_id + (uint64_t)(*device_id_argument++ - '0');
			else
				return EINVAL;
		}
		configuration->device_id = tmp_device_id;
	}
	int ignored;
	uint64_t measurement_delay;
	if (cwd_decode_integer_argument(argc, argv, "--measurement_delay", &ignored, &measurement_delay))
		configuration->periodic_mesurement_delay = measurement_delay;
	if (cwd_check_flag_argument(argc, argv, "--use_extended_url"))
		configuration->use_extended_url = 1;
	if (cwd_check_flag_argument(argc, argv, "--disable_ui"))
		configuration->disable_ui = 1;
	if (cwd_check_flag_argument(argc, argv, "--disable_12v_power"))
		configuration->disable_12v_power = 1;
	if (cwd_check_flag_argument(argc, argv, "--disable_cooler"))
		configuration->disable_cooler = 1;
	if (cwd_check_flag_argument(argc, argv, "--print_measurements"))
		configuration->print_measurements = 1;
	if (cwd_check_flag_argument(argc, argv, "--skip_insert"))
		configuration->skip_insert = 1;
	if (cwd_check_flag_argument(argc, argv, "--print_server_responses"))
		configuration->print_server_responses = 1;
	if (cwd_check_flag_argument(argc, argv, "--log_measurements"))
		configuration->log_measurements = 1;
	if (cwd_check_flag_argument(argc, argv, "--disable_capasitance_sensor"))
		configuration->disable_capasitance_sensor = 1;
	if (cwd_check_flag_argument(argc, argv, "--disable_thermistor"))
		configuration->disable_thermistor = 1;
	if (cwd_check_flag_argument(argc, argv, "--offline"))
		configuration->disable_thermistor = 1;
	if (cwd_check_flag_argument(argc, argv, "--disable_temperature"))
		configuration->disable_temperature = 1;
	if (cwd_decode_integer_argument(argc, argv, "--loop_delay", &ignored, &measurement_delay))
		configuration->loop_delay = measurement_delay;
		
	int error = cwd_set_affinity(configuration->processor_count - 1, configuration->processor_indices);
	if (error)
		return error;
	error = set_fifo_scheduling(1);
	if (error)
	{
		cwd_set_affinity(configuration->processor_count, configuration->processor_indices);
		return error;
	}
	uint64_t last_refil_time;
	if (cwd_get_last_tank_refill_time(&last_refil_time) || last_refil_time < 1554309666)
		cwd_set_last_tank_refill_time(1554309666);
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
			if (read_error == EINTR)
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

int wait_for_12_v_power_insert()
{
	int error = 0;
	for (int power_plugged = 0; !error && !power_plugged;)
		error = cwd_is_12v_power_plugged(&power_plugged);
	return error;
}

void print_content_separator()
{
	for (int i = 64; i--;)
		printf("-");
	printf("\n");
}

char* append_string(char* low, const char* high)
{
	size_t high_length = strlen(high);
	memcpy(low, high, high_length);
	return low + high_length;
}

int main(int argc, char** argv)
{
	char version_string[32];
	size_t version_string_length;
	struct cwd_device_configuration_t configuration;
	pid_t ui_process = -1;
	int ui_output_pipe = -1;
	char ui_process_output_tmp;
	char ui_process_output[4];
	time_t previous_measurement_time;
	time_t current_time;
	float temparature = 0.0f;
	time_t cooler_start_time;
	time_t cooler_on_time = (time_t)~0;
	float water_level = 100.0f;
	float new_water_level;
	uint64_t last_refill_time = (uint64_t)~0;
	char date_buffer[20];
	int pressure_sensor_value;
	int capasitance_sensor_value;
	int temperature_sensor_value;
	int idle_loop;
	int idle_level = 0;
	int error = 0;
	char* log_buffer;
	
	if (cwd_init_process(argc, argv, &configuration))
	{
		printf("Initialization failed (cwd_init_process)\n");
		return EXIT_FAILURE;
	}
	memset(version_string, 0, 32 * sizeof(char));
	version_string_length = cwd_print_u64(4, version_string, configuration.version);
	version_string[version_string_length++] = '.';
	version_string_length += cwd_print_u64(4, version_string + version_string_length, configuration.version_extension);
	version_string[version_string_length++] = '.';
	version_string_length += cwd_print_u64(4, version_string + version_string_length, configuration.version_patch);
	version_string[version_string_length] = 0;
	cwd_save_file("/home/pi/cwd/cwd_version.txt", version_string_length, version_string);
	printf("Executing Cool Water Dispenser software version %s\n", version_string);
	if (chdir(configuration.directory) == -1)
	{
		printf("Initialization failed (chdir \"%s\")\n", configuration.directory);
		return EXIT_FAILURE;
	}
	if (configuration.log_measurements)
	{
		log_buffer = (char*)malloc(4096 * sizeof(char));
		if (!log_buffer)
		{
			printf("Initialization failed no memory\n");
			return EXIT_FAILURE;
		}
	}
	error = EAGAIN;
	while (error == EAGAIN)
	{
		error = cwd_device_initialization();
		if (error == EAGAIN && configuration.skip_insert)
			error = 0;
	}
	if (error)
	{
		printf("Initialization failed\n (cwd_device_initialization)");
		return EXIT_FAILURE;
	}
	print_content_separator();
	printf("Initialization succesfull\n"
		"Server API version %i\n"
		"Server \"%s\"\n"
		"Devive id %llu\n"
		"Program directory \"%s\"\n"
		"CURL installed %s\n"
		"Full build for BCM2835 %s\n"
		"PrinLab PTC support %s\n"
		"Printed FSR support %s\n"
		"DS18B20 support %s\n%s",
		configuration.server_api_version,
		configuration.server,
		(unsigned long long)configuration.device_id,
		configuration.directory,
		configuration.is_curl_installed ? "YES" : "NO",
		configuration.full_bcm2835_build ? "YES" : "NO",
		configuration.prinlab_ptc_support ? "YES" : "NO",
		configuration.printed_fsr_support ? "YES" : "NO",
		configuration.ds18b20_support ? "YES" : "NO",
		(configuration.is_curl_installed && configuration.full_bcm2835_build && (configuration.prinlab_ptc_support || configuration.ds18b20_support) && configuration.printed_fsr_support) ? "" : "WARNING THIS CONFIGURATION CAN'T BE USED FOR THE FINAL SYSTEM IT IS INCOMPLETE!\n");
	fflush(stdout);
	
	if (!configuration.disable_12v_power)
		cwd_enable_12v_power(1);
		
	current_time = time(0);
	cwd_print_time((uint64_t)current_time, date_buffer);
	date_buffer[19] = 0;
	printf("Current time %s\n", date_buffer);
	if (configuration.disable_temperature)
		error = ENOSYS;
	else
		error = cwd_read_temperature(configuration.disable_thermistor ? CWD_TEMPERATUTE_SENSOR_DALLAS : CWD_TEMPERATUTE_SENSOR_ANY, &temparature);
	if (!error)
		printf("Current temperature %f C\n", temparature);
	else
		printf("Current temperature unknown C error %i\n", error);
	error = configuration.disable_capasitance_sensor ? ENOSYS : cwd_measure_water_level_from_tank_capasitance_raw(&capasitance_sensor_value);
	if (!error)
		printf("Current water level capacitance sensor %i\n", capasitance_sensor_value);
	else
		printf("Current water level capacitance sensor unknown error %i\n", error);
	error = cwd_read_pressure_sensor(&pressure_sensor_value);
	if (!error)
		printf("Current water level pressure sensor %i\n", pressure_sensor_value);
	else
		printf("Current water level pressure sensor unknown error %i\n", error);
	if (configuration.disable_capasitance_sensor)
	{
		error = cwd_read_pressure_sensor(&pressure_sensor_value);
		if (!error)
			water_level = 100.0f * cwd_calculate_water_level_from_pressure_value(pressure_sensor_value);
	}
	else
		error = cwd_get_tank_water_level_in_percents(&water_level);
	if (!error)
		printf("Current water level %f %%\n", water_level);
	else
		printf("Current water level unknown %% error %i\n", error);
	if (configuration.disable_temperature)
		error = ENOSYS;
	else
		error = cwd_read_thermistor_adc(&temperature_sensor_value);
	if (!error)
		printf("Current PTC temperature sensor %i\n", temperature_sensor_value);
	else
		printf("Current PTC temperature sensor unknown error %i\n", error);
	error = cwd_get_last_tank_refill_time(&last_refill_time);
	if (!error)
	{
		cwd_print_time((uint64_t)last_refill_time, date_buffer);
		date_buffer[19] = 0;
		printf("Last water refill time %s\n", date_buffer);
	}
	else
		printf("Last water refill time unknown error %i\n", error);
	fflush(stdout);
	
	if (cwd_check_flag_argument(argc, argv, "--shutdown"))
	{
		cwd_enable_12v_power(0);
		cwd_motor_enable(0);
		cwd_enable_cooler(0);
		cwd_shutdown_leds();
		return EXIT_SUCCESS;
	}
	
	if (cwd_check_flag_argument(argc, argv, "--light_show"))
	{
		cwd_enable_12v_power(0);
		cwd_motor_enable(0);
		cwd_enable_cooler(0);
		cwd_ligth_demo(-1);
	}
	else
		cwd_set_leds_to_blue();
	cwd_shutdown_leds();
	
	int pump_water_direction;
	uint64_t pump_water_time;
	if (cwd_decode_integer_argument(argc, argv, "--pump_water", &pump_water_direction, &pump_water_time))
	{
		cwd_pump_water(pump_water_direction ? -1 : 1, (uint64_t)1000000000 * pump_water_time);
		if (pump_water_time)
			return EXIT_SUCCESS;
	}

	error = 0;
	previous_measurement_time = 0;
	current_time = time(0);
	cwd_send_device_startup(configuration.server_api_version, configuration.server, configuration.device_id, (uint64_t)current_time, configuration.use_extended_url, configuration.print_server_responses);
	cwd_get_device_configuration(configuration.server_api_version, configuration.server, configuration.device_id, &configuration, configuration.use_extended_url, configuration.print_server_responses);
	
	while (!error)
	{
		idle_loop = 1;
		
		current_time = time(0);
		int pressure_error = cwd_read_pressure_sensor(&pressure_sensor_value);
		int capasitance_error = configuration.disable_capasitance_sensor ? ENOSYS : cwd_measure_water_level_from_tank_capasitance_raw(&capasitance_sensor_value);
		int temperature_error;
		if (configuration.disable_temperature)
			temperature_error = ENOSYS;
		else
			temperature_error = cwd_read_temperature(configuration.disable_thermistor ? CWD_TEMPERATUTE_SENSOR_DALLAS : CWD_TEMPERATUTE_SENSOR_ANY, &temparature);
		int water_level_error;
		if (configuration.disable_capasitance_sensor)
		{
			water_level_error = pressure_error;
			if (!pressure_error)
				new_water_level = 100.0f * cwd_calculate_water_level_from_pressure_value(pressure_sensor_value);
		}
		else
			water_level_error = cwd_get_tank_water_level_in_percents(&new_water_level);
		if (new_water_level - water_level > 25.0f)
			cwd_set_last_tank_refill_time((uint64_t)current_time);
		water_level = new_water_level;
		int refill_error = cwd_get_last_tank_refill_time(&last_refill_time);
		
		if ((uint64_t)(current_time - previous_measurement_time) >= configuration.periodic_mesurement_delay)
		{
			if (configuration.print_measurements)
			{
				print_content_separator();
				cwd_print_time((uint64_t)current_time, date_buffer);
				date_buffer[19] = 0;
				printf("Measurements %s\n", date_buffer);
				if (!temperature_error)
					printf("Current temperature %f C\n", temparature);
				else
					printf("Current temperature unknown C error %i\n", temperature_error);
				if (!water_level_error)
					printf("Current water level %f %%\n", water_level);
				else
					printf("Current water level unknown %% error %i\n", water_level_error);
				if (!refill_error)
				{
					cwd_print_time(last_refill_time, date_buffer);
					date_buffer[19] = 0;
					printf("Last water refill time %s\n", date_buffer);
				}
				else
					printf("Last water refill time unknown error %i\n", refill_error);
				if (!capasitance_error)
					printf("Current water level capacitance sensor %i\n", capasitance_sensor_value);
				else
					printf("Current water level capacitance sensor unknown error %i\n", capasitance_error);
				if (!pressure_error)
					printf("Current water level pressure sensor %i\n", pressure_sensor_value);
				else
					printf("Current water level pressure sensor unknown error %i\n", pressure_error);
			}
			if (configuration.log_measurements)
			{
				char* write_log = log_buffer;
				write_log = append_string(write_log, "----------------------------------------------------------------\nMeasurements ");
				cwd_print_time((uint64_t)current_time, write_log);
				write_log += 19;
				write_log = append_string(write_log, "\n");
				if (!temperature_error)
				{
					write_log = append_string(write_log, "Current temperature ");
					write_log += cwd_print_f32_n_dot3(32, write_log, temparature);
					write_log = append_string(write_log, " C\n");
				}
				else
					write_log = append_string(write_log, "Current temperature unknown C\n");
				if (!water_level_error)
				{
					write_log = append_string(write_log, "Current water level ");
					write_log += cwd_print_f32_n_dot3(32, write_log, water_level);
					write_log = append_string(write_log, " %\n");
				}
				else
					write_log = append_string(write_log, "Current water level unknown %\n");
				if (!refill_error)
				{
					write_log = append_string(write_log, "Last water refill time ");
					cwd_print_time((uint64_t)last_refill_time, write_log);
					write_log += 19;
					write_log = append_string(write_log, "\n");
				}
				else
					write_log = append_string(write_log, "Last water refill time unknown\n");
				if (!capasitance_error)
				{
					write_log = append_string(write_log, "Current water level capacitance ");
					write_log += cwd_print_f32_n_dot3(32, write_log, (float)capasitance_sensor_value);
					write_log = append_string(write_log, "\n");
				}
				else
					write_log = append_string(write_log, "Current water level capacitance sensor unknown\n");
				if (!pressure_error)
				{
					write_log = append_string(write_log, "Current water level pressure sensor ");
					write_log += cwd_print_f32_n_dot3(32, write_log, (float)pressure_sensor_value);
					write_log = append_string(write_log, "\n");
				}
				else
					write_log = append_string(write_log, "Current water level pressure sensor unknown\n");
				cwd_append_to_file("cwd_log.txt", (size_t)((uintptr_t)write_log - (uintptr_t)log_buffer), log_buffer);
			}
			if (!configuration.offline)
			{
				cwd_send_periodic_mesurements(configuration.server_api_version, configuration.server, configuration.device_id, current_time, water_level, temparature, last_refill_time, configuration.use_extended_url, configuration.print_server_responses);
				cwd_get_device_configuration(configuration.server_api_version, configuration.server, configuration.device_id, &configuration, configuration.use_extended_url, configuration.print_server_responses);
			}
			previous_measurement_time = current_time;
		}
		
		if (!configuration.disable_cooler)
		{
			if (cooler_on_time != (time_t)~0)
				cooler_on_time = time(0) - cooler_start_time;
			if (!temperature_error && !water_level_error && water_level > 5.0f)
			{
				if (cooler_on_time != (time_t)~0)
				{
					if (cooler_on_time > 60 && configuration.target_water_temperature > temparature)
					{
						cwd_enable_cooler(0);
						cooler_on_time = (time_t)~0;
					}
				}
				else if (cooler_on_time == (time_t)~0 && configuration.target_water_temperature < temparature)
				{
					cwd_enable_cooler(1);
					cooler_start_time = time(0);
					cooler_on_time = 0;
				}
			}
			else if (cooler_on_time != (time_t)~0)
			{
				cwd_enable_cooler(0);
				cooler_on_time = (time_t)~0;
			}
		}
		
		if (!configuration.disable_ui)
		{
			if (ui_process != -1 && ui_output_pipe == -1)
			{
				cwd_wait_for_process(ui_process);
				cwd_shutdown_leds();
				ui_process = -1;
			}
			if (ui_process == -1)
			{
				error = cwd_create_ui_process(configuration.ui_process_executable, configuration.ui_process_argument_count, (const char**)configuration.ui_process_arguments, configuration.ui_process_processor_index, &ui_process, &ui_output_pipe);
				if (!error)
				{
					memset(ui_process_output, (int)'\n', sizeof(ui_process_output));
					idle_loop = 0;
				}
				else
				{
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
					int event_number = -1;
					if (!memcmp(ui_process_output, "\n!X\n", 4))
						event_number = 0;
					else if (!memcmp(ui_process_output, "\n!1\n", 4))
						event_number = 1;
					else if (!memcmp(ui_process_output, "\n!2\n", 4))
						event_number = 2;
					else if (!memcmp(ui_process_output, "\n!3\n", 4))
						event_number = 3;
					if (event_number != -1)
					{
						print_content_separator();
						cwd_print_time((uint64_t)current_time, date_buffer);
						date_buffer[19] = 0;
						const char* event_srings[4] = { "Bypassing", "Small order", "Medium order", "Large order" };
						printf("%s %s\n", event_srings[event_number], date_buffer);
						if (event_number == 0)
							cwd_send_bypassing(configuration.server_api_version, configuration.server, configuration.device_id, current_time, configuration.use_extended_url, configuration.print_server_responses);
						else if (event_number > 0 && event_number < 4)
							cwd_send_order(configuration.server_api_version, configuration.server, configuration.device_id, current_time, event_number, configuration.use_extended_url, configuration.print_server_responses);
					}
					idle_loop = 0;
				}
				else if (error == EAGAIN)
					error = 0;
				else
				{
					error = 0;
					close(ui_output_pipe);
					ui_output_pipe = -1;
					idle_loop = 0;
				}
			}
		}
		
		if (configuration.periodic_mesurement_delay && idle_loop)
		{
			if (idle_level < 5)
				++idle_level;
			fflush(stdout);
			sleep(idle_level);
		}
		else
			idle_level = 0;
			
		if (configuration.loop_delay)
			cwd_wait_ns(configuration.loop_delay);
	}
	
	if (ui_output_pipe != -1)
		close(ui_output_pipe);
	if (ui_process != -1)
		cwd_wait_for_process(ui_process);
	cwd_device_close();
	
	free(log_buffer);
	
	return EXIT_FAILURE;
}
