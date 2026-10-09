#include "surrender/srHeap.h"
#include "surrender/srMemoryAllocator.h"

#include <stdlib.h>
#include <string.h>

#include "surrender/srDebug.h"

#pragma intrinsic(memset)

// FUNCTION: SURRENDER 0x100359C0
srHeap::srHeap()
{
    critical_section = new srCriticalSection;
    system_block_count = 0;
    block_sequence = 0;
    active_block_count = 0;
    large_blocks = 0;
    cached_block = 0;
    block_size = 0x7fe0;
    memset(small_free_lists, 0, sizeof(small_free_lists));
    small_blocks = 0;
    current_block = 0;
    current_block_offset = 0;
    partial_blocks = 0;
    block = 0;
    medium_blocks = 0;
}

// FUNCTION: SURRENDER 0x10035A50
srHeap::~srHeap()
{
    freeAll();
    delete critical_section;
}

// FUNCTION: SURRENDER 0x10035A90
void srHeap::freeSystemBlock(void* allocation)
{
    ::free(allocation);
}

// FUNCTION: SURRENDER 0x10035AA0
void srHeap::releaseCachedBlock()
{
    if (cached_block != 0) {
        freeSystemBlock(cached_block);
    }
    cached_block = 0;
}

// FUNCTION: SURRENDER 0x10035AC0
srHeap::Block* srHeap::allocateBlock(w8_ulong size)
{
    w8_ulong allocation_size = size + 0x20;
    Block* block = cached_block;
    ++block_sequence;
    if (block == 0 || block->alloc_size != allocation_size) {
        block = static_cast<Block*>(malloc(allocation_size + sizeof(Block)));
        if (block == 0) {
            return 0;
        }
        // reinterpret-ok: block header pointer rounding to the 0x20-aligned
        // allocation payload.
        block->allocation = reinterpret_cast<void*>((reinterpret_cast<w8_ulong_ptr>(block) + 0x3f) &
                                                    ~static_cast<w8_ulong_ptr>(0x1f));
        block->alloc_size = allocation_size;
        ++system_block_count;
    } else {
        cached_block = 0;
    }
    block->previous = 0;
    block->next = 0;
    block->largest_free_size = 0;
    block->largest_free_block = 0;
    block->guard0 = 0xdeadbabe;
    block->guard1 = 0xcafed00d;
    ++active_block_count;
    return block;
}

// FUNCTION: SURRENDER 0x10035B50
void srHeap::releaseBlock(Block* block)
{
    checkBlock(block);
    if (block->alloc_size == block_size && cached_block == 0) {
        cached_block = block;
    } else {
        freeSystemBlock(block);
    }
    --active_block_count;
}

// FUNCTION: SURRENDER 0x10035BB0
void srHeap::freeAll()
{
    srCriticalSectionAccess access(critical_section);
    if (current_block != 0) {
        releaseBlock(current_block);
    }
    Block* block = small_blocks;
    while (block != 0) {
        Block* next = block->next;
        releaseBlock(block);
        block = next;
    }
    block = large_blocks;
    while (block != 0) {
        Block* next = block->next;
        releaseBlock(block);
        block = next;
    }
    block = partial_blocks;
    while (block != 0) {
        Block* next = block->next;
        releaseBlock(block);
        block = next;
    }
    block = medium_blocks;
    while (block != 0) {
        Block* next = block->next;
        releaseBlock(block);
        block = next;
    }
    releaseCachedBlock();
    current_block = 0;
    small_blocks = 0;
    large_blocks = 0;
    partial_blocks = 0;
    medium_blocks = 0;
    memset(small_free_lists, 0, sizeof(small_free_lists));
}

