#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   This one was invisible until writer attribution was corrected. The census
   guesses which function contains a vptr write from inter-function padding, and
   for this constructor it guessed wrong, so the two stores were credited to
   different functions and the pair never formed. Resolving the write sites
   through Ghidra's own containment - object_model.attribute_writers over
   `function-of` - puts both stores back in 0x00489ED0 and the family
   appears.

   The complete destructor stores the base table 0x005ECA60, not the
   0x005ECA5C the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ECA5C;

class W8Vector005ECA5C : public W8GrowableVector<W8VectorElement005ECA5C*> {
public:
    W8Vector005ECA5C(int initial_capacity);
    virtual ~W8Vector005ECA5C() override;
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00489ed0
W8Vector005ECA5C::W8Vector005ECA5C(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ECA5C*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x00489f30
W8Vector005ECA5C::~W8Vector005ECA5C()
{
}
