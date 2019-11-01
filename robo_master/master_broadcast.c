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

int master_broadcast()
{
	struct sockaddr_in broadcast_address = { 0 };  
	broadcast_address.sin_family = AF_INET;        
	broadcast_address.sin_port = htons(1234);   
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
	
	while (!error)
	{
		if (sendto(sock, "TVT17SPL_MASTER", 16, 0, (struct sockaddr*)&broadcast_address, sizeof(struct sockaddr_in)) == -1)
		{
			error = errno;
			perror("Error sending packet");
		}
		sleep(5);
	}

	close(sock);
	return error;
}

int main()
{
	master_broadcast();
	return 0;
}
