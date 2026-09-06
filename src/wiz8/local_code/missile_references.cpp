#include "wiz8/engine_code/Missile.h"
#include "wiz8/vector.h"

/* The containing original translation-unit spelling remains unknown. The two
   nested growable-vector layouts are established directly by this body's
   count/data/RemoveAt accesses. */

struct W8MissileReferenceOwner {
    unsigned char unknown_000[0x110];
    W8GrowableVector<W8Missile*> missiles_110;
};

// GLOBAL: WIZ8 0x00689b58
W8GrowableVector<W8MissileReferenceOwner*> g_missile_reference_owners_00689b58;

// FUNCTION: WIZ8 0x005019a0
void DetachMissileReferences005019A0(W8Missile* missile)
{
    int owner_index;

    for (owner_index = 0;
         owner_index < g_missile_reference_owners_00689b58.GetCount();
         ++owner_index) {
        W8MissileReferenceOwner* owner =
            *g_missile_reference_owners_00689b58.GetAt(owner_index);
        int missile_index = owner->missiles_110.IndexOf(missile);

        if (missile_index != -1) {
            owner->missiles_110.RemoveAt(missile_index);
            return;
        }
    }
}
