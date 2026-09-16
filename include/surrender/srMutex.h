#pragma once

#include "srHeap.h"

/* Provider-side utility. The SR vtable is known, but no known consumer imports
   srMutex symbols; provider ABI evidence alone does not justify dllimport. */
class srMutex {
public:
    srMutex();
    srMutex(const srMutex& mutex);
    virtual ~srMutex();
    srMutex& operator=(const srMutex& mutex);

    int accessAvailable();
    void getAccess();
    void releaseAccess();

private:
    HANDLE handle_04;
    long access_count_08;
};

static_assert(sizeof(srMutex) == 0x0c, "srMutex_must_be_0x0c");
