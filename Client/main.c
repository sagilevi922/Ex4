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
static DWORD recv_data_thread(void)
{
	TransferResult_t recv_res;
	while (1)
	{
		char* accepted_str = NULL;
		recv_res = receive_string(&accepted_str, m_socket);

		if (recv_res == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else if (recv_res == TRNS_DISCONNECTED)
		{
			printf("Server closed connection. Bye!\n");
			return 0x555;
		}
		else
		{
			printf("%s\n", accepted_str);
		}

		free(accepted_str);
	}

	return 0;
}

// FAIL RETURN 1
int check_recieved(TransferResult_t recv_res)
{
	if (recv_res == TRNS_FAILED)
	{
		/*printf("Socket error while trying to write data to socket\n");*/
		return 1;
	}
	else if (recv_res == TRNS_DISCONNECTED)
	{
		printf("Server closed connection. Bye!\n");
		return 1;
	}
	return 0;
}

int disconnect()
{
	TransferResult_t send_res;
	printf("Quitting...\n");
	send_res = send_string("CLIENT_DISCONNECT", m_socket);
	if (send_res == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket - CLIENT_DISCONNECT\n");
		return 1;
	}
	return 0;
}

// 0 mean there is oppent, 1 means no oppennet
int get_versus_respond()
{
	char* accepted_str = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];
	int no_oppennet = 0;
	TransferResult_t recv_res;
	TransferResult_t send_res;

	send_res = send_string("CLIENT_VERSUS", m_socket);
	if (send_res == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return -1;
	}

	if (set_socket_timeout(VERSUS_TIMEOUT, m_socket)) // setting timeout for socket 30 sec
		return -1;

	printf("waiting for input from server\n");
	recv_res = receive_string(&accepted_str, m_socket);

	if (set_socket_timeout(CLIENT_TIMEOUT, m_socket)) // return timeout for socket 15 sec
		return -1;

	if (check_recieved(recv_res))
	{ // NOTHING RECIEVE FROM SERVER
		printf("NOTHING RECIEVE FROM SERVER\n");
		return -2; // TODO take care of timeout
	}

	else // recieved a msg
	{
		if (STRINGS_ARE_EQUAL(msg_type, "SERVER_NO_OPPENNTS")) 
		{
			free(accepted_str);
			printf("NO OPPENNTS\n");
			return 1;
		}
		else // there is oppennet
		{
			get_msg_type_and_params(accepted_str, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);
			if (STRINGS_ARE_EQUAL(msg_type, "SERVER_INVITE"))
			{
				printf(SERVER_INVITE_MSG);
				free(accepted_str);
				return 0;

			}
		}
	}
	return -1; // fail
}

//Sending data to the server
static DWORD send_data_thread(LPVOID lpParam)
{
	char send_str[256];
	TransferResult_t send_res;
	TransferResult_t recv_res;
	int choice = 0, no_oppennet = 0, done = 0;
	char* accepted_str = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];

	thread_args_client* temp_arg = (thread_args_client*)lpParam;
	char* server_address; int server_port; char* username; SOCKADDR_IN client_service;
	server_address = temp_arg->server_address;
	server_port = temp_arg->server_port;
	username= temp_arg->username;
	client_service= temp_arg->client_service;

	while (!done)
	{
		accepted_str = NULL;
		printf("waiting for input from server\n");
		recv_res = receive_string(&accepted_str, m_socket);
		if (check_recieved(recv_res))
			return 0x555;

		printf("%s\n", accepted_str);

		if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_MAIN_MENU"))
		{
			while (1) // while didnt started a game, keep bugging him
			{
				printf(SERVER_MAIN_MENU_MSG);
				choice = get_input_choice();
				if (choice == 2) //quitting the game
				{
					disconnect();
					free(accepted_str);
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
						free(accepted_str);
						return 0;
					}
					else if (no_oppennet == -2) // timeout at wait - 30 sec
					{// show fail and recinnect msg
						if (connect_to_server(server_address, server_port, username, client_service))
						{ // TODO FREE PROPER
							free(accepted_str);
							return 1;
						}
					}
					else // found an oppenet start game
					{
						if (start_game())
							printf("game failed");
						else
							continue;
					}
				}
			}
		}
		
		else // unreconize msg
		{
			get_msg_type_and_params(accepted_str, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);

			if (STRINGS_ARE_EQUAL(msg_type, "SERVER_INVITE"))
			{
				printf(SERVER_INVITE_MSG);
				free(accepted_str);
				continue;
			}
			else
				printf("%s what?????\n", accepted_str);
		}

		send_res = send_string(send_str, m_socket);
		if (send_res == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		free(accepted_str);

	}
	// closesocket
}
int count_digits(int value)
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
	while (count_digits(n) != 4)
	{
		scanf("%d", &n);
		if (count_digits(n) != 4)
			printf("Invalid input - Please enter 4 digits num with differnt digits\n");
	}	
	return n;
}

