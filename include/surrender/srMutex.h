#pragma once

#include "srHeap.h"

class srMutex {
public:
    SR_DLL_IMPORT srMutex();
    SR_DLL_IMPORT srMutex(const srMutex& mutex);
    virtual SR_DLL_IMPORT ~srMutex();
    SR_DLL_IMPORT srMutex& operator=(const srMutex& mutex);

    SR_DLL_IMPORT int accessAvailable();
    SR_DLL_IMPORT void getAccess();
    SR_DLL_IMPORT void releaseAccess();

private:
    HANDLE handle_04;
    long access_count_08;
};

static_assert(sizeof(srMutex) == 0x0c, "srMutex_must_be_0x0c");
