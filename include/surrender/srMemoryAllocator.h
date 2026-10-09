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

    SR_DLL_IMPORT void* allocate(w8_ulong size, const char* name);
    SR_DLL_IMPORT void* allocate(w8_ulong count, w8_ulong size, const char* name);
    SR_DLL_IMPORT void dump() const;
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT const char* getName(void* allocation) const;
    SR_DLL_IMPORT w8_ulong getSize(void* allocation) const;
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
        w8_ulong total_size;
        w8_ulong requested_size;
        w8_ulong reserved[2];
    };

    SR_DLL_IMPORT Block* align(void* allocation);

    Block* first_block;
    w8_ulong allocated_bytes;
    w8_ulong allocation_count;
    e_alignSize alignment;
    int clear;
};

W8_ABI_ASSERT(sizeof(srMemoryAllocator) == 0x14, "srMemoryAllocator_must_be_0x14");
