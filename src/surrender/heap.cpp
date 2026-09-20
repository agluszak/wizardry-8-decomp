#include "surrender/srHeap.h"

#include <stdlib.h>
#include <string.h>

#include "surrender/srDebug.h"

#pragma intrinsic(memset)

namespace {

// RAII critical-section guard: free() and freeAll() carry an EH funclet that
// runs LeaveCriticalSection on unwind, matching an object of this shape.
class HeapAccess {
public:
    HeapAccess(srCriticalSection* critical_section) : critical_section_(critical_section)
    {
        critical_section_->getAccess();
    }

    ~HeapAccess()
    {
        critical_section_->releaseAccess();
    }

private:
    srCriticalSection* critical_section_;
};

} // namespace

// FUNCTION: SURRENDER 0x100359C0
srHeap::srHeap()
{
    critical_section_b0 = new srCriticalSection;
    system_block_count_ac = 0;
    block_sequence_a8 = 0;
    active_block_count_a0 = 0;
    block_list_98 = 0;
    cached_block_9c = 0;
    block_size_a4 = 0x7fe0;
    memset(small_free_lists_00, 0, sizeof(small_free_lists_00));
    block_list_88 = 0;
    current_block_84 = 0;
    current_block_offset_80 = 0;
    block_list_8c = 0;
    block_90 = 0;
    block_list_94 = 0;
}

// FUNCTION: SURRENDER 0x10035A50
srHeap::~srHeap()
{
    freeAll();
    srCriticalSection* section = critical_section_b0;
    if (section != 0) {
        section->getAccess();
        section->releaseAccess();
        delete section;
    }
}

// FUNCTION: SURRENDER 0x10035A90
void srHeap::freeSystemBlock(void* allocation)
{
    ::free(allocation);
}

// FUNCTION: SURRENDER 0x10035AA0
void srHeap::releaseCachedBlock()
{
    if (cached_block_9c != 0) {
        freeSystemBlock(cached_block_9c);
    }
    cached_block_9c = 0;
}

// FUNCTION: SURRENDER 0x10035AC0
srHeap::Block* srHeap::allocateBlock(unsigned long size)
{
    unsigned long allocation_size = size + 0x20;
    Block* block = cached_block_9c;
    ++block_sequence_a8;
    if (block == 0 || block->allocation_size_04 != allocation_size) {
        block = static_cast<Block*>(malloc(allocation_size + 0x20));
        if (block == 0) {
            return 0;
        }
        // reinterpret-ok: block header pointer rounding to the 0x20-aligned
        // allocation payload.
        block->allocation_00 =
            reinterpret_cast<void*>((reinterpret_cast<unsigned long>(block) + 0x3f) & 0xffffffe0);
        block->allocation_size_04 = allocation_size;
        ++system_block_count_ac;
    } else {
        cached_block_9c = 0;
    }
    block->previous_0c = 0;
    block->next_08 = 0;
    block->largest_free_size_10 = 0;
    block->largest_free_block_14 = 0;
    block->guard_18 = 0xdeadbabe;
    block->guard_1c = 0xcafed00d;
    ++active_block_count_a0;
    return block;
}

// FUNCTION: SURRENDER 0x10035B50
void srHeap::releaseBlock(Block* block)
{
    checkBlock(block);
    if (block->allocation_size_04 == block_size_a4 && cached_block_9c == 0) {
        cached_block_9c = block;
    } else {
        freeSystemBlock(block);
    }
    --active_block_count_a0;
}

// FUNCTION: SURRENDER 0x10035BB0
void srHeap::freeAll()
{
    HeapAccess access(critical_section_b0);
    if (current_block_84 != 0) {
        releaseBlock(current_block_84);
    }
    Block* block = block_list_88;
    while (block != 0) {
        Block* next = block->next_08;
        releaseBlock(block);
        block = next;
    }
    block = block_list_98;
    while (block != 0) {
        Block* next = block->next_08;
        releaseBlock(block);
        block = next;
    }
    block = block_list_8c;
    while (block != 0) {
        Block* next = block->next_08;
        releaseBlock(block);
        block = next;
    }
    block = block_list_94;
    while (block != 0) {
        Block* next = block->next_08;
        releaseBlock(block);
        block = next;
    }
    releaseCachedBlock();
    current_block_84 = 0;
    block_list_88 = 0;
    block_list_98 = 0;
    block_list_8c = 0;
    block_list_94 = 0;
    memset(small_free_lists_00, 0, sizeof(small_free_lists_00));
}

