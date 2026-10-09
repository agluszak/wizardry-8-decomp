#pragma once

#include "srHeap.h"

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srMemoryPool {
public:
    enum e_fit { FIT_FIRST = 0, FIT_BEST = 1 };

    srMemoryPool(void* memory, w8_long size, w8_long alignment);
    ~srMemoryPool();

    void* allocate(w8_long size);
    void dump();
    void free(void* allocation);
    w8_long getAlignment() const;
    int getLockStatus(void* allocation) const;
    e_fit getPolicy() const;
    w8_long getSize() const;
    w8_long getSize(const void* allocation) const;
    void lock(void* allocation);
    int maskArea(const void* memory, w8_long size);
    w8_long memAvail();
    w8_long memFreeTotal() const;
    w8_long memUsed() const;
    void setPolicy(e_fit policy);
    void unlock(void* allocation);

private:
    struct Entry {
        Entry* previous;
        Entry* next;
        w8_long offset;
        w8_long size;
        int locked;
    };

    W8_ABI_ASSERT(sizeof(Entry) == 0x14, "srMemoryPool_Entry_must_be_0x14");

    Entry* addEntry(Entry* previous, Entry* next);
    // FUNCTION: SURRENDER 0x100369E0
    // RECOMP: ?convertPtr@srMemoryPool@@ABEJPBX@Z
    w8_long convertPtr(const void* allocation) const
    {
        return static_cast<const char*>(allocation) - static_cast<const char*>(memory);
    }
    void defrag(Entry* entry);
    Entry* find(w8_long offset) const;
    Entry* findArea(w8_long offset) const;
    Entry* findBestFit(w8_long size) const;
    Entry* findFirstFit(w8_long size) const;
    Entry* findPlacing(w8_long offset) const;
    Entry* findSpace(w8_long size) const;
    void freeInternal(Entry* entry);
    // FUNCTION: SURRENDER 0x100369D0
    // RECOMP: ?hashVal@srMemoryPool@@ABEKJ@Z
    w8_ulong hashVal(w8_long offset) const
    {
        return (offset >> 5) & 0xff;
    }

    e_fit policy;
    w8_long size;
    void* memory;
    w8_long used;
    w8_long largest_free;
    w8_ulong alignment;
    Entry* first_free;
    Entry* allocations[256];
    int largest_free_dirty;
};

W8_ABI_ASSERT(sizeof(srMemoryPool) == 0x420, "srMemoryPool_must_be_0x420");
