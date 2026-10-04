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

/* Provider-side export, independent of the consumer import contract above.
   A class whose retail vtable holds a genuine vector deleting destructor was
   exported whole: VC6 emits a distinct `vector deleting destructor' only for
   a class-level dllexport and otherwise aliases it to the scalar one. A
   class without that evidence whose default-argument constructor has an
   exported `default constructor closure' (??_F) marks that constructor
   alone, which is enough for VC6 to emit the closure. */
#if defined(SURRENDER_BUILD)
#define SR_DLL_EXPORT __declspec(dllexport)
#else
#define SR_DLL_EXPORT
#endif

/* The SDK's zero fill pre-aligns the destination to an 8-byte boundary:
   VC6 lowers each site to a head memset + dword-body memset split rather
   than the single rep stosd a plain memset produces. Fills shorter than the
   alignment head, and empty fills, skip the split; the signed size tests
   fold away at constant-size sites (createOptimalPalette 0x10005690 keeps
   them for its variable palette and node-level fills). Inlined everywhere;
   no standalone emission exists in retail. */
inline void srZeroMemory(void* destination, long size)
{
    if (size > 0) {
        /* reinterpret-ok: raw address alignment is storage the type system
           cannot express. */
        unsigned long misalign = reinterpret_cast<unsigned long>(destination) & 7;
        if (size >= 8 && misalign != 0) {
            unsigned long head = 8 - misalign;
            memset(destination, 0, head);
            /* reinterpret-ok: byte-granular advance past the head fill. */
            memset(reinterpret_cast<unsigned char*>(destination) + head, 0, size - head);
        } else {
            memset(destination, 0, size);
        }
    }
}

/* Float-to-int through the FPU's current rounding mode: every use site
   lowers to an inline `fld value` / `fistp result` pair instead of the
   _ftol call a C conversion emits, so it rounds to nearest rather than
   truncating. The pixel format converters quantize dot-product results
   and the exponent table indexes through it; no standalone emission. */
inline long srFloatToInt(float value)
{
    long result;
    __asm {
        fld value
        fistp result
    }
    return result;
}

inline long srFloatToInt(double value)
{
    long result;
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
        unsigned long alloc_size;
        Block* next;
        Block* previous;
        unsigned long largest_free_size;
        Chunk* largest_free_block;
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
        Chunk* previous;
        Chunk* next_0c;
        Chunk* free_previous;
        Chunk* free_next;
        unsigned long free_18;
        char unused_1c[3];
        char tag;
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

    void* small_free_lists[32];
    unsigned long current_block_offset;
    Block* current_block;
    Block* small_blocks;
    Block* partial_blocks;
    Block* block_90;
    Block* medium_blocks;
    Block* large_blocks;
    Block* cached_block;
    unsigned long active_block_count;
    unsigned long block_size;
    unsigned long block_sequence;
    unsigned long system_block_count_ac;
    srCriticalSection* critical_section;
};

static_assert(sizeof(srHeap) == 0xb4, "srHeap_must_be_0xb4");

extern SR_DLL_IMPORT class srHeap srHeap;
