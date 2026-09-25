#include "surrender/srMemoryPool.h"

#include "surrender/srDebug.h"

// FUNCTION: SURRENDER 0x10036850
srMemoryPool::Entry* srMemoryPool::addEntry(Entry* previous, Entry* next)
{
    Entry* entry = new Entry;
    entry->previous_00 = previous;
    entry->next_04 = next;
    if (previous != 0) {
        previous->next_04 = entry;
    }
    if (next != 0) {
        next->previous_00 = entry;
    }
    return entry;
}

// FUNCTION: SURRENDER 0x10036AA0
void* srMemoryPool::allocate(long size)
{
    if (size < 1) {
        return 0;
    }
    long aligned = ((alignment_14 - 1 + size) / alignment_14) * alignment_14;
    Entry* space = findSpace(aligned);
    if (space == 0) {
        return 0;
    }
    long offset = space->offset_08;
    unsigned long bucket = hashVal(offset);
    Entry* entry = addEntry(0, allocations_1c[bucket]);
    allocations_1c[bucket] = entry;
    largest_free_dirty_41c = 1;
    used_0c += aligned;
    space->size_0c -= aligned;
    space->offset_08 += aligned;
    entry->size_0c = aligned;
    entry->offset_08 = offset;
    entry->locked_10 = 0;
    if (space->size_0c == 0) {
        space->previous_00->next_04 = space->next_04;
        space->next_04->previous_00 = space->previous_00;
        if (space == first_free_18) {
            first_free_18 = space->next_04;
        }
        delete space;
    }
    return static_cast<char*>(memory_08) + offset;
}

// FUNCTION: SURRENDER 0x10036DE0
srMemoryPool::srMemoryPool(void* memory, long size, long alignment)
{
    memory_08 = memory;
    /* reinterpret-ok: pool alignment arithmetic requires the raw address. */
    long padding = reinterpret_cast<long>(memory) % alignment;
    alignment_14 = alignment;
    if (padding > 0) {
        padding = alignment - padding;
    }
    size_04 = size - padding;
    srZeroMemory(allocations_1c, sizeof(allocations_1c));
    Entry* entry = addEntry(0, 0);
    first_free_18 = entry;
    entry->size_0c = size_04;
    entry->offset_08 = padding;
    entry = addEntry(first_free_18, 0);
    entry->size_0c = 0;
    entry->offset_08 = padding + 1 + size_04;
    entry->locked_10 = 1;
    entry = addEntry(0, first_free_18);
    first_free_18 = entry;
    entry->size_0c = 0;
    entry->locked_10 = 1;
    entry->offset_08 = padding - 1;
    largest_free_dirty_41c = 1;
    used_0c = 0;
    policy_00 = FIT_FIRST;
}

// FUNCTION: SURRENDER 0x10036D80
srMemoryPool::~srMemoryPool()
{
    Entry* entry = first_free_18;
    while (entry != 0) {
        Entry* next = entry->next_04;
        delete entry;
        entry = next;
    }
    for (unsigned long i = 0; i < 256; i++) {
        entry = allocations_1c[i];
        while (entry != 0) {
            Entry* next = entry->next_04;
            delete entry;
            entry = next;
        }
    }
    memory_08 = 0;
    size_04 = 0;
    first_free_18 = 0;
    largest_free_10 = 0;
}

// FUNCTION: SURRENDER 0x10036A80
srMemoryPool& srMemoryPool::operator=(const srMemoryPool& pool)
{
    memcpy(this, &pool, sizeof(srMemoryPool));
    return *this;
}

// FUNCTION: SURRENDER 0x10036BB0
void srMemoryPool::defrag(Entry* entry)
{
    Entry* cursor = entry->previous_00;
    while (cursor != 0 && cursor->size_0c + cursor->offset_08 == entry->offset_08) {
        entry = cursor;
        cursor = cursor->previous_00;
    }
    for (Entry* next = entry->next_04;
         next != 0 && entry->offset_08 + entry->size_0c == next->offset_08;
         next = next->next_04) {
        entry->size_0c += next->size_0c;
        next->size_0c = -1;
    }
    Entry* dead = entry->next_04;
    while (dead != 0 && dead->size_0c == -1) {
        Entry* next = dead->next_04;
        dead->previous_00->next_04 = next;
        next->previous_00 = dead->previous_00;
        if (dead == first_free_18) {
            first_free_18 = next;
        }
        delete dead;
        dead = next;
    }
}

// FUNCTION: SURRENDER 0x10036F00
void srMemoryPool::dump()
{
    srPrintf("Free blocks:\n\n");
    for (Entry* entry = first_free_18; entry != 0; entry = entry->next_04) {
        srPrintf("%08p %06d\n", static_cast<char*>(memory_08) + entry->offset_08,
                 entry->size_0c);
    }
    srPrintf("\nUsed blocks:\n\n");
    for (unsigned long i = 0; i < 256; i++) {
        for (Entry* entry = allocations_1c[i]; entry != 0; entry = entry->next_04) {
            srPrintf("%08p %06d (%03d)\n",
                     static_cast<char*>(memory_08) + entry->offset_08, entry->size_0c,
                     hashVal(entry->offset_08));
        }
    }
    srPrintf("memory  used       : %d bytes (%d kB)\n", used_0c, used_0c / 1024);
    long free_total = size_04 - used_0c;
    srPrintf("memory  free       : %d bytes (%d kB)\n", free_total, free_total / 1024);
    srPrintf("largest free block : %d bytes (%d kB)\n", memAvail(), memAvail() / 1024);
    srPrintf("total memory       : %d bytes (%d kB)\n", size_04, size_04 / 1024);
    srPrintf("pool memory start address : %p\n", memory_08);
}

