// UDP server that use blocking sockets

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_PORT 15000  // Port number of server that will be used for communication with clients
#define DHCP_PORT 67
#define BUFFER_SIZE 512		// Size of buffer that will be used for sending and receiving messages to clients
#define NUMBER_OF_ADDRESS_POOL 5

#define LENGHT 312
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACKNOWLEDGE 5
#define DHCP_END 6

typedef struct DHCP_message {
    long ciaddr;
    long siaddr;
    char options[LENGHT];
};

typedef struct IPAddress {
    long ciAddr;
    bool isTaken;
};

int main()
{
    DHCP_message package;

    struct in_addr ip_addr;

    const char* addrs[5] = { "192.168.10.6", "192.168.10.7", "192.168.10.8", "192.168.10.9", "192.168.10.10" };

    IPAddress pool[5];

    for (int i = 0; i < NUMBER_OF_ADDRESS_POOL; i++) {
        pool[i].ciAddr = inet_addr(addrs[i]);
        pool[i].isTaken = false;
    }

    // Server address
    sockaddr_in serverAddress;

	// Buffer we will use to send and receive clients' messages
    char dataBuffer[BUFFER_SIZE];

	// WSADATA data structure that is to receive details of the Windows Sockets implementation
    WSADATA wsaData;

	// Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }
    

    // Initialize serverAddress structure used by bind function
    memset((char*)&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET; 			// set server address protocol family
    serverAddress.sin_addr.s_addr = INADDR_ANY;		// use all available addresses of server
    serverAddress.sin_port = htons(DHCP_PORT);

    // Create a socket1
    SOCKET serverSocket = socket(AF_INET,      // IPv4 address famly
								 SOCK_DGRAM,   // datagram socket
								 IPPROTO_UDP); // UDP

	// Check if socket creation succeeded
    if (serverSocket == INVALID_SOCKET)
    {
        printf("Creating socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind server address structure (type, port number and local address) to socket1
    int iResult = bind(serverSocket,(SOCKADDR *)&serverAddress, sizeof(serverAddress));

	// Check if socket is succesfully binded to server datas
    if (iResult == SOCKET_ERROR)
    {
        printf("Socket bind failed with error: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

	printf("Simple UDP server INADDR_ANY started and waiting client messages.\n");
    
    //set serverSocket in nonblocking mode 
    unsigned long  mode = 1;
    iResult = ioctlsocket(serverSocket, FIONBIO, &mode);
    if (iResult != 0)
        printf("ioctlsocket failed with error.");

    // Main server loop
    while(1)
    {
        // Declare and initialize client address that will be set from recvfrom
        sockaddr_in clientAddress;
		memset(&clientAddress, 0, sizeof(clientAddress));

		// Set whole buffer to zero
        memset(dataBuffer, 0, BUFFER_SIZE);

		// size of client address
		int sockAddrLen = sizeof(clientAddress);

		// Receive client message
        iResult = recvfrom(serverSocket,				// Own socket
			               (char*)&package,					// Buffer that will be used for receiving message
						   sizeof(package),					// Maximal size of buffer
						   0,							// No flags
						   (SOCKADDR *)&clientAddress,	// Client information from received message (ip address and port)
						   &sockAddrLen);				// Size of sockadd_in structure

        if (iResult != SOCKET_ERROR) {
            if (package.options[4] == DHCP_DISCOVER || package.options[4] == DHCP_OFFER) {

                #pragma region DHCP DISCOVER
                // Set end of string
                dataBuffer[iResult] = '\0';

                char ipAddress[16]; // 15 spaces for decimal notation (for example: "192.168.100.200") + '\0'

                // Copy client ip to local char[]
                strcpy_s(ipAddress, sizeof(ipAddress), inet_ntoa(clientAddress.sin_addr));

                // Convert port number from network byte order to host byte order
                unsigned short clientPort = ntohs(clientAddress.sin_port);

                printf("\nClient (connected from ip: %s, port: %d) sent DHCP DISCOVER message.\n", ipAddress, clientPort);
                #pragma endregion

                #pragma region DHCP OFFER
                for (int i = 0; i < NUMBER_OF_ADDRESS_POOL; i++) {
                    if (pool[i].isTaken == false) {
                        package.ciaddr = pool[i].ciAddr;
                        break;
                    }
                }

                // Send message to client
                iResult = sendto(serverSocket,						// Own socket
                    (char*)&package,						// Text of message
                    sizeof(package),				// Message size
                    0,									// No flags
                    (SOCKADDR*)&clientAddress,		// Address structure of server (type, IP address and port)
                    sizeof(clientAddress));			// Size of sockadr_in structure

                ip_addr.s_addr = package.ciaddr;

                char address[16];
                strcpy_s(address, inet_ntoa(ip_addr));

                printf("\nYou OFFER this IP address: %s\n", address);
                #pragma endregion
            }

            if (package.options[4] == DHCP_REQUEST || package.options[4] == DHCP_ACKNOWLEDGE) {

                #pragma region DHCP REQUEST
                ip_addr.s_addr = package.ciaddr;

                char addressChoose[16];
                strcpy_s(addressChoose, inet_ntoa(ip_addr));

                printf("\nClient has chosen this IP address: %s\n", addressChoose);
                #pragma endregion

                #pragma region DHCP ACK
                if (package.siaddr == inet_addr("127.0.0.1")) {

                    for (int i = 0; i < NUMBER_OF_ADDRESS_POOL; i++) {
                        if (package.ciaddr == pool[i].ciAddr) {
                            if (pool[i].isTaken == true) {
                                package.ciaddr = 0;
                            }
                            else {
                                pool[i].isTaken = true;
                            }
                            break;
                        }
                    }

                    // Send message to client
                    iResult = sendto(serverSocket,						// Own socket
                        (char*)&package,						// Text of message
                        sizeof(package),				// Message size
                        0,									// No flags
                        (SOCKADDR*)&clientAddress,		// Address structure of server (type, IP address and port)
                        sizeof(clientAddress));			// Size of sockadr_in structure

                    printf("\nIP address is ACKNOWLEDGED and taken..\n");
                }
                #pragma endregion
            }

            if (package.options[4] == DHCP_END) {

                for (int i = 0; i < NUMBER_OF_ADDRESS_POOL; i++) {
                    if (package.ciaddr == pool[i].ciAddr && pool[i].isTaken == true) {
                        pool[i].isTaken = false;
                        break;
                    }
                }

                printf("\nClient has freed address.\n");
            }
        }
        else {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                Sleep(1000);
            }
            else {
                // Check if message is succesfully received
                if (iResult == SOCKET_ERROR)
                {
                    printf("recvfrom failed with error: %d\n", WSAGetLastError());
                    continue;
                }
            }
        }

		// Possible server-shutdown logic could be put here
    }

    // Close server application
    iResult = closesocket(serverSocket);
    if (iResult == SOCKET_ERROR)
    {
        printf("closesocket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
        return 1;
    }
	
	printf("Server successfully shut down.\n");

	// Close Winsock library
	WSACleanup();

	return 0;
}