#pragma once

#include "srHeap.h"

/* Provider-side utility. The SR vtable is known, but no known consumer imports
   srMutex symbols; provider ABI evidence alone does not justify dllimport. */
// VTABLE: SURRENDER 0x10076DE4 srMutex
class SR_DLL_EXPORT srMutex {
public:
    srMutex();
    /* The copy bodies are consistent with ordinary memberwise copying. */

    virtual ~srMutex();

    int accessAvailable();
    void getAccess();
    void releaseAccess();

private:
    HANDLE handle;
    long access_count;
};

static_assert(sizeof(srMutex) == 0x0c, "srMutex_must_be_0x0c");
