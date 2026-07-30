#pragma once

#include <iostream>
#include <windows.h>

#if defined(_MSC_VER) && !defined(SURRENDER_BUILD)
#define SR_DLL_IMPORT __declspec(dllimport)
#else
#define SR_DLL_IMPORT
#endif

class srHeap {
public:
    SR_DLL_IMPORT srHeap();
    SR_DLL_IMPORT ~srHeap();

    SR_DLL_IMPORT void* allocate(unsigned long size);
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT void free(void* allocation, unsigned int size);
    SR_DLL_IMPORT void freeAll();
    SR_DLL_IMPORT unsigned long msize(void* allocation);
    SR_DLL_IMPORT void dump(std::ostream& stream);

private:
    struct Block {
        void* allocation_00;
        unsigned long allocation_size_04;
        Block* next_08;
        Block* previous_0c;
        unsigned long largest_free_size_10;
        void* largest_free_block_14;
        unsigned long guard_18;
        unsigned long guard_1c;
    };

    static_assert(sizeof(Block) == 0x20, "srHeap_Block_must_be_0x20");

    void* small_free_lists_00[32];
    unsigned long current_block_offset_80;
    Block* current_block_84;
    Block* block_list_88;
    Block* block_list_8c;
    Block* block_90;
    Block* block_list_94;
    Block* block_list_98;
    Block* cached_block_9c;
    unsigned long active_block_count_a0;
    unsigned long block_size_a4;
    unsigned long block_sequence_a8;
    unsigned long system_block_count_ac;
    CRITICAL_SECTION* critical_section_b0;
};

static_assert(sizeof(srHeap) == 0xb4, "srHeap_must_be_0xb4");

extern SR_DLL_IMPORT class srHeap srHeap;
