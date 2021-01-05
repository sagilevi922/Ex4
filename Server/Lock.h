// Includes --------------------------------------------------------------------

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>


// Structers --------------------------------------------------------------------

#ifndef LOCK_H_
#define LOCK_H_

// A Lock structure to contains all the neccesry data sturcts to lock section. 

typedef struct lock_s {
    int readers;
    HANDLE mutex;
    HANDLE roomEmpty;
    HANDLE turnstile;
} lock;

#endif


// Function Definitions --------------------------------------------------------

//Initialize the Lock struct, and returns a pointer to it
lock* InitializeLock();

//Lock a speific section for read file purpose - allows for another readers to enter locked section.
// doesnt allow writing. so when the first reader enterd to the locked secion, it says - "Room not empty" untill the last one leaves.
// gets a lock pointer and returns 0 if failed. or 1 if succeed
int lock_read(lock* lock);

//Release from Lock read. 
// gets a lock pointer and returns 0 if failed. or 1 if succeed
int release_read(lock* lock);

//Lock a speific section for write file purpose. doesnt allow any other threads to read or write, only the one who locked for write
// gets a lock pointer and returns 0 if failed. or 1 if succeed
int lock_write(lock* lock);

//Release from write read.
// gets a lock pointer and returns 0 if failed. or 1 if succeed
int release_write(lock* lock);

//Release all lock resources.
//gets a lock pointer and returns 0 if failed. or 1 if succeed
int DestroyLock(lock* lock);