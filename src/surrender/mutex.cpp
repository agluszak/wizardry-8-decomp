#include "surrender/srMutex.h"

// FUNCTION: SURRENDER 0x100458D0
srMutex::srMutex(const srMutex& mutex)
{
    handle_04 = mutex.handle_04;
    access_count_08 = mutex.access_count_08;
}

// FUNCTION: SURRENDER 0x100458F0
srMutex& srMutex::operator=(const srMutex& mutex)
{
    handle_04 = mutex.handle_04;
    access_count_08 = mutex.access_count_08;
    return *this;
}

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
