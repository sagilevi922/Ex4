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
#include "HardCodedData.h"
#include "msg.h"
#include "Lock.h"
#include "file_handler.h"
#include "main.h"
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#define NUM_OF_CLIENTS 2

#define MAX_THREADS 3

#define MAX_LOOPS 3


/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

HANDLE ThreadHandles[MAX_THREADS];
SOCKET ThreadInputs[MAX_THREADS];
int active_users = 0;
int win = 0;
int global_round = 0;
int global_read = 0;

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

static int FindFirstUnusedThreadSlot();
static void CleanupWorkerThreads();
static DWORD ServiceThread(LPVOID lpParam);

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
thread_args* create_thread_arg(SOCKET* socket, lock* lock, HANDLE semaphore_gun)
{
	thread_args* temp_arg = (thread_args*)malloc(sizeof(thread_args));
	if (NULL == temp_arg)
	{
		printf("memory allocation failed");
		return NULL;
	}

	temp_arg->socket = socket;
	temp_arg->lock = lock;
	temp_arg->semaphore_gun = semaphore_gun;

	return temp_arg;
}

int init_input_vars(char* input_args[], int num_of_args, int* server_port)
{
	if (num_of_args != 2) //Not enough arguments.
	{
		printf("Invalid input, Please provide encrypted file path, enc/dec key, number of threads and action mode('e'/'d')");
		return 1;
	}

	if (*input_args[1] == '0') // TODO test for port length
		*server_port = 0;
	else
	{
		*server_port = strtol(input_args[1], NULL, 10);
		if (*server_port == 0) // case of failed strtol
		{
			printf("invalid argument for server_port");
			return 1;
		}
	}
	return 0;
}


/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

static int FindFirstUnusedThreadSlot(HANDLE semaphore_gun, thread_args** thread_args)
{
	int Ind;
	bool release_res;

	for (Ind = 0; Ind < NUM_OF_CLIENTS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);
			//DWORD Res = 1;

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				free(*(thread_args+ Ind));
				thread_args[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

static void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_CLIENTS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], INFINITE);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind]);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				printf("Waiting for thread failed. Ending program\n");
				return;
			}
		}
	}
} 
int get_oppennet_user_name(int first, int username_length, char* oppenet_username, lock* lock, HANDLE semaphore_gun)
{
	int start_pos = 0;
	int end_pos = 0;
	int bytes_to_read = 0;
	int i = 0;
	HANDLE hFile = NULL;
	DWORD dwFileSize = 0;
	//if (!lock_read(lock)) { // Locking Read lock
	//	printf("Error while Locking for read...\n");
	//	return 1;
	//}
	bool wait_res;
	bool release_res;

	if (!lock_write(lock)) { // Locking for write
		printf("Error while locking for write...\n");
		return 1;
	}

	hFile = get_input_file_handle(THREADS_FILE_NAME);
	if (NULL == hFile) {
		printf("Error while opening Tasks File. Exit program\n");
		return 1;
	}
	dwFileSize = GetFileSize(hFile, NULL);
	printf("dwFileSize%d\n", dwFileSize);

	if (first)
	{
		printf("first\n");
		start_pos = username_length;
		end_pos = dwFileSize;
		bytes_to_read = end_pos - start_pos;
		printf("start_pos%d\n", start_pos);
		printf("bytes to read%d\n", bytes_to_read);

	}
	else
	{
		printf("second\n");
		start_pos = 0;
		end_pos = dwFileSize - username_length;
		bytes_to_read = end_pos - start_pos;
		printf("start_pos%d\n", start_pos);
		printf("bytes to read%d\n", bytes_to_read);
	}

	for (i = 0; i < bytes_to_read; i++)
	{
		oppenet_username[i] = txt_file_to_str(hFile, start_pos + i, 1, &oppenet_username[i]); // gets pointer to str containing the input text

		if (oppenet_username[i] == NULL)
			return 1;
		// TODO better exit
	}
	oppenet_username[i] = '\0';

	if (close_handles_proper(hFile) != 1) {
		return 1;
	}
	global_read++;
	if (global_read == 1)
	{
		release_write(lock); // Releasing Write lock
		printf("IM WAITING remove\n");
		wait_res = WaitForSingleObject(semaphore_gun, MAX_WAITING_TIME);
		if (wait_res != WAIT_OBJECT_0)
		{
			printf("semaphore_gun WaitForSingleObject timed out\n");
			//strcpy(SendStr, "SERVER_NO_OPPENNTS"); // TODO fix handle no oppenet
			remove(THREADS_FILE_NAME); // in case of timeout deleting the file before writing
		}
		else
			printf("got free\n");
	}
	else if (global_read == 2)
	{
		release_write(lock); // Releasing Write lock
		printf("IM freeing remove\n");
		remove(THREADS_FILE_NAME); // reseting the file before releasing
		release_res = ReleaseSemaphore(semaphore_gun, 1, NULL);
		if (release_res == FALSE)
			return 1;
	}
	global_read = 0;

	return 0;
}
void calc_move_result(char* real_num, char* guess_num, int results[])
{//results[0]=number of bulls in the current move. result[1]=number of cows in the current move.
	int i = 0, j = 0;

	for (i = 0; i < 4; i++)//guess_num_iter
	{
		for (j = 0; j < 4; j++)
		{
			if (guess_num[i] == real_num[j])//real_num_iter
			{
				if (i == j)//BULL
				{
					results[0]++;
					continue;
				}
				results[1]++;//COW

			}
		}

	}
}

