#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override. The census fingerprint that selects it - an
   eighty-four byte constructor, a seventeen byte complete destructor and
   deleting destructors of forty-four and thirty - comes from
   the recorded polymorphism census and its reviewed class-family evidence.

   The complete destructor stores the base table 0x005EC2BC, not the
   0x005EC2B8 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005EC2B8;

// VTABLE: WIZ8 0x005ec2b8
class W8Vector005EC2B8 : public W8GrowableVector<W8VectorElement005EC2B8*> {
public:
    W8Vector005EC2B8(int initial_capacity);
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00451a40
W8Vector005EC2B8::W8Vector005EC2B8(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EC2B8*>(initial_capacity)
{
}

// SYNTHETIC: WIZ8 0x00451b40
// W8GrowableVector<W8VectorElement005EC2B8*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451b70
// W8Vector005EC2B8::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451aa0
// W8GrowableVector<W8VectorElement005EC2B8*>::~W8GrowableVector<W8VectorElement005EC2B8*>
