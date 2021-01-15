
/*
Authors – Matan Achiel - 205642119, Sagi Levi - 205663545
Project – Ex4 - Client - main.
Description – This program is the main program - main.c
gets a 3 args - the server's ip address and port number, and the client's username
It validiates the input args, managing the whole communication between one client to the server - incoming and outcoming messages
by creating a unique socket and thread to represent it.
*/
//Defines...........................
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//Includes...........................
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

//Global variables
SOCKET m_socket;

//Implementations..........................................

//Reading data coming from the server. return 0 or the error code else.
static DWORD recv_data_thread(void)
{
	TransferResult_t RecvRes;
	while (1)
	{
		char* AcceptedStr = NULL;
		RecvRes = receive_string(&AcceptedStr, m_socket);

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

// gets the status of reading data from the server - the enum variable recv_res and return 0 for a success or 1 for a failure.
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

//disconnect the client from the server. return 0 for a success or 1 else.
int disconnect(SOCKET* m_socket)
{
	TransferResult_t send_res;
	printf("disconnecting...\n");
	send_res = send_string("CLIENT_DISCONNECT", *m_socket);
	if (send_res == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket - CLIENT_DISCONNECT\n");
		return 1;
	}
	disconnect_socket(m_socket);
	return 0;
}
//sends a the server that the current connected client wants to play (CLIENT_VERSUS message), and recieve an answer from the server:
//weather there is an opponent for the game or not , and if there is there will be a game invite message.
//returns 0 for game invite, 1 for no opponents, and -1 for any error.
int get_versus_respond()
{
	char* accepted_str = NULL;
	char params[MAX_PARAM_LENG];
	char msg_type[MSG_TYPE_MAX_LENG];
	int no_oppennet = 0; // 0 means there is opponent, 1 means no opponent
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

	printf("waiting for feedback about CLIENT_VERSUS from server\n");
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
		printf("msg recived from CLIENT_VERSUS: %s \n", accepted_str);

		if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_NO_OPPENNTS"))
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

//gets a pointer to all the the parameters of the thread of the current connected client - lpParam
// and n=managing the connection messages of the client with the server, handling answeringthe server to the main menu.
//the client's choice - quit or look for an opponent, starting the game and handling any disconnections or faliures of connection.
//returns 0 for success or the error code else.
static DWORD send_data_thread(LPVOID lpParam){
	char SendStr[256], params[MAX_PARAM_LENG], msg_type[MSG_TYPE_MAX_LENG];
	TransferResult_t send_res, recv_res;
	int choice = 0, done = 0;
	char* accepted_str = NULL;
	int no_oppennet = 0;// 0 means there is opponent, 1 means no opponent
	thread_args_client* temp_arg = (thread_args_client*)lpParam;
	char* server_address; int server_port; char* username; SOCKADDR_IN clientService;
	server_address = temp_arg->server_address;
	server_port = temp_arg->server_port;
	username= temp_arg->username;
	clientService= temp_arg->client_service;
	int game_finished = 0;
	while (!done){
		accepted_str = NULL;
		printf("waiting for input from server\n");
		recv_res = receive_string(&accepted_str, m_socket);
		if (check_recieved(recv_res))//error while readig data from server's socket.
			return 0x555;
		printf("%s\n", accepted_str);
		if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_MAIN_MENU")){
			while (1) { // while hasn't started a game, keep bugging him
				printf(SERVER_MAIN_MENU_MSG);
				choice = get_input_choice();
				if (choice == 2) {//quitting the game
					disconnect(&m_socket);
					free(accepted_str);
					return 0;
				}
				else{ //wants to play someone
					no_oppennet = get_versus_respond();
					if (no_oppennet == 1)
						continue;
					else if (no_oppennet == -1) { // error at the func
								disconnect(&m_socket);
								free(accepted_str);
								return 0;
							}
						else if (no_oppennet == -2) // timeout at wait - 30 sec
								{// show fail and reconnect msg
									if (reconnect_msg(2, server_address, server_port, &m_socket)) //closesocket
										return 1;
									if (connect_to_server(server_address, server_port, username, clientService))
									{ // TODO FREE PROPER
										free(accepted_str);
										return 1;
									}// go back to recieve
									game_finished = 1;
									break;
								}
					else { //found an opponent, starting a game
						if (start_game()){
							printf("game failed\n");
							game_finished = 1; // accept oppennet quit 
							break;
						}
						else{
							game_finished = 1;
							break;
						}}}
			}}// here the game started
		else if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_OPPONENT_QUIT"))
			{
					free(accepted_str);
					printf(SERVER_OPPONENT_QUIT_MSG);
					continue;
			}
			else // unrecognized msg - none of the above, message with parameters
			{
				printf("unrecognized msg\n");
				get_msg_type_and_params(accepted_str, &msg_type, &params);
				printf("params: %s\n", params);
				printf("msg_type is: %s\n", msg_type);
				if (STRINGS_ARE_EQUAL(msg_type, "SERVER_INVITE")){
					printf(SERVER_INVITE_MSG);
					free(accepted_str);
					continue;
				}
				else
					printf("%s what?????\n", accepted_str);
			}
		if (game_finished){
			game_finished = 0;
			continue;
		}
		send_res = send_string(SendStr, m_socket);
		if (send_res == TRNS_FAILED){
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		free(accepted_str);
	}// CLOSESOCKET - WHY NEED TO CLOSE SOCKETS HERE, WHEN THE GAME RUNNING WAS GOOD? NEED TO TO IT IN CASE OF ERROR.
	return 0; //succsess game and connection
}

//Gets an integer number - value, and returns the number of digits in it.
int count_digits(int value)
{
	int result = 0;
	while (value != 0) {
		value /= 10;
		result++;
	}
	return result;
}

//Gets the user's move for one turn from the cmd - his number or his current guess for the opponent number.
//Verify the input's validity and only when it is valid returns it.
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

//Gets the parameters of the current move - params, which includes the number of bulls, number of cows, and the opponent username and move.
//It also gets an indicator if there is a winner in this move or not, and display the currect message to the client.
void get_results(char* params, int win_mode)
{
	char bull = 'a';
	char cows = 'a';
	char opponent_username[USERNAME_MAX_LENG];
	char opponent_move[NUM_INPUT_LENGTH];

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
		opponent_username[j] = params[i];
		j++;
		i++;
	}
	opponent_username[j] = '\0';
	j = 0;
	i++;
	while (params[i] != '\0')
	{
		opponent_move[j] = params[i];
		j++;
		i++;
	}
	opponent_move[j] = '\0';

	if (win_mode)
		printf("%s%s%s\n", opponent_username, SERVER_WIN_MSG, opponent_move);
	else
	{
		printf("%s%c\n", GAME_RESULTS_MSG1, bull);
		printf("%s%c\n", GAME_RESULTS_MSG2, cows);
		printf("%s%s%s\n", opponent_username, GAME_RESULTS_MSG3, opponent_move);
	}

}

// Handels the whole game moves - for each one, gets a message from the server, identifies it and send a proper reply/
//return the error code for aby failure or 0 else.
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

	if (set_socket_timeout(CLIENT_TIMEOUT, m_socket)) // setting timeout for socket
		return 1;

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
			input_num = get_user_input_num(); // convert input_num to string [buf]
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
		else if (STRINGS_ARE_EQUAL(accepted_str, "SERVER_OPPONENT_QUIT"))
		{
			free(accepted_str);
			printf(SERVER_OPPONENT_QUIT_MSG);
			return 0;
		}
		else // got a message with parameters from the server
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
				printf("%s - unreconzie msg\n", accepted_str);
		}

		send_res = send_string(send_str, m_socket);
		if (send_res == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		free(accepted_str);

	}
	return 0;
}

