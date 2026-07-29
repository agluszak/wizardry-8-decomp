#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   Its four bodies are spread across the image rather than sitting together,
   which is not a sign they are unrelated. VC6 emits each of them as its own
   COMDAT in every unit that needs it and its linker does not fold duplicates,
   so the surviving copy of a destructor can come from a different unit than the
   constructor. What ties them together is the census: 0x005EC018 is
   installed by this constructor and by nothing else in the image.

   The complete destructor stores the base table 0x005EC004, not the
   0x005EC018 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005EC018;

// VTABLE: WIZ8 0x005ec018
class W8Vector005EC018 : public W8GrowableVector<W8VectorElement005EC018*> {
public:
    W8Vector005EC018(int initial_capacity);
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x004cad80
W8Vector005EC018::W8Vector005EC018(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EC018*>(initial_capacity)
{
}

// SYNTHETIC: WIZ8 0x00438f40
// W8GrowableVector<W8VectorElement005EC018*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00438f70
// W8Vector005EC018::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00438c70
// W8GrowableVector<W8VectorElement005EC018*>::~W8GrowableVector<W8VectorElement005EC018*>