// FUNCTION: SURRENDER 0x10035CB0
void srHeap::dump(std::ostream& stream)
{
    srCriticalSectionAccess access(critical_section);
    w8_ulong total = 0;
    Block* block;
    for (block = small_blocks; block != 0; block = block->next) {
        srStreamPrintf(stream, "%p  bytes %-8d  (small heap)\n", block->allocation,
                       block->alloc_size);
        total += block->alloc_size;
    }
    block = current_block;
    if (block != 0) {
        srStreamPrintf(stream, "%p  bytes %-8d  (small heap/active)\n", block->allocation,
                       block->alloc_size);
        total += block->alloc_size;
    }
    for (block = medium_blocks; block != 0; block = block->next) {
        srStreamPrintf(stream, "%p  bytes %-8d  (medium heap)\n", block->allocation,
                       block->alloc_size);
        total += block->alloc_size;
    }
    for (block = partial_blocks; block != 0; block = block->next) {
        srStreamPrintf(stream, "%p  bytes %-8d  (medium heap/partial)\n", block->allocation,
                       block->alloc_size);
        total += block->alloc_size;
    }
    for (block = large_blocks; block != 0; block = block->next) {
        srStreamPrintf(stream, "%p  bytes %-8d  (large heap)\n", block->allocation,
                       block->alloc_size);
        total += block->alloc_size;
    }
    srStreamPrintf(stream, "\nPages allocated : %d\n", active_block_count);
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
    Block* block = chunk->owner;
    checkBlock(block);
    int largest_below = block->largest_free_size < 0x220;
    Chunk* previous = chunk->previous;
    Chunk* merged;
    if (previous == 0 || previous->free == 0) {
        chunk->free_previous = 0;
        Chunk* largest = block->largest_free_block;
        chunk->free_next = largest;
        if (largest != 0) {
            largest->free_previous = chunk;
        }
        block->largest_free_block = chunk;
        block->largest_free_size = chunk->size;
        chunk->free = 1;
        merged = chunk;
    } else {
        previous->size += chunk->size;
        previous->next = chunk->next;
        if (chunk->next != 0) {
            chunk->next->previous = previous;
        }
        merged = previous;
    }
    Chunk* next = merged->next;
    if (next != 0 && next->free != 0) {
        merged->size += next->size;
        merged->next = next->next;
        if (next->next != 0) {
            next->next->previous = merged;
        }
        if (next->free_previous != 0) {
            next->free_previous->free_next = next->free_next;
        } else {
            block->largest_free_block = next->free_next;
        }
        if (next->free_next != 0) {
            next->free_next->free_previous = next->free_previous;
        }
    }
    if (block->largest_free_block == 0) {
        block->largest_free_size = 0;
    } else {
        block->largest_free_size = block->largest_free_block->size;
    }
    if (block->largest_free_size < merged->size) {
        block->largest_free_size = merged->size;
        if (merged->free_previous != 0) {
            merged->free_previous->free_next = merged->free_next;
        }
        if (merged->free_next != 0) {
            merged->free_next->free_previous = merged->free_previous;
        }
        Chunk* largest = block->largest_free_block;
        merged->free_next = largest;
        merged->free_previous = 0;
        if (largest != 0) {
            largest->free_previous = merged;
        }
        block->largest_free_block = merged;
    }
    if (largest_below && block->largest_free_size >= 0x220) {
        if (block->previous != 0) {
            block->previous->next = block->next;
        }
        if (block->next != 0) {
            block->next->previous = block->previous;
        }
        if (block == medium_blocks) {
            medium_blocks = block->next;
        }
        block->next = 0;
        Block* previous = this->block;
        block->previous = previous;
        if (previous != 0) {
            previous->next = block;
        }
        this->block = block;
        if (block->previous == 0) {
            partial_blocks = block;
        }
    }
}

// FUNCTION: SURRENDER 0x10035F80
void* srHeap::splitFree(Block* block, w8_ulong size)
{
    checkBlock(block);
    Chunk* chunk = block->largest_free_block;
    if (chunk->size >= size + 0x20) {
        // reinterpret-ok: the carved chunk starts at a byte offset inside the
        // block's raw allocation payload.
        Chunk* carved =
            reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunk->size - size);
        carved->owner = block;
        carved->previous = chunk;
        carved->next = chunk->next;
        carved->free_previous = 0;
        carved->free_next = 0;
        carved->size = size;
        carved->free = 0;
        if (chunk->next != 0) {
            chunk->next->previous = carved;
        }
        w8_ulong remaining = chunk->size - size;
        chunk->size = remaining;
        chunk->next = carved;
        block->largest_free_size = remaining;
        carved->tag = '\xfe';
        return carved + 1;
    }
    if (chunk->free_previous != 0) {
        chunk->free_previous->free_next = chunk->free_next;
    } else {
        block->largest_free_block = chunk->free_next;
    }
    if (chunk->free_next != 0) {
        chunk->free_next->free_previous = chunk->free_previous;
    }
    chunk->free_previous = 0;
    chunk->free_next = 0;
    chunk->free = 0;
    block->largest_free_size = 0;
    chunk->tag = '\xfe';
    return chunk + 1;
}

