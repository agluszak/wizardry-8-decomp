#include "surrender/srThread.h"

#include <process.h>

// GLOBAL: SURRENDER 0x100A49A4
long srThread::yieldCount;

// FUNCTION: SURRENDER 0x100458C0
srThread& srThread::operator=(const srThread& thread)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10045B10
unsigned long srThread::begin(void(__cdecl* entry)(void*), void* argument)
{
    return _beginthread(entry, 0, argument);
}

// FUNCTION: SURRENDER 0x10045B30 SYMBOL
// ?end@srThread@@SAXXZ
void srThread::end()
{
    _endthread();
}

// FUNCTION: SURRENDER 0x10045B40 SYMBOL
// ?getHandle@srThread@@SAKXZ
unsigned long srThread::getHandle()
{
    return GetCurrentThreadId();
}

// FUNCTION: SURRENDER 0x100458B0
long srThread::getYieldCount()
{
    return yieldCount;
}

// FUNCTION: SURRENDER 0x10045B50
void srThread::yield(unsigned long milliseconds)
{
    yieldCount = yieldCount + 1;
    Sleep(milliseconds);
}
