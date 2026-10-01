#pragma once

#include "srHeap.h"

/* Provider-side utility. The SR vtable is known, but no known consumer imports
   srMutex symbols; provider ABI evidence alone does not justify dllimport. */
// VTABLE: SURRENDER 0x10076DE4 srMutex
class SR_DLL_EXPORT srMutex {
public:
    srMutex();
    /* Retail's copies-then-vftable emission is the implicit special-member
       lowering. Class-level dllexport emits both standalone copies. */
    // SYNTHETIC: SURRENDER 0x100458D0
    // srMutex::srMutex(const srMutex&)
    // SYNTHETIC: SURRENDER 0x100458F0
    // srMutex::operator=
    virtual ~srMutex();

    int accessAvailable();
    void getAccess();
    void releaseAccess();

private:
    HANDLE handle_04;
    long access_count_08;
};

static_assert(sizeof(srMutex) == 0x0c, "srMutex_must_be_0x0c");
