#include <winsock2.h>
#include <Windows.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
	struct sockaddr_in any_address = { 0 };
	any_address.sin_family = AF_INET;
	any_address.sin_port = htons(1732);
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
	uint8_t buffer[512];

	printf("Waiting broadcast from master\n");
	int message_size = recvfrom(sock, (char*)buffer, sizeof(buffer), 0, (struct sockaddr*)&source_address, &source_address_size);
	if (message_size == -1)
	{
		printf("Error in receiving\n");
		closesocket(sock);
		return 0;
	}
	
	if (message_size < 16 || memcmp(buffer, "TVT17SPL_MASTER", 15))
	{
		printf("Error incorrect message received\n");
		closesocket(sock);
		return 0;
	}

	unsigned long master_ip = ntohl(source_address.sin_addr.s_addr);
	uint8_t master_id = buffer[15];
	printf("Master %lu device found at address %lu.%lu.%lu.%lu\n", (unsigned long)master_id, (master_ip >> 24), (master_ip >> 16) & 0xFF, (master_ip >> 8) & 0xFF, master_ip & 0xFF);
	size_t other_device_count = ((size_t)message_size - 16) / 5;
	if (other_device_count)
		for (size_t i = 0; i != other_device_count; ++i)
		{
			unsigned long device_ip = (unsigned long)buffer[16 + (i * 5)] | ((unsigned long)buffer[16 + (i * 5) + 1] << 8) | ((unsigned long)buffer[16 + (i * 5) + 2] << 16) | ((unsigned long)buffer[16 + (i * 5) + 3] << 24);
			uint8_t device_id = buffer[16 + (i * 5) + 4];
			printf("Device %lu found at address %lu.%lu.%lu.%lu\n", (unsigned long)device_id, (device_ip >> 24), (device_ip >> 16) & 0xFF, (device_ip >> 8) & 0xFF, device_ip & 0xFF);
		}
	else
		printf("No other devices");
	closesocket(sock);
	WSACleanup();
	return 0;
}