int write_input_to_file(int* first,int* no_oppennet, int username_length, char* username, lock* lock, char* SendStr, HANDLE semaphore_gun)
{

	HANDLE oFile = NULL;
	DWORD dwFileSize = 0;
	bool wait_res;
	bool release_res;

	if (!lock_write(lock)) { // Locking for write
		printf("Error while locking for write...\n");
		return 1;
	}

	oFile = create_file_for_write(THREADS_FILE_NAME, 0);
	if (NULL == oFile) {
		printf("Error while opening file to write\n");
		release_write(lock);
		return 1;
	}
	dwFileSize = GetFileSize(oFile, NULL);
	printf("After open file handle to write, it size: %d\n", dwFileSize);
	if (dwFileSize) // Im first
		*first = 0;
	else
		*first = 1;

	write_to_file(username, username_length, oFile, dwFileSize);
	if (close_handles_proper(oFile) != 1)
		return 1;

	if (*first)
	{
		
		printf("IM WAITING after write to file\n");
		release_write(lock); // Releasing Write loc
		wait_res = WaitForSingleObject(semaphore_gun, MAX_WAITING_TIME);
		if (wait_res != WAIT_OBJECT_0)
		{
			printf("semaphore_gun WaitForSingleObject timed out\n");
			strcpy(SendStr, "SERVER_NO_OPPENNTS");
		}
		else
		{
			*no_oppennet = 0;
			printf("got free after write to file\n");
		}
	}
	else
	{
		printf("IM freeing after write to file\n");
		release_write(lock); // Releasing Write lock
		release_res = ReleaseSemaphore(semaphore_gun, 1, NULL);
		if (release_res == FALSE)
			return 1;
		*no_oppennet = 0;
	}
	return 0;
}
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
//Service thread is the thread that opens for each successful client connection and "talks" to the client.

