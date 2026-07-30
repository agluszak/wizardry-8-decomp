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

static_assert(sizeof(srCriticalSection) == 0x18,
              "srCriticalSection_must_be_0x18");
