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
    struct CacheEntry {
        void* allocation;
        unsigned long size;
    };

    CacheEntry entries_00[16];
    unsigned long used_mask_80;
    unsigned long cached_bytes_84;
    unsigned long limit_88;
    CRITICAL_SECTION critical_section_8c;
};

static_assert(sizeof(srGlobalRecycler) == 0xa4,
              "srGlobalRecycler_must_be_0xa4");
