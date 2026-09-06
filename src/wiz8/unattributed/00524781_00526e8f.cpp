#include "wiz8/character.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_state.h"

extern "C" {
extern int g_value_68c4c0;
extern unsigned char g_flag_68c4f7;
}

extern unsigned char g_flag_68c4a0;
extern unsigned char g_flag_68c4f6;
extern W8NpcState* g_npc_state_68c4ac;
extern W8MonsterManagerEntry* GetNpcGroupEntry(W8NpcState* npc);

// GLOBAL: WIZ8 0x0068C4A0
unsigned char g_flag_68c4a0;
// GLOBAL: WIZ8 0x0068C4AC
W8NpcState* g_npc_state_68c4ac;
// GLOBAL: WIZ8 0x0068C4C0
int g_value_68c4c0;
// GLOBAL: WIZ8 0x0068C4F6
unsigned char g_flag_68c4f6;
// GLOBAL: WIZ8 0x0068C4F7
unsigned char g_flag_68c4f7;

/* Address quarantine 00524781-00526e8f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00525E50
bool IsValue68C4C0Clear(void)
{
    return g_value_68c4c0 == 0;
}

// FUNCTION: WIZ8 0x00525DD0
unsigned char Function525DD0(void)
{
    return g_flag_68c4a0 != 0 || g_flag_68c4f6 != 0;
}

// FUNCTION: WIZ8 0x00525DF0
unsigned char Function525DF0(unsigned char require_group_entry)
{
    if (g_flag_68c4f7 != 0) {
        return 0;
    }
    if (g_flag_68c4a0 == 0 && g_flag_68c4f6 == 0 && g_value_68c4c0 == 0) {
        return 0;
    }
    if (g_npc_state_68c4ac == 0) {
        return 0;
    }
    if (GetNpcGroupEntry(g_npc_state_68c4ac) != 0 && require_group_entry == 0) {
        return 0;
    }
    return 1;
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
    if (g_status_685170.buffers.party_rows[slot].occupied == 0) {
        return 0;
    }
    character = &g_status_685170.buffers.characters[slot];
    if (character->hp_current == 0) {
        return 0;
    }
    eligible = character->unknown_0b01 < 0xd;
    return eligible;
}
