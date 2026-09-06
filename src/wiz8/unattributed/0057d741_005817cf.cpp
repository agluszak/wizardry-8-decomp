#include "wiz8/unattributed/quarantine_common.h"

#include "wiz8/engine_code/BitArray.h"
#include "wiz8/vector.h"

#include <stdlib.h>

/* Address quarantine 0057d741-005817cf; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

class W8VectorElement005EEA28;

/* No recovered allocation path identifies the object at 0x0068F1F4 yet.
   Keep its demonstrated virtual-delete contract local to its only consumer
   until that concrete type is established. */
struct W8Releasable0057FA20 {
    virtual ~W8Releasable0057FA20();
};

/* Lifecycle record 8's own state, all of it released by the finalizer below and
   nothing here naming what any of it holds. The list is vector.cpp's, created by
   this record's initializer at 0x0057E5D0. */
/* vector.cpp defines this with C++ linkage; the spelling has to agree or the
   reference resolves to the image base under /FORCE. */
extern W8GrowableVector<W8VectorElement005EEA28*>* g_list_0068F258;

extern "C" {

/* Two index arrays, released the way BitArray requires: FreeIndex is not a
   destructor, so the storage goes back through the global operator. */
// GLOBAL: WIZ8 0x0068F288
BitArray* g_bits_68f288;
// GLOBAL: WIZ8 0x0068F28C
BitArray* g_bits_68f28c;
// GLOBAL: WIZ8 0x0068F280
void* g_block_68f280;

/* Three allocations released together, the outer one last. Only that shape is
   established, so the two it owns stay positional. */
struct W8Record0068F284 {
    void* m_owned_000;
    void* m_owned_004;
};

// GLOBAL: WIZ8 0x0068F284
W8Record0068F284* g_record_68f284;
// GLOBAL: WIZ8 0x0068F29C
srClass* g_class_68f29c;
// GLOBAL: WIZ8 0x0068F2A0
srClass* g_class_68f2a0;
// GLOBAL: WIZ8 0x0068F2A4
srClass* g_class_68f2a4;
// GLOBAL: WIZ8 0x0068F2A8
srClass* g_class_68f2a8;

/* Released through slot 0 with the deleting flag, which is all this one shows. */
// GLOBAL: WIZ8 0x0068F1F4
W8Releasable0057FA20* g_releasable_68f1f4;

}

/* Lifecycle record 8's finalizer, the fifth slot of its record and the third
   allocate/release pair that establishes what that slot is for. Every guard is
   the original's own, and each pointer is cleared after its release. */
// FUNCTION: WIZ8 0x0057fa20
unsigned char Screen8Finalize(void)
{
    if (g_list_0068F258) {
        delete g_list_0068F258;
        g_list_0068F258 = 0;
    }
    BitArray* bits = g_bits_68f288;
    if (bits) {
        bits->FreeIndex();
        ::operator delete(bits);
        g_bits_68f288 = 0;
    }
    bits = g_bits_68f28c;
    if (bits) {
        bits->FreeIndex();
        ::operator delete(bits);
        g_bits_68f28c = 0;
    }
    if (g_block_68f280) {
        free(g_block_68f280);
        g_block_68f280 = 0;
    }
    W8Record0068F284* record = g_record_68f284;
    if (record) {
        if (record->m_owned_000) {
            ::operator delete(record->m_owned_000);
        }
        if (record->m_owned_004) {
            ::operator delete(record->m_owned_004);
        }
        ::operator delete(record);
        g_record_68f284 = 0;
    }
    if (g_class_68f29c) {
        g_class_68f29c->release();
        g_class_68f29c = 0;
    }
    if (g_class_68f2a0) {
        g_class_68f2a0->release();
        g_class_68f2a0 = 0;
    }
    if (g_class_68f2a4) {
        g_class_68f2a4->release();
        g_class_68f2a4 = 0;
    }
    if (g_class_68f2a8) {
        g_class_68f2a8->release();
        g_class_68f2a8 = 0;
    }
    if (g_releasable_68f1f4) {
        delete g_releasable_68f1f4;
        g_releasable_68f1f4 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x0057DBB0
unsigned char GetFlag68F105(void)
{
    return g_flag_68f105;
}
// FUNCTION: WIZ8 0x0057DBC0
unsigned char GetFlag68F104(void)
{
    return g_flag_68f104;
}
