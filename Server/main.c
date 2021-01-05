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

#define MSG_MAX_LENG 100

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

HANDLE ThreadHandles[MAX_THREADS];
SOCKET ThreadInputs[MAX_THREADS];
int active_users = 0;

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

static int FindFirstUnusedThreadSlot(HANDLE semaphore_gun)
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

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
//Service thread is the thread that opens for each successful client connection and "talks" to the client.
static DWORD ServiceThread(LPVOID lpParam)
{
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
	char* AcceptedStr = NULL;
	char msg_type[MSG_TYPE_MAX_LENG];
	int first = 0; // if im the first reader 
	SOCKET *t_socket;
	lock* lock;
	thread_args* temp_arg = (thread_args*)lpParam;
	int start_pos = 0;
	int end_pos = 0;
	t_socket = temp_arg->socket;
	lock = temp_arg->lock;
	RecvRes = ReceiveString(&AcceptedStr, *t_socket);  // get username
	get_msg_type_and_params(AcceptedStr, &msg_type, &username);
	printf("username is : %s\n", username);
	printf("msg_type is : %s\n", msg_type);
	username_length = strlen(username);

	HANDLE semaphore_gun;
	bool wait_res;
	semaphore_gun = temp_arg->semaphore_gun;


	if (RecvRes == TRNS_FAILED)
	{
		printf("Service socket error while reading, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}
	else if (RecvRes == TRNS_DISCONNECTED)
	{
		printf("Connection closed while reading, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}
	else
	{
		printf("Got string : %s\n", AcceptedStr);
	}
	
	// check how active users
	printf("trying to connect, current actrive users: %d\n", active_users);
	if (active_users==2)
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

	strcpy_s(SendStr,16, "SERVER_APPROVED");
	SendRes = SendString(SendStr, *t_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}

	strcpy_s(SendStr, 17, "SERVER_MAIN_MENU");

	SendRes = SendString(SendStr, *t_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}

	while (!Done)
	{
		AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, *t_socket);

		if (RecvRes == TRNS_FAILED)
		{
			printf("Service socket error while reading, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Connection closed while reading, closing thread.\n");
			closesocket(*t_socket);
			return 1;
		}
		else
			printf("Got string : %s\n", AcceptedStr);


		if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_DISCONNECT"))
		{
			break;
		}
		else if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_VERSUS"))
		{
			/// FIND OPPONNET 
			/// OPEN FILE LOCK FOR WRITE, WRITE USER NAME REMEMBER IF FILE
			/// EMPTY, IF IT IS IM FIRST, IF NOT IM SECOND,
			/// WAIT FOR THE OTHER ONE TO WRITE HIS USERNAME
			/// READ OPPENNET USERNAME

			if (!lock_write(lock)) { // Locking for write
				printf("Error while locking for write...");
				return 1;
			}

			oFile = create_file_for_write(THREADS_FILE_NAME, 0);
			if (NULL == oFile) {
				printf("Error while opening file to write\n");
				release_write(lock);
				free(newline);
				return 1;
			}
			dwFileSize = GetFileSize(oFile, NULL);
			printf("dwFileSize is: %d", dwFileSize);
			if (dwFileSize) // Im first
				first = 0;
			else
				first = 1;
			
			write_to_file(username, username_length, oFile, dwFileSize);
			if (close_handles_proper(oFile) != 1)
				return 1;
			release_write(lock); // Releasing Write lock
			if (first)
			{
				printf("IM WAITING\n");
				wait_res = WaitForSingleObject(semaphore_gun, MAX_WAITING_TIME);
				if (wait_res != WAIT_OBJECT_0)
					return 1;
				printf("got free\n");
			}
			else
			{
				printf("IM freeing\n");
				release_res = ReleaseSemaphore(semaphore_gun, 1, NULL);
				if (release_res == FALSE)
					return 1;
			}


			if (!lock_read(lock)) { // Locking Read lock
				printf("Error while Locking for read...");
				continue;
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
				end_pos = end_pos - start_pos;
				printf("start_pos%d\n", start_pos);
				printf("bytes to read%d\n", end_pos);

			}
			else
			{
				printf("second\n");

				start_pos = 0;
				end_pos = dwFileSize - username_length;
				end_pos = end_pos-start_pos;
				printf("start_pos%d\n", start_pos);
				printf("bytes to read%d\n", end_pos);

			}

			//TODO FIX READFILE
			txt_file_to_str(hFile, start_pos, end_pos, &oppenet_username);


			if (close_handles_proper(hFile) != 1) {
				release_read(lock);
				return 1;
			}
			if (!release_read(lock)) { // Releasing Read lock
				printf("Error while release lock for read...");
				if (close_handles_proper(hFile) != 1)
					return 1;
				continue;
			}

			strcpy(SendStr, "SERVER_INVITE:");
			strcat_s(SendStr, MSG_MAX_LENG, username);
		}

		//	strcpy(SendStr, "SERVER_INVITE:");
		//	strcat_s(SendStr, MSG_MAX_LENG, "Oppenet name");
		//}
		else // fint the unkown 
		{
			//get_msg_type_and_params(AcceptedStr, &msg_type, &username);
			//printf("username is : %s", username);
			//printf("msg_type is : %s", msg_type);
			free(AcceptedStr);
			continue;
		}
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

// SERVER MAIN
int main(int argc, char* argv[])
{
	int server_port = 0;

	int Ind;
	int Loop;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	thread_args* thread_arg=NULL;
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
		SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			goto server_cleanup_3;
			//TODO CLEAR SERVE
		}
		thread_arg= create_thread_arg(&AcceptSocket, lock, semaphore_gun);
		if (NULL == thread_arg)
		{
			printf("Unable to init queue.\n");
			DestroyLock(lock);
			return 1;
		}
		Ind = FindFirstUnusedThreadSlot(semaphore_gun); // clean threads that are finished

		printf("Client Connected.\n");

		ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
											// AcceptSocket, instead close 
											// ThreadInputs[Ind] when the
											// time comes.
		ThreadHandles[Ind] = CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)ServiceThread,
			(thread_arg),
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
