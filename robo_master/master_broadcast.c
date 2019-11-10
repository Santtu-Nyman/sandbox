#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "jsonpl.h"

#define MASTER_BROADCAST_PORT 1732
#define MAXIMUM_DEVICE_COUNT 100

struct device_t
{
	uint32_t ip_address;
	uint8_t device_id;
};

int load_file(const char* name, size_t* size, void** data)
{
	int error;
	struct stat stats;
	int file = open(name, O_RDONLY);
	if (file == -1)
		return errno;
	if (fstat(file, &stats) == -1)
	{
		error = errno;
		close(file);
		return error;
	}
	if (sizeof(off_t) > sizeof(size_t) && stats.st_size > (off_t)SIZE_MAX)
	{
		error = EFBIG;
		close(file);
		return error;
	}
	size_t memory_size = (size_t)stats.st_size;
	uintptr_t buffer = (uintptr_t)malloc(memory_size);
	if (!buffer)
	{
		error = errno;
		close(file);
		return error;
	}
	for (size_t loaded = 0; loaded != memory_size;)
	{
		ssize_t read_result = read(file, (void*)(buffer + loaded), ((memory_size - loaded) < (size_t)SSIZE_MAX) ? (memory_size - loaded) : (size_t)SSIZE_MAX);
		if (read_result == -1)
		{
			error = errno;
			if (error != EINTR)
			{
				close(file);
				return error;
			}
			read_result = 0;
		}
		loaded += (size_t)read_result;
	}
	close(file);
	*size = memory_size;
	*data = (void*)buffer;
	return 0;
}

int load_json_from_file(const char* file_name, jsonpl_value_t** value_tree)
{
	size_t file_size;
	void* file_data;
	int error = load_file(file_name, &file_size, &file_data);
	if (error)
		return error;
	
	size_t tree_size = jsonpl_parse_text(file_size, file_data, 0, 0);
	if (!tree_size)
	{
		free(file_data);
		return EBADMSG;
	}
		
	jsonpl_value_t* tree = (jsonpl_value_t*)malloc(tree_size);
	if (!tree)
	{
		free(file_data);
		return EBADMSG;
	}
	
	jsonpl_parse_text(file_size, file_data, tree_size, tree);
	
	free(file_data);
	*value_tree = tree;
	return 0;
}

jsonpl_value_t* find_child_by_name(jsonpl_value_t* value_tree, const char* child_name)
{
	if (value_tree->type == JSONPL_TYPE_OBJECT)
		for (size_t i = 0; i != value_tree->object.value_count; ++i)
			if (!strcmp(child_name, value_tree->object.table[i].name))
				return value_tree->object.table[i].value;
	return 0;
}

int create_device_list(jsonpl_value_t* json_list, size_t* device_count, struct device_t** device_list)
{
	if (json_list->type != JSONPL_TYPE_ARRAY)
		return EBADMSG;
	size_t count = 0;
	for (size_t i = 0; i != json_list->array.value_count; ++i)
		if (json_list->array.table[i]->type == JSONPL_TYPE_OBJECT)
		{
			jsonpl_value_t* id = find_child_by_name(json_list->array.table[i], "id");
			jsonpl_value_t* ip = find_child_by_name(json_list->array.table[i], "ip");
			if (id->type == JSONPL_TYPE_NUMBER && id->number_value >= 0.0 && id->number_value <= 255.0 &&
				ip->type == JSONPL_TYPE_STRING && inet_addr(ip->string.value) != INADDR_NONE)
					++count;
		}
	struct device_t* list = (struct device_t*)malloc(count ? count * sizeof(struct device_t) : 1);
	if (!list)
		return ENOMEM;
	for (size_t c = 0, i = 0; c != count; ++i)
		if (json_list->array.table[i]->type == JSONPL_TYPE_OBJECT)
		{
			jsonpl_value_t* id = find_child_by_name(json_list->array.table[i], "id");
			jsonpl_value_t* ip = find_child_by_name(json_list->array.table[i], "ip");
			if (id->type == JSONPL_TYPE_NUMBER && id->number_value >= 0.0 && id->number_value <= 255.0 &&
				ip->type == JSONPL_TYPE_STRING && inet_addr(ip->string.value) != INADDR_NONE)
			{
				list[c].ip_address = ntohl(inet_addr(ip->string.value));
				list[c].device_id = (uint32_t)id->number_value;
				++c;
			}
		}
	*device_count = count;
	*device_list = list;
	return 0;
}

