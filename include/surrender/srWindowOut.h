#pragma once

#include <ostream>

/* SurRender's diagnostic-console stream. The constructor creates a window-backed stream buffer and
   gives it focus; the destructor deletes the buffer. */
// VTABLE: SURRENDER 0x10076E20 srWindowOut
// class srWindowOut
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srWindowOut : public std::ostream {
public:
    srWindowOut(unsigned long handle, const char* title, long width, unsigned long height);
    virtual ~srWindowOut();
};
