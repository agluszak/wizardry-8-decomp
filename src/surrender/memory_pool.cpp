#include "surrender/srMemoryPool.h"

#include "surrender/srDebug.h"

// FUNCTION: SURRENDER 0x10036850
srMemoryPool::Entry* srMemoryPool::addEntry(Entry* previous, Entry* next)
{
    Entry* entry = new Entry;
    entry->previous = previous;
    entry->next = next;
    if (previous != 0) {
        previous->next = entry;
    }
    if (next != 0) {
        next->previous = entry;
    }
    return entry;
}

// FUNCTION: SURRENDER 0x10036AA0
void* srMemoryPool::allocate(w8_long size)
{
    if (size < 1) {
        return 0;
    }
    w8_long aligned = ((alignment - 1 + size) / alignment) * alignment;
    Entry* space = findSpace(aligned);
    if (space == 0) {
        return 0;
    }
    w8_long offset = space->offset;
    w8_ulong bucket = hashVal(offset);
    Entry* entry = addEntry(0, allocations[bucket]);
    allocations[bucket] = entry;
    largest_free_dirty = 1;
    used += aligned;
    space->size -= aligned;
    space->offset += aligned;
    entry->size = aligned;
    entry->offset = offset;
    entry->locked = 0;
    if (space->size == 0) {
        space->previous->next = space->next;
        space->next->previous = space->previous;
        if (space == first_free) {
            first_free = space->next;
        }
        delete space;
    }
    return static_cast<char*>(memory) + offset;
}

// FUNCTION: SURRENDER 0x10036DE0
srMemoryPool::srMemoryPool(void* memory, w8_long size, w8_long alignment)
{
    this->memory = memory;
    /* reinterpret-ok: pool alignment arithmetic requires the raw address. */
    // reinterpret-ok: the historical allocator represents area locations as integer addresses
    w8_long padding = static_cast<w8_long>(reinterpret_cast<ptrdiff_t>(memory) % alignment);
    this->alignment = alignment;
    if (padding > 0) {
        padding = alignment - padding;
    }
    this->size = size - padding;
    srZeroMemory(allocations, sizeof(allocations));
    Entry* entry = addEntry(0, 0);
    first_free = entry;
    entry->size = this->size;
    entry->offset = padding;
    entry = addEntry(first_free, 0);
    entry->size = 0;
    entry->offset = padding + 1 + this->size;
    entry->locked = 1;
    entry = addEntry(0, first_free);
    first_free = entry;
    entry->size = 0;
    entry->locked = 1;
    entry->offset = padding - 1;
    largest_free_dirty = 1;
    used = 0;
    policy = FIT_FIRST;
}

// FUNCTION: SURRENDER 0x10036D80
srMemoryPool::~srMemoryPool()
{
    Entry* entry = first_free;
    while (entry != 0) {
        Entry* next = entry->next;
        delete entry;
        entry = next;
    }
    for (w8_ulong i = 0; i < 256; i++) {
        entry = allocations[i];
        while (entry != 0) {
            Entry* next = entry->next;
            delete entry;
            entry = next;
        }
    }
    memory = 0;
    size = 0;
    first_free = 0;
    largest_free = 0;
}

// FUNCTION: SURRENDER 0x10036BB0
void srMemoryPool::defrag(Entry* entry)
{
    Entry* cursor = entry->previous;
    while (cursor != 0 && cursor->size + cursor->offset == entry->offset) {
        entry = cursor;
        cursor = cursor->previous;
    }
    for (Entry* next = entry->next; next != 0 && entry->offset + entry->size == next->offset;
         next = next->next) {
        entry->size += next->size;
        next->size = -1;
    }
    Entry* dead = entry->next;
    while (dead != 0 && dead->size == -1) {
        Entry* next = dead->next;
        dead->previous->next = next;
        next->previous = dead->previous;
        if (dead == first_free) {
            first_free = next;
        }
        delete dead;
        dead = next;
    }
}

// FUNCTION: SURRENDER 0x10036F00
void srMemoryPool::dump()
{
    srPrintf("Free blocks:\n\n");
    for (Entry* entry = first_free; entry != 0; entry = entry->next) {
        srPrintf("%08p %06d\n", static_cast<char*>(memory) + entry->offset, entry->size);
    }
    srPrintf("\nUsed blocks:\n\n");
    for (w8_ulong i = 0; i < 256; i++) {
        for (Entry* entry = allocations[i]; entry != 0; entry = entry->next) {
            srPrintf("%08p %06d (%03d)\n", static_cast<char*>(memory) + entry->offset, entry->size,
                     hashVal(entry->offset));
        }
    }
    srPrintf("memory  used       : %d bytes (%d kB)\n", used, used / 1024);
    w8_long free_total = size - used;
    srPrintf("memory  free       : %d bytes (%d kB)\n", free_total, free_total / 1024);
    srPrintf("largest free block : %d bytes (%d kB)\n", memAvail(), memAvail() / 1024);
    srPrintf("total memory       : %d bytes (%d kB)\n", size, size / 1024);
    srPrintf("pool memory start address : %p\n", memory);
}