int game_progress(int username_length, char* player_number, char* username, char* oppenet_username, SOCKET* t_socket, lock* lock,HANDLE semaphore_gun)
{
	int round_results[2] = { 0 };
	char SendStr[MSG_MAX_LENG];
	char* msg = NULL;

	TransferResult_t SendRes;
	TransferResult_t RecvRes;


	char* AcceptedStr = NULL;
	char msg_type[MSG_TYPE_MAX_LENG];

	bool wait_res;
	int no_oppennet = 1;

	char params[MAX_PARAM_LENG];
	char oppennet_number[NUM_INPUT_LENGTH];
	char player_curr_guess[NUM_INPUT_LENGTH];
	char oppennet_curr_guess[NUM_INPUT_LENGTH];
	char results_str[5];
	int first = 0;
	int game_on = 1;
	int im_the_winner = 0;
	bool release_res;

	printf("player_number: %s\n", player_number);

	// exchange players real numbers
	if (write_input_to_file(&first, &no_oppennet, NUM_INPUT_LENGTH - 1, player_number, lock, SendStr, semaphore_gun))
		return 1;

	if (!no_oppennet)
	{
		get_oppennet_user_name(first, NUM_INPUT_LENGTH - 1, oppennet_number, lock, semaphore_gun);
		printf("my player_number is: %s ,oppennet_number: %s\n", player_number, oppennet_number);
	}
	remove(THREADS_FILE_NAME);

	printf("game started\n");

	while (game_on)
	{
			round_results[0] =0;
			round_results[1] = 0;
			if (global_round %2)
				printf("\nRound number: %d\n", global_round/2);

			global_round++;
			if (global_round%2!=0)
			{
				printf("IM WAITING for next round\n");
				wait_res = WaitForSingleObject(semaphore_gun, MAX_WAITING_TIME);
				if (wait_res != WAIT_OBJECT_0)
				{
					printf("semaphore_gun WaitForSingleObject timed out\n");
					strcpy(SendStr, "SERVER_NO_OPPENNTS");
				}
				else
					printf("got free for next round\n");
			}
			else
			{
				printf("IM freeing for next round\n");
				release_res = ReleaseSemaphore(semaphore_gun, 1, NULL);
				if (release_res == FALSE)
					return 1;
			}
			if (win == 1 || win == 2)
				break;


			strcpy_s(SendStr, 27, "SERVER_PLAYER_MOVE_REQUEST");

			printf("Sending move request to %s \n", username);
			SendRes = SendString(SendStr, *t_socket);
			if (SendRes == TRNS_FAILED)
			{
				free(AcceptedStr);
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}

			AcceptedStr = NULL;
			printf("waiting for move from: %s \n", username);

			RecvRes = ReceiveString(&AcceptedStr, *t_socket);
			if (Transmit_res(RecvRes, t_socket))
				return 1;

			printf("got a msg from: %s \n", username);
			get_msg_type_and_params(AcceptedStr, &msg_type, &params);
			printf("msg_type is: %s\n", msg_type);
			printf("params: %s\n", params);

			if (STRINGS_ARE_EQUAL(msg_type, "CLIENT_PLAYER_MOVE"))
			{
				strcpy_s(player_curr_guess, 5, params);
				printf("player_curr_guess: %s\n", player_curr_guess);

				no_oppennet = 1;

				if (write_input_to_file(&first, &no_oppennet, NUM_INPUT_LENGTH - 1, player_curr_guess, lock, SendStr, semaphore_gun))
					return 1;

				if (!no_oppennet)
				{
					get_oppennet_user_name(first, NUM_INPUT_LENGTH - 1, oppennet_curr_guess, lock, semaphore_gun);
					printf("my guess is: %s ,oppent guess: %s\n", player_curr_guess, oppennet_curr_guess);
				}

				calc_move_result(oppennet_number, player_curr_guess, round_results);
				printf("my guess is: %s ,oppennet_number: %s\n", player_curr_guess, oppennet_number);
				printf("cows is: %d ,bulls: %d\n", round_results[1], round_results[0]);

				strcpy_s(SendStr, 21, "SERVER_GAME_RESULTS:");

				results_str[0] = round_results[0] + '0';
				results_str[1] = ';';
				results_str[2] = round_results[1] + '0';
				results_str[3] = ';';
				results_str[4] = '\0';

				strcat_s(SendStr, MSG_MAX_LENG, results_str);
				strcat_s(SendStr, MSG_MAX_LENG, oppenet_username);
				strcat_s(SendStr, MSG_MAX_LENG, ";");
				strcat_s(SendStr, MSG_MAX_LENG, oppennet_curr_guess);
				remove(THREADS_FILE_NAME);

				printf("about to send game results to %s\n%s \n", username, SendStr);

				SendRes = SendString(SendStr, *t_socket);
				if (SendRes == TRNS_FAILED)
				{
					free(AcceptedStr);
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
					return 1;
				}
				if (round_results[0] == 4)
				{
					win++;
					im_the_winner = 1;
				}
			}
	}

	strcpy_s(SendStr, 21, "SERVER_WIN:");
	if (win == 1)
	{
		if (im_the_winner == 1)
		{
			strcat_s(SendStr, MSG_MAX_LENG, username);
			strcat_s(SendStr, MSG_MAX_LENG, ";");
			strcat_s(SendStr, MSG_MAX_LENG, player_number);
		}
		else
		{
			strcat_s(SendStr, MSG_MAX_LENG, oppenet_username);
			strcat_s(SendStr, MSG_MAX_LENG, ";");
			strcat_s(SendStr, MSG_MAX_LENG, oppennet_number);
		}

		SendRes = SendString(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED)
		{
			free(AcceptedStr);
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
	}
	else if (win == 2)
	{
		strcpy_s(SendStr, 21, "SERVER_DRAW");
		SendRes = SendString(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED)
		{
			free(AcceptedStr);
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
	}
	return 0;
}

int accept_new_player(SOCKET* t_socket, int* username_length, char** username)
{
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	char SendStr[MSG_MAX_LENG];
	char* AcceptedStr = NULL;
	char msg_type[MSG_TYPE_MAX_LENG];

	RecvRes = ReceiveString(&AcceptedStr, *t_socket);  // get username
	get_msg_type_and_params(AcceptedStr, &msg_type, username);
	*username_length = strlen(username);
	if (Transmit_res(RecvRes, t_socket))
		return 1;

	// check how active users
	printf("trying to connect, current actrive users: %d\n", active_users);
	if (active_users == 2)
	{
		printf("No slots available for client, dropping the connection.\n");
		strcpy_s(SendStr, 27, "SERVER_DENIED:room is full");

		SendRes = SendString(SendStr, *t_socket);

		if (SendRes == TRNS_FAILED)
		{
			printf("Service socket error while writing, closing thread.\n");
		}
		printf("Conversation ended.\n");
		closesocket(*t_socket); //Closing the socket, dropping the connection.
		return 1;
	}
	active_users++;
	strcpy_s(SendStr, 16, "SERVER_APPROVED");
	SendRes = SendString(SendStr, *t_socket);
	printf("my username is: %s\n", username);
	printf("my socket is  : %d\n", *t_socket);
	printf("my msg is  : %s\n", SendStr);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}

	strcpy_s(SendStr, 17, "SERVER_MAIN_MENU");
	printf("my username is: %s\n", username);
	printf("my socket is  : %d\n", *t_socket);
	printf("my msg is4  : %s\n", SendStr);
	SendRes = SendString(SendStr, *t_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}
	return 0;
}

static DWORD ServiceThread(LPVOID lpParam)
{
	int round_results[2] = { 0 };
	int write_res = 0;
	int i = 0; 
	bool release_res;
	printf("thread start.\n");
	char SendStr[MSG_MAX_LENG];
	char* msg = NULL;
	BOOL Done = FALSE;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	int Ind = 0;
	//Sleep(1000);
	HANDLE oFile, hFile;
	char* newline = NULL;
	DWORD dwFileSize = 0;
	int username_length = 0;
	char username[USERNAME_MAX_LENG];
	char oppenet_username[USERNAME_MAX_LENG];
	//const char* oppenet_username;
	char* AcceptedStr = NULL;
	char msg_type[MSG_TYPE_MAX_LENG];
	int first = 0; // if im the first reader 
	SOCKET *t_socket;
	lock* lock;
	thread_args* temp_arg = (thread_args*)lpParam;
	int start_pos = 0;
	int end_pos = 0;
	HANDLE semaphore_gun;
	int bytes_to_read = 0;
	bool wait_res;
	int no_oppennet = 1;
	semaphore_gun = temp_arg->semaphore_gun;
	lock = temp_arg->lock;
	t_socket = temp_arg->socket;
	char params[MAX_PARAM_LENG];
	char player_number[NUM_INPUT_LENGTH];
	char oppennet_number[NUM_INPUT_LENGTH];
	char player_curr_guess[NUM_INPUT_LENGTH];
	char oppennet_curr_guess[NUM_INPUT_LENGTH];
	
	if (accept_new_player(t_socket, &username_length, &username))
		return 1;

	while (!Done)
	{

		AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, *t_socket);

		if (Transmit_res(RecvRes, t_socket))
			return 1;
		else
			printf("Got string : %s\n", AcceptedStr);


		if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_DISCONNECT"))
		{
			printf("CLIENT_DISCONNECT\n");
			break;
		}
		else if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_VERSUS"))
		{
			/// FIND OPPONNET 
			/// OPEN FILE LOCK FOR WRITE, WRITE USER NAME REMEMBER IF FILE
			/// EMPTY, IF IT IS IM FIRST, IF NOT IM SECOND,
			/// WAIT FOR THE OTHER ONE TO WRITE HIS USERNAME
			/// READ OPPENNET USERNAME

			if (write_input_to_file(&first, &no_oppennet, username_length, username, lock, SendStr, semaphore_gun))
				return 1;

			if (!no_oppennet)
			{
				get_oppennet_user_name(first, username_length, oppenet_username, lock, semaphore_gun);
				printf("my username is: %s ,oppent username: %s\n", username, oppenet_username);
				strcpy(SendStr, "SERVER_INVITE:");
				strcat_s(SendStr, MSG_MAX_LENG, oppenet_username);
				SendRes = SendString(SendStr, *t_socket);
				if (SendRes == TRNS_FAILED)
				{
					free(AcceptedStr);
					printf("Service socket error while writing, closing thread.\n");
					closesocket(*t_socket);
					return 1;
				}
				strcpy(SendStr, "SERVER_SETUP_REQUEST");
			}
			//remove(THREADS_FILE_NAME); not sync
		}

		else // fint the unkown 
		{

			get_msg_type_and_params(AcceptedStr, &msg_type, &params);
			printf("params: %s\n", params);
			printf("msg_type is: %s\n", msg_type);

			if (STRINGS_ARE_EQUAL(msg_type, "CLIENT_SETUP"))
			{
				game_progress(username_length, params, username, oppenet_username, t_socket, lock, semaphore_gun);
			}
			else
				printf("%s what?????\n", AcceptedStr);

		}
		no_oppennet = 1; // no need anymore, maybe for another menu

		printf("my username is: %s\n", username);
		printf("my socket is  : %d\n", *t_socket);
		printf("my msg is3  : %s\n", SendStr);

		SendRes = SendString(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED)
		{
			free(AcceptedStr);
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}

		free(AcceptedStr);
	}

	printf("Conversation ended.\n");
	closesocket(*t_socket);
	return 0;
}
//Gets the real number of the opponent - real_num, and the player's guess - guess_num,
//calculate the number of bulls and cows of the player and update the results in  the last argument: results.

