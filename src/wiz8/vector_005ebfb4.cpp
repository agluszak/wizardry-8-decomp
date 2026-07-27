#include "wiz8/vector.h"

/* The second growable-vector family recovered whole, and the one the shared-Grow
   question actually names: 0x0042A260 is the two-store constructor
   docs/libraries/wiz8-foundation-types.md points at. Put beside the single-store
   constructor at 0x004390F0 the two differ by exactly twelve bytes, which is the
   two extra six-byte vtable stores and nothing else - so the difference between
   the forms is whether the original declared a class deriving from the
   instantiation, not some second shape inside the template.

   Unlike the sound-event list in Engine Code\GrObject.cpp, this constructor is
   out of line and keeps its capacity parameter, so it also carries the clamp to
   at least one that every inlined default-capacity site folds away.

   No assertion or string anchors these bodies to a translation unit, so the file
   is named for the class and the class for its vtable. Its two callers are
   0x00421B87 and 0x005A20D2. */

class W8VectorElement005EBFB4;

class W8Vector005EBFB4 : public W8GrowableVector<W8VectorElement005EBFB4*> {
public:
    W8Vector005EBFB4(int initial_capacity);
    virtual ~W8Vector005EBFB4();
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x0042A260
W8Vector005EBFB4::W8Vector005EBFB4(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EBFB4*>(initial_capacity)
{
}

/* Empty, and what the image holds for it is the base's body: the derived vptr
   store is overwritten by the inlined base destructor before anything can
   observe it, so VC6 drops it and this stores 0x005EBFB8 rather than the
   0x005EBFB4 the constructor above installs. */
// FUNCTION: WIZ8 0x0042A2C0
W8Vector005EBFB4::~W8Vector005EBFB4()
{
}
