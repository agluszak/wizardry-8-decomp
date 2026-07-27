#include "wiz8/vector.h"

/* Another growable-vector family, and the second reason the size fingerprint
   can miss one. The first was census sizes; this one is that the fingerprint
   looks for an eighty-four byte out-of-line constructor, and this family has
   none. Its constructor is inlined into the factory at 0x0057E5D0, which heap
   allocates the list, constructs it in place with the default capacity - the
   clamp and the multiply both folded away, leaving the bare operator new(20)
   that wiz8/vector.h describes - stores it in a file-scope pointer and reports
   whether the allocation succeeded.

   So a family whose writer is much larger than eighty-four bytes is not
   necessarily a heap builder to be skipped. It can be an ordinary family whose
   constructor had no out-of-line copy, and the way to tell is the pair of
   deleting destructors: 44 and 30 bytes here, exactly as everywhere else.

   The factory's return needed the branches spelled out. Written as
   `return list != 0;` the body comes out two bytes long and one instruction
   short, because that form computes the flag; the original tests the pointer
   and returns a literal from each arm, which is what the canonical's extra
   instruction in fewer bytes is. A byte-valued return that is one instruction
   *shorter* than the canonical is worth reading as a collapsed branch.

   The complete destructor stores the base table 0x005EEA2C, not the 0x005EEA28
   the constructor installs, because its own store is dead against the inlined
   base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005EEA28;

class W8Vector005EEA28 : public W8GrowableVector<W8VectorElement005EEA28*> {
public:
    W8Vector005EEA28();
    virtual ~W8Vector005EEA28();
};                                       /* 0x10 */

/* The list itself, reached through this pointer rather than through an owner. */
static W8Vector005EEA28* g_list_0068F258;

__forceinline W8Vector005EEA28::W8Vector005EEA28()
{
}

// FUNCTION: WIZ8 0x00585440
W8Vector005EEA28::~W8Vector005EEA28()
{
}

// FUNCTION: WIZ8 0x0057E5D0
unsigned char CreateList005EEA28(void)
{
    W8Vector005EEA28* list;

    list = new W8Vector005EEA28();
    g_list_0068F258 = list;
    if (!list) {
        return 0;
    }
    return 1;
}
