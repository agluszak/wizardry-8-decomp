#pragma once

#include "srHeap.h"

class srMemoryPool {
public:
    enum e_fit {
        FIT_FIRST = 0,
        FIT_BEST = 1
    };

    SR_DLL_IMPORT srMemoryPool(void* memory, long size, long alignment);
    SR_DLL_IMPORT ~srMemoryPool();
    SR_DLL_IMPORT srMemoryPool& operator=(const srMemoryPool& pool);

    SR_DLL_IMPORT void* allocate(long size);
    SR_DLL_IMPORT void dump();
    SR_DLL_IMPORT void free(void* allocation);
    SR_DLL_IMPORT long getAlignment() const;
    SR_DLL_IMPORT int getLockStatus(void* allocation) const;
    SR_DLL_IMPORT e_fit getPolicy() const;
    SR_DLL_IMPORT long getSize() const;
    SR_DLL_IMPORT long getSize(const void* allocation) const;
    SR_DLL_IMPORT void lock(void* allocation);
    SR_DLL_IMPORT int maskArea(const void* memory, long size);
    SR_DLL_IMPORT long memAvail();
    SR_DLL_IMPORT long memFreeTotal() const;
    SR_DLL_IMPORT long memUsed() const;
    SR_DLL_IMPORT void setPolicy(e_fit policy);
    SR_DLL_IMPORT void unlock(void* allocation);

private:
    struct Entry {
        Entry* previous_00;
        Entry* next_04;
        long offset_08;
        long size_0c;
        int locked_10;
    };

    static_assert(sizeof(Entry) == 0x14,
                  "srMemoryPool_Entry_must_be_0x14");

    SR_DLL_IMPORT Entry* addEntry(Entry* previous, Entry* next);
    SR_DLL_IMPORT long convertPtr(const void* allocation) const;
    SR_DLL_IMPORT void defrag(Entry* entry);
    SR_DLL_IMPORT Entry* find(long offset) const;
    SR_DLL_IMPORT Entry* findArea(long offset) const;
    SR_DLL_IMPORT Entry* findBestFit(long size) const;
    SR_DLL_IMPORT Entry* findFirstFit(long size) const;
    SR_DLL_IMPORT Entry* findPlacing(long offset) const;
    SR_DLL_IMPORT Entry* findSpace(long size) const;
    SR_DLL_IMPORT void freeInternal(Entry* entry);
    SR_DLL_IMPORT unsigned long hashVal(long offset) const;

    e_fit policy_00;
    long size_04;
    void* memory_08;
    long used_0c;
    long largest_free_10;
    long alignment_14;
    Entry* first_free_18;
    Entry* allocations_1c[256];
    int largest_free_dirty_41c;
};

static_assert(sizeof(srMemoryPool) == 0x420,
              "srMemoryPool_must_be_0x420");
