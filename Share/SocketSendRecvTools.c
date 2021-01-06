#define _CRT_SECURE_NO_WARNINGS

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/* 
 This file was written for instruction purposes for the 
 course "Introduction to Systems Programming" at Tel-Aviv
 University, School of Electrical Engineering, Winter 2011, 
 by Amnon Drory, based on example code by Johnson M. Hart.
*/
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#include "SocketSendRecvTools.h"

#include <stdio.h>
#include <string.h>

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

TransferResult_t SendBuffer( const char* Buffer, int BytesToSend, SOCKET sd )
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;
	
	while ( RemainingBytesToSend > 0 )  
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send (sd, CurPlacePtr, RemainingBytesToSend, 0);
		if ( BytesTransferred == SOCKET_ERROR ) 
		{
			printf("send() failed, error %d\n", WSAGetLastError() );
			return TRNS_FAILED;
		}
		
		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}


TransferResult_t SendString( const char *Str, SOCKET sd )
{
	/* Send the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;

	/* The request is sent in two parts. First the Length of the string (stored in 
	   an int variable ), then the string itself. */
		
	TotalStringSizeInBytes = (int)( strlen(Str) + 1 ); // terminating zero also sent	

	SendRes = SendBuffer( 
		(const char *)( &TotalStringSizeInBytes ),
		(int)( sizeof(TotalStringSizeInBytes) ), // sizeof(int) 
		sd );

	if ( SendRes != TRNS_SUCCEEDED ) return SendRes ;

	SendRes = SendBuffer( 
		(const char *)( Str ),
		(int)( TotalStringSizeInBytes ), 
		sd );

	return SendRes;
}


TransferResult_t ReceiveBuffer( char* OutputBuffer, int BytesToReceive, SOCKET sd )
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;
	
	while ( RemainingBytesToReceive > 0 )  
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if ( BytesJustTransferred == SOCKET_ERROR ) 
		{
			//printf("recv() failed, error %d\n", WSAGetLastError() );
			return TRNS_FAILED;
		}		
		else if ( BytesJustTransferred == 0 )
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

TransferResult_t ReceiveString( char** OutputStrPtr, SOCKET sd )
{
	/* Recv the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char* StrBuffer = NULL;

	if ( ( OutputStrPtr == NULL ) || ( *OutputStrPtr != NULL ) )
	{
		printf("The first input to ReceiveString() must be " 
			   "a pointer to a char pointer that is initialized to NULL. For example:\n"
			   "\tchar* Buffer = NULL;\n"
			   "\tReceiveString( &Buffer, ___ )\n" );
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in 
	   an int variable ), then the string itself. */
		
	RecvRes = ReceiveBuffer( 
		(char *)( &TotalStringSizeInBytes ),
		(int)( sizeof(TotalStringSizeInBytes) ), // 4 bytes
		sd );

	if ( RecvRes != TRNS_SUCCEEDED ) return RecvRes;

	StrBuffer = (char*)malloc( TotalStringSizeInBytes * sizeof(char) );

	if ( StrBuffer == NULL )
		return TRNS_FAILED;

	RecvRes = ReceiveBuffer( 
		(char *)( StrBuffer ),
		(int)( TotalStringSizeInBytes), 
		sd );

	if ( RecvRes == TRNS_SUCCEEDED ) 
		{ *OutputStrPtr = StrBuffer; }
	else
	{
		free( StrBuffer );
	}
		
	return RecvRes;
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

//int recieve_str(char* AcceptedStr, SOCKET* t_socket)
//{
//	TransferResult_t RecvRes;
//	RecvRes = ReceiveString(&AcceptedStr, *t_socket);
//
//	if (RecvRes == TRNS_FAILED)
//	{
//		printf("Service socket error while reading, closing thread.\n");
//		closesocket(*t_socket);
//		return 1;
//	}
//	else if (RecvRes == TRNS_DISCONNECTED)
//	{
//		printf("Connection closed while reading, closing thread.\n");
//		closesocket(*t_socket);
//		return 1;
//	}
//	else
//		printf("Got string : %s\n", AcceptedStr);
//	return 0;
//}