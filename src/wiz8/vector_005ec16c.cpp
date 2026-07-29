#include "wiz8/vector.h"

/* A third growable-vector family, first identified from the recorded
   polymorphism census by pairing the two one-slot tables a single writer
   installs at offset zero. This family stood out with
   an eighty-four byte constructor, and it turned out to be the same four bodies
   on the same declarations as src/wiz8/vector_005ebfb4.cpp - constructor,
   complete destructor and both deleting destructors, byte-exact with no
   iteration.

   That is the point of recording it. The shape is now mechanical, and the
   forty-odd families still listed as unreviewed are work of a known size rather
   than a question.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005EC16C;

class W8Vector005EC16C : public W8GrowableVector<W8VectorElement005EC16C*> {
public:
    W8Vector005EC16C(int initial_capacity);
    virtual ~W8Vector005EC16C() override;
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00446090
W8Vector005EC16C::W8Vector005EC16C(int initial_capacity)
    : W8GrowableVector<W8VectorElement005EC16C*>(initial_capacity)
{
}

/* Stores the base table 0x005EC170, not the 0x005EC16C the constructor above
   installs, for the reason the second-vtable section of
   docs/libraries/wiz8-foundation-types.md sets out. */
// FUNCTION: WIZ8 0x004460F0
W8Vector005EC16C::~W8Vector005EC16C()
{
}
