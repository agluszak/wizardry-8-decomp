#include "surrender/srThread.h"

#include <process.h>

// GLOBAL: SURRENDER 0x100A49A4
w8_long srThread::yieldCount;

// FUNCTION: SURRENDER 0x10045B10
w8_ulong srThread::begin(void(__cdecl* entry)(void*), void* argument)
{
    return _beginthread(entry, 0, argument);
}

// FUNCTION: SURRENDER 0x10045B30 SYMBOL
// RECOMP: ?end@srThread@@SAXXZ
void srThread::end()
{
    _endthread();
}

// FUNCTION: SURRENDER 0x10045B40 SYMBOL
// RECOMP: ?getHandle@srThread@@SAKXZ
w8_ulong srThread::getHandle()
{
    return GetCurrentThreadId();
}

// FUNCTION: SURRENDER 0x100458B0
w8_long srThread::getYieldCount()
{
    return yieldCount;
}

// FUNCTION: SURRENDER 0x10045B50
void srThread::yield(w8_ulong milliseconds)
{
    yieldCount = yieldCount + 1;
    Sleep(milliseconds);
}
