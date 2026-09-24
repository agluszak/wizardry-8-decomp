#pragma once

#include "srHeap.h"

class srMemoryAllocator {
public:
    enum e_alignSize { ALIGN_SIZE_32 = 0x20 };

    SR_DLL_IMPORT srMemoryAllocator();
    SR_DLL_IMPORT ~srMemoryAllocator();
    SR_DLL_IMPORT srMemoryAllocator& operator=(const srMemoryAllocator& other);

    SR_DLL_IMPORT void* allocate(unsigned long size, const char* name);
    SR_DLL_IMPORT void* allocate(unsigned long count, unsigned long size, const char* name);
    SR_DLL_IMPORT void dump() const;
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT const char* getName(void* allocation) const;
    SR_DLL_IMPORT unsigned long getSize(void* allocation) const;
    SR_DLL_IMPORT void setAlignment(e_alignSize alignment);

private:
    /* Retail mangles the parameter as PAVBlock: the nested record was declared
       class, not struct. */
    class Block {
    public:
        Block* next_00;
        Block* previous_04;
        void* raw_allocation_08;
        /* Written through by allocate's strcpy - mutable storage despite the
           read-only getName accessor. */
        char* name_0c;
        unsigned long allocation_size_10;
        unsigned long requested_size_14;
        unsigned long reserved_18[2];
    };

    /* Retail exports the standalone copy; without provider dllexport the
       compiler folds the single call site inside allocate and nothing emits. */
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    SR_DLL_IMPORT Block* align(void* allocation);

    Block* first_block_00;
    unsigned long allocated_bytes_04;
    unsigned long allocation_count_08;
    e_alignSize alignment_0c;
    int clear_10;
};

static_assert(sizeof(srMemoryAllocator) == 0x14, "srMemoryAllocator_must_be_0x14");
