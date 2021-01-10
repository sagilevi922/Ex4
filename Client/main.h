
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

