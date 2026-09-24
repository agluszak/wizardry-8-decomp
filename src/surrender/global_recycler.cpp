#include "surrender/srGlobalRecycler.h"

namespace {

/* RAII guard proven by the methods' EH funclets: unwind runs the guard dtor,
   which calls LeaveCriticalSection on the stored section pointer. */
class RecyclerAccess {
public:
    RecyclerAccess(CRITICAL_SECTION* critical_section) : critical_section_(critical_section)
    {
        EnterCriticalSection(critical_section_);
    }

    ~RecyclerAccess()
    {
        LeaveCriticalSection(critical_section_);
    }

private:
    CRITICAL_SECTION* critical_section_;
};

} // namespace

// FUNCTION: SURRENDER 0x10035530
void srGlobalRecycler::releaseAllUnused()
{
    RecyclerAccess access(&critical_section_8c.critical_section_00);
    for (unsigned long index = 0; index < 16; ++index) {
        if ((used_mask_80 & (1UL << index)) == 0) {
            freeEntry(index);
        }
    }
}

// FUNCTION: SURRENDER 0x100355A0
void srGlobalRecycler::setLimit(unsigned long limit)
{
    RecyclerAccess access(&critical_section_8c.critical_section_00);
    for (unsigned long index = 0; index < 16; ++index) {
        if (cached_bytes_84 <= limit) {
            break;
        }
        if ((used_mask_80 & (1UL << index)) == 0) {
            freeEntry(index);
        }
    }
    limit_88 = limit;
}

/* __stdcall adapter wrapping the thiscall srHeap::allocate member; only
   allocate() below calls it, so its authored spelling stays unresolved. */
// FUNCTION: SURRENDER 0x10035620
static void* __stdcall Function100035620(unsigned long size)
{
    return srHeap.allocate(size);
}

// FUNCTION: SURRENDER 0x10035640
void srGlobalRecycler::freeEntry(unsigned long index)
{
    void* allocation = entries_00[index].allocation;
    if (allocation != 0) {
        srHeap.free(allocation);
        cached_bytes_84 -= entries_00[index].size;
        entries_00[index].allocation = 0;
        entries_00[index].size = 0;
        used_mask_80 &= ~(1UL << index);
    }
}

// FUNCTION: SURRENDER 0x100356A0
srGlobalRecycler::srGlobalRecycler()
{
    used_mask_80 = 0;
    InitializeCriticalSection(&critical_section_8c.critical_section_00);
    for (unsigned long index = 0; index < 16; ++index) {
        entries_00[index].allocation = 0;
        entries_00[index].size = 0;
    }
    used_mask_80 = 0;
    cached_bytes_84 = 0;
    limit_88 = 0x100000;
}

// FUNCTION: SURRENDER 0x100356F0
srGlobalRecycler::~srGlobalRecycler()
{
    {
        RecyclerAccess access(&critical_section_8c.critical_section_00);
    }
    for (unsigned long index = 0; index < 16; ++index) {
        freeEntry(index);
    }
}

// FUNCTION: SURRENDER 0x10035760
void* srGlobalRecycler::allocate(unsigned long size)
{
    if (size < 0x4000) {
        return Function100035620(size);
    }
    RecyclerAccess access(&critical_section_8c.critical_section_00);
    if ((used_mask_80 & 0xffff) != 0xffff) {
        const unsigned long rounded = (size + 0x3ff) & ~0x3ffUL;
        long best_fit = -1;
        long smallest = -1;
        unsigned long fit_size = ~0UL;
        unsigned long small_size = ~0UL;
        for (unsigned long index = 0; index < 16; ++index) {
            if ((used_mask_80 & (1UL << index)) == 0) {
                unsigned long entry_size = entries_00[index].size;
                if (entry_size < small_size) {
                    smallest = (long)index;
                    small_size = entry_size;
                }
                if (rounded <= entry_size && entry_size < fit_size) {
                    best_fit = (long)index;
                    fit_size = entry_size;
                }
            }
        }
        if (best_fit != -1) {
            used_mask_80 |= 1UL << best_fit;
            return entries_00[best_fit].allocation;
        }
        freeEntry(smallest);
        if (rounded + cached_bytes_84 <= limit_88) {
            void* allocation = Function100035620(rounded);
            entries_00[smallest].allocation = allocation;
            entries_00[smallest].size = rounded;
            cached_bytes_84 += rounded;
            used_mask_80 |= 1UL << smallest;
            return entries_00[smallest].allocation;
        }
    }
    return Function100035620(size);
}

// SYNTHETIC: SURRENDER 0x100358D0
// ~srGlobalRecycler EH-unwind emission: drains critical_section_8c

// FUNCTION: SURRENDER 0x100358F0
void srGlobalRecycler::free(void* allocation)
{
    if (allocation != 0) {
        RecyclerAccess access(&critical_section_8c.critical_section_00);
        for (unsigned long index = 0; index < 16; ++index) {
            if (entries_00[index].allocation == allocation) {
                used_mask_80 &= ~(1UL << index);
                return;
            }
        }
        srHeap.free(allocation);
    }
}