void get_results(char* params, int win_mode)
{
	char bull = 'a', cows = 'a';
	char oppenet_username[USERNAME_MAX_LENG];
	char oppenet_move[NUM_INPUT_LENGTH];
	int i = 4, j = 0;

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
	char send_str[256];
	TransferResult_t send_res;
	TransferResult_t recv_res;
	int choice = 0;
	char* accepted_str = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];
	int done = 0;

	int input_num = 0;
	char input_num_str[NUM_INPUT_LENGTH];


	while (!done)
	{
		accepted_str = NULL;
		printf("waiting for input from server - START GAME\n");
		recv_res = receive_string(&accepted_str, m_socket);

		if (check_recieved(recv_res))
			return 0x555;

		printf("%s\n", accepted_str);

		if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_SETUP_REQUEST"))
		{
			printf(SERVER_SETUP_REQUEST_MSG);
			input_num = get_user_input_num();
			// convert input_num to string [buf]

			strcpy_s(send_str,14, "CLIENT_SETUP:");
			_itoa(input_num, input_num_str, 10);
			strcat_s(send_str, MSG_MAX_LENG, input_num_str);

		}
		else if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_PLAYER_MOVE_REQUEST"))
		{
			printf(SERVER_PLAYER_MOVE_REQUEST_MSG);
			input_num = get_user_input_num();
			// convert input_num to string [buf]

			strcpy_s(send_str, 20, "CLIENT_PLAYER_MOVE:");
			_itoa(input_num, input_num_str, 10);
			strcat_s(send_str, MSG_MAX_LENG, input_num_str);

		}
		else if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_DRAW"))
		{
			free(accepted_str);
			printf(SERVER_DRAW_MSG);
			return 0;
		}
		else // with params msg
		{
			get_msg_type_and_params(accepted_str, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);

			if (STRINGS_ARE_EQUAL(msg_type, "SERVER_GAME_RESULTS"))
			{
				free(accepted_str);
				get_results(params,0);
				continue;
			}
			else if (STRINGS_ARE_EQUAL(msg_type, "SERVER_WIN"))
			{
				free(accepted_str);
				get_results(params,1);
				return 0;
			}

			else
				printf("%s what?????\n", accepted_str);
		}

		send_res = send_string(send_str, m_socket);
		if (send_res == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		free(accepted_str);

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
	TransferResult_t recv_res;
	TransferResult_t send_res;
	char* accepted_str = NULL;
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
			accepted_str = NULL;
			printf("%s%s:%d\n", SUCCESSFUL_CONNECT_MSG, server_address, server_port);

			////// client step 2 - CLIENT_REQUEST
			strcpy(msg, "CLIENT_REQUEST:");

			strcat_s(msg, (USERNAME_MAX_LENG + CLIENT_REQUEST_LENG + 1), username);

			send_res = send_string(msg, m_socket);

			if (send_res == TRNS_FAILED)
			{
				printf("Socket error while trying to write data to socket\n");
				return 1;
			}

			if (set_socket_timeout(CLIENT_TIMEOUT, m_socket)) // setting timeout for socket
				return 1;

			recv_res = receive_string(&accepted_str, m_socket);
			printf("%s\n", accepted_str);

			// if timout sending disconnect messege and try to reconnect
			if (check_recieved(recv_res))
			{
				strcpy(msg, "CLIENT_DISCONNECT");

				send_res = send_string(msg, m_socket);

				if (send_res == TRNS_FAILED)
				{
					printf("Socket error while trying to write data to socket CLIENT_DISCONNECT\n");
					return 1;
				}

				if (reconnect_msg(2, server_address, server_port, &m_socket))
					return 1;
			}

			if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_DENIED:room is full"))
			{
				if (reconnect_msg(3, server_address, server_port, &m_socket))
					return 1;

			}
			if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_APPROVED"))
			{
				free(accepted_str);
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
	temp_arg->client_service = clientService;

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
	 // Connect to a server.
	 //Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(SERVER_PORT); //Setting the port to connect to.

	//AF_INET is the Internet address family.
	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.

	if (connect_to_server(server_address, server_port, username, clientService))
	{ // TODO FREE PROPER
		return 1;
	}

	thread_args_client* thread_args_client=NULL;
	thread_args_client = create_client_thread_arg(server_address, server_port, username, clientService);

	hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)send_data_thread,
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