#pragma once

#include <ostream>

/* SurRender's diagnostic-console stream: a basic_ostream<char>-shaped
   object (0x38 bytes: shared vbptr + basic_ios<char> vbase, the same layout
   as srOStream_withassign). The constructor creates an internal
   window-backed stream buffer ("srDebugWndClass") and gives it focus; the
   destructor deletes the buffer. No known consumer imports the class. */
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

    /* Implicit members the class-level export emits: the copy constructor and
       assignment reproduce basic_ios<char>'s memberwise copyfmt shape, and the
       vbase/vector-deleting destructors carry the virtual-inheritance ABI. */
    // SYNTHETIC: SURRENDER 0x10047A10
    // ??0srWindowOut@@QAE@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x10047B00
    // srWindowOut::operator=
    // SYNTHETIC: SURRENDER 0x10047BA0
    // srWindowOut::`vbase destructor'
    // SYNTHETIC: SURRENDER 0x10047BF0
    // srWindowOut::`vector deleting destructor'

};
