#pragma once

#include <iosfwd>
#include <string.h>
#include <windows.h>

#include "srCriticalSection.h"

#if defined(_MSC_VER) && !defined(SURRENDER_BUILD)
#define SR_DLL_IMPORT __declspec(dllimport)
#else
#define SR_DLL_IMPORT
#endif

#if defined(SURRENDER_BUILD)
#define SR_DLL_EXPORT __declspec(dllexport)
#else
#define SR_DLL_EXPORT
#endif

/* Zero fill that pre-aligns the destination to an 8-byte boundary. */
inline void srZeroMemory(void* destination, w8_long size)
{
    if (size > 0) {
        /* reinterpret-ok: raw address alignment is storage the type system
           cannot express. */
        // reinterpret-ok: address bits determine the existing eight-byte alignment prefix
        w8_ulong misalign = reinterpret_cast<w8_ulong_ptr>(destination) & 7;
        if (size >= 8 && misalign != 0) {
            w8_ulong head = 8 - misalign;
            memset(destination, 0, head);
            /* reinterpret-ok: byte-granular advance past the head fill. */
            memset(reinterpret_cast<unsigned char*>(destination) + head, 0, size - head);
        } else {
            memset(destination, 0, size);
        }
    }
}

/* Float-to-int through the FPU's current rounding mode (round to nearest, not truncation). */
inline w8_long srFloatToInt(float value)
{
    w8_long result;
    __asm {
        fld value
        fistp result
    }
    return result;
}

inline w8_long srFloatToInt(double value)
{
    w8_long result;
    __asm {
        fld value
        fistp result
    }
    return result;
}

class srHeap {
public:
    SR_DLL_IMPORT srHeap();
    SR_DLL_IMPORT ~srHeap();

    SR_DLL_IMPORT void* allocate(w8_ulong size);
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT void free(void* allocation, unsigned int size);
    SR_DLL_IMPORT void freeAll();
    SR_DLL_IMPORT w8_ulong msize(void* allocation);
    SR_DLL_IMPORT void dump(std::ostream& stream);

private:
    struct Chunk;

    struct Block {
        void* allocation;
        w8_ulong alloc_size;
        Block* next;
        Block* previous;
        w8_ulong largest_free_size;
        Chunk* largest_free_block;
        w8_ulong guard0;
        w8_ulong guard1;
    };

    W8_ABI_ASSERT(sizeof(Block) == 0x20, "srHeap_Block_must_be_0x20");

    /* In-block allocation record for the pooled mid-size path. The header sits immediately before
       the user pointer; its last byte is the allocation tag read by free(). */
    struct Chunk {
        Block* owner;
        w8_ulong size;
        Chunk* previous;
        Chunk* next;
        Chunk* free_previous;
        Chunk* free_next;
        w8_ulong free;
        char unused[3];
        char tag;
    };

    W8_ABI_ASSERT(sizeof(Chunk) == 0x20, "srHeap_Chunk_must_be_0x20");

    Block* allocateBlock(w8_ulong size);
    void releaseBlock(Block* block);
    void releaseCachedBlock();
    void freeSystemBlock(void* allocation);
    void checkBlock(Block* block);
    void* splitFree(Block* block, w8_ulong size);
    void* allocatePooled(w8_ulong size);
    void freePooled(void* allocation);
    void* allocateSystem(w8_ulong size);
    void freeSystem(void* allocation);

    void* small_free_lists[32];
    w8_ulong current_block_offset;
    Block* current_block;
    Block* small_blocks;
    Block* partial_blocks;
    Block* block;
    Block* medium_blocks;
    Block* large_blocks;
    Block* cached_block;
    w8_ulong active_block_count;
    w8_ulong block_size;
    w8_ulong block_sequence;
    w8_ulong system_block_count;
    srCriticalSection* critical_section;
};

W8_ABI_ASSERT(sizeof(srHeap) == 0xb4, "srHeap_must_be_0xb4");

extern SR_DLL_IMPORT class srHeap srHeap;
