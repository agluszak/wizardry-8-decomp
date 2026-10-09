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
    srWindowOut(w8_ulong handle, const char* title, w8_long width, w8_ulong height);
    virtual ~srWindowOut();
};
