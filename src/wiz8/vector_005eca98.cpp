#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   `just wiz8 report class-candidates`, which pairs and ranks these families in
   families.csv.

   The complete destructor stores the base table 0x005ECA9C, not the
   0x005ECA98 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ECA98;

class W8Vector005ECA98 : public W8GrowableVector<W8VectorElement005ECA98*> {
public:
    W8Vector005ECA98(int initial_capacity);
    virtual ~W8Vector005ECA98();
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x0048CDA0
W8Vector005ECA98::W8Vector005ECA98(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ECA98*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x0048CE00
W8Vector005ECA98::~W8Vector005ECA98()
{
}
