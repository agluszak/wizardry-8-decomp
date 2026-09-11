#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"

extern unsigned char g_flag_68c4a0;
extern unsigned char g_flag_68c4f6;
extern W8NpcState* g_npc_state_68c4ac;

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

/* NPC interaction availability and its party-slot eligibility query. The
   original translation-unit spelling is not established; this descriptive
   name is provisional. */




/* Report whether a party slot can be picked: in range, its slot row occupied,
   the character still on its feet, and the 0x0b01 gate under 0x0d. That gate's
   meaning is not established, so the name stays address-qualified; this is a
   third observed threshold beside the 0x12 and 0x0f already recorded on it.

   The two status buffers are read as what their sizes say they are: 0xc310 is
   eight W8Character at the 0x1862 stride and 0x830 is eight 0x106-byte rows. */
// FUNCTION: WIZ8 0x00524a10
bool IsPartySlotEligible00524A10(int slot)
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
