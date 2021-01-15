
/*
Authors – Matan Achiel - 205642119, Sagi Levi - 205663545
Project – Ex4 - Server - main.
Description – This program is the main program - main.c
gets a 1 argument - Server port number.
It validiates the input args, open socket for each client, and handle the game logic for the clients,
rejects third client.
by creating a unique socket and thread to represent it.
*/


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
#include <conio.h> 

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

HANDLE thread_handles[MAX_THREADS];
SOCKET thread_inputs[MAX_THREADS];
int active_users = 0;
int win = 0;
int global_round = 0;
int global_read = 0;
int about_to_close = 0;
int server_up = 1;
SOCKET main_socket = INVALID_SOCKET;

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

	if (*input_args[1] == '0') //
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

static int find_unused_thread_ind(thread_args** thread_args)
{
	int Ind;
	bool release_res;

	for (Ind = 0; Ind < NUM_OF_CLIENTS; Ind++)
	{
		if (thread_handles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(thread_handles[Ind], 0);
			//DWORD Res = 1;

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(thread_handles[Ind]);
				thread_handles[Ind] = NULL;
				free(*(thread_args + Ind));
				thread_args[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}

static void clean_working_threads()
{
	int Ind;
	about_to_close = 1;
	for (Ind = 0; Ind <= NUM_OF_CLIENTS; Ind++)
	{
		if (thread_handles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(thread_handles[Ind], WAIT_FOR_THREAD_TIME);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(thread_inputs[Ind]);
				CloseHandle(thread_handles[Ind]);
				thread_handles[Ind] = NULL;
				break;
			}
			else
			{
				printf("Waiting for thread failed. Teminating Thread\n");
				TerminateThread(thread_handles[Ind], BRUTAL_TERMINATION_CODE);
			}
		}
	}
	server_up = 0;
	closesocket(main_socket);
}

int get_oppenet_info(int first, int client_input_length, char* oppenet_input, lock* lock, HANDLE semaphore_gun)
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
		start_pos = client_input_length;
		end_pos = dwFileSize;
		bytes_to_read = end_pos - start_pos;
		printf("start_pos%d\n", start_pos);
		printf("bytes to read%d\n", bytes_to_read);

	}
	else
	{
		printf("second\n");
		start_pos = 0;
		end_pos = dwFileSize - client_input_length;
		bytes_to_read = end_pos - start_pos;
		printf("start_pos%d\n", start_pos);
		printf("bytes to read%d\n", bytes_to_read);
	}

	for (i = 0; i < bytes_to_read; i++)
	{
		oppenet_input[i] = txt_file_to_str(hFile, start_pos + i, 1, &oppenet_input[i]); // gets pointer to str containing the input text

		if (oppenet_input[i] == NULL)
			return 1;
	}
	oppenet_input[i] = '\0';

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

int write_input_to_file(int* first, int* no_oppennet, int input_length, char* client_input, lock* lock, char* SendStr, HANDLE semaphore_gun)
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

	write_to_file(client_input, input_length, oFile, dwFileSize);
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
			remove(THREADS_FILE_NAME);
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

int game_progress(int username_length, char* player_number, char* username, char* oppenet_username, SOCKET* t_socket, lock* lock, HANDLE semaphore_gun){
	int round_results[2] = { 0 }, no_oppennet = 1, first = 0, game_on = 1, im_the_winner = 0;
	char SendStr[MSG_MAX_LENG], msg_type[MSG_TYPE_MAX_LENG], params[MAX_PARAM_LENG], oppennet_number[NUM_INPUT_LENGTH], player_curr_guess[NUM_INPUT_LENGTH], oppennet_curr_guess[NUM_INPUT_LENGTH], results_str[5];
	char *msg = NULL, *AcceptedStr = NULL;
	TransferResult_t SendRes, RecvRes; 
	bool wait_res, release_res;
	if (write_input_to_file(&first, &no_oppennet, NUM_INPUT_LENGTH - 1, player_number, lock, SendStr, semaphore_gun)) 
		return 1;
	if (!no_oppennet){
		if (get_oppenet_info(first, NUM_INPUT_LENGTH - 1, oppennet_number, lock, semaphore_gun)) // Getting eachother number
			return 1;
	}
	win = 0;
	while (game_on){ // game started
		round_results[0] = 0; round_results[1] = 0;
		global_round++;
		if (global_round % 2 != 0){
			wait_res = WaitForSingleObject(semaphore_gun, MAX_WAITING_TIME); // WAITING for next round
			if (wait_res != WAIT_OBJECT_0){
				printf("semaphore_gun WaitForSingleObject timed out\n");
				return 1;
			}} // else got free for next round
		else if (ReleaseSemaphore(semaphore_gun, 1, NULL) == FALSE)
				return 1;
		if (win == 1 || win == 2)
			break;
		strcpy_s(SendStr, 27, "SERVER_PLAYER_MOVE_REQUEST"); // Sending move request to username
		SendRes = send_string(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED){
			free(AcceptedStr);
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		AcceptedStr = NULL;
		RecvRes = receive_string(&AcceptedStr, *t_socket); // waiting for move from username
		if (transmit_res(RecvRes, t_socket))
			return 1;
		get_msg_type_and_params(AcceptedStr, &msg_type, &params);
		if (STRINGS_ARE_EQUAL(msg_type, "CLIENT_PLAYER_MOVE")){
			strcpy_s(player_curr_guess, 5, params); // getting player_curr_guess 
			no_oppennet = 1;
			if (write_input_to_file(&first, &no_oppennet, NUM_INPUT_LENGTH - 1, player_curr_guess, lock, SendStr, semaphore_gun))
				return 1;
			if (!no_oppennet){ // reading eachother gueses
				if (get_oppenet_info(first, NUM_INPUT_LENGTH - 1, oppennet_curr_guess, lock, semaphore_gun))
					return 1;
			}
			if (active_users == 1)
				return 1;
			calc_move_result(oppennet_number, player_curr_guess, round_results); // getting round results
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
			SendRes = send_string(SendStr, *t_socket); // send game results to username
			if (SendRes == TRNS_FAILED){
				free(AcceptedStr);
				printf("Service socket error while writing, closing thread.\n");
				closesocket(*t_socket);
				return 1;
			}
			if (round_results[0] == 4){
				win++;
				im_the_winner = 1;
			}}}
	strcpy_s(SendStr, 21, "SERVER_WIN:");
	if (win == 1){
		if (im_the_winner == 1)
			strcat_s(SendStr, MSG_MAX_LENG, username);
		else
			strcat_s(SendStr, MSG_MAX_LENG, oppenet_username);
		strcat_s(SendStr, MSG_MAX_LENG, ";");
		strcat_s(SendStr, MSG_MAX_LENG, oppennet_number);
		SendRes = send_string(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED){
			free(AcceptedStr);
			printf("Service socket error while writing, closing thread..\n");
			closesocket(*t_socket);
			return 1;
		}}
	else if (win == 2){
		strcpy_s(SendStr, 21, "SERVER_DRAW");
		SendRes = send_string(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED){
			free(AcceptedStr);
			printf("Service socket error while writing, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}}
	return 0;
}

int accept_new_player(SOCKET* t_socket, int* username_length, char** username)
{
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	char SendStr[MSG_MAX_LENG];
	char* AcceptedStr = NULL;
	char msg_type[MSG_TYPE_MAX_LENG];

	RecvRes = receive_string(&AcceptedStr, *t_socket);  // get username
	get_msg_type_and_params(AcceptedStr, &msg_type, username);
	if (!STRINGS_ARE_EQUAL(msg_type, "CLIENT_REQUEST"))
		return 1;
	*username_length = strlen(username);
	if (transmit_res(RecvRes, t_socket))
		return 1;

	// check how many active users
	printf("trying to connect, current actrive users: %d\n", active_users);
	if (active_users == 2)
	{
		printf("No slots available for client, dropping the connection.\n");
		strcpy_s(SendStr, 27, "SERVER_DENIED:room is full");

		SendRes = send_string(SendStr, *t_socket);

		if (SendRes == TRNS_FAILED)
			printf("Service socket error while writing, closing thread.\n");
		return 1;
	}
	active_users++;
	strcpy_s(SendStr, 16, "SERVER_APPROVED");
	SendRes = send_string(SendStr, *t_socket);
	printf("my username is: %s\n", username);
	printf("my socket is  : %d\n", *t_socket);
	printf("my msg is  : %s\n", SendStr);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		active_users--;
		return 1;
	}

	strcpy_s(SendStr, 17, "SERVER_MAIN_MENU");
	printf("my username is: %s\n", username);
	printf("my socket is  : %d\n", *t_socket);
	printf("my msg is4  : %s\n", SendStr);
	SendRes = send_string(SendStr, *t_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		active_users--;
		return 1;
	}
	if (set_socket_timeout(INFINITE, *t_socket))
	{
		printf("set_socket_timeout INFINITE\n");

		closesocket(*t_socket); //Closing the socket, dropping the connection.
		return 1;
	}
	return 0;
}

static DWORD client_thread(LPVOID lp_param)
{
	int round_results[2] = { 0 };
	int write_res = 0, i = 0, Ind = 0, username_length = 0, start_pos = 0, end_pos = 0, bytes_to_read = 0, no_oppennet = 1 , first = 0;
	bool release_res, error_indicator = 0, Done = FALSE, wait_res;
	printf("thread start.\n");
	char SendStr[MSG_MAX_LENG], * msg = NULL, * newline = NULL, username[USERNAME_MAX_LENG], oppenet_username[USERNAME_MAX_LENG], * AcceptedStr = NULL, exit_string[5];
	char msg_type[MSG_TYPE_MAX_LENG], params[MAX_PARAM_LENG], player_number[NUM_INPUT_LENGTH], oppennet_number[NUM_INPUT_LENGTH], player_curr_guess[NUM_INPUT_LENGTH], oppennet_curr_guess[NUM_INPUT_LENGTH];
	TransferResult_t SendRes, RecvRes;
	HANDLE oFile, hFile, semaphore_gun;
	DWORD dwFileSize = 0;
	SOCKET* t_socket;
	lock* lock;
	thread_args* temp_arg = (thread_args*)lp_param;
	semaphore_gun = temp_arg->semaphore_gun;
	lock = temp_arg->lock;
	t_socket = temp_arg->socket;
	if (set_socket_timeout(SERVER_TIMEOUT, *t_socket)){
		printf("set_socket_timeout SERVER_TIMEOUT\n");
		closesocket(*t_socket); //Closing the socket, dropping the connection.
		return 1;
	}
	if (accept_new_player(t_socket, &username_length, &username)){
		printf("Conversation ended.\n");
		closesocket(*t_socket); //Closing the socket, dropping the connection.
		return 1;
	}
	while (!Done){
		if (about_to_close)
			break;
		AcceptedStr = NULL;
		printf("Waiting for new input\n");
		RecvRes = receive_string(&AcceptedStr, *t_socket);
		if (transmit_res(RecvRes, t_socket)) {
			error_indicator = 1;
			break;
		}
		else
			printf("Got string : %s\n", AcceptedStr);
		if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_DISCONNECT")){
			printf("CLIENT_DISCONNECT\n"); // DELETE
			break;
		}
		else if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_VERSUS")){
			if (write_input_to_file(&first, &no_oppennet, username_length, username, lock, SendStr, semaphore_gun)){
				error_indicator = 1;
				break;
			}
			if (!no_oppennet){
				if (get_oppenet_info(first, username_length, oppenet_username, lock, semaphore_gun)){
					error_indicator = 1;
					break;
				}
				strcpy_s(SendStr, 15 ,"SERVER_INVITE:");
				strcat_s(SendStr, MSG_MAX_LENG, oppenet_username);
				SendRes = send_string(SendStr, *t_socket);
				if (SendRes == TRNS_FAILED){
					printf("Service socket error while writing, closing thread.\n");
					error_indicator = 1;
					break;
				}
				strcpy_s(SendStr, 21 ,"SERVER_SETUP_REQUEST");
			}}
		else {
			get_msg_type_and_params(AcceptedStr, &msg_type, &params);
			if (STRINGS_ARE_EQUAL(msg_type, "CLIENT_SETUP")){
				if (game_progress(username_length, params, username, oppenet_username, t_socket, lock, semaphore_gun)){
					if (active_users == 1){
						strcpy_s(SendStr, 21 ,"SERVER_OPPONENT_QUIT");
						SendRes = send_string(SendStr, *t_socket);
						if (SendRes == TRNS_FAILED){
							printf("Service socket error while writing, closing thread.\n");
							error_indicator = 1;
							break;
						}}}
				strcpy_s(SendStr, 17, "SERVER_MAIN_MENU");
				if (set_socket_timeout(INFINITE, *t_socket)){
					closesocket(*t_socket); //Closing the socket, dropping the connection.
					error_indicator = 1;
					break;
				}}
			else
				printf("Unreconize messege: %s\n", AcceptedStr);
		}
		no_oppennet = 1; // no need anymore, maybe for another menu
		SendRes = send_string(SendStr, *t_socket);
		if (SendRes == TRNS_FAILED){
			printf("Service socket error while writing, closing thread.\n");
			error_indicator = 1;
			break;
		}}
	free(AcceptedStr);
	printf("Conversation ended.\n");
	active_users--;
	closesocket(*t_socket);
	return 0;
}

static DWORD polling_thread()
{
	char exit_word[EXIT_WORD_LENGTH + 1];
	while (1) {
		if (_kbhit() != 0) {
			scanf_s(" %s", exit_word, EXIT_WORD_LENGTH + 1);
			if (STRINGS_ARE_EQUAL(exit_word, EXIT_WORD))
			{
				printf("exiting...\n");
				break;
			}
		}
		else
			Sleep(POLLING_TIME);
		}
	clean_working_threads();
	return 0;
}

int clean_main_socket()
{
	if (closesocket(main_socket) == SOCKET_ERROR)
	{
		if (server_up)
			printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
	}
	if (WSACleanup() == SOCKET_ERROR)
	{
		if (server_up)
			printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
	}
	if (!server_up)
		return 0;
	return 1;
}

int main(int argc, char* argv[])
{
	thread_args* thread_args[MAX_THREADS];
	SOCKET accepted_sockets[MAX_THREADS];
	int server_port = 0, ind, loop, bind_res,listen_res, startup_res;
	unsigned long address;
	SOCKADDR_IN service;
	WSADATA wsa_data;
	HANDLE semaphore_gun = NULL;
	lock* lock = NULL;
	if (remove(THREADS_FILE_NAME) == 0) // reseting the game file
		printf("Deleted successfully\n");
	else
		printf("Unable to delete the file\n");
	HANDLE poll_thread = NULL;
	poll_thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)polling_thread,NULL,0,NULL);
	if (init_input_vars(argv, argc, &server_port))
		return 1;
	startup_res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (startup_res != NO_ERROR){ // Tell the user that we could not find a usable WinSock DLL.                              
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());     
		return;
	}
	main_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (main_socket == INVALID_SOCKET){
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		if (WSACleanup() == SOCKET_ERROR)
			printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		return 1;
	}
	address = inet_addr(SERVER_ADDRESS_STR);
	if (address == INADDR_NONE){
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		if (closesocket(main_socket) == SOCKET_ERROR)
			printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
		if (WSACleanup() == SOCKET_ERROR)
			printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		return 1;
	}
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = address;
	service.sin_port = htons(SERVER_PORT); //The htons function converts a u_short from host to TCP/IP network byte order 
									   //( which is big-endian ).
	bind_res = bind(main_socket, (SOCKADDR*)&service, sizeof(service));
	if (bind_res == SOCKET_ERROR){
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		if (WSACleanup() == SOCKET_ERROR)
			printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		return 1;
	}
	listen_res = listen(main_socket, SOMAXCONN);
	if (listen_res == SOCKET_ERROR){
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		if (clean_main_socket())
			return 1;
		return 0;
	}
	for (ind = 0; ind < NUM_OF_CLIENTS; ind++)
		thread_handles[ind] = NULL;
	printf("Waiting for a client to connect...\n");
	lock = InitializeLock();
	if (NULL == lock){
		printf("Unable to init lock.\n");;
		return 1;
	}
	semaphore_gun = CreateSemaphore(NULL, 0, 1, NULL);  // creats a semphore for paralllel threads func
	if (NULL == semaphore_gun)
		return 1;
	while(server_up){ // while(server up) -- accept new clients
		ind = find_unused_thread_ind(thread_args); // clean threads that are finished
		accepted_sockets[ind] = accept(main_socket, NULL, NULL);
		if (accepted_sockets[ind] == INVALID_SOCKET){
			if (server_up)
				printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			DestroyLock(lock);
			clean_working_threads();
			if (clean_main_socket())
				return 1;
			return 0;
		}
		thread_args[ind] = create_thread_arg(&accepted_sockets[ind], lock, semaphore_gun);
		if (NULL == thread_args[ind]){
			DestroyLock(lock);
			clean_working_threads();
			if (clean_main_socket())
				return 1;
			return 0;
		}
		printf("Client Connected.\n");
		thread_inputs[ind] = accepted_sockets[ind]; // shallow copy: don't close 
											// AcceptSocket, instead close 
											// ThreadInputs[Ind] when the
											// time comes.
		thread_handles[ind] = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)client_thread,(thread_args[ind]),0,NULL);
	} 
	return 0;
}
