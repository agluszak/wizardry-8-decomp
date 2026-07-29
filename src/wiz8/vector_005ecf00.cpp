#include "wiz8/vector.h"

#include <stdlib.h>
#include <string.h>

/* Another growable-vector family reached through its owner rather than through
   a constructor of its own. The factory at 0x004A9750 allocates a 0x40-byte
   record with malloc, clears it, hangs a freshly constructed list off +0x0c and
   returns the record. The clear is a memset the compiler turned into a sixteen
   dword string store, which is why the decompiler shows a counted loop rather
   than a call.

   The record has no vtable and is not a class here: malloc and a blanket clear
   are what the image shows, so only the list pointer at +0x0c is established and
   the rest of the 0x40 stays opaque.

   The complete destructor stores the base table 0x005ECF04, not the 0x005ECF00
   the construction installs, because its own store is dead against the inlined
   base destructor and VC6 drops it.

   Nothing anchors these bodies to a translation unit, so the file is named for
   the class and the class for its vtable. */

class W8VectorElement005ECF00;

class W8Vector005ECF00 : public W8GrowableVector<W8VectorElement005ECF00*> {
public:
    W8Vector005ECF00();
    virtual ~W8Vector005ECF00() override;
};                                       /* 0x10 */

struct W8Record004A9750 {
    unsigned char unknown_000[0xc];
    W8Vector005ECF00* m_list_c;          /* 0x0c */
    unsigned char unknown_010[0x30];
};                                       /* 0x40 */

__forceinline W8Vector005ECF00::W8Vector005ECF00()
{
}

// FUNCTION: WIZ8 0x004AAB10
W8Vector005ECF00::~W8Vector005ECF00()
{
}

// FUNCTION: WIZ8 0x004A9750
W8Record004A9750* CreateRecord004A9750(void)
{
    W8Record004A9750* record;

    record = (W8Record004A9750*)malloc(sizeof(W8Record004A9750));
    if (!record) {
        return 0;
    }
    memset(record, 0, sizeof(W8Record004A9750));
    record->m_list_c = new W8Vector005ECF00();
    return record;
}
