
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

//Gets the path of an input file: mission_file, a pointer to a pueue: queue, and lock for multithreading tasks handling: lock
//it inits a sturct that characterize a thread, with all the inpput arguments, and return a pointer to it.
thread_args* create_thread_arg(char* mission_file, lock* lock);