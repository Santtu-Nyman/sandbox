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

int main()
{

    struct sockaddr_in any_address = { 0 };  
    any_address.sin_family = AF_INET;        
    any_address.sin_port = htons(1234);   
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
    char buffer[16];
    
    printf("Waiting broadcast from master\n");
    size_t message_size = (size_t)recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&source_address, &source_address_size);
    if (message_size == (size_t)-1)
    {
	error = errno;
        perror("Error receiving");
	close(sock);
        return error;
    }
    
    if (message_size != 16 || memcmp(buffer, "TVT17SPL_MASTER", 16))
    {
	error = EBADMSG;
        perror("Error incorrect message received");
	close(sock);
        return error;
    }
 
    unsigned long source_ip = ntohl(source_address.sin_addr.s_addr);
    printf("Master device found at address %u.%u.%u.%u\n", (source_ip >> 24) & 0xFF, (source_ip >> 16) & 0xFF, (source_ip >> 8) & 0xFF, source_ip & 0xFF);
 
    close(sock);
    return error;
}