// FUNCTION: SURRENDER 0x10036930
srMemoryPool::Entry* srMemoryPool::find(long offset) const
{
    if (offset >= 0 && offset < size_04) {
        for (Entry* entry = allocations_1c[hashVal(offset)]; entry != 0;
             entry = entry->next_04) {
            if (entry->offset_08 == offset) {
                return entry;
            }
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100369A0
srMemoryPool::Entry* srMemoryPool::findArea(long offset) const
{
    for (Entry* entry = first_free_18; entry != 0; entry = entry->next_04) {
        if (entry->offset_08 <= offset && offset < entry->size_0c + entry->offset_08) {
            return entry;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10036880
srMemoryPool::Entry* srMemoryPool::findBestFit(long size) const
{
    Entry* best = 0;
    long best_size = 0x40000000;
    for (Entry* entry = first_free_18; entry != 0; entry = entry->next_04) {
        long entry_size = entry->size_0c;
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
srMemoryPool::Entry* srMemoryPool::findFirstFit(long size) const
{
    for (Entry* entry = first_free_18; entry != 0; entry = entry->next_04) {
        if (size <= entry->size_0c) {
            return entry;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10036B90
srMemoryPool::Entry* srMemoryPool::findPlacing(long offset) const
{
    Entry* previous = 0;
    for (Entry* entry = first_free_18; entry != 0 && entry->offset_08 <= offset;
         entry = entry->next_04) {
        previous = entry;
    }
    return previous;
}

// FUNCTION: SURRENDER 0x100368E0
srMemoryPool::Entry* srMemoryPool::findSpace(long size) const
{
    switch (policy_00) {
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
    if (entry->locked_10 == 0) {
        Entry* placing = findPlacing(entry->offset_08);
        if (placing->offset_08 + placing->size_0c == entry->offset_08) {
            placing->size_0c += entry->size_0c;
        } else {
            placing = addEntry(placing, placing->next_04);
            if (placing == 0) {
                return;
            }
            placing->size_0c = entry->size_0c;
            placing->offset_08 = entry->offset_08;
        }
        used_0c -= entry->size_0c;
        if (entry->previous_00 != 0) {
            entry->previous_00->next_04 = entry->next_04;
        }
        if (entry->next_04 != 0) {
            entry->next_04->previous_00 = entry->previous_00;
        }
        unsigned long bucket = hashVal(entry->offset_08);
        if (entry == allocations_1c[bucket]) {
            allocations_1c[bucket] = entry->next_04;
        }
        delete entry;
        defrag(placing);
        largest_free_dirty_41c = 1;
    }
}

// FUNCTION: SURRENDER 0x10036990
long srMemoryPool::getAlignment() const
{
    return alignment_14;
}

// FUNCTION: SURRENDER 0x10036C40
int srMemoryPool::getLockStatus(void* allocation) const
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        return entry->locked_10;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10036980
srMemoryPool::e_fit srMemoryPool::getPolicy() const
{
    return policy_00;
}

// FUNCTION: SURRENDER 0x10036960
long srMemoryPool::getSize() const
{
    return size_04;
}

// FUNCTION: SURRENDER 0x10036B70
long srMemoryPool::getSize(const void* allocation) const
{
    Entry* entry = find(convertPtr(allocation));
    if (entry == 0) {
        return 0;
    }
    return entry->size_0c;
}

// FUNCTION: SURRENDER 0x10036C60
void srMemoryPool::lock(void* allocation)
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        entry->locked_10 = 1;
    }
}

// FUNCTION: SURRENDER 0x100369F0
int srMemoryPool::maskArea(const void* memory, long size)
{
    long offset = convertPtr(memory);
    if (size < 1) {
        return 0;
    }
    Entry* area = findArea(offset);
    if (area == 0) {
        return 0;
    }
    long old_size = area->size_0c;
    used_0c += size;
    largest_free_dirty_41c = 1;
    /* Retail stores the absolute pointer minus the relative entry offset
       here; keep the established behavior.
       reinterpret-ok: retail subtracts the raw pointer value, not an offset. */
    area->size_0c = reinterpret_cast<long>(memory) - area->offset_08;
    long end = offset + size;
    if (end < area->offset_08 + old_size) {
        Entry* tail = addEntry(area, area->next_04);
        tail->size_0c = (old_size - area->size_0c) - size;
        tail->offset_08 = end;
    }
    return 1;
}

// FUNCTION: SURRENDER 0x10036EC0
long srMemoryPool::memAvail()
{
    if (!largest_free_dirty_41c) {
        return largest_free_10;
    }
    long largest = 0;
    for (Entry* entry = first_free_18; entry != 0; entry = entry->next_04) {
        if (largest < entry->size_0c) {
            largest = entry->size_0c;
        }
    }
    largest_free_10 = largest;
    largest_free_dirty_41c = 0;
    return largest;
}

// FUNCTION: SURRENDER 0x10036910
long srMemoryPool::memFreeTotal() const
{
    return size_04 - used_0c;
}

// FUNCTION: SURRENDER 0x10036920
long srMemoryPool::memUsed() const
{
    return used_0c;
}

// FUNCTION: SURRENDER 0x10036970
void srMemoryPool::setPolicy(e_fit policy)
{
    policy_00 = policy;
}

// FUNCTION: SURRENDER 0x10036C80
void srMemoryPool::unlock(void* allocation)
{
    Entry* entry = find(convertPtr(allocation));
    if (entry != 0) {
        entry->locked_10 = 0;
    }
}

// SYNTHETIC: SURRENDER 0X10037040
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10037050
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X10037080
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X10037090
// std::_Winit global atexit registrar
