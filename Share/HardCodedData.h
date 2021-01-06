#pragma once
//#ifndef HARD_CODED_DATA_H
//#define HARD_CODED_DATA_H
//
///*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
//
//#define SERVER_ADDRESS_STR "127.0.0.1"
//
//#define SERVER_PORT 2345
//
//#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )
//
///*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
//
//#endif // HARD_CODED_DATA_H
//	

#define ERROR_CODE ((int)(-1))
#define THREADS_LIMIT 64
#define MAX_WAITING_TIME 100000 //15000
#define CLIENT_TIMEOUT 15000  //15000
#define VERSUS_TIMEOUT CLIENT_TIMEOUT*2 //30000
#define BRUTAL_TERMINATION_CODE 0x55
#define MAX_PARAM_LENG 50 // FIX TO MAX SIZE
#define MSG_TYPE_MAX_LENG 30
#define THREADS_FILE_NAME "INIT_GAME.txt"
#define NUM_INPUT_LENGTH 5
#define MSG_MAX_LENG 100
