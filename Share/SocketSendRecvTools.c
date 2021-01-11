// SocketSendRecvTools.c
/*
Authors – Matan Achiel - 205642119, Sagi Levi - 205663545
Project – Ex4 - Server.
Description – This moudule is for handling the messages' transformation between the server and the client
setting socket timeout and disconnect socket in a failure
*/
#define _CRT_SECURE_NO_WARNINGS

#include "SocketSendRecvTools.h"

#include <stdio.h>
#include <string.h>

TransferResult_t send_buffer( const char* buffer, int bytes_to_send, SOCKET sd )
{
	const char* cur_place_ptr = buffer;
	int bytes_transferred;
	int remaining_bytes_to_send = bytes_to_send;
	
	while ( remaining_bytes_to_send > 0 )  
	{
		/* send does not guarantee that the entire message is sent */
		bytes_transferred = send (sd, cur_place_ptr, remaining_bytes_to_send, 0);
		if ( bytes_transferred == SOCKET_ERROR ) 
		{
			printf("send() failed, error %d\n", WSAGetLastError() );
			return TRNS_FAILED;
		}
		
		remaining_bytes_to_send -= bytes_transferred;
		cur_place_ptr += bytes_transferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

TransferResult_t send_string( const char *Str, SOCKET sd )
{
	/* Send the the request to the server on socket sd */
	int total_string_size_in_bytes;
	TransferResult_t send_res;

	/* The request is sent in two parts. First the Length of the string (stored in 
	   an int variable ), then the string itself. */
		
	total_string_size_in_bytes = (int)( strlen(Str) + 1 ); // terminating zero also sent	

	send_res = send_buffer( 
		(const char *)( &total_string_size_in_bytes ),
		(int)( sizeof(total_string_size_in_bytes) ), // sizeof(int) 
		sd );

	if ( send_res != TRNS_SUCCEEDED ) return send_res ;

	send_res = send_buffer( 
		(const char *)( Str ),
		(int)( total_string_size_in_bytes ), 
		sd );

	return send_res;
}

TransferResult_t receive_buffer( char* output_buffer, int bytes_to_receive, SOCKET sd )
{
	char* cur_place_ptr = output_buffer;
	int bytes_just_transferred;
	int remaining_bytes_to_receive = bytes_to_receive;
	
	while ( remaining_bytes_to_receive > 0 )  
	{
		/* send does not guarantee that the entire message is sent */
		bytes_just_transferred = recv(sd, cur_place_ptr, remaining_bytes_to_receive, 0);
		if ( bytes_just_transferred == SOCKET_ERROR ) 
		{
			//printf("recv() failed, error %d\n", WSAGetLastError() );
			return TRNS_FAILED;
		}		
		else if ( bytes_just_transferred == 0 )
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		remaining_bytes_to_receive -= bytes_just_transferred;
		cur_place_ptr += bytes_just_transferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

TransferResult_t receive_string( char** output_str_ptr, SOCKET sd )
{
	/* Recv the the request to the server on socket sd */
	int total_string_size_in_bytes;
	TransferResult_t recv_res;
	char* str_buffer = NULL;

	if ( ( output_str_ptr == NULL ) || ( *output_str_ptr != NULL ) )
	{
		printf("The first input to receive_string() must be " 
			   "a pointer to a char pointer that is initialized to NULL. For example:\n"
			   "\tchar* buffer = NULL;\n"
			   "\treceive_string( &Buffer, ___ )\n" );
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in 
	   an int variable ), then the string itself. */
	recv_res = receive_buffer( 
		(char *)( &total_string_size_in_bytes ),
		(int)( sizeof(total_string_size_in_bytes) ), // 4 bytes
		sd );

	if ( recv_res != TRNS_SUCCEEDED ) return recv_res;

	str_buffer = (char*)malloc( total_string_size_in_bytes * sizeof(char) );

	if ( str_buffer == NULL )
		return TRNS_FAILED;

	recv_res = receive_buffer( 
		(char *)( str_buffer ),
		(int)( total_string_size_in_bytes), 
		sd );

	if ( recv_res == TRNS_SUCCEEDED ) 
		{ *output_str_ptr = str_buffer; }
	else
	{
		free( str_buffer );
	}
		
	return recv_res;
}

int transmit_res(TransferResult_t msg_res, SOCKET* t_socket)
{
	if (msg_res == TRNS_FAILED)
	{
		printf("Service socket error while reading, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}
	else if (msg_res == TRNS_DISCONNECTED)
	{
		printf("Connection closed while reading, closing thread.\n");
		closesocket(*t_socket);
		return 1;
	}
	return 0;
}

int msg_creator(int size, char** msg, char* msg_type, char* param)
{
	if (NULL == *msg)
	{
		*msg = (char*)malloc(size * sizeof(char));
		if (NULL == *msg)
		{
			printf("memory allocation failed");
			return 1;
		}
		strcpy(*msg, msg_type);
		strcat_s(*msg, size, param);
	}
	else
	{
		strcat_s(*msg, size, ";");
		strcat_s(*msg, size, param);
	}
	return 0;
}

int set_socket_timeout(DWORD timeout, SOCKET sd)
{
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(DWORD)))
	{
		perror("setsockopt");
		return 1;
	}
	return 0;
}

void disconnect_socket(SOCKET* sd)
{ // DISCONNECT AND CREATES NEW SOCKET
	closesocket(*sd);

	*sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Check for errors to ensure that the socket is a valid socket.
	if (*sd == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return NULL;
	}
}

void get_msg_type_and_params(char* input_msg, char* msg_type, char* params)
{
	int i = 0;
	while (*input_msg != ':')
	{
		msg_type[i] = *input_msg;
		i++;
		input_msg++;

	}
	msg_type[i] = '\0';
	i = 0;
	input_msg++;
	while (*input_msg != '\0')
	{
		params[i] = *input_msg;
		i++;
		input_msg++;
	}
	params[i] = '\0';
}