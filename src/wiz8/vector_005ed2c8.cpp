#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   the recorded polymorphism census and its reviewed class-family evidence.

   The complete destructor stores the base table 0x005ED2CC, not the
   0x005ED2C8 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ED2C8;

// VTABLE: WIZ8 0x005ed2c8
class W8Vector005ED2C8 : public W8GrowableVector<W8VectorElement005ED2C8*> {
public:
    W8Vector005ED2C8(int initial_capacity);
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x004cad00
W8Vector005ED2C8::W8Vector005ED2C8(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ED2C8*>(initial_capacity)
{
}

// SYNTHETIC: WIZ8 0x004cade0
// W8GrowableVector<W8VectorElement005ED2C8*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004cae10
// W8Vector005ED2C8::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004cad60
// W8GrowableVector<W8VectorElement005ED2C8*>::~W8GrowableVector<W8VectorElement005ED2C8*>