static volatile sig_atomic_t reload_configuration = 0;
void hangup_signal_handler(int signal_number, siginfo_t* signal_info, void* context)
{
	reload_configuration = 1;
}

int master_broadcast(const char* device_list_file_name)
{
	struct sigaction signal_handler = { 0 };
	signal_handler.sa_sigaction = hangup_signal_handler;
	signal_handler.sa_flags = SA_SIGINFO;
	struct sigaction previous_signal_handler;
	
	if (sigaction(SIGHUP, &signal_handler, &previous_signal_handler))
		printf("Warning failed to set reload signal handler\n");
	
	uint8_t master_id = 0;
	size_t device_count = 0;
	struct device_t* device_list = 0;
	
	struct sockaddr_in broadcast_address = { 0 };  
	broadcast_address.sin_family = AF_INET;        
	broadcast_address.sin_port = htons(MASTER_BROADCAST_PORT);   
	broadcast_address.sin_addr.s_addr = INADDR_BROADCAST;
	
	int error = 0;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		error = errno;
		perror("Socket creation error");
		return error;
	}
	
	static const int broadcast_enable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(int)) == -1)
	{
		error = errno;
		perror("Error in setting Broadcast option");
		close(sock);
		return error;
	}
	
	uint8_t buffer[512];
	
	reload_configuration = 1;
	while (!error)
	{
		if (reload_configuration)
		{
			reload_configuration = 0;
			if (device_list)
			{
				free(device_list);
				device_count = 0;
				device_list = 0;
			}
			jsonpl_value_t* configuration;
			if (load_json_from_file(device_list_file_name, &configuration))
			{
				printf("Warning failed to load device list\n");
			}
			else
			{
				jsonpl_value_t* json_master_device_id = find_child_by_name(configuration, "master_device_id");
				if (json_master_device_id && json_master_device_id->type == JSONPL_TYPE_NUMBER && json_master_device_id->number_value >= 0.0 && json_master_device_id->number_value <= 255.0)
					master_id = (uint8_t)json_master_device_id->number_value;
				jsonpl_value_t* json_device_list = find_child_by_name(configuration, "device_list");
				if (json_device_list)
				{
					if (create_device_list(json_device_list, &device_count, &device_list))
						printf("Warning failed to create device list from json file\n");
				}
				else
					printf("Warning no device list in json file\n");
				free(configuration);
				if (device_count > MAXIMUM_DEVICE_COUNT)
					device_count = MAXIMUM_DEVICE_COUNT;
			}
			memcpy(buffer, "TVT17SPL_MASTER", 15);
			buffer[15] = master_id;
			for (size_t i = 0; i != device_count; ++i)
			{
				buffer[16 + (i * 5)] = (uint8_t)(device_list[i].ip_address & 0xFF);
				buffer[16 + (i * 5) + 1] = (uint8_t)((device_list[i].ip_address >> 8) & 0xFF);
				buffer[16 + (i * 5) + 2] = (uint8_t)((device_list[i].ip_address >> 16) & 0xFF);
				buffer[16 + (i * 5) + 3] = (uint8_t)((device_list[i].ip_address >> 24) & 0xFF);
				buffer[16 + (i * 5) + 4] = device_list[i].device_id;
			}
		}
		if (sendto(sock, buffer, 16 + (device_count * 5), 0, (struct sockaddr*)&broadcast_address, sizeof(struct sockaddr_in)) == -1)
		{
			error = errno;
			perror("Error sending packet");
		}
		sleep(5);
	}

	close(sock);
	if (device_list)
		free(device_list);
	sigaction(SIGHUP, &previous_signal_handler, 0);
	return error;
}

int main(int argc, char** argv)
{
	master_broadcast(argc > 1 ? argv[1] : "master_broadcast.json");
	return 0;
}
