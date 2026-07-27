#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   This one was invisible until writer attribution was corrected. The census
   guesses which function contains a vptr write from inter-function padding, and
   for this constructor it guessed wrong, so the two stores were credited to
   different functions and the pair never formed. Resolving the write sites
   through Ghidra's own containment - object_model.attribute_writers over
   `function-of` - puts both stores back in 0x00445FF0 and the family
   appears.

   The complete destructor stores the base table 0x005EC168, not the
   0x005EC164 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005EC164;

class W8Vector005EC164 : public W8GrowableVector<W8VectorElement005EC164*> {
public:
    W8Vector005EC164(int initial_capacity);
    virtual ~W8Vector005EC164();
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00445FF0
W8Vector005EC164::W8Vector005EC164(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EC164*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x00446050
W8Vector005EC164::~W8Vector005EC164()
{
}
