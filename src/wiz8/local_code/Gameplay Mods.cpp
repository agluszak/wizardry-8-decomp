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

/* Clear the party modifier block, fold the party's effect slots, the combat
   effect run and each character's own equipment and persistent blocks into
   their derived modifiers, then drive the sky node from the stored light
   byte. */
// FUNCTION: WIZ8 0x0050E700
void RebuildPartyEffectBlock0050E700(void)
{
    memset(&g_status_685170.party_modifiers_22e3, 0,
           sizeof(W8GameplayModifierBlock));
    ApplyPartyEffectSlots(g_status_685170.effect_slots_17af,
                          &g_status_685170.party_modifiers_22e3);
    if (g_in_combat_00683f94 != 0) {
        unsigned char value = g_status_685170.party_modifiers_22e3.armor_bonus_05;

        for (int index = 0; index < 9; ++index) {
            W8EffectSlot* slot = &g_combat_state->effect_slots[index];
            if (slot->active != 0 && slot->effect_id == 0x31) {
                value -= slot->amount;
                g_status_685170.party_modifiers_22e3.armor_bonus_05 = value;
            }
        }
        ApplyCombatEffectSlots(g_combat_state->effect_slots_tail,
                               &g_status_685170.party_modifiers_22e3);
    }
    int active = 0;
    unsigned int slot_byte = 0;
    while (slot_byte <= 0x82f) {
        W8Character* character = &g_party_characters[active];
        if (g_party_slot_rows[active].occupied != 0
            && character->hp_current != 0 && character->unknown_0b01 == 0
            && CharacterHasTrait00547940(character, 10) != 0) {
            break;
        }
        slot_byte += 0x106;
        ++active;
    }
    if (slot_byte < 0x830) {
        g_status_685170.party_modifiers_22e3.flag_42 = 1;
        g_status_685170.party_modifiers_22e3.flag_43 = 1;
        g_status_685170.party_modifiers_22e3.flag_44 = 1;
    }
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_party_slot_rows[party_slot].occupied != 0) {
            W8Character* character = &g_party_characters[party_slot];

            memset(&character->bonus_1770, 0, sizeof(W8GameplayModifierBlock));
            ApplyModifierBlock(&character->bonus_1770,
                               &character->equipment_bonus_1709);
            ApplyModifierBlock(&character->bonus_1770, &character->unknown_16a2);
            if (character->in_party != 0) {
                ApplyModifierBlock(&character->bonus_1770,
                                   &g_status_685170.party_modifiers_22e3);
            }
            Function4ED9D0(character);
        }
    }
    if (g_status_685170.party_modifiers_22e3.light_47 == 0) {
        SetSkyNodeVisible(0);
        return;
    }
    SetSkyNodeVisible(1);
    SetSkyNodeValue1D0(
        (int)((float)g_status_685170.party_modifiers_22e3.light_47
              + g_environment_near_scale_005ec0b0));
}
