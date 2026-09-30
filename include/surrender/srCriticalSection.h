#pragma once

#include <windows.h>

class srCriticalSection {
public:
    srCriticalSection()
    {
        InitializeCriticalSection(&critical_section_00);
    }

    ~srCriticalSection()
    {
        DeleteCriticalSection(&critical_section_00);
    }

    void getAccess()
    {
        EnterCriticalSection(&critical_section_00);
    }

    void releaseAccess()
    {
        LeaveCriticalSection(&critical_section_00);
    }

private:
    CRITICAL_SECTION critical_section_00;
};

static_assert(sizeof(srCriticalSection) == 0x18, "srCriticalSection_must_be_0x18");

/* Scoped acquisition evidenced by the stored section pointer and the release
   destructor called by retail unwind funclets in the registry, GERD, heap and
   node families. The original guard spelling is unknown. */
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
