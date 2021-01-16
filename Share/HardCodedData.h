//A header that includes all the constants for the solution: both server and client sides.
#pragma once
//#ifndef HARD_CODED_DATA_H
//#define HARD_CODED_DATA_H

//#endif // HARD_CODED_DATA_H
//	

#define ERROR_CODE ((int)(-1))
#define THREADS_LIMIT 64
#define MAX_WAITING_TIME 15000 //15000 -- for semphpore timing
#define SERVER_TIMEOUT 15000 //15000
#define WAIT_FOR_THREAD_TIME 15000 //15000
#define NUM_OF_CLIENTS 2
#define MAX_THREADS 3
#define MAX_LOOPS 3
#define CLIENT_TIMEOUT 15000  //15000
#define VERSUS_TIMEOUT CLIENT_TIMEOUT*2 //30000
#define BRUTAL_TERMINATION_CODE 0x55
#define MAX_PARAM_LENG 50 // FIX TO MAX SIZE
#define MSG_TYPE_MAX_LENG 30
#define THREADS_FILE_NAME "GameSession.txt"
#define NUM_INPUT_LENGTH 5
#define MSG_MAX_LENG 100
#define EXIT_WORD_LENGTH 4
#define EXIT_WORD "exit"
#define POLLING_TIME 1000

#define SERVER_ADDRESS_MAX_LENG 16
#define USERNAME_MAX_LENG 21
#define SERVER_ADDRESS_STR "127.0.0.1"
#define STRINGS_ARE_EQUAL( str1, str2 ) ( strcmp( (str1), (str2) ) == 0 )

