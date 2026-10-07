#include "surrender/srGlobalRecycler.h"

namespace {

/* Scoped critical-section guard. */
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
    RecyclerAccess access(&critical_section.critical_section);
    for (unsigned long index = 0; index < 16; ++index) {
        if ((used_mask & (1UL << index)) == 0) {
            freeEntry(index);
        }
    }
}

// FUNCTION: SURRENDER 0x100355A0
void srGlobalRecycler::setLimit(unsigned long limit)
{
    RecyclerAccess access(&critical_section.critical_section);
    for (unsigned long index = 0; index < 16; ++index) {
        if (cached_bytes <= limit) {
            break;
        }
        if ((used_mask & (1UL << index)) == 0) {
            freeEntry(index);
        }
    }
    this->limit = limit;
}

// FUNCTION: SURRENDER 0x10035620
static void* __stdcall AllocateRecyclerStorage(unsigned long size)
{
    return srHeap.allocate(size);
}

// FUNCTION: SURRENDER 0x10035640
void srGlobalRecycler::freeEntry(unsigned long index)
{
    void* allocation = entries[index].allocation;
    if (allocation != 0) {
        srHeap.free(allocation);
        cached_bytes -= entries[index].size;
        entries[index].allocation = 0;
        entries[index].size = 0;
        used_mask &= ~(1UL << index);
    }
}

// FUNCTION: SURRENDER 0x100356A0
srGlobalRecycler::srGlobalRecycler()
{
    used_mask = 0;
    InitializeCriticalSection(&critical_section.critical_section);
    for (unsigned long index = 0; index < 16; ++index) {
        entries[index].allocation = 0;
        entries[index].size = 0;
    }
    used_mask = 0;
    cached_bytes = 0;
    limit = 0x100000;
}

// FUNCTION: SURRENDER 0x100356F0
srGlobalRecycler::~srGlobalRecycler()
{
    {
        RecyclerAccess access(&critical_section.critical_section);
    }
    for (unsigned long index = 0; index < 16; ++index) {
        freeEntry(index);
    }
}

// FUNCTION: SURRENDER 0x10035760
void* srGlobalRecycler::allocate(unsigned long size)
{
    if (size < 0x4000) {
        return AllocateRecyclerStorage(size);
    }
    RecyclerAccess access(&critical_section.critical_section);
    if ((used_mask & 0xffff) != 0xffff) {
        const unsigned long rounded = (size + 0x3ff) & ~0x3ffUL;
        long best_fit = -1;
        long smallest = -1;
        unsigned long fit_size = ~0UL;
        unsigned long small_size = ~0UL;
        for (unsigned long index = 0; index < 16; ++index) {
            if ((used_mask & (1UL << index)) == 0) {
                unsigned long entry_size = entries[index].size;
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
            used_mask |= 1UL << best_fit;
            return entries[best_fit].allocation;
        }
        freeEntry(smallest);
        if (rounded + cached_bytes <= limit) {
            void* allocation = AllocateRecyclerStorage(rounded);
            entries[smallest].allocation = allocation;
            entries[smallest].size = rounded;
            cached_bytes += rounded;
            used_mask |= 1UL << smallest;
            return entries[smallest].allocation;
        }
    }
    return AllocateRecyclerStorage(size);
}

// FUNCTION: SURRENDER 0x100358F0
void srGlobalRecycler::free(void* allocation)
{
    if (allocation != 0) {
        RecyclerAccess access(&critical_section.critical_section);
        for (unsigned long index = 0; index < 16; ++index) {
            if (entries[index].allocation == allocation) {
                used_mask &= ~(1UL << index);
                return;
            }
        }
        srHeap.free(allocation);
    }
}
