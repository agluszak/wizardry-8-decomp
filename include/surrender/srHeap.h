#pragma once

#include <iostream>
#include <string.h>
#include <windows.h>

#include "srCriticalSection.h"

#if defined(_MSC_VER) && !defined(SURRENDER_BUILD)
#define SR_DLL_IMPORT __declspec(dllimport)
#else
#define SR_DLL_IMPORT
#endif

/* The SDK's zero fill pre-aligns the destination to an 8-byte boundary:
   VC6 lowers each site to a head memset + dword-body memset split rather
   than the single rep stosd a plain memset produces. Inlined everywhere;
   no standalone emission exists in retail. */
inline void srZeroMemory(void* destination, unsigned long size)
{
    /* reinterpret-ok: raw address alignment is storage the type system
       cannot express. */
    unsigned long misalign = reinterpret_cast<unsigned long>(destination) & 7;
    if (misalign != 0) {
        unsigned long head = 8 - misalign;
        memset(destination, 0, head);
        /* reinterpret-ok: byte-granular advance past the head fill. */
        memset(reinterpret_cast<unsigned char*>(destination) + head, 0, size - head);
    } else {
        memset(destination, 0, size);
    }
}

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
    struct Chunk;

    struct Block {
        void* allocation_00;
        unsigned long allocation_size_04;
        Block* next_08;
        Block* previous_0c;
        unsigned long largest_free_size_10;
        Chunk* largest_free_block_14;
        unsigned long guard_18;
        unsigned long guard_1c;
    };

    static_assert(sizeof(Block) == 0x20, "srHeap_Block_must_be_0x20");

    /* In-block allocation record for the pooled mid-size path. The 0x20-byte
       header sits immediately before the user pointer; the byte at +0x1f is
       the allocation tag read by free(). */
    struct Chunk {
        Block* owner_00;
        unsigned long size_04;
        Chunk* previous_08;
        Chunk* next_0c;
        Chunk* free_previous_10;
        Chunk* free_next_14;
        unsigned long free_18;
        char unused_1c[3];
        char tag_1f;
    };

    static_assert(sizeof(Chunk) == 0x20, "srHeap_Chunk_must_be_0x20");

    Block* allocateBlock(unsigned long size);
    void releaseBlock(Block* block);
    void releaseCachedBlock();
    void freeSystemBlock(void* allocation);
    void checkBlock(Block* block);
    void* splitFree(Block* block, unsigned long size);
    void* allocatePooled(unsigned long size);
    void freePooled(void* allocation);
    void* allocateSystem(unsigned long size);
    void freeSystem(void* allocation);

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
    srCriticalSection* critical_section_b0;
};

static_assert(sizeof(srHeap) == 0xb4, "srHeap_must_be_0xb4");

extern SR_DLL_IMPORT class srHeap srHeap;
