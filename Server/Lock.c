// Lock.c
/*
Authors – Matan Achiel - 205642119, Sagi Levi - 205663545
Project – Ex3 - factor.
Description – This moudle is for syncing beetwen different threads that try to 
read and write from the same file.
Allows for locking certain sections of code, so only specific threads can read or write.
*/

// Includes --------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <math.h>
#include "HardCodedData.h"

#include "Lock.h"

// Function Definitions --------------------------------------------------------

lock* InitializeLock()
{
    lock* new_lock = (struct lock*)malloc(sizeof(lock));
    if (NULL == new_lock)
    {
        printf("Failed to allocate memory for lock.\n");
        return NULL;
    }

    HANDLE mutex = CreateMutexA(NULL, FALSE, NULL);
    if (NULL == mutex)
    {
        free(new_lock);
        return NULL;
    }

    HANDLE turnstile = CreateMutexA(NULL, FALSE, NULL);
    if (NULL == turnstile)
    {
        free(new_lock);

        return NULL;
    }

    HANDLE roomEmpty = CreateSemaphoreA(NULL, 1, 1, NULL);  // creats a semphore for paralllel threads func
    if (NULL == roomEmpty)
    {
        free(new_lock);
        return NULL;
    }
    //want to change init value to 2
    new_lock->readers = 0;

    new_lock->mutex = mutex;
    new_lock->roomEmpty = roomEmpty;
    new_lock->turnstile = turnstile;

    return new_lock;
}

int lock_read(lock* lock)
{
    DWORD wait_res;
    int release_res;
    HANDLE mutex = lock->mutex;
    HANDLE turnstile = lock->turnstile;
    HANDLE roomEmpty = lock->roomEmpty;

    wait_res = WaitForSingleObject(turnstile, MAX_WAITING_TIME);
    if (wait_res != WAIT_OBJECT_0)
    {
        printf_s("Failed wait for single object - turnstile, error: %d\n", GetLastError());
        return 0;
    }


    release_res = ReleaseMutex(turnstile);
    if (release_res == 0)
    {
        printf_s("Failed release for object - turnstile, error: %d\n", GetLastError());
        return 0;
    }

    wait_res = WaitForSingleObject(mutex, MAX_WAITING_TIME);
    if (wait_res != WAIT_OBJECT_0)
    {
        printf_s("Failed wait for single object - mutex, error: %d\n", GetLastError());
        return 0;
    }

    lock->readers += 1;
    if (lock->readers == 1)
    {
        wait_res = WaitForSingleObject(roomEmpty, MAX_WAITING_TIME);
        if (wait_res != WAIT_OBJECT_0)
        {
            printf_s("Failed wait for single object - roomEmpty, error: %d\n", GetLastError());
            return 0;
        }
    }

    release_res = ReleaseMutex(mutex);
    if (release_res == 0)
    {
        printf_s("Failed release for object - mutex, error: %d\n", GetLastError());
        return 0;
    }

    return 1;
}

int release_read(lock* lock)
{
    DWORD wait_res;
    int release_res;
    HANDLE mutex = lock->mutex;
    HANDLE turnstile = lock->turnstile;
    HANDLE roomEmpty = lock->roomEmpty;

    wait_res = WaitForSingleObject(mutex, MAX_WAITING_TIME);
    if (wait_res != WAIT_OBJECT_0)
    {
        printf_s("Failed wait for single object - mutex, error: %d\n", GetLastError());
        return 0;
    }

    lock->readers -= 1;
    if (lock->readers ==0)
    {
        release_res = ReleaseSemaphore(roomEmpty, 1, NULL);
        if (release_res == 0)
        {
            printf_s("Failed release for object - roomEmpty, error: %d\n", GetLastError());
            return 0;
        }
    }

    release_res = ReleaseMutex(mutex);
    if (release_res == 0)
    {
        printf_s("Failed release for object - mutex, error: %d\n", GetLastError());
        return 0;
    }

    return 1;
}

int lock_write(lock* lock)
{
    DWORD wait_res;
    HANDLE turnstile = lock->turnstile;
    HANDLE roomEmpty = lock->roomEmpty;

    wait_res = WaitForSingleObject(turnstile, MAX_WAITING_TIME);
    if (wait_res != WAIT_OBJECT_0)
    {
        printf_s("Failed wait for single object - turnstile, error: %d\n", GetLastError());
        return 0;
    }

    wait_res = WaitForSingleObject(roomEmpty, MAX_WAITING_TIME);
    if (wait_res != WAIT_OBJECT_0)
    {
        printf_s("Failed wait for single object - roomEmpty, error: %d\n", GetLastError());
        return 0;
    }

    return 1;

}

int release_write(lock* lock)
{
    int release_res;
    HANDLE turnstile = lock->turnstile;
    HANDLE roomEmpty = lock->roomEmpty;

    release_res = ReleaseMutex(turnstile);
    if (release_res == 0)
    {
        printf_s("Failed release for object - turnstile, error: %d\n", GetLastError());
        return 0;
    }

    release_res = ReleaseSemaphore(roomEmpty, 1, NULL);
    if (release_res == 0)
    {
        printf_s("Failed release for object - roomEmpty, error: %d\n", GetLastError());
        return 0;
    }
    return 1;
}

int DestroyLock(lock* lock)

{
    if (close_handles_proper(lock->mutex) != 1)
        return 0;
    if (close_handles_proper(lock->roomEmpty) != 1)
        return 0;
    if (close_handles_proper(lock->turnstile) != 1)
        return 0;
    free(lock);
    return 1;
}