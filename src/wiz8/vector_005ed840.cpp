#include "wiz8/vector.h"

/* A growable-vector family of the shape docs/libraries/wiz8-foundation-types.md
   describes: a class deriving from a W8GrowableVector instantiation and adding
   no member and no override.

   It is the one family the size fingerprint missed, and why is worth knowing.
   The census sizes a body by the distance to the next body it identified, so
   its numbers are inflated by inter-function padding - nops here, not the 0xCC
   the name suggests - and can swallow a body the census never identified at
   all. That is what happens after 0x005178C0: an eighteen-byte body sits
   in the gap, making the census call a thirty-byte deleting destructor sixty
   four bytes long. Fingerprinting on census sizes therefore drops real matches
   as well as inventing false ones; measuring the body after stripping the
   padding finds this one and, checked across the whole census, exactly this
   one.

   The complete destructor stores the base table 0x005ED844, not the
   0x005ED840 the constructor installs, because its own store is dead
   against the inlined base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ED840;

class W8Vector005ED840 : public W8GrowableVector<W8VectorElement005ED840*> {
public:
    W8Vector005ED840(int initial_capacity);
    virtual ~W8Vector005ED840() override;
};                                       /* 0x10 */

// FUNCTION: WIZ8 0x00517810
W8Vector005ED840::W8Vector005ED840(int initial_capacity)
    : W8GrowableVector<W8VectorElement005ED840*>(initial_capacity)
{
}

// FUNCTION: WIZ8 0x00517870
W8Vector005ED840::~W8Vector005ED840()
{
}
