#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   `just wiz8 report class-candidates`, which pairs and ranks these families in
   families.csv.

   The complete destructor stores the base table 0x005ED7D8, not the
   0x005ED7D4 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ED7D4;

class W8Vector005ED7D4 : public W8GrowableVector<W8VectorElement005ED7D4*> {
public:
    W8Vector005ED7D4(int initial_capacity);
    virtual ~W8Vector005ED7D4();
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00501EB0
W8Vector005ED7D4::W8Vector005ED7D4(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ED7D4*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x00501F10
W8Vector005ED7D4::~W8Vector005ED7D4()
{
}
