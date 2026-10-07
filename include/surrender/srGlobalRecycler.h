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

    /* By-value critical section with a trivial constructor and a draining destructor; the owner
       initializes, enters and leaves it explicitly. */
    class CriticalSection {
    public:
        ~CriticalSection()
        {
            EnterCriticalSection(&critical_section);
            LeaveCriticalSection(&critical_section);
            DeleteCriticalSection(&critical_section);
        }

        CRITICAL_SECTION critical_section;
    };

    CacheEntry entries[16];
    unsigned long used_mask;
    unsigned long cached_bytes;
    unsigned long limit;
    CriticalSection critical_section;
};

static_assert(sizeof(srGlobalRecycler) == 0xa4, "srGlobalRecycler_must_be_0xa4");
