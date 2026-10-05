#include "surrender/srMutex.h"

// FUNCTION: SURRENDER 0x10045A40
srMutex::srMutex()
{
    access_count = 0;
    handle = CreateMutexA(0, 0, 0);
}

// FUNCTION: SURRENDER 0x10045A70
srMutex::~srMutex()
{
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
}

// FUNCTION: SURRENDER 0x10045AA0
int srMutex::accessAvailable()
{
    if (WaitForSingleObject(handle, 0) == WAIT_ABANDONED) {
        return 0;
    }
    ReleaseMutex(handle);
    return 1;
}

// FUNCTION: SURRENDER 0x10045AD0
void srMutex::getAccess()
{
    WaitForSingleObject(handle, INFINITE);
    access_count++;
}

// FUNCTION: SURRENDER 0x10045AF0
void srMutex::releaseAccess()
{
    ReleaseMutex(handle);
    access_count--;
}
