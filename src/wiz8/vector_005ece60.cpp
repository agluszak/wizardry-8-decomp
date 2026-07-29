#include "wiz8/vector_005ece60.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   the recorded polymorphism census and its reviewed class-family evidence.

   The complete destructor stores the base table 0x005ECE64, not the
   0x005ECE60 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

// FUNCTION: WIZ8 0x004a5c30
W8Vector005ECE60::W8Vector005ECE60(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ECE60*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x004a5c90
W8Vector005ECE60::~W8Vector005ECE60()
{
}
