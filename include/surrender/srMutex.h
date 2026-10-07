#pragma once

#include "srHeap.h"

// VTABLE: SURRENDER 0x10076DE4 srMutex
class SR_DLL_EXPORT srMutex {
public:
    srMutex();

    virtual ~srMutex();

    int accessAvailable();
    void getAccess();
    void releaseAccess();

private:
    HANDLE handle;
    long access_count;
};

static_assert(sizeof(srMutex) == 0x0c, "srMutex_must_be_0x0c");