// FUNCTION: SURRENDER 0x10035CB0
void srHeap::dump(std::ostream& stream)
{
    HeapAccess access(critical_section_b0);
    unsigned long total = 0;
    Block* block;
    for (block = block_list_88; block != 0; block = block->next_08) {
        srStreamPrintf(stream, "%p  bytes %-8d  (small heap)\n", block->allocation_00,
                       block->allocation_size_04);
        total += block->allocation_size_04;
    }
    block = current_block_84;
    if (block != 0) {
        srStreamPrintf(stream, "%p  bytes %-8d  (small heap/active)\n", block->allocation_00,
                       block->allocation_size_04);
        total += block->allocation_size_04;
    }
    for (block = block_list_94; block != 0; block = block->next_08) {
        srStreamPrintf(stream, "%p  bytes %-8d  (medium heap)\n", block->allocation_00,
                       block->allocation_size_04);
        total += block->allocation_size_04;
    }
    for (block = block_list_8c; block != 0; block = block->next_08) {
        srStreamPrintf(stream, "%p  bytes %-8d  (medium heap/partial)\n", block->allocation_00,
                       block->allocation_size_04);
        total += block->allocation_size_04;
    }
    for (block = block_list_98; block != 0; block = block->next_08) {
        srStreamPrintf(stream, "%p  bytes %-8d  (large heap)\n", block->allocation_00,
                       block->allocation_size_04);
        total += block->allocation_size_04;
    }
    srStreamPrintf(stream, "\nPages allocated : %d\n", active_block_count_a0);
    srStreamPrintf(stream, "Memory used     : %d kB\n", (total + 0x3ff) >> 10);
}

// FUNCTION: SURRENDER 0x10035E10
void srHeap::checkBlock(Block*) {}

// FUNCTION: SURRENDER 0x10035E20
void srHeap::freePooled(void* allocation)
{
    // reinterpret-ok: pooled chunks carry their 0x20-byte header immediately
    // before the user pointer.
    Chunk* chunk = reinterpret_cast<Chunk*>(allocation) - 1;
    Block* block = chunk->owner_00;
    checkBlock(block);
    int largest_below = block->largest_free_size_10 < 0x220;
    Chunk* previous = chunk->previous_08;
    Chunk* merged;
    if (previous == 0 || previous->free_18 == 0) {
        chunk->free_previous_10 = 0;
        Chunk* largest = block->largest_free_block_14;
        chunk->free_next_14 = largest;
        if (largest != 0) {
            largest->free_previous_10 = chunk;
        }
        block->largest_free_block_14 = chunk;
        block->largest_free_size_10 = chunk->size_04;
        chunk->free_18 = 1;
        merged = chunk;
    } else {
        previous->size_04 += chunk->size_04;
        previous->next_0c = chunk->next_0c;
        if (chunk->next_0c != 0) {
            chunk->next_0c->previous_08 = previous;
        }
        merged = previous;
    }
    Chunk* next = merged->next_0c;
    if (next != 0 && next->free_18 != 0) {
        merged->size_04 += next->size_04;
        merged->next_0c = next->next_0c;
        if (next->next_0c != 0) {
            next->next_0c->previous_08 = merged;
        }
        if (next->free_previous_10 != 0) {
            next->free_previous_10->free_next_14 = next->free_next_14;
        } else {
            block->largest_free_block_14 = next->free_next_14;
        }
        if (next->free_next_14 != 0) {
            next->free_next_14->free_previous_10 = next->free_previous_10;
        }
    }
    if (block->largest_free_block_14 == 0) {
        block->largest_free_size_10 = 0;
    } else {
        block->largest_free_size_10 = block->largest_free_block_14->size_04;
    }
    if (block->largest_free_size_10 < merged->size_04) {
        block->largest_free_size_10 = merged->size_04;
        if (merged->free_previous_10 != 0) {
            merged->free_previous_10->free_next_14 = merged->free_next_14;
        }
        if (merged->free_next_14 != 0) {
            merged->free_next_14->free_previous_10 = merged->free_previous_10;
        }
        Chunk* largest = block->largest_free_block_14;
        merged->free_next_14 = largest;
        merged->free_previous_10 = 0;
        if (largest != 0) {
            largest->free_previous_10 = merged;
        }
        block->largest_free_block_14 = merged;
    }
    if (largest_below && block->largest_free_size_10 >= 0x220) {
        if (block->previous_0c != 0) {
            block->previous_0c->next_08 = block->next_08;
        }
        if (block->next_08 != 0) {
            block->next_08->previous_0c = block->previous_0c;
        }
        if (block == block_list_94) {
            block_list_94 = block->next_08;
        }
        block->next_08 = 0;
        Block* previous = block_90;
        block->previous_0c = previous;
        if (previous != 0) {
            previous->next_08 = block;
        }
        block_90 = block;
        if (block->previous_0c == 0) {
            block_list_8c = block;
        }
    }
}

