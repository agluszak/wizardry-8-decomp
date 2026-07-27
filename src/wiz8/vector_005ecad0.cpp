#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   `just wiz8 report class-candidates`, which pairs and ranks these families in
   families.csv.

   The complete destructor stores the base table 0x005ECAD4, not the
   0x005ECAD0 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ECAD0;

class W8Vector005ECAD0 : public W8GrowableVector<W8VectorElement005ECAD0*> {
public:
    W8Vector005ECAD0(int initial_capacity);
    virtual ~W8Vector005ECAD0();
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x0048F190
W8Vector005ECAD0::W8Vector005ECAD0(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ECAD0*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x0048F1F0
W8Vector005ECAD0::~W8Vector005ECAD0()
{
}
