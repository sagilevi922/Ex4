// Includes --------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// Function Definitions --------------------------------------------------------

// gets a string of the input file name, open a file with this name if exist,
// and then return a handle to this file
// if fails returns NULL
HANDLE get_input_file_handle(char* input_file_name);

// Gets a handle for the input file, and a start position to where
// it should start reading, and how many bytes to read
// and copy it to a dynamic string.
// if fails returns NULL
char* txt_file_to_str(HANDLE hFile, int start_pos, int input_size);

// gets a string of the output file name, open a file with this name if exist,
// and then return a handle to this file
// if fails returns NULL
HANDLE create_file_for_write(char* output_file_name, int line_length);

// gets a string of the A new line to write, it size, and handle to the output file, and the size of the file
// and writes that line to the end of the output file
void write_to_file(char* new_line, int new_line_size, HANDLE oFile, DWORD dwFileSize);