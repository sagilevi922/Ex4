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
#include "HardCodedData.h"
#include "main.h"


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


// FAIL RETURN 1
int check_recieved(TransferResult_t RecvRes)
{
	if (RecvRes == TRNS_FAILED)
	{
		/*printf("Socket error while trying to write data to socket\n");*/
		return 1;
	}
	else if (RecvRes == TRNS_DISCONNECTED)
	{
		printf("Server closed connection. Bye!\n");
		return 1;
	}
	return 0;
}

int disconnect()
{
	TransferResult_t SendRes;
	printf("Quitting...\n");
	SendRes = SendString("CLIENT_DISCONNECT", m_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket - CLIENT_DISCONNECT\n");
		return 1;
	}
	return 0;
}
// 0 mean there is oppent, 1 means no oppennet
int get_versus_respond()
{
	char* AcceptedStr = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];
	int no_oppennet = 0;
	TransferResult_t RecvRes;
	TransferResult_t SendRes;

	SendRes = SendString("CLIENT_VERSUS", m_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return -1;
	}

	if (set_socket_timeout(VERSUS_TIMEOUT, m_socket)) // setting timeout for socket 30 sec
		return -1;

	printf("waiting for input from server\n");
	RecvRes = ReceiveString(&AcceptedStr, m_socket);

	if (set_socket_timeout(CLIENT_TIMEOUT, m_socket)) // return timeout for socket 15 sec
		return -1;

	if (check_recieved(RecvRes))
	{ // NOTHING RECIEVE FROM SERVER
		printf("NOTHING RECIEVE FROM SERVER\n");
		return -2; // TODO take care of timeout
	}

	else // recieved a msg
	{
		if (STRINGS_ARE_EQUAL(msg_type, "SERVER_NO_OPPENNTS")) 
		{
			free(AcceptedStr);
			printf("NO OPPENNTS\n");
			return 1;
		}
		else // there is oppennet
		{
			get_msg_type_and_params(AcceptedStr, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);
			if (STRINGS_ARE_EQUAL(msg_type, "SERVER_INVITE"))
			{
				printf(SERVER_INVITE_MSG);
				free(AcceptedStr);
				return 0;

			}
		}
	}
	return -1; // fail
}
//Sending data to the server
static DWORD SendDataThread(LPVOID lpParam)
{
	char SendStr[256];
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	int choice = 0;
	char* AcceptedStr = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];
	int no_oppennet = 0;
	int done = 0;

	thread_args_client* temp_arg = (thread_args_client*)lpParam;
	char* server_address; int server_port; char* username; SOCKADDR_IN clientService;
	server_address = temp_arg->server_address;
	server_port = temp_arg->server_port;
	username= temp_arg->username;
	clientService= temp_arg->clientService;


	while (!done)
	{
		AcceptedStr = NULL;
		printf("waiting for input from server\n");
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (check_recieved(RecvRes))
			return 0x555;

		printf("%s\n", AcceptedStr);

		if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_MAIN_MENU"))
		{
			while (1) // while didnt started a game, keep bugging him
			{
				printf(SERVER_MAIN_MENU_MSG);
				choice = get_input_choice();
				if (choice == 2) //quitting the game
				{
					disconnect();
					free(AcceptedStr);
					return 0;
				}
				else //want to play someone
				{
					no_oppennet = get_versus_respond();
					if (no_oppennet == 1)
						continue;
					else if (no_oppennet == -1) // error at the the func
					{
						disconnect();
						free(AcceptedStr);
						return 0;
					}
					else if (no_oppennet == -2) // timeout at wait - 30 sec
					{// show fail and recinnect msg
						if (connect_to_server(server_address, server_port, username, clientService))
						{ // TODO FREE PROPER
							free(AcceptedStr);
							return 1;
						}
					}
					else // found an oppenet start game
					{
						start_game();
					}
				}
			}
		}
		
		else // unreconize msg
		{
			get_msg_type_and_params(AcceptedStr, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);

			if (STRINGS_ARE_EQUAL(msg_type, "SERVER_INVITE"))
			{
				printf(SERVER_INVITE_MSG);
				free(AcceptedStr);
				continue;
			}
			else
				printf("%s what?????\n", AcceptedStr);
		}

		SendRes = SendString(SendStr, m_socket);
		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		free(AcceptedStr);

	}
	// closesocket
}
int countDigits(int value)
{
	int result = 0;
	while (value != 0) {
		value /= 10;
		result++;
	}
	return result;
}

