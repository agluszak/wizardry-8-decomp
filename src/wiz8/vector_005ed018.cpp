#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   the recorded polymorphism census and its reviewed class-family evidence.

   The complete destructor stores the base table 0x005ED01C, not the
   0x005ED018 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ED018;

class W8Vector005ED018 : public W8GrowableVector<W8VectorElement005ED018*> {
public:
    W8Vector005ED018(int initial_capacity);
    virtual ~W8Vector005ED018() override;
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x004af690
W8Vector005ED018::W8Vector005ED018(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ED018*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x004af6f0
W8Vector005ED018::~W8Vector005ED018()
{
}
