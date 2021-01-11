#ifndef SOCKET_SEND_RECV_TOOLS_H
#define SOCKET_SEND_RECV_TOOLS_H

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")


typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;

/**
 * send_buffer() uses a socket to send a buffer.
 *
 * Accepts:
 * -------
 * Buffer - the buffer containing the data to be sent.
 * BytesToSend - the number of bytes from the Buffer to send.
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if sending succeeded
 * TRNS_FAILED - otherwise
 */
TransferResult_t send_buffer( const char* buffer, int bytes_to_send, SOCKET sd );

/**
 * send_string() uses a socket to send a string.
 * str - the string to send. 
 * sd - the socket used for communication.
 */ 
TransferResult_t send_string( const char *str, SOCKET sd );

/**
 * Accepts:
 * -------
 * receive_buffer() uses a socket to receive a buffer.
 * output_buffer - pointer to a buffer into which data will be written
 * output_buffer_size - size in bytes of Output Buffer
 * bytes_received_ptr - output parameter. if function returns TRNS_SUCCEEDED, then this 
 *					  will point at an int containing the number of bytes received.
 * sd - the socket used for communication.
 *
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */ 
TransferResult_t receive_buffer( char* output_buffer, int remaining_bytes_to_receive, SOCKET sd );

/**
 * receive_string() uses a socket to receive a string, and stores it in dynamic memory.
 * 
 * Accepts:
 * -------
 * output_str_ptr - a pointer to a char-pointer that is initialized to NULL, as in:
 *
 *		char *buffer = NULL;
 *		receive_string( &buffer, ___ );
 *
 * a dynamically allocated string will be created, and (*output_str_ptr) will point to it.
 * 
 * sd - the socket used for communication.
 * 
 * Returns:
 * -------
 * TRNS_SUCCEEDED - if receiving and memory allocation succeeded
 * TRNS_DISCONNECTED - if the socket was disconnected
 * TRNS_FAILED - otherwise
 */ 
TransferResult_t receive_string( char** output_str_ptr, SOCKET sd );

//Gets an enum msg_res which contains the message transmission's status - failure/ disconnection/ succsess and a pointer to the accept socket
//that is assign for a client in the server - t_socket. It handles errors and closing resources if needed and returns 1 in those cases.
// else returns 0.
int transmit_res(TransferResult_t msg_res, SOCKET* t_socket);

//Gets a pointer to a message (string) - msg, and parse it into the other (empty) arguments: the message's type to msg_type
// and the message's parameters to param.
int msg_creator(int size, char** msg, char* msg_type, char* param);

//Gets a timeout value - timeout, and a SOCKET - sd, and set it's timeout according to the first argument.
//returns 1 for a failure or 0 else.
int set_socket_timeout(DWORD timeout, SOCKET sd);

//Gets a SOCKET sd, the handles closing it's resources - closing it and verify it was a valid proccess. Then clean WSA resources.
void disconnect_socket(SOCKET* sd);

//Gets an input message - input_msg, message type - msg_type, and the message parameters - params
void get_msg_type_and_params(char* input_msg, char* msg_type, char* params);

#endif // SOCKET_SEND_RECV_TOOLS_H