// FUNCTION: SURRENDER 0x10036030
void* srHeap::allocatePooled(w8_ulong size)
{
    Block* block = partial_blocks;
    w8_ulong needed = (size + 0x1f & 0xffffffe0) + 0x20;
    while (block != 0) {
        if (needed <= block->largest_free_size) {
            break;
        }
        block = block->next;
    }
    if (block == 0) {
        block = allocateBlock(block_size);
        if (block == 0) {
            return 0;
        }
        checkBlock(block);
        block->previous = 0;
        Block* previous = partial_blocks;
        block->next = previous;
        if (previous != 0) {
            previous->previous = block;
        } else {
            this->block = block;
        }
        partial_blocks = block;
        Chunk* chunk = static_cast<Chunk*>(block->allocation);
        block->largest_free_size = block_size;
        block->largest_free_block = chunk;
        chunk->owner = block;
        chunk->size = block_size;
        chunk->previous = 0;
        chunk->next = 0;
        chunk->free_previous = 0;
        chunk->free_next = 0;
        chunk->free = 1;
    }
    void* allocation = splitFree(block, needed);
    if (block->largest_free_size < 0x220) {
        if (block == this->block) {
            this->block = block->previous;
        }
        if (block == partial_blocks) {
            partial_blocks = block->next;
        }
        if (block->previous != 0) {
            block->previous->next = block->next;
        }
        if (block->next != 0) {
            block->next->previous = block->previous;
        }
        block->previous = 0;
        Block* next = medium_blocks;
        block->next = next;
        if (next != 0) {
            next->previous = block;
        }
        medium_blocks = block;
        return allocation;
    }
    if (block != partial_blocks) {
        this->block->next = partial_blocks;
        partial_blocks->previous = this->block;
        this->block = block->previous;
        block->previous->next = 0;
        block->previous = 0;
        partial_blocks = block;
    }
    return allocation;
}

// FUNCTION: SURRENDER 0x10036190
void srHeap::freeSystem(void* allocation)
{
    // reinterpret-ok: the owning block pointer sits five bytes under the user
    // pointer, immediately before the 0xff tag byte.
    Block* block = *reinterpret_cast<Block**>(static_cast<char*>(allocation) - 5);
    if (block->previous != 0) {
        block->previous->next = block->next;
    }
    if (block->next != 0) {
        block->next->previous = block->previous;
    }
    if (block == large_blocks) {
        large_blocks = block->next;
    }
    releaseBlock(block);
}

// FUNCTION: SURRENDER 0x100361D0
void* srHeap::allocateSystem(w8_ulong size)
{
    Block* block = allocateBlock(size + 0x20);
    if (block == 0) {
        return 0;
    }
    char* tag = static_cast<char*>(block->allocation) + 0x1f;
    *tag = '\xff';
    // reinterpret-ok: the unaligned back-pointer slot overlaps the chunk tail;
    // freeSystem() and msize() read it through the same byte offset.
    *reinterpret_cast<Block**>(tag - 4) = block;
    block->previous = 0;
    Block* previous = large_blocks;
    block->next = previous;
    if (previous != 0) {
        previous->previous = block;
    }
    large_blocks = block;
    return tag + 1;
}

// FUNCTION: SURRENDER 0x10036220
w8_ulong srHeap::msize(void* allocation)
{
    srCriticalSection* lock = critical_section;
    lock->getAccess();
    if (allocation == 0) {
        lock->releaseAccess();
        return 0;
    }
    // reinterpret-ok: allocation tag byte immediately preceding the user area.
    unsigned char tag = *(static_cast<unsigned char*>(allocation) - 1);
    switch (tag) {
    case 0xfe: {
        // reinterpret-ok: pooled chunks carry their 0x20-byte header
        // immediately before the user pointer.
        w8_ulong size = (reinterpret_cast<Chunk*>(allocation) - 1)->size;
        lock->releaseAccess();
        return size;
    }
    case 0xff: {
        // reinterpret-ok: system allocations carry their block pointer five
        // bytes under the user pointer.
        w8_ulong size =
            (*reinterpret_cast<Block**>(static_cast<char*>(allocation) - 5))->alloc_size;
        lock->releaseAccess();
        return size;
    }
    default:
        lock->releaseAccess();
        return (tag << 4 | 0xf) + 1;
    }
}

// FUNCTION: SURRENDER 0x100362A0
void* srHeap::allocate(w8_ulong size)
{
    if (size == 0) {
        size = 1;
    }
    srCriticalSection* lock = critical_section;
    lock->getAccess();
    void* result;
    if (size < 0x200) {
        w8_ulong index = size >> 4;
        char* chunk = static_cast<char*>(small_free_lists[index]);
        if (chunk == 0) {
            if (current_block == 0) {
                current_block = allocateBlock(block_size);
                current_block_offset = 0xf;
            }
            if (block_size - current_block_offset <= (size | 0xf)) {
                current_block->next = small_blocks;
                small_blocks = current_block;
                current_block = allocateBlock(block_size);
                if (current_block == 0) {
                    lock->releaseAccess();
                    return 0;
                }
                current_block_offset = 0xf;
            }
            char* chunk = static_cast<char*>(current_block->allocation) + current_block_offset;
            *chunk = static_cast<char>(index);
            current_block_offset += (size | 0xf) + 1;
            lock->releaseAccess();
            return chunk + 1;
        }
        small_free_lists[index] = *reinterpret_cast<void**>(chunk + 1);
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
        srCriticalSectionAccess access(critical_section);
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
            *reinterpret_cast<void**>(allocation) = small_free_lists[*tag];
            small_free_lists[*tag] = tag;
            break;
        }
    }
}

