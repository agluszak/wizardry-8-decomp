#pragma once

#include "srHeap.h"

class SR_DLL_EXPORT srThread {
public:
    static w8_ulong begin(void(__cdecl* entry)(void*), void* argument);
    static void end();
    static w8_ulong getHandle();
    static w8_long getYieldCount();
    static void yield(w8_ulong milliseconds);

private:
    static w8_long yieldCount;
};

static_assert(sizeof(srThread) == 0x01, "srThread_must_be_0x01");
