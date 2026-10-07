#pragma once

#include "srHeap.h"

class SR_DLL_EXPORT srThread {
public:
    static unsigned long begin(void(__cdecl* entry)(void*), void* argument);
    static void end();
    static unsigned long getHandle();
    static long getYieldCount();
    static void yield(unsigned long milliseconds);

private:
    static long yieldCount;
};

static_assert(sizeof(srThread) == 0x01, "srThread_must_be_0x01");
