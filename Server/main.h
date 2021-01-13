
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <math.h>
#include "Lock.h"
#pragma comment(lib, "Ws2_32.lib")


#ifndef MAIN_H_
#define MAIN_H_
//Structs..................................................
typedef struct thread_arguments {
	SOCKET* socket;
	lock* lock;
	HANDLE semaphore_gun;
} thread_args;
#endif

//Declerations.............................................

//Gets a pointer to the client socket: socket, and lock for multithreading tasks handling: lock
//it inits a sturct that characterize a thread, with all the inpput arguments, and return a pointer to it.
thread_args* create_thread_arg(SOCKET* socket, lock* lock, HANDLE semaphore_gun);

//Gets all the input arguments - input_args, their amount - num_of_args, pointers to the server's port 
//validiate that all the input 
//and initiate the pointer with the data from the first argument.
//returns 0 for success or 1 else.
int init_input_vars(char* input_args[], int num_of_args, int* server_port);

//Gets the allocated variable for the thread argument - thread_args, 
// return the available index of ThreadHandles to be occupy
// beside, find if there is a thread who finished to work, closes its handle then free its alocated arg.
static int find_unused_thread_ind(thread_args** thread_args);

//In case the procceses about to end, this function closes each working thread
// indicating by a global varaible about_to_close, and gives each thread 15 sec to close
// before termintaing them.
static void clean_working_threads();

// Read for each client its oppenet info, been used in any stage of the game
// getting each other username or guesses, get argument who indicate who wrote first
// an argument who says how much this client had wrote - client_input_length.
// a lock - lock, and a semaphore_gun for timing the reading part and resetiing the file in the end of the function
int get_oppenet_info(int first, int username_length, char* oppenet_username, lock* lock, HANDLE semaphore_gun);

//Gets all the clients guesses for one round - real_num, guess_num, and a pointer to 
// results array - results, and updates the result for this guess.
void calc_move_result(char* real_num, char* guess_num, int results[]);

// write for each client its info, been used in any stage of the game
// getting each other username or guesses, gets a pointer argument who indicate who wrote first - first
// an argument who says how much this client had wrote - client_input_length, a pointer to indicate if 
// there was a timeout for waiting to another oppent meaning no opennet - no_oppennet. and an argument for sending a msg in that case - SendStr
// a lock - lock, and a semaphore_gun for timing the writing part.
//returns 0 for success or 1 else.
int write_input_to_file(int* first, int* no_oppennet, int client_input_length, char* client_input, lock* lock, char* SendStr, HANDLE semaphore_gun);

// after starting a game beetwen two clients, this function handle each round untill a winner or a tie
// or timeout has occured
// getting username length, gets a pointer to this client number - player_number, pointer to client username -username
// his oppent username - oppenet_username, pointer to this client socket - t_socket, pointer to lock and semphore for timing - lock, semaphore_gun
//returns 0 for success or 1 else.
int game_progress(int username_length, char* player_number, char* username, char* oppenet_username, SOCKET* t_socket, lock* lock, HANDLE semaphore_gun);

// when a new thread for the client oppended, this function handling in getting the client username
// and approiving him
// getting a pointer to username length - username_length , gets a pointer to username - username, 
// and updates them with the client msg.
//returns 0 for success or 1 else.
int accept_new_player(SOCKET* t_socket, int* username_length, char** username);

//Service thread is the thread that opens for each successful client connection and "talks" to the client.
//opens for each sucsseful connection, get the ponter to params - lpParam that points to thread_arguments
static DWORD client_thread(LPVOID lpParam);
//Gets the real number of the opponent - real_num, and the player's guess - guess_num,
//calculate the number of bulls and cows of the player and update the results in  the last argument: results.


//Main thread that gets a server port, and opens the connection, and thread for each client it accepts
//returns 0 for success or 1 else.
int main(int argc, char* argv[]);
