#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 00524781-00526e8f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00525E50
bool IsValue68C4C0Clear(void)
{
    return g_value_68c4c0 == 0;
}

/* Report whether a party slot can be picked: in range, its slot row occupied,
   the character still on its feet, and the 0x0b01 gate under 0x0d. That gate's
   meaning is not established, so the name stays address-qualified; this is a
   third observed threshold beside the 0x12 and 0x0f already recorded on it.

   The two status buffers are read as what their sizes say they are: 0xc310 is
   eight W8Character at the 0x1862 stride and 0x830 is eight 0x106-byte rows. */
// FUNCTION: WIZ8 0x00524a10
unsigned char IsPartySlotEligible00524A10(int slot)
{
    W8Character* character;
    bool eligible;

    if (slot < 0) {
        return 0;
    }
    if (slot >= 8) {
        return 0;
    }
    if (static_cast<unsigned char*>(g_status_685170.buffers.buffer_08)[slot * 0x106] == 0) {
        return 0;
    }
    character = &static_cast<W8Character*>(g_status_685170.buffers.buffer_04)[slot];
    if (character->hp_current == 0) {
        return 0;
    }
    eligible = character->unknown_0b01 < 0xd;
    return eligible;
}