int get_user_input_num()
{
	int n = 0;
	while (countDigits(n) != 4)
	{
		scanf("%d", &n);
		if (countDigits(n) != 4)
			printf("Invalid input - Please enter 4 digits num with differnt digits\n");
	}	
	return n;
}

void get_results(char* params, int win_mode)
{
	char bull = 'a';
	char cows = 'a';
	char oppenet_username[USERNAME_MAX_LENG];
	char oppenet_move[NUM_INPUT_LENGTH];


	int i = 4;
	int j = 0;
	if (win_mode)
		i = 0;
	else
	{
		bull = params[0];
		cows = params[2];
	}

	while (params[i] != ';')
	{
		oppenet_username[j] = params[i];
		j++;
		i++;
	}
	oppenet_username[j] = '\0';
	j = 0;
	i++;
	while (params[i] != '\0')
	{
		oppenet_move[j] = params[i];
		j++;
		i++;
	}
	oppenet_move[j] = '\0';

	if (win_mode)
		printf("%s%s%s\n", oppenet_username, SERVER_WIN_MSG, oppenet_move);
	else
	{
		printf("%s%c\n", GAME_RESULTS_MSG1, bull);
		printf("%s%c\n", GAME_RESULTS_MSG2, cows);
		printf("%s%s%s\n", oppenet_username, GAME_RESULTS_MSG3, oppenet_move);
	}

}

// Client.exe <server ip> <server port> <username>
int start_game()
{
	char SendStr[256];
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	int choice = 0;
	char* AcceptedStr = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];
	int done = 0;

	int input_num = 0;
	char input_num_str[NUM_INPUT_LENGTH];


	while (!done)
	{
		AcceptedStr = NULL;
		printf("waiting for input from server - START GAME\n");
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (check_recieved(RecvRes))
			return 0x555;

		printf("%s\n", AcceptedStr);

		if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_SETUP_REQUEST"))
		{
			printf(SERVER_SETUP_REQUEST_MSG);
			input_num = get_user_input_num();
			// convert input_num to string [buf]

			strcpy_s(SendStr,14, "CLIENT_SETUP:");
			_itoa(input_num, input_num_str, 10);
			strcat_s(SendStr, MSG_MAX_LENG, input_num_str);

		}
		else if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_PLAYER_MOVE_REQUEST"))
		{
			printf(SERVER_PLAYER_MOVE_REQUEST_MSG);
			input_num = get_user_input_num();
			// convert input_num to string [buf]

			strcpy_s(SendStr, 20, "CLIENT_PLAYER_MOVE:");
			_itoa(input_num, input_num_str, 10);
			strcat_s(SendStr, MSG_MAX_LENG, input_num_str);

		}
		else if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_DRAW"))
		{
			free(AcceptedStr);
			printf(SERVER_DRAW_MSG);
			continue;
		}
		else // with params msg
		{
			get_msg_type_and_params(AcceptedStr, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);

			if (STRINGS_ARE_EQUAL(msg_type, "SERVER_GAME_RESULTS"))
			{
				free(AcceptedStr);
				get_results(params,0);
				continue;
			}
			else if (STRINGS_ARE_EQUAL(msg_type, "SERVER_WIN"))
			{
				free(AcceptedStr);
				get_results(params,1);
				continue;
			}

			else
				printf("%s what?????\n", AcceptedStr);
		}

		SendRes = SendString(SendStr, m_socket);
		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		free(AcceptedStr);

	}


}
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
int reconnect_msg(int msg, char* server_address, int server_port, SOCKET* m_socket)
{ // msg == 1  ----> failed, msg == 2 ---> failed with disconnection, 3----> denied

	int choice = 0;
	if (msg == 1 || msg ==2)
		printf("%s%s:%d.\n", FAILED__CONNECT_MSG, server_address, server_port);
	else
		printf("%s%s:%d %s\n", SERVER_DENIED_REQ_1, server_address, server_port, SERVER_DENIED_REQ_2);
	printf("%s", WAITING_OPTIONS);
	choice = get_input_choice();
	if (choice == 2)
	{
		printf("Exiting...\n");
		WSACleanup();
		return 1;
	}
	if (msg == 3 || msg == 2)
	{
		disconnect_socket(m_socket);
		if (NULL == *m_socket)
		{
			WSACleanup();
			return 1;
		}
	}
	return 0;
}

