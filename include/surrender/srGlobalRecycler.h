#pragma once

#include <windows.h>

#include "srHeap.h"

class srGlobalRecycler {
public:
    SR_DLL_IMPORT srGlobalRecycler();
    SR_DLL_IMPORT ~srGlobalRecycler();

    SR_DLL_IMPORT void* allocate(unsigned long size);
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT void releaseAllUnused();
    SR_DLL_IMPORT void setLimit(unsigned long limit);

private:
    void freeEntry(unsigned long index);

    struct CacheEntry {
        void* allocation;
        unsigned long size;
    };

    /* By-value CS member with a trivial ctor and a draining dtor
       (Enter/Leave/DeleteCriticalSection): the methods Initialize/Enter/
       Leave the contained section explicitly, and ~srGlobalRecycler's EH
       funclet (0x100358D0) destroys this+0x8c. */
    class CriticalSection {
    public:
        ~CriticalSection()
        {
            EnterCriticalSection(&critical_section_00);
            LeaveCriticalSection(&critical_section_00);
            DeleteCriticalSection(&critical_section_00);
        }

        CRITICAL_SECTION critical_section_00;
    };

    CacheEntry entries_00[16];
    unsigned long used_mask_80;
    unsigned long cached_bytes_84;
    unsigned long limit_88;
    CriticalSection critical_section_8c;
};

static_assert(sizeof(srGlobalRecycler) == 0xa4,
              "srGlobalRecycler_must_be_0xa4");
