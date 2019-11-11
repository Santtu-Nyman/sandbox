#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#define MASTER_BROADCAST_PORT 1732

int main()
{
    struct sockaddr_in any_address = { 0 };  
    any_address.sin_family = AF_INET;        
    any_address.sin_port = htons(MASTER_BROADCAST_PORT);   
    any_address.sin_addr.s_addr = INADDR_ANY;

    int error = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
	    error = errno;
	    perror("Error in creating socket");
	    return error;
    }
 
    static const int broadcast = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1)
    {
		error = errno;
        perror("Error enabling broadcast");
        close(sock);
        return error;
    }

    if (bind(sock, (struct sockaddr*)&any_address, sizeof(struct sockaddr_in)) == -1)
    {
	error = errno;
        perror("Error in bindind");
	close(sock);
        return error;
    }

    struct sockaddr_in source_address; 
    int source_address_size = sizeof(struct sockaddr_in);
    uint8_t buffer[512];
    
    printf("Waiting broadcast from master\n");
    size_t message_size = (size_t)recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&source_address, &source_address_size);
    if (message_size == (size_t)-1)
    {
	error = errno;
        perror("Error receiving");
	close(sock);
        return error;
    }
    
    close(sock);
    
    if (message_size < 16 || memcmp(buffer, "TVT17SPL_MASTER", 15))
    {
	error = EBADMSG;
        perror("Error incorrect message received");
        return error;
    }
 
    unsigned long master_ip = ntohl(source_address.sin_addr.s_addr);
    uint8_t master_id = buffer[15];
    printf("Master device %lu found at address %lu.%lu.%lu.%lu\n", (unsigned long)master_id, (master_ip >> 24) & 0xFF, (master_ip >> 16) & 0xFF, (master_ip >> 8) & 0xFF, master_ip & 0xFF);
    size_t other_device_count = (message_size - 16) / 5;
    if (other_device_count)
	for (size_t device_index = 0; device_index != other_device_count; ++device_index)
	{
	    unsigned long device_ip = (unsigned long)buffer[16 + (device_index * 5)] | ((unsigned long)buffer[16 + (device_index * 5) + 1] << 8) | ((unsigned long)buffer[16 + (device_index * 5) + 2] << 16) | ((unsigned long)buffer[16 + (device_index * 5) + 3] << 24);
	    uint8_t device_id = buffer[16 + (device_index * 5) + 4];
	    printf("Device %lu found at address %lu.%lu.%lu.%lu\n", (unsigned long)device_id, (device_ip >> 24) & 0xFF, (device_ip >> 16) & 0xFF, (device_ip >> 8) & 0xFF, device_ip & 0xFF);
	}
    else
	printf("No other devices\n");
 
    return error;
}
