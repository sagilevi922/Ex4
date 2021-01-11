//Defines..................................................
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//Includes.................................................
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <math.h>
#pragma comment(lib, "Ws2_32.lib")


#ifndef MAIN_H_
#define MAIN_H_
//Structs..................................................
typedef struct thread_arguments_client {
	char* server_address;
	int server_port;
	char* username;
	SOCKADDR_IN client_service;
} thread_args_client;


#endif

//Declerations.............................................

//Reading data coming from the server. return 0 or the error code else.
static DWORD recv_data_thread(void);

// gets the status of reading data from the server - the enum variable recv_res and return 0 for a success or 1 for a failure.
int check_recieved(TransferResult_t recv_res);

//disconnect the client from the server. return 0 for a success or 1 else.
int disconnect(SOCKET* m_socket);

//sends a the server that the current connected client wants to play (CLIENT_VERSUS message), and recieve an answer from the server:
//weather there is an opponent for the game or not , and if there is there will be a game invite message.
//returns 0 for game invite, 1 for no opponents, and -1 for any error.
int get_versus_respond();

//gets a pointer to all the the parameters of the thread of the current connected client - lpParam
// and n=managing the connection messages of the client with the server, handling answeringthe server to the main menu.
//the client's choice - quit or look for an opponent, starting the game and handling any disconnections or faliures of connection.
//returns 0 for success or the error code else.
static DWORD send_data_thread(LPVOID lpParam);

//Gets an integer number - value, and returns the number of digits in it.
int count_digits(int value);

//Gets the user's move for one turn from the cmd - his number or his current guess for the opponent number.
//Verify the input's validity and only when it is valid returns it.
int get_user_input_num();

//Gets the parameters of the current move - params, which includes the number of bulls, number of cows, and the opponent username and move.
//It also gets an indicator if there is a winner in this move or not, and display the currect message to the client.
void get_results(char* params, int win_mode);

// Handels the whole game moves - for each one, gets a message from the server, identifies it and send a proper reply/
//return the error code for aby failure or 0 else.
int start_game();

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
int get_input_choice();

//gets an indicator for message type of a failure - msg, the address of the server - server_address,
//the server port - server_port, and a socket of the client - m_socket. It itentifies the message's type
//and printing a proper message for the client: failure/ disconnection/ server denied
//it returns 0 for success or 1 else.
int reconnect_msg(int msg, char* server_address, int server_port, SOCKET* m_socket);

//gets the address of the server - server_address,
//the server port - server_port, the client's username - username, and the address of the socket of the client - clientService. 
//It handles all the proccess of connecting the client to the server.it returns 0 for success or 1 for any failure.
int connect_to_server(char* server_address, int server_port, char* username, SOCKADDR_IN clientService);
//Gets the address of the server - server_address,
//the server port - server_port, the client's username - username, and the address of the socket of the client - clientService. 
//It creates a struct to characterize the client's thread containing all the arguments, and returns a pointer to it.
thread_args_client* create_client_thread_arg(char* server_address, int server_port, char* username, SOCKADDR_IN client_service);