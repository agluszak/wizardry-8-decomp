#include "surrender/srMutex.h"

// SYNTHETIC: SURRENDER 0x10045930
// srMutex::`vector deleting destructor'

// FUNCTION: SURRENDER 0x10045A40
srMutex::srMutex()
{
    access_count_08 = 0;
    handle_04 = CreateMutexA(0, 0, 0);
}

// FUNCTION: SURRENDER 0x10045A70
srMutex::~srMutex()
{
    WaitForSingleObject(handle_04, INFINITE);
    CloseHandle(handle_04);
}

// FUNCTION: SURRENDER 0x10045AA0
int srMutex::accessAvailable()
{
    if (WaitForSingleObject(handle_04, 0) == WAIT_ABANDONED) {
        return 0;
    }
    ReleaseMutex(handle_04);
    return 1;
}

// FUNCTION: SURRENDER 0x10045AD0
void srMutex::getAccess()
{
    WaitForSingleObject(handle_04, INFINITE);
    access_count_08++;
}

// FUNCTION: SURRENDER 0x10045AF0
void srMutex::releaseAccess()
{
    ReleaseMutex(handle_04);
    access_count_08--;
}

// SYNTHETIC: SURRENDER 0x10045910
// srMutex scalar deleting destructor
