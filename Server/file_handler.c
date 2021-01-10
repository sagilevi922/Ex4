// file_handler.c
/*
Authors – Matan Achiel - 205642119, Sagi Levi - 205663545
Project – Ex3 - factor.
Description – This moudle is for handling files, create HANDLE
for read or write, and also can read straign from file
*/

// Includes --------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "HardCodedData.h"

// Function Definitions --------------------------------------------------------
int close_handles_proper(HANDLE file_handle)
{
	//arguments check - exrported function
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		printf("Invalid HANDLE value, can't close this HANDLE.\n");
	}
	int ret_val = 0;
	if (file_handle != 0)
		ret_val = CloseHandle(file_handle);
	if (FALSE == ret_val)
	{
		printf("Error when closing\n");
		return ERROR_CODE;
	}
	return 1;

}
HANDLE get_input_file_handle(char* input_file_name)
{
	HANDLE hFile;
	hFile = CreateFileA(input_file_name,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf(("Terminal failure: unable to open file \"%s\" for read.\n"), input_file_name);
		return NULL;
	}
	if (GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		printf("Invalid input file name. no such file existing. Finish program");
		return NULL;
	}
	return hFile;
}

void txt_file_to_str(HANDLE hFile, int start_pos, int input_size, char* input_txt)
{
	DWORD file_ptr;
	DWORD dwBytesRead = 0;

	//arguments check - exrported function
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	if (start_pos < 0 || input_size < 0)
	{
		printf("invalid starting position for current thread\n");

		close_handles_proper(hFile);
		return NULL;
	}

	file_ptr = SetFilePointer(
		hFile,
		start_pos, //number of chars
		NULL, //no need of 32 high bits
		FILE_BEGIN //starting point- begin of file
	);

	if (file_ptr == INVALID_SET_FILE_POINTER) // Test for failure
	{
		printf("Can't set file pointer of outputfile. exit\n");
		if (close_handles_proper(hFile) != 1)
			return 1;
	}

	if (FALSE == ReadFile(hFile, input_txt, input_size, &dwBytesRead, NULL))
	{
		printf("Terminal failure: Unable to read from file.\n GetLastError=%08x\n", GetLastError());
		close_handles_proper(hFile);
		free(input_txt);
		return NULL;
	}

	if (dwBytesRead > 0 && (int)dwBytesRead <= input_size)
	{
		if (input_size > 1)
			input_txt[dwBytesRead] = '\0'; // NULL character
	}
	else if (dwBytesRead == 0)
	{
		printf("No data read from file\n");
	}
	if (input_size > 1)
		return input_txt;
	return *input_txt;
}

HANDLE create_file_for_write(char* output_file_name, int line_length)
{
	DWORD end_file_ptr;
	HANDLE hFile;
	//extern char action_mode;
	DWORD dwFileSize;
	hFile = CreateFileA(output_file_name,               // file to open
		GENERIC_WRITE,          // open for reading
		FILE_SHARE_WRITE,       // share for write
		NULL,                  // default security
		OPEN_ALWAYS,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf(("Terminal failure: unable to open the output file for write.\n"));
		return NULL;
	}
	dwFileSize = GetFileSize(hFile, NULL);

	if (INVALID_FILE_SIZE == dwFileSize) {
		printf("Invalid File Size\n");
		return NULL;
	}

	//end_file_ptr = SetFilePointer(
	//	hFile,
	//	dwFileSize + line_length-2, //number of chars
	//	NULL, //no need of 32 high bits
	//	FILE_BEGIN //starting point- begin of file
	//);

	//if (end_file_ptr == INVALID_SET_FILE_POINTER) // Test for failure
	//{
	//	printf("Can't set file pointer of outputfile. exit\n");
	//	return NULL;
	//}

	//int set_end_of_file = SetEndOfFile(hFile);
	//if (!set_end_of_file) {
	//	printf("Can't set file pointer of outputfile. exit\n");
	//	return NULL;
	//}

	return hFile;
}

void write_to_file(char* new_line, int new_line_size, HANDLE oFile, DWORD dwFileSize)
{

	DWORD  dwBytesWritten = 0;
	DWORD  file_ptr;
	printf("trying to write to file: %s\n", new_line);

	file_ptr = SetFilePointer(
		oFile,
		dwFileSize, //number of chars
		NULL, //no need of 32 high bits
		FILE_BEGIN //starting point- begin of file
	);

	if (FALSE == WriteFile(oFile, new_line, new_line_size, &dwBytesWritten, NULL))
	{
		printf("Terminal failure: Unable to write to file.\n GetLastError=%08x\n", GetLastError());
		close_handles_proper(oFile);
	}

}