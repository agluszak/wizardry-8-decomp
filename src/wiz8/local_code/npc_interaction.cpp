#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/message_box.h"

#include <wchar.h>

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

// FUNCTION: WIZ8 0x005294c0
unsigned char GetNpcQuoteText(W8NpcState* npc, unsigned int type, wchar_t* output)
{
    if (npc == 0 || npc->script_file == 0 || output == 0 || type >= npc->script_file->quote_count) {
        return 0;
    }

    W8NpcScriptQuote* record = &npc->script_file->quotes[type];
    if (record->subquotes == 0) {
        return 0;
    }

    swprintf(output, L"%S", record->subquotes[0]);
    return 1;
}
