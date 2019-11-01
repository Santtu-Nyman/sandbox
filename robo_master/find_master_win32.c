#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>

int main()
{
	struct sockaddr_in any_address = { 0 };
	any_address.sin_family = AF_INET;
	any_address.sin_port = htons(1234);
	any_address.sin_addr.s_addr = INADDR_ANY;

	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
	{
		printf("Error initializing Winsock DLL\n");
		return 0;
	}

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("Error in creating socket\n");
		closesocket(sock);
		return 0;
	}

	static const BOOL broadcast_enable = TRUE;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast_enable, sizeof(BOOL)) == -1)
	{
		printf("Error enabling Broadcast\n");
		closesocket(sock);
		return 0;
	}

	if (bind(sock, (struct sockaddr*)&any_address, sizeof(struct sockaddr_in)) == -1)
	{
		printf("Error in binding\n");
		closesocket(sock);
		return 0;
	}

	struct sockaddr_in source_address;
	int source_address_size = sizeof(struct sockaddr_in);
	char buffer[16];

	printf("Waiting broadcast from master\n");
	int message_size = recvfrom(sock, buffer, 16, 0, (struct sockaddr*)&source_address, &source_address_size);
	if (message_size == -1)
	{
		printf("Error in receiving\n");
		closesocket(sock);
		return 0;
	}
	
	if (message_size != 16 || memcmp(buffer, "TVT17SPL_MASTER", 16))
	{
		printf("Error incorrect message received\n");
		closesocket(sock);
		return 0;
	}

	unsigned long source_ip = ntohl(source_address.sin_addr.s_addr);
	printf("Master device found at address %lu.%lu.%lu.%lu\n", (source_ip >> 24), (source_ip >> 16) & 0xFF, (source_ip >> 8) & 0xFF, source_ip & 0xFF);

	closesocket(sock);
	WSACleanup();
	return 0;
}