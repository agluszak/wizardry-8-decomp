#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"
#include "wiz8/message_box.h"

// GLOBAL: WIZ8 0x0068C4A0
unsigned char g_flag_68c4a0;
// GLOBAL: WIZ8 0x0068C4AC
W8NpcState* g_npc_state_68c4ac;
// GLOBAL: WIZ8 0x0068C4F6
unsigned char g_flag_68c4f6;
// GLOBAL: WIZ8 0x0068C4F7
unsigned char g_flag_68c4f7;

/* NPC interaction availability and its party-slot eligibility query. Live
   query: 0x00524A10 is a gap between Local Code\Conditions & Enchantments.cpp
   (upper 0x00524780) and Local Code\NPC Scripting.cpp (lower 0x00524CA0). */

/* Report whether a party slot can be picked: in range, occupied, still on its
   feet, and highest_condition below HOSTILE. */
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
    eligible = character->highest_condition < W8_CONDITION_HOSTILE;
    return eligible;
}

/* Free queued NPC message-box lines and clear the line count. Retail then
   STOSD-zeroes the 0xcc-byte scripting object those counters live in; the
   remaining named flags in that run are already BSS-zero on new game. */
// FUNCTION: WIZ8 0x00524c50
void ClearNpcMessageQueue(void)
{
    int index;

    for (index = 0; index < g_message_box_line_count; ++index) {
        delete g_message_box_lines[index];
    }
    g_message_box_line_count = 0;
    g_message_box_line_capacity = 0;
    g_message_box_lines = 0;
    g_flag_68c4a0 = 0;
    g_npc_state_68c4ac = 0;
    g_flag_68c4f6 = 0;
    g_flag_68c4f7 = 0;
}
