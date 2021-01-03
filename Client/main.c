#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "SocketExampleShared.h"
#include "SocketSendRecvTools.h"
#include "msg.h"


SOCKET m_socket;

//Reading data coming from the server
static DWORD RecvDataThread(void)
{
	TransferResult_t RecvRes;

	while (1)
	{
		char* AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (RecvRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Server closed connection. Bye!\n");
			return 0x555;
		}
		else
		{
			printf("%s\n", AcceptedStr);
		}

		free(AcceptedStr);
	}

	return 0;
}

int check_recieved(char* recieved_str)
{
	if (recieved_str == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 1;
	}
	else if (recieved_str == TRNS_DISCONNECTED)
	{
		printf("Server closed connection. Bye!\n");
		return 1;
	}
	return 0;
}



//Sending data to the server
static DWORD SendDataThread(void)
{
	char SendStr[256];
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	int choice = 0;
	while (1)
	{
		char* AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (check_recieved(AcceptedStr))
			return 0x555;

		printf("%s\n", AcceptedStr);

		if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_APPROVED"))
		{
			printf(SERVER_MAIN_MENU_MSG);
			choice = get_input_choice();
			if (choice == 2) //quitting the game
			{

				SendRes = SendString("CLIENT_DISCONNECT", m_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Socket error while trying to write data to socket\n");
					return 0x555;
				}
				free(AcceptedStr);
				printf("Quitting...\n");

				return;
			}
			else //want to play someone
			{
				SendRes = SendString("CLIENT_VERSUS", m_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Socket error while trying to write data to socket\n");
					return 0x555;
				}
			}
		}
		else // unreconize msg
		{
			printf("%s what?????\n", AcceptedStr);
			// fix recoginze msg likes SERVER_INVITE:Oppenet name
		}
		free(AcceptedStr);


		//gets_s(SendStr, sizeof(SendStr)); //Reading a string from the keyboard

		//if (STRINGS_ARE_EQUAL(SendStr, "quit"))
		//	return 0x555; //"quit" signals an exit from the client side

		//SendRes = SendString(SendStr, m_socket);

		//if (SendRes == TRNS_FAILED)
		//{
		//	printf("Socket error while trying to write data to socket\n");
		//	return 0x555;
		//}
	}
}
// Client.exe <server ip> <server port> <username>

int init_input_vars(char* input_args[], int num_of_args, int* server_port, char** server_address, char** username)
{
	if (num_of_args != 4) //Not enough arguments.
	{
		printf("Invalid input')");
		return 1;
	}
	if (*input_args[2] == '0') // TODO test for port length
		*server_port = 0;
	else
	{
		*server_port = strtol(input_args[2], NULL, 10);
		if (*server_port == 0) // case of failed strtol
		{
			printf("invalid argument for server_port");
			return 1;
		}
	}

	*server_address = input_args[1];
	*username = input_args[3];

	return 0;
}

// Client.exe <server ip> <server port> <username>
int get_input_choice()
{
	char user_input[256];
	// TODO FIX SIZE OF INPUT 

	while (1)
	{
		gets_s(user_input, sizeof(user_input)); //Reading a string from the keyboard

		if (STRINGS_ARE_EQUAL(user_input, "1"))
			return 1;

		else if (STRINGS_ARE_EQUAL(user_input, "2"))
		{
			return 2;
		}
		else
			printf("Please choose again: 1/2?\n");
	}
}

//CLIENT MAIN

int main(int argc, char* argv[])
{
	int server_port = 0;
	char* server_address;
	char* username;

	SOCKADDR_IN clientService;
	HANDLE hThread;

	if (init_input_vars(argv, argc, &server_port, &server_address, &username))
		return 1;

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.

	//Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.

	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}
	/*
	 The parameters passed to the socket function can be changed for different implementations.
	 Error detection is a key part of successful networking code.
	 If the socket call fails, it returns INVALID_SOCKET.
	 The if statement in the previous code is used to catch any errors that may have occurred while creating
	 the socket. WSAGetLastError returns an error number associated with the last error that occurred.
	 */


	 //For a client to communicate on a network, it must connect to a server.
	 // Connect to a server.

	 //Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(SERVER_PORT); //Setting the port to connect to.

	/*
		AF_INET is the Internet address family.
	*/


	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	int reconnect = 1;
	int choice = 0;

	while (reconnect)
	{

		if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
		{
			printf("%s%s:%d\n", FAILED__CONNECT_MSG, server_address, server_port);
			printf("%s", WAITING_OPTIONS);
			choice = get_input_choice();
			if (choice == 2)
			{
				printf("Exiting...\n");
				WSACleanup();
				return;
			}
		}
		else // Succsefull connection
			reconnect = 0;
	}
	//TODO 4 AND 5 SUDDEN DISCONNECTION

	// Succsefull connection message
	printf("%s%s:%d ", SUCCESSFUL_CONNECT_MSG, server_address, server_port);
	// Send and receive data.
	/*
		In this code, two integers are used to keep track of the number of bytes that are sent and received.
		The send and recv functions both return an integer value of the number of bytes sent or received,
		respectively, or an error. Each function also takes the same parameters:
		the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/
	TransferResult_t SendRes;

	////// client step 2 - CLIENT_REQUEST
	char msg[USERNAME_MAX_LENG + CLIENT_REQUEST_LENG + 1];
	strcpy(msg, "CLIENT_REQUEST:");

	strcat_s(msg, (USERNAME_MAX_LENG + CLIENT_REQUEST_LENG + 1), username);

	SendRes = SendString(msg, m_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}

	hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SendDataThread,
		NULL,
		0,
		NULL
	);
	WaitForSingleObject(
		hThread,
		INFINITE); /* Waiting for the process to end */


	TerminateThread(hThread, 0x555);

	CloseHandle(hThread);
	closesocket(m_socket);

	WSACleanup();

	return;
}