// FUNCTION: SURRENDER 0x10035F80
void* srHeap::splitFree(Block* block, unsigned long size)
{
    checkBlock(block);
    Chunk* chunk = block->largest_free_block_14;
    if (chunk->size_04 >= size + 0x20) {
        // reinterpret-ok: the carved chunk starts at a byte offset inside the
        // block's raw allocation payload.
        Chunk* carved =
            reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunk->size_04 - size);
        carved->owner_00 = block;
        carved->previous_08 = chunk;
        carved->next_0c = chunk->next_0c;
        carved->free_previous_10 = 0;
        carved->free_next_14 = 0;
        carved->size_04 = size;
        carved->free_18 = 0;
        if (chunk->next_0c != 0) {
            chunk->next_0c->previous_08 = carved;
        }
        unsigned long remaining = chunk->size_04 - size;
        chunk->size_04 = remaining;
        chunk->next_0c = carved;
        block->largest_free_size_10 = remaining;
        carved->tag_1f = '\xfe';
        return carved + 1;
    }
    if (chunk->free_previous_10 != 0) {
        chunk->free_previous_10->free_next_14 = chunk->free_next_14;
    } else {
        block->largest_free_block_14 = chunk->free_next_14;
    }
    if (chunk->free_next_14 != 0) {
        chunk->free_next_14->free_previous_10 = chunk->free_previous_10;
    }
    chunk->free_previous_10 = 0;
    chunk->free_next_14 = 0;
    chunk->free_18 = 0;
    block->largest_free_size_10 = 0;
    chunk->tag_1f = '\xfe';
    return chunk + 1;
}

// FUNCTION: SURRENDER 0x10036030
void* srHeap::allocatePooled(unsigned long size)
{
    Block* block = block_list_8c;
    unsigned long needed = (size + 0x1f & 0xffffffe0) + 0x20;
    while (block != 0) {
        if (needed <= block->largest_free_size_10) {
            break;
        }
        block = block->next_08;
    }
    if (block == 0) {
        block = allocateBlock(block_size_a4);
        if (block == 0) {
            return 0;
        }
        checkBlock(block);
        block->previous_0c = 0;
        Block* previous = block_list_8c;
        block->next_08 = previous;
        if (previous != 0) {
            previous->previous_0c = block;
        } else {
            block_90 = block;
        }
        block_list_8c = block;
        Chunk* chunk = static_cast<Chunk*>(block->allocation_00);
        block->largest_free_size_10 = block_size_a4;
        block->largest_free_block_14 = chunk;
        chunk->owner_00 = block;
        chunk->size_04 = block_size_a4;
        chunk->previous_08 = 0;
        chunk->next_0c = 0;
        chunk->free_previous_10 = 0;
        chunk->free_next_14 = 0;
        chunk->free_18 = 1;
    }
    void* allocation = splitFree(block, needed);
    if (block->largest_free_size_10 < 0x220) {
        if (block == block_90) {
            block_90 = block->previous_0c;
        }
        if (block == block_list_8c) {
            block_list_8c = block->next_08;
        }
        if (block->previous_0c != 0) {
            block->previous_0c->next_08 = block->next_08;
        }
        if (block->next_08 != 0) {
            block->next_08->previous_0c = block->previous_0c;
        }
        block->previous_0c = 0;
        Block* next = block_list_94;
        block->next_08 = next;
        if (next != 0) {
            next->previous_0c = block;
        }
        block_list_94 = block;
        return allocation;
    }
    if (block != block_list_8c) {
        block_90->next_08 = block_list_8c;
        block_list_8c->previous_0c = block_90;
        block_90 = block->previous_0c;
        block->previous_0c->next_08 = 0;
        block->previous_0c = 0;
        block_list_8c = block;
    }
    return allocation;
}

// FUNCTION: SURRENDER 0x10036190
void srHeap::freeSystem(void* allocation)
{
    // reinterpret-ok: the owning block pointer sits five bytes under the user
    // pointer, immediately before the 0xff tag byte.
    Block* block = *reinterpret_cast<Block**>(static_cast<char*>(allocation) - 5);
    if (block->previous_0c != 0) {
        block->previous_0c->next_08 = block->next_08;
    }
    if (block->next_08 != 0) {
        block->next_08->previous_0c = block->previous_0c;
    }
    if (block == block_list_98) {
        block_list_98 = block->next_08;
    }
    releaseBlock(block);
}

