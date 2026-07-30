#pragma once

#include "srHeap.h"

class srThread {
public:
    SR_DLL_IMPORT srThread& operator=(const srThread& thread);

    static SR_DLL_IMPORT unsigned long begin(
        void (__cdecl* entry)(void*), void* argument);
    static SR_DLL_IMPORT void end();
    static SR_DLL_IMPORT unsigned long getHandle();
    static SR_DLL_IMPORT long getYieldCount();
    static SR_DLL_IMPORT void yield(unsigned long milliseconds);

private:
    static SR_DLL_IMPORT long yieldCount;
};

static_assert(sizeof(srThread) == 0x01, "srThread_must_be_0x01");
