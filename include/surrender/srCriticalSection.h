#pragma once

#include <windows.h>

class srCriticalSection {
public:
    srCriticalSection()
    {
        InitializeCriticalSection(&critical_section);
    }

    ~srCriticalSection()
    {
        EnterCriticalSection(&critical_section);
        LeaveCriticalSection(&critical_section);
        DeleteCriticalSection(&critical_section);
    }

    void getAccess()
    {
        EnterCriticalSection(&critical_section);
    }

    void releaseAccess()
    {
        LeaveCriticalSection(&critical_section);
    }

private:
    CRITICAL_SECTION critical_section;
};

static_assert(sizeof(srCriticalSection) == 0x18, "srCriticalSection_must_be_0x18");

/* Scoped acquisition; the original guard spelling is unknown. */
class srCriticalSectionAccess {
public:
    explicit srCriticalSectionAccess(srCriticalSection* section) : section_(section)
    {
        section_->getAccess();
    }

    // FUNCTION: SURRENDER 0x10010660
    ~srCriticalSectionAccess()
    {
        section_->releaseAccess();
    }

private:
    srCriticalSection* section_;
};