//Gets all the input arguments from the user - input_args, their amount - num_of_args, pointers to the server's port 
//,the server's address and the username - server_port, server_address, username and initiate all the ointers with the data from the first argument.
//returns 0 for success or 1 else.
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

// Gets the clients coice from a given menu - 1 or 2, and returns it.
int get_input_choice()
{
	char user_input[256];
	// TODO FIX SIZE OF INPUT - sagi thinks it is supposed to be 1 char. can be only 1 or 2.

	while (1)
	{
		gets_s(user_input, sizeof(user_input)); //Reading a string from the keyboard

		if (STRINGS_ARE_EQUAL(user_input, "1"))
			return 1;

		else if (STRINGS_ARE_EQUAL(user_input, "2"))
		{
			return 2;
		}
		else if (!user_input)
			continue;
		else if (STRINGS_ARE_EQUAL(user_input, "\n"))
			continue;
		else if (STRINGS_ARE_EQUAL(user_input, ""))
			continue;
		else
			printf("Please choose again: 1/2?\n");
	}
}

//gets an indicator for message type of a failure - msg, the address of the server - server_address,
//the server port - server_port, and a socket of the client - m_socket. It itentifies the message's type
//and printing a proper message for the client: failure/ disconnection/ server denied
//it returns 0 for success or 1 else.
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

//gets the address of the server - server_address,
//the server port - server_port, the client's username - username, and the address of the socket of the client - clientService. 
//It handles all the proccess of connecting the client to the server.it returns 0 for success or 1 for any failure.
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
			if (reconnect_msg(1, server_address, server_port, &m_socket))
			{
				closesocket(m_socket);//closesocket
				return 1;
			}
		}
		else // Succsefull connection
		{
			accepted_str = NULL;
			printf("%s%s:%d\n", SUCCESSFUL_CONNECT_MSG, server_address, server_port);

			////// client step 2 - CLIENT_REQUEST
			strcpy_s(msg, 16, "CLIENT_REQUEST:");

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

			// if timeout sending disconnect messege and try to reconnect
			if (check_recieved(recv_res))
			{
				strcpy_s(msg,18,"CLIENT_DISCONNECT");

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

//Gets the address of the server - server_address,
//the server port - server_port, the client's username - username, and the address of the socket of the client - clientService. 
//It creates a struct to characterize the client's thread containing all the arguments, and returns a pointer to it.
thread_args_client* create_client_thread_arg(char* server_address, int server_port, char* username, SOCKADDR_IN client_service)
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
	temp_arg->client_service = client_service;

	return temp_arg;
}

int main(int argc, char* argv[])
{
	int server_port = 0;
	char* server_address;
	char* username;

	SOCKADDR_IN client_service;
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

	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return 1;
	}
	
	 //For a client to communicate on a network, it must connect to a server.
	 // Connect to a server.
	 //Create a sockaddr_in object clientService and set  values.
	client_service.sin_family = AF_INET;
	client_service.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	client_service.sin_port = htons(SERVER_PORT); //Setting the port to connect to.

	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if (connect_to_server(server_address, server_port, username, client_service))
	{
		closesocket(m_socket);
		WSACleanup();
		return 1;
	}

	thread_args_client* thread_args_client=NULL;
	thread_args_client = create_client_thread_arg(server_address, server_port, username, client_service);
	hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)send_data_thread,
		thread_args_client,
		0,
		NULL
	);

	WaitForSingleObject(hThread, INFINITE); /* Waiting for the process to end */
	TerminateThread(hThread, 0x555);
	CloseHandle(hThread);
	closesocket(m_socket);
	WSACleanup();

	return 0;
}