int connect_to_server(char* server_address, int server_port, char* username, SOCKADDR_IN clientService)
{
	int reconnect = 1;
	int choice = 0;
	TransferResult_t RecvRes;
	TransferResult_t SendRes;
	char* AcceptedStr = NULL;
	char msg[USERNAME_MAX_LENG + CLIENT_REQUEST_LENG + 1];

	while (reconnect)
	{

		if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
		{
			if (reconnect_msg(1, server_address, server_port, &m_socket)) //closesocket
				return 1;
		}
		else // Succsefull connection
		{
			AcceptedStr = NULL;
			printf("%s%s:%d\n", SUCCESSFUL_CONNECT_MSG, server_address, server_port);

			////// client step 2 - CLIENT_REQUEST
			strcpy(msg, "CLIENT_REQUEST:");

			strcat_s(msg, (USERNAME_MAX_LENG + CLIENT_REQUEST_LENG + 1), username);

			SendRes = SendString(msg, m_socket);

			if (SendRes == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return 1;
			}

			if (set_socket_timeout(CLIENT_TIMEOUT, m_socket)) // setting timeout for socket
				return 1;

			RecvRes = ReceiveString(&AcceptedStr, m_socket);
			printf("%s\n", AcceptedStr);

			// if timout sending disconnect messege and try to reconnect
			if (check_recieved(RecvRes))
			{
				strcpy(msg, "CLIENT_DISCONNECT");

				SendRes = SendString(msg, m_socket);

				if (SendRes == TRNS_FAILED)
				{
					printf("Socket error while trying to write data to socket CLIENT_DISCONNECT\n");
					return 1;
				}

				if (reconnect_msg(2, server_address, server_port, &m_socket))
					return 1;
			}

			if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_DENIED:room is full"))
			{
				if (reconnect_msg(3, server_address, server_port, &m_socket))
					return 1;

			}
			if (STRINGS_ARE_EQUAL(AcceptedStr, "SERVER_APPROVED"))
			{
				free(AcceptedStr);
				reconnect = 0;
			}
		}
	}
	return 0;
}


thread_args_client* create_client_thread_arg(char* server_address, int server_port, char* username, SOCKADDR_IN clientService)
{
	thread_args_client* temp_arg = (thread_args_client*)malloc(sizeof(thread_args_client));
	if (NULL == temp_arg)
	{
		printf("memory allocation failed");
		return NULL;
	}

	temp_arg->server_address = server_address;
	temp_arg->server_port = server_port;
	temp_arg->username = username;
	temp_arg->clientService = clientService;

	return temp_arg;
}


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


	if (connect_to_server(server_address, server_port, username, clientService))
	{ // TODO FREE PROPER
		return 1;
	}

	// Succsefull connection message
	// Send and receive data.
	/*
		In this code, two integers are used to keep track of the number of bytes that are sent and received.
		The send and recv functions both return an integer value of the number of bytes sent or received,
		respectively, or an error. Each function also takes the same parameters:
		the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/

	thread_args_client* thread_args_client=NULL;

	thread_args_client = create_client_thread_arg(server_address, server_port, username, clientService);


	hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SendDataThread,
		thread_args_client,
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