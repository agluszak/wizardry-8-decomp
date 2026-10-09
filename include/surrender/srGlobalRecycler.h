#pragma once

#include <windows.h>

#include "srHeap.h"

class srGlobalRecycler {
public:
    SR_DLL_IMPORT srGlobalRecycler();
    SR_DLL_IMPORT ~srGlobalRecycler();

    SR_DLL_IMPORT void* allocate(w8_ulong size);
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT void releaseAllUnused();
    SR_DLL_IMPORT void setLimit(w8_ulong limit);

private:
    void freeEntry(w8_ulong index);

    struct CacheEntry {
        void* allocation;
        w8_ulong size;
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
    w8_ulong used_mask;
    w8_ulong cached_bytes;
    w8_ulong limit;
    CriticalSection critical_section;
};

W8_ABI_ASSERT(sizeof(srGlobalRecycler) == 0xa4, "srGlobalRecycler_must_be_0xa4");
