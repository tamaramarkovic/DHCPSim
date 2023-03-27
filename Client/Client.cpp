// UDP client that uses blocking sockets

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma warning(disable:4996) 

#define SERVER_IP_ADDRESS "127.0.0.1"		// IPv4 address of server
#define SERVER_ANY_ADDRESS "127.1.2.3"
#define SERVER_PORT_ANY 15000					// Port number of server that will be used for communication with clients
#define SERVER_PORT_IP 15001					// Port number of server that will be used for communication with clients
#define DHCP_PORT 67
#define DHCP_LOCAL_BROADCASTADDRESS "127.255.255.255"
#define BUFFER_SIZE 512						// Size of buffer that will be used for sending and receiving messages to client

typedef struct DHCP_message {
    long ciaddr;
    long siaddr;
};

int main()
{
    DHCP_message package;

    // Server address structure
    sockaddr_in serverAddress1;
    sockaddr_in serverAddress2;
    sockaddr_in broadcastAddress;

    // Size of server address structure
	int sockAddrLen1 = sizeof(serverAddress1);
    int sockAddrLen2 = sizeof(serverAddress2);
    int sockAddrLenBroadcast = sizeof(broadcastAddress);

	// Buffer that will be used for sending and receiving messages to client
    char dataBuffer[BUFFER_SIZE];

	// WSADATA data structure that is used to receive details of the Windows Sockets implementation
    WSADATA wsaData;
    
	// Initialize windows sockets for this process
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    
	// Check if library is succesfully initialized
	if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

   // Initialize memory for address structure
    memset((char*)&serverAddress1, 0, sizeof(serverAddress1));
    memset((char*)&serverAddress2, 0, sizeof(serverAddress2));
    memset((char*)&broadcastAddress, 0, sizeof(broadcastAddress));
    
    // Initialize address structure of server
    serverAddress1.sin_family = AF_INET;			// IPv4 address famly
    serverAddress1.sin_addr.s_addr = inet_addr(SERVER_ANY_ADDRESS);	// Set server IP address using string
    serverAddress1.sin_port = htons(SERVER_PORT_ANY);	// Set server port

	 // Initialize address structure of server
    serverAddress2.sin_family = AF_INET;								// IPv4 address famly
    serverAddress2.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// Set server IP address using string
    serverAddress2.sin_port = htons(SERVER_PORT_IP);					// Set server port

    // Initialize address structure of server
    broadcastAddress.sin_family = AF_INET;			// IPv4 address famly
    broadcastAddress.sin_addr.s_addr = inet_addr(DHCP_LOCAL_BROADCASTADDRESS);	// Set server IP address using string
    broadcastAddress.sin_port = htons(DHCP_PORT);	// Set server port

	// Create a socket
    SOCKET clientSocket = socket(AF_INET,      // IPv4 address famly
								 SOCK_DGRAM,   // Datagram socket
								 IPPROTO_UDP); // UDP protocol

	// Check if socket creation succeeded
    if (clientSocket == INVALID_SOCKET)
    {
        printf("Creating socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

	// Read string from user into outgoing buffer
    //gets_s(dataBuffer, BUFFER_SIZE);

    //DHCP DISCOVER
    printf("DISCOVER Your IP address. (press any key)\n");

    package.ciaddr = 0;
    package.siaddr = 0;

    _getch();

    #pragma region DHCP DISCOVER
    //client send message to all servers
    // Send message to server
    iResult = sendto(clientSocket,						// Own socket
        (char*)&package,						// Text of message
        sizeof(package),				// Message size
        0,									// No flags
        (SOCKADDR*)&broadcastAddress,		// Address structure of server (type, IP address and port)
        sizeof(broadcastAddress));			// Size of sockadr_in structure

    #pragma endregion
    
    #pragma region DHCP OFFER
    struct in_addr client_addr;

    //Client receive offer from servers
    // Receive server message
    iResult = recvfrom(clientSocket,				// Own socket
        (char*)&package,					// Buffer that will be used for receiving message
        sizeof(package),					// Maximal size of buffer
        0,							// No flags
        (SOCKADDR*)&serverAddress1,	// Client information from received message (ip address and port)
        &sockAddrLen1);				// Size of sockadd_in structure

    client_addr.s_addr = package.ciaddr;

    char address1 [16];
    strcpy_s(address1, inet_ntoa(client_addr));

    // Receive server message
    iResult = recvfrom(clientSocket,				// Own socket
        (char*)&package,					// Buffer that will be used for receiving message
        sizeof(package),					// Maximal size of buffer
        0,							// No flags
        (SOCKADDR*)&serverAddress2,	// Client information from received message (ip address and port)
        &sockAddrLen2);				// Size of sockadd_in structure

    client_addr.s_addr = package.ciaddr;

    char address2 [16];
    strcpy_s(address2, inet_ntoa(client_addr));
    #pragma endregion

    #pragma region DHCP REQUEST
    //client send request for address to specified server
    printf("\nREQUEST address:\n\t1. %s\n\t2. %s\n", address1, address2);
    gets_s(dataBuffer, BUFFER_SIZE);

    if (strcmp(dataBuffer,"1") == 0) {

        package.ciaddr = inet_addr(address1);

        package.siaddr = serverAddress1.sin_addr.s_addr;

        printf("\nYour request for %s address is sent.\n", address1);
    }
    else if(strcmp(dataBuffer, "2") == 0) {
        
        package.ciaddr = inet_addr(address2);

        package.siaddr = serverAddress2.sin_addr.s_addr;

        printf("\nYour request for %s address is sent.\n", address2);
    }

    // Send message to server
    iResult = sendto(clientSocket,						// Own socket
        (char*)&package,						// Text of message
        sizeof(package),				// Message size
        0,									// No flags
        (SOCKADDR*)&broadcastAddress,		// Address structure of server (type, IP address and port)
        sizeof(broadcastAddress));			// Size of sockadr_in structure
    #pragma endregion

    #pragma region DHCP ACKNOWLEDGE

    if (package.siaddr == serverAddress1.sin_addr.s_addr) {
        // Receive server message
        iResult = recvfrom(clientSocket,				// Own socket
            (char*)&package,					// Buffer that will be used for receiving message
            sizeof(package),					// Maximal size of buffer
            0,							// No flags
            (SOCKADDR*)&serverAddress1,	// Client information from received message (ip address and port)
            &sockAddrLen1);				// Size of sockadd_in structure
    }
    else if (package.siaddr == serverAddress2.sin_addr.s_addr) {
        // Receive server message
        iResult = recvfrom(clientSocket,				// Own socket
            (char*)&package,					// Buffer that will be used for receiving message
            sizeof(package),					// Maximal size of buffer
            0,							// No flags
            (SOCKADDR*)&serverAddress2,	// Client information from received message (ip address and port)
            &sockAddrLen2);				// Size of sockadd_in structure
    }

    printf("\nServer has ACKNOWLEDGED Your IP address.\n");
    #pragma endregion

    // Check if message is succesfully received
    if (iResult == SOCKET_ERROR)
    {
        printf("recvfrom failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

	// Check if message is succesfully sent. If not, close client application
	if (iResult == SOCKET_ERROR)
    {
        printf("sendto failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

	// Only for demonstration purpose
	printf("\nPress any key to exit: ");
	_getch();

    #pragma region CLIENT SHUT DOWN
    if (package.siaddr == serverAddress1.sin_addr.s_addr) {
        // Send message to server
        iResult = sendto(clientSocket,						// Own socket
            (char*)&package,						// Text of message
            sizeof(package),				// Message size
            0,									// No flags
            (SOCKADDR*)&serverAddress1,		// Address structure of server (type, IP address and port)
            sizeof(serverAddress1));			// Size of sockadr_in structure
    }
    else if (package.siaddr == serverAddress2.sin_addr.s_addr) {
        // Send message to server
        iResult = sendto(clientSocket,						// Own socket
            (char*)&package,						// Text of message
            sizeof(package),				// Message size
            0,									// No flags
            (SOCKADDR*)&serverAddress2,		// Address structure of server (type, IP address and port)
            sizeof(serverAddress2));			// Size of sockadr_in structure
    }

#pragma endregion
	// Close client application
    iResult = closesocket(clientSocket);
    if (iResult == SOCKET_ERROR)
    {
        printf("closesocket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
        return 1;
    }

	// Close Winsock library
    WSACleanup();

	// Client has succesfully sent a message
    return 0;
}
