#pragma once

#include "srHeap.h"

class SR_DLL_EXPORT srMemoryAllocator {
public:
    enum e_alignSize { ALIGN_SIZE_32 = 0x20 };

    SR_DLL_IMPORT srMemoryAllocator();
    SR_DLL_IMPORT ~srMemoryAllocator();
#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srMemoryAllocator& operator=(const srMemoryAllocator& other);
#endif

    SR_DLL_IMPORT void* allocate(unsigned long size, const char* name);
    SR_DLL_IMPORT void* allocate(unsigned long count, unsigned long size, const char* name);
    SR_DLL_IMPORT void dump() const;
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT const char* getName(void* allocation) const;
    SR_DLL_IMPORT unsigned long getSize(void* allocation) const;
    SR_DLL_IMPORT void setAlignment(e_alignSize alignment);

private:
    class Block {
    public:
        Block* next;
        Block* previous;
        void* raw_allocation;
        /* Written through by allocate's strcpy - mutable storage despite the
           read-only getName accessor. */
        char* name;
        unsigned long total_size;
        unsigned long requested_size;
        unsigned long reserved[2];
    };

    SR_DLL_IMPORT Block* align(void* allocation);

    Block* first_block;
    unsigned long allocated_bytes;
    unsigned long allocation_count;
    e_alignSize alignment;
    int clear;
};

static_assert(sizeof(srMemoryAllocator) == 0x14, "srMemoryAllocator_must_be_0x14");
