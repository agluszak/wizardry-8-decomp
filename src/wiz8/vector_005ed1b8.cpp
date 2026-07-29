#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   Its four bodies are spread across the image rather than sitting together,
   which is not a sign they are unrelated. VC6 emits each of them as its own
   COMDAT in every unit that needs it and its linker does not fold duplicates,
   so the surviving copy of a destructor can come from a different unit than the
   constructor. What ties them together is the census: 0x005ED1B8 is
   installed by this constructor and by nothing else in the image.

   The complete destructor stores the base table 0x005ED1B4, not the
   0x005ED1B8 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ED1B8;

class W8Vector005ED1B8 : public W8GrowableVector<W8VectorElement005ED1B8*> {
public:
    W8Vector005ED1B8(int initial_capacity);
    virtual ~W8Vector005ED1B8() override;
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00585340
W8Vector005ED1B8::W8Vector005ED1B8(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ED1B8*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x004bdfe0
W8Vector005ED1B8::~W8Vector005ED1B8()
{
}