// FUNCTION: SURRENDER 0x10036470
void srHeap::free(void* allocation, unsigned int)
{
    free(allocation);
}

// FUNCTION: SURRENDER 0x10036500
srMemoryAllocator::srMemoryAllocator()
{
    first_block = 0;
    allocated_bytes = 0;
    allocation_count = 0;
    alignment = ALIGN_SIZE_32;
    clear = 1;
}

// FUNCTION: SURRENDER 0x10036520
srMemoryAllocator::~srMemoryAllocator() {}

// FUNCTION: SURRENDER 0x100042D0
void srMemoryAllocator::setAlignment(e_alignSize alignment)
{
    this->alignment = alignment;
}

// FUNCTION: SURRENDER 0x10036530
srMemoryAllocator::Block* srMemoryAllocator::align(void* allocation)
{
    /* The header precedes the aligned user address (0x20 bytes on Windows). */
    // reinterpret-ok: block alignment is computed on the raw allocation bits.
    return reinterpret_cast<Block*>(
        // reinterpret-ok: the system-block header precedes its aligned allocation
        ((reinterpret_cast<w8_ulong_ptr>(allocation) + alignment + sizeof(Block) - 1) &
         ~static_cast<w8_ulong_ptr>(alignment - 1)) -
        sizeof(Block));
}

// FUNCTION: SURRENDER 0x10036570
void* srMemoryAllocator::allocate(w8_ulong count, w8_ulong size, const char* name)
{
    w8_ulong requested = count * size;
    w8_ulong allocation_size = alignment + sizeof(Block) - 1 + requested;
    if (name != 0) {
        allocation_size += strlen(name) + 1;
    }
    void* raw = operator new(allocation_size);
    memset(raw, 0, allocation_size);
    if (raw == 0) {
        return 0;
    }
    Block* block = align(raw);
    block->requested_size = requested;
    block->raw_allocation = raw;
    block->total_size = allocation_size;
    if (name == 0) {
        block->name = 0;
    } else {
        // reinterpret-ok: the name string is stored right after the user area.
        block->name = reinterpret_cast<char*>(block) + sizeof(Block) + requested;
        strcpy(block->name, name);
    }
    block->next = first_block;
    block->previous = 0;
    if (first_block != 0) {
        first_block->previous = block;
    }
    first_block = block;
    ++allocation_count;
    allocated_bytes += block->total_size;
    return block + 1;
}

// FUNCTION: SURRENDER 0x10036550
void* srMemoryAllocator::allocate(w8_ulong size, const char* name)
{
    return allocate(1, size, name);
}

// FUNCTION: SURRENDER 0x100366F0
w8_ulong srMemoryAllocator::getSize(void* allocation) const
{
    return (static_cast<Block*>(allocation) - 1)->requested_size;
}

// FUNCTION: SURRENDER 0x10036700
const char* srMemoryAllocator::getName(void* allocation) const
{
    return (static_cast<Block*>(allocation) - 1)->name;
}

// FUNCTION: SURRENDER 0x10036710
void srMemoryAllocator::dump() const
{
    srPrintf("Memory dump\n");
    srPrintf("\nAddress      Size    Tag  Name\n");
    srPrintf("-------------------------------------------------------------------\n");
    for (Block* block = first_block; block != 0; block = block->next) {
        const char* name = block->name;
        if (name == 0) {
            name = "<unnamed>";
        }
        srPrintf("%8p %8d %s\n", block + 1, block->requested_size, name);
    }
    srPrintf("-------------------------------------------------------------------\n");
    srPrintf("Total memory used %d bytes (%d Kb) for %d entries.\n", allocated_bytes,
             (w8_long)(allocated_bytes + 0x3ff) / 1024, allocation_count);
    srPrintf("Alignment: %d Clear: %s\n", alignment, clear != 0 ? "Yes" : "No");
}

// FUNCTION: SURRENDER 0x100366A0
void srMemoryAllocator::free(void* allocation)
{
    Block* block = static_cast<Block*>(allocation) - 1;
    if (block->next != 0) {
        block->next->previous = block->previous;
    }
    if (block->previous != 0) {
        block->previous->next = block->next;
    }
    if (block == first_block) {
        first_block = block->next;
    }
    allocated_bytes -= block->total_size;
    --allocation_count;
    operator delete(block->raw_allocation);
}

// GLOBAL: SURRENDER 0x100A48D0
class srHeap srHeap;
