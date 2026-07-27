#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   This class is constructed in many places - two functions install
   0x005EC27C between them - but only 0x00501E50 is an out-of-line
   copy of the constructor. The rest are six-byte thunks and large bodies that
   inlined it, which is what an inline constructor looks like when VC6 emits a
   copy for some call sites and folds it into others. Only the out-of-line copy
   is claimed here; a second emission would need its own file, and there is not
   one.

   The complete destructor stores the base table 0x005EBFE8, not the
   0x005EC27C the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005EC27C;

class W8Vector005EC27C : public W8GrowableVector<W8VectorElement005EC27C*> {
public:
    W8Vector005EC27C(int initial_capacity);
    virtual ~W8Vector005EC27C();
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00501E50
W8Vector005EC27C::W8Vector005EC27C(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EC27C*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x00451B20
W8Vector005EC27C::~W8Vector005EC27C()
{
}
