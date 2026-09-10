#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/float_constants.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/MonsterManager.h"

#include <string.h>

/*
 * Local Code\Gameplay Mods.cpp.
 *
 * The party-wide effect block and the per-character bonus blocks it folds
 * into.
 */

/* The source records Function50EDC0 applies: twelve 0x11-byte effect slots. */
// GLOBAL: WIZ8 0x0068691F
unsigned char g_effect_source_list_0068691f[12 * 0x11];

/* The party-wide 0x67-byte bonus block ResetPartyEffectBlock wipes and this
   unit refills. */
// GLOBAL: WIZ8 0x00687453
unsigned char g_party_effect_block_00687453[0x67];

/* Clear the party effect state, fold the active item effects, the party-wide
   block and each character's own equipment and carried blocks into their
   derived bonuses, then drive the sky node from the stored light byte. */
// FUNCTION: WIZ8 0x0050E700
void RebuildPartyEffectBlock0050E700(void)
{
    memset(&g_status_685170.unknown_22e3[0], 0, 0x67);
    Function50EDC0(
        g_effect_source_list_0068691f, g_party_effect_block_00687453);
    if (g_in_combat_00683f94 != 0) {
        unsigned char value = g_status_685170.unknown_22e3[5];

        for (int index = 0; index < 9; ++index) {
            W8CombatEffectSlot* slot = &g_combat_state->effect_slots[index];
            if (slot->active != 0 && slot->visual_index == 0x31) {
                value -= slot->amount;
                g_status_685170.unknown_22e3[5] = value;
            }
        }
        Function50EF50(
            reinterpret_cast<unsigned char*>(g_combat_state->effect_slots_tail), /* reinterpret-ok: packed effect slots feed the block helper */
            g_party_effect_block_00687453);
    }
    int active = 0;
    unsigned int slot_byte = 0;
    while (slot_byte <= 0x82f) {
        W8Character* character = &g_party_characters[active];
        if (g_party_slot_rows[active].occupied != 0
            && character->hp_current != 0 && character->unknown_0b01 == 0
            && Function547940(character, 10) != 0) {
            break;
        }
        slot_byte += 0x106;
        ++active;
    }
    if (slot_byte < 0x830) {
        g_status_685170.unknown_22e3[0x42] = 1;
        g_status_685170.unknown_22e3[0x43] = 1;
        g_status_685170.unknown_22e3[0x44] = 1;
    }
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0) {
            W8Character* character = &g_party_characters[party_slot];
            unsigned char* block =
                reinterpret_cast<unsigned char*>(&character->bonus_1770); /* reinterpret-ok: packed derived bonus block */
            memset(block, 0, 0x67);
            Function50F090(block, character->equipment_bonus_1709);
            Function50F090(block, character->unknown_16a2);
            if (*reinterpret_cast<char*>(character + 1) != 0) /* reinterpret-ok: unmodelled character flag at +4 */
            {
                Function50F090(block, g_party_effect_block_00687453);
            }
            Function4ED9D0(character);
        }
    }
    if (g_status_685170.unknown_22e3[0x47] == 0) {
        SetSkyNodeVisible(0);
        return;
    }
    SetSkyNodeVisible(1);
    SetSkyNodeValue1D0((int)((float)g_status_685170.unknown_22e3[0x47]
                             + g_environment_near_scale_005ec0b0));
}
