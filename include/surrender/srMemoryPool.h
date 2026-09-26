#pragma once

#include "srHeap.h"

/* Provider-side utility. No known Wizardry/JPEG/ZIP consumer imports
   srMemoryPool symbols, so its provider declarations are not dllimport. */
class srMemoryPool {
public:
    enum e_fit {
        FIT_FIRST = 0,
        FIT_BEST = 1
    };

    srMemoryPool(void* memory, long size, long alignment);
    ~srMemoryPool();
    srMemoryPool& operator=(const srMemoryPool& pool);

    void* allocate(long size);
    void dump();
    void free(void* allocation);
    long getAlignment() const;
    int getLockStatus(void* allocation) const;
    e_fit getPolicy() const;
    long getSize() const;
    long getSize(const void* allocation) const;
    void lock(void* allocation);
    int maskArea(const void* memory, long size);
    long memAvail();
    long memFreeTotal() const;
    long memUsed() const;
    void setPolicy(e_fit policy);
    void unlock(void* allocation);

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

    Entry* addEntry(Entry* previous, Entry* next);
    /* Retail inlines both helpers at every call site yet still exports the
       standalone copies; member-level provider export roots the emission. */
    // FUNCTION: SURRENDER 0x100369E0
    // ?convertPtr@srMemoryPool@@ABEJPBX@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    long convertPtr(const void* allocation) const
    {
        return static_cast<const char*>(allocation) -
               static_cast<const char*>(memory_08);
    }
    void defrag(Entry* entry);
    Entry* find(long offset) const;
    Entry* findArea(long offset) const;
    Entry* findBestFit(long size) const;
    Entry* findFirstFit(long size) const;
    Entry* findPlacing(long offset) const;
    Entry* findSpace(long size) const;
    void freeInternal(Entry* entry);
    // FUNCTION: SURRENDER 0x100369D0
    // ?hashVal@srMemoryPool@@ABEKJ@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    unsigned long hashVal(long offset) const
    {
        return (offset >> 5) & 0xff;
    }

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