// FUNCTION: SURRENDER 0x100361D0
void* srHeap::allocateSystem(unsigned long size)
{
    Block* block = allocateBlock(size + 0x20);
    if (block == 0) {
        return 0;
    }
    char* tag = static_cast<char*>(block->allocation_00) + 0x1f;
    *tag = '\xff';
    // reinterpret-ok: the unaligned back-pointer slot overlaps the chunk tail;
    // freeSystem() and msize() read it through the same byte offset.
    *reinterpret_cast<Block**>(tag - 4) = block;
    block->previous_0c = 0;
    Block* previous = block_list_98;
    block->next_08 = previous;
    if (previous != 0) {
        previous->previous_0c = block;
    }
    block_list_98 = block;
    return tag + 1;
}

// FUNCTION: SURRENDER 0x10036220
unsigned long srHeap::msize(void* allocation)
{
    srCriticalSection* lock = critical_section_b0;
    lock->getAccess();
    if (allocation == 0) {
        lock->releaseAccess();
        return 0;
    }
    // reinterpret-ok: allocation tag byte immediately preceding the user area.
    unsigned char tag = *(static_cast<unsigned char*>(allocation) - 1);
    switch (tag) {
    case 0xfe:
        lock->releaseAccess();
        // reinterpret-ok: pooled chunks carry their 0x20-byte header
        // immediately before the user pointer.
        return (reinterpret_cast<Chunk*>(allocation) - 1)->size_04;
    case 0xff:
        lock->releaseAccess();
        // reinterpret-ok: system allocations carry their block pointer five
        // bytes under the user pointer.
        return (*reinterpret_cast<Block**>(static_cast<char*>(allocation) - 5))->allocation_size_04;
    default:
        lock->releaseAccess();
        return (tag << 4 | 0xf) + 1;
    }
}

// FUNCTION: SURRENDER 0x100362A0
void* srHeap::allocate(unsigned long size)
{
    if (size == 0) {
        size = 1;
    }
    srCriticalSection* lock = critical_section_b0;
    lock->getAccess();
    void* result;
    if (size < 0x200) {
        size |= 0xf;
        unsigned long index = size >> 4;
        char* chunk = static_cast<char*>(small_free_lists_00[index]);
        if (chunk == 0) {
            if (current_block_84 == 0) {
                current_block_84 = allocateBlock(block_size_a4);
                current_block_offset_80 = 0xf;
            }
            if (block_size_a4 - current_block_offset_80 <= size) {
                current_block_84->next_08 = block_list_88;
                block_list_88 = current_block_84;
                current_block_84 = allocateBlock(block_size_a4);
                if (current_block_84 == 0) {
                    lock->releaseAccess();
                    return 0;
                }
                current_block_offset_80 = 0xf;
            }
            char* chunk =
                static_cast<char*>(current_block_84->allocation_00) + current_block_offset_80;
            *chunk = static_cast<char>(index);
            current_block_offset_80 += size + 1;
            lock->releaseAccess();
            return chunk + 1;
        }
        small_free_lists_00[index] = *reinterpret_cast<void**>(chunk + 1);
        result = chunk + 1;
    } else if (size < 0x4000) {
        result = allocatePooled(size);
    } else {
        result = allocateSystem(size);
    }
    lock->releaseAccess();
    return result;
}

// FUNCTION: SURRENDER 0x100363E0
void srHeap::free(void* allocation)
{
    if (allocation != 0) {
        HeapAccess access(critical_section_b0);
        // reinterpret-ok: allocation tag byte immediately preceding the user
        // area.
        unsigned char* tag = reinterpret_cast<unsigned char*>(allocation) - 1;
        switch (*tag) {
        case 0xfe:
            freePooled(allocation);
            break;
        case 0xff:
            freeSystem(allocation);
            break;
        default:
            // reinterpret-ok: the freed user word stores the next free pointer.
            *reinterpret_cast<void**>(allocation) = small_free_lists_00[*tag];
            small_free_lists_00[*tag] = tag;
            break;
        }
    }
}

// FUNCTION: SURRENDER 0x10036470
void srHeap::free(void* allocation, unsigned int)
{
    free(allocation);
}

// GLOBAL: SURRENDER 0x100A48D0
class srHeap srHeap;