// SERVER MAIN
int main(int argc, char* argv[])
{
	thread_args* thread_args[MAX_THREADS];
	SOCKET AcceptedSockets[MAX_THREADS];
	if (remove(THREADS_FILE_NAME) == 0)
		printf("Deleted successfully\n");
	else
		printf("Unable to delete the file\n");

	int server_port = 0;

	int Ind;
	int Loop;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;

	if (init_input_vars(argv, argc, &server_port))
		return 1;

	// Initialize Winsock.
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (StartupRes != NO_ERROR)
	{
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return;
	}

	/* The WinSock DLL is acceptable. Proceed. */

	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
		//TODO CLEAR SERVE
	}


	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		goto server_cleanup_2;
		//TODO CLEAR SERVE

	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(SERVER_PORT); //The htons function converts a u_short from host to TCP/IP network byte order 
									   //( which is big-endian ).
	/*
		The three lines following the declaration of sockaddr_in service are used to set up
		the sockaddr structure:
		AF_INET is the Internet address family.
		"127.0.0.1" is the local IP address to which the socket will be bound.
		2345 is the port number to which the socket will be bound.
	*/

	// Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
		//TODO CLEAR SERVE

	}

	// Listen on the Socket.
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
		//TODO CLEAR SERVE

	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_CLIENTS; Ind++)
		ThreadHandles[Ind] = NULL;

	printf("Waiting for a client to connect...\n");
	lock* lock = InitializeLock();
	if (NULL == lock)
	{
		printf("Unable to init lock.\n");;
		return 1;
	}

	HANDLE semaphore_gun = NULL;
	semaphore_gun = CreateSemaphore(NULL, 0, 1, NULL);  // creats a semphore for paralllel threads func
	if (NULL == semaphore_gun)
	{
		return 1;
	}

	for (Loop = 0; Loop < 4; Loop++)
	{
		Ind = FindFirstUnusedThreadSlot(semaphore_gun, thread_args); // clean threads that are finished

		AcceptedSockets[Ind] = accept(MainSocket, NULL, NULL);
		if (AcceptedSockets[Ind] == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			goto server_cleanup_3;
			//TODO CLEAR SERVE
		}

		printf("index: %d", Ind);
		thread_args[Ind] = create_thread_arg(&AcceptedSockets[Ind], lock, semaphore_gun);
		
		if (NULL == thread_args[Ind])
		{
			DestroyLock(lock);
			return 1;
		}

		printf("Client Connected.\n");

		ThreadInputs[Ind] = AcceptedSockets[Ind]; // shallow copy: don't close 
											// AcceptSocket, instead close 
											// ThreadInputs[Ind] when the
											// time comes.
		ThreadHandles[Ind] = CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)ServiceThread,
			(thread_args[Ind]),
			0,
			NULL
		);
	} // for ( Loop = 0; Loop < MAX_LOOPS; Loop++ )

server_cleanup_3:
	CleanupWorkerThreads();

server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR)
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());

server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR)
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
}