// FUNCTION: SURRENDER 0x10036930
srMemoryPool::Entry* srMemoryPool::find(w8_long offset) const
{
    if (offset >= 0 && offset < size) {
        for (Entry* entry = allocations[hashVal(offset)]; entry != 0; entry = entry->next) {
            if (entry->offset == offset) {
                return entry;
            }
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100369A0
srMemoryPool::Entry* srMemoryPool::findArea(w8_long offset) const
{
    for (Entry* entry = first_free; entry != 0; entry = entry->next) {
        if (entry->offset <= offset && offset < entry->size + entry->offset) {
            return entry;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10036880
srMemoryPool::Entry* srMemoryPool::findBestFit(w8_long size) const
{
    Entry* best = 0;
    w8_long best_size = 0x40000000;
    for (Entry* entry = first_free; entry != 0; entry = entry->next) {
        w8_long entry_size = entry->size;
        if (entry_size >= size && entry_size < best_size) {
            if (entry_size == size) {
                return entry;
            }
            best = entry;
            best_size = entry_size;
        }
    }
    return best;
}

// FUNCTION: SURRENDER 0x100368C0
srMemoryPool::Entry* srMemoryPool::findFirstFit(w8_long size) const
{
    for (Entry* entry = first_free; entry != 0; entry = entry->next) {
        if (size <= entry->size) {
            return entry;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10036B90
srMemoryPool::Entry* srMemoryPool::findPlacing(w8_long offset) const
{
    Entry* previous = 0;
    for (Entry* entry = first_free; entry != 0 && entry->offset <= offset; entry = entry->next) {
        previous = entry;
    }
    return previous;
}

// FUNCTION: SURRENDER 0x100368E0
srMemoryPool::Entry* srMemoryPool::findSpace(w8_long size) const
{
    switch (policy) {
    case FIT_FIRST:
        return findFirstFit(size);
    case FIT_BEST:
        return findBestFit(size);
    default:
        return 0;
    }
}

// FUNCTION: SURRENDER 0x10036D50
void srMemoryPool::free(void* allocation)
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        freeInternal(entry);
    }
}

// FUNCTION: SURRENDER 0x10036CA0
void srMemoryPool::freeInternal(Entry* entry)
{
    if (entry->locked == 0) {
        Entry* placing = findPlacing(entry->offset);
        if (placing->offset + placing->size == entry->offset) {
            placing->size += entry->size;
        } else {
            placing = addEntry(placing, placing->next);
            if (placing == 0) {
                return;
            }
            placing->size = entry->size;
            placing->offset = entry->offset;
        }
        used -= entry->size;
        if (entry->previous != 0) {
            entry->previous->next = entry->next;
        }
        if (entry->next != 0) {
            entry->next->previous = entry->previous;
        }
        w8_ulong bucket = hashVal(entry->offset);
        if (entry == allocations[bucket]) {
            allocations[bucket] = entry->next;
        }
        delete entry;
        defrag(placing);
        largest_free_dirty = 1;
    }
}

// FUNCTION: SURRENDER 0x10036990
w8_long srMemoryPool::getAlignment() const
{
    return alignment;
}

// FUNCTION: SURRENDER 0x10036C40
int srMemoryPool::getLockStatus(void* allocation) const
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        return entry->locked;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10036980
srMemoryPool::e_fit srMemoryPool::getPolicy() const
{
    return policy;
}

// FUNCTION: SURRENDER 0x10036960
w8_long srMemoryPool::getSize() const
{
    return size;
}

// FUNCTION: SURRENDER 0x10036B70
w8_long srMemoryPool::getSize(const void* allocation) const
{
    Entry* entry = find(convertPtr(allocation));
    if (entry == 0) {
        return 0;
    }
    return entry->size;
}

// FUNCTION: SURRENDER 0x10036C60
void srMemoryPool::lock(void* allocation)
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        entry->locked = 1;
    }
}

// FUNCTION: SURRENDER 0x100369F0
int srMemoryPool::maskArea(const void* memory, w8_long size)
{
    w8_long offset = convertPtr(memory);
    if (size < 1) {
        return 0;
    }
    Entry* area = findArea(offset);
    if (area == 0) {
        return 0;
    }
    w8_long old_size = area->size;
    used += size;
    largest_free_dirty = 1;
    /* reinterpret-ok: stores the absolute pointer minus the relative entry offset. */
    // reinterpret-ok: the historical allocator represents area locations as integer addresses
    area->size = reinterpret_cast<w8_long>(memory) - area->offset;
    w8_long end = offset + size;
    if (end < area->offset + old_size) {
        Entry* tail = addEntry(area, area->next);
        tail->size = (old_size - area->size) - size;
        tail->offset = end;
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10036EC0
w8_long srMemoryPool::memAvail()
{
    if (!largest_free_dirty) {
        return largest_free;
    }
    w8_long largest = 0;
    for (Entry* entry = first_free; entry != 0; entry = entry->next) {
        if (largest < entry->size) {
            largest = entry->size;
        }
    }
    largest_free = largest;
    largest_free_dirty = 0;
    return largest;
}

// FUNCTION: SURRENDER 0x10036910
w8_long srMemoryPool::memFreeTotal() const
{
    return size - used;
}

// FUNCTION: SURRENDER 0x10036920
w8_long srMemoryPool::memUsed() const
{
    return used;
}

// FUNCTION: SURRENDER 0x10036970
void srMemoryPool::setPolicy(e_fit policy)
{
    this->policy = policy;
}

// FUNCTION: SURRENDER 0x10036C80
void srMemoryPool::unlock(void* allocation)
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        entry->locked = 0;
    }
}
