#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"
#include "wiz8/message_box.h"

// SYNTHETIC: WIZ8 0x00524A70
// `dynamic initializer for 'g_npc_scripting''
// SYNTHETIC: WIZ8 0x00524A90
// `dynamic atexit destructor for 'g_npc_scripting''
// SYNTHETIC: WIZ8 0x00524AE0
// W8NpcScriptingState::W8NpcScriptingState
// SYNTHETIC: WIZ8 0x00524AA0
// W8NpcScriptingState::~W8NpcScriptingState

// GLOBAL: WIZ8 0x0068C430
W8NpcScriptingState g_npc_scripting;

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

// FUNCTION: WIZ8 0x00524c50
void ClearNpcMessageQueue(void)
{
    int index;

    for (index = 0; index < g_npc_scripting.message_lines.GetCount(); ++index) {
        delete *g_npc_scripting.message_lines.GetAt(index);
    }
    g_npc_scripting.message_lines.Clear();
    memset(static_cast<void*>(&g_npc_scripting), 0, sizeof(g_npc_scripting));
}
