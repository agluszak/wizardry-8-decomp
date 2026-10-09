#pragma once

#include "srHeap.h"

/* Four-byte packed color, stored B,G,R,A (the little-endian 0xAARRGGBB dword). e_index is the
   logical ARGB channel, not a memory offset. */
class srARGB {
public:
    enum e_index { INDEX_ALPHA = 0, INDEX_RED = 1, INDEX_GREEN = 2, INDEX_BLUE = 3 };

    srARGB() {}

    void* operator new[](size_t size)
    {
        return srHeap.allocate(size);
    }

    void operator delete[](void* allocation)
    {
        srHeap.free(allocation);
    }

    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
};

static_assert(sizeof(srARGB) == 4, "srARGB_must_be_4");
