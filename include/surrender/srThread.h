#pragma once

#include "srHeap.h"

/* Provider-side utility. No known Wizardry/JPEG/ZIP consumer imports srThread
   symbols, so its declarations must not carry consumer dllimport codegen. */
class srThread {
public:
    srThread& operator=(const srThread& thread);

    static unsigned long begin(
        void (__cdecl* entry)(void*), void* argument);
    static void end();
    static unsigned long getHandle();
    static long getYieldCount();
    static void yield(unsigned long milliseconds);

private:
    static long yieldCount;
};

static_assert(sizeof(srThread) == 0x01, "srThread_must_be_0x01");
