#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/float_constants.h"
#include "wiz8/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/utility.h"

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
    memset(&g_status_685170.party_modifiers_22e3, 0, sizeof(W8GameplayModifierBlock));
    ApplyPartyEffectSlots(g_status_685170.effect_slots_17af, &g_status_685170.party_modifiers_22e3);
    if (gXStatus.fCombatMode != 0) {
        unsigned char value = g_status_685170.party_modifiers_22e3.armor_bonus_05;

        for (int index = 0; index < 9; ++index) {
            W8EffectSlot* slot = &g_combat_state->effect_slots[index];
            if (slot->active != 0 && slot->effect_id == 0x31) {
                value -= slot->amount;
                g_status_685170.party_modifiers_22e3.armor_bonus_05 = value;
            }
        }
        // clang-format off
        ApplyCombatEffectSlots(
            reinterpret_cast<const W8EffectSlot*>(g_combat_state->effect_storage_85a), /* reinterpret-ok: six 0x11-byte records at +0x85a; helper unrecovered */
            &g_status_685170.party_modifiers_22e3);
        // clang-format on
    }
    int active = 0;
    unsigned int slot_byte = 0;
    while (slot_byte <= 0x82f) {
        W8Character* character = &g_status_685170.buffers.characters[active];
        if (g_status_685170.buffers.party_rows[active].occupied != 0 &&
            character->hp_current != 0 && character->highest_condition == 0 &&
            CharacterHasTrait00547940(character, 10) != 0) {
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
        if (g_status_685170.buffers.party_rows[party_slot].occupied != 0) {
            W8Character* character = &g_status_685170.buffers.characters[party_slot];

            memset(&character->bonus_1770, 0, sizeof(W8GameplayModifierBlock));
            ApplyModifierBlock(&character->bonus_1770, &character->equipment_bonus_1709);
            ApplyModifierBlock(&character->bonus_1770, &character->unknown_16a2);
            if (character->in_party != 0) {
                ApplyModifierBlock(&character->bonus_1770, &g_status_685170.party_modifiers_22e3);
            }
            RecalculateCharacterDerivedStats(character);
        }
    }
    if (g_status_685170.party_modifiers_22e3.light_47 == 0) {
        SetSkyNodeVisible(0);
        return;
    }
    SetSkyNodeVisible(1);
    SetCameraLightIntensity00483E30((float)g_status_685170.party_modifiers_22e3.light_47 +
                                    g_environment_near_scale_005ec0b0);
}

/* Fold the worn items into one character's equipment bonus block: the twelve
   equipment slots are walked, the two alternate hand slots are skipped, the
   two hand slots contribute no attack values, and the six per-item
   resistance bytes are summed and clamped into the block's resistance run. */
// FUNCTION: WIZ8 0x0050e980
void AccumulateEquipmentModifiers(W8Character* character, W8GameplayModifierBlock* equipment_bonus)
{
    int resistance_totals[6] = {0};
    unsigned int slot;
    int index;

    for (slot = 0; slot < 12; ++slot) {
        int item_id = character->equipment[slot].item_id;
        if (slot == 8 || slot == 9 || item_id == -1) {
            continue;
        }
        const W8ItemDatabaseRecord* record = &g_item_records[item_id];
        if (slot != 6 && slot != 7) {
            equipment_bonus->value_00 += record->attack_damage_bonus;
            equipment_bonus->value_01 += record->attack_hit_bonus;
        }
        equipment_bonus->unknown_08[1] += record->modifier_06c;
        equipment_bonus->unknown_08[2] += record->modifier_06d;
        equipment_bonus->unknown_08[3] += record->modifier_06e;
        if (record->modifier_0b1_index != -1) {
            equipment_bonus->unknown_13[record->modifier_0b1_index] += record->modifier_0b1_value;
        }
        if (record->modifier_0b3_index != -1) {
            equipment_bonus->attribute_adjustments[record->modifier_0b3_index] +=
                record->modifier_0b3_value;
        }
        for (index = 0; index < 6; ++index) {
            resistance_totals[index] += record->resistance_bonus_06f[index];
        }
    }

    for (index = 0; index < 6; ++index) {
        int value = resistance_totals[index];
        if (value < -0x7c) {
            value = -0x7d;
        } else if (value > 0x7c) {
            value = 0x7d;
        }
        equipment_bonus->resistance_bonus[index] = (signed char)value;
    }
}

/* Add one modifier block into another: the byte fields sum, the six flags
   latch, and the light and max-combined bytes keep the larger source. */
// FUNCTION: WIZ8 0x0050f090
void ApplyModifierBlock(W8GameplayModifierBlock* target, const W8GameplayModifierBlock* source)
{
    unsigned int index;

    target->value_00 += source->value_00;
    target->value_01 += source->value_01;
    target->value_02 += source->value_02;
    target->value_03 += source->value_03;
    target->value_4b += source->value_4b;
    target->armor_bonus_04 += source->armor_bonus_04;
    target->armor_bonus_05 += source->armor_bonus_05;
    target->damage_reduction_adjustment += source->damage_reduction_adjustment;
    target->resistance_bonus_all += source->resistance_bonus_all;
    target->unknown_08[0] += source->unknown_08[0];
    target->unknown_08[1] += source->unknown_08[1];
    target->unknown_08[2] += source->unknown_08[2];
    target->unknown_08[3] += source->unknown_08[3];
    for (index = 0; index < 7; ++index) {
        target->attribute_adjustments[index] += source->attribute_adjustments[index];
    }
    for (index = 0; index < 0x29; ++index) {
        target->unknown_13[index] += source->unknown_13[index];
    }
    for (index = 0; index < 6; ++index) {
        target->resistance_bonus[index] += source->resistance_bonus[index];
    }
    if (source->flag_42 != 0) {
        target->flag_42 = 1;
    }
    if (source->flag_43 != 0) {
        target->flag_43 = 1;
    }
    if (source->flag_44 != 0) {
        target->flag_44 = 1;
    }
    if (source->out_of_formation != 0) {
        target->out_of_formation = 1;
    }
    if (source->flag_46 != 0) {
        target->flag_46 = 1;
    }
    if (source->flag_4a != 0) {
        target->flag_4a = 1;
    }
    if (target->light_47 < source->light_47) {
        target->light_47 = source->light_47;
    }
    if (target->value_48 < source->value_48) {
        target->value_48 = source->value_48;
    }
    if (target->value_49 < source->value_49) {
        target->value_49 = source->value_49;
    }
}

/* Fold the party's twelve effect slots into the shared modifier block. */
// FUNCTION: WIZ8 0x0050EDC0
void ApplyPartyEffectSlots(const W8EffectSlot* source, W8GameplayModifierBlock* target)
{
    const W8EffectSlot* slot = source;

    for (int index = 0; index < 12; ++index, ++slot) {
        if (slot->active != 0) {
            unsigned char amount = static_cast<unsigned char>(slot->amount);
            unsigned int percent = slot->percent;
            unsigned char adjusted = amount;

            switch (slot->effect_id) {
            case 8:
                AdjustByteByPercent(&adjusted, percent);
                target->light_47 = adjusted << 1;
                break;
            case 0x11:
                target->out_of_formation = 1;
                break;
            case 0x14:
                adjusted = static_cast<unsigned char>((slot->amount + 1) / 2);
                AdjustByteByPercent(&adjusted, percent);
                target->value_01 += adjusted;
                break;
            case 0x1a:
                adjusted = static_cast<unsigned char>((slot->amount + 5) * 5);
                AdjustByteByPercent(&adjusted, percent);
                target->value_49 = adjusted;
                break;
            case 0x20:
                adjusted = static_cast<unsigned char>((slot->amount + 1) / 2);
                AdjustByteByPercent(&adjusted, percent);
                target->armor_bonus_05 += adjusted;
                break;
            case 0x21:
                AdjustByteByPercent(&adjusted, percent);
                target->value_48 = adjusted;
                break;
            case 0x28:
                adjusted = static_cast<unsigned char>(slot->amount * 4 + 7);
                AdjustByteByPercent(&adjusted, percent);
                target->damage_reduction_adjustment += adjusted;
                break;
            case 0x2d:
                target->flag_4a = 1;
                break;
            }
        }
    }
}

/* Fold the six combat effect records at +0x85a into the shared modifier block. */
// FUNCTION: WIZ8 0x0050EF50
void ApplyCombatEffectSlots(const W8EffectSlot* source, W8GameplayModifierBlock* target)
{
    const W8EffectSlot* slot = source;

    for (int index = 0; index < 6; ++index, ++slot) {
        if (slot->active != 0) {
            unsigned char adjusted;
            unsigned int percent = slot->percent;

            switch (slot->effect_id) {
            case 2:
                adjusted = 2;
                AdjustByteByPercent(&adjusted, percent);
                target->armor_bonus_04 += adjusted;
                target->value_01 += adjusted;
                break;
            case 0x35:
                adjusted = static_cast<unsigned char>(slot->amount * 7);
                AdjustByteByPercent(&adjusted, percent);
                target->resistance_bonus[0] += adjusted;
                target->resistance_bonus[1] += adjusted;
                target->resistance_bonus[2] += adjusted;
                target->resistance_bonus[3] += adjusted;
                break;
            case 0x3b:
                adjusted = static_cast<unsigned char>(slot->amount * 10);
                AdjustByteByPercent(&adjusted, percent);
                target->resistance_bonus[5] += adjusted;
                target->resistance_bonus[4] += adjusted;
                break;
            }
        }
    }
}

/* Rebuild one character's derived modifier block from the equipment,
   persistent and party blocks without rerunning the derived stats. */
// FUNCTION: WIZ8 0x0050f030
void RebuildCharacterModifierBlock(W8Character* character)
{
    memset(&character->bonus_1770, 0, sizeof(W8GameplayModifierBlock));
    ApplyModifierBlock(&character->bonus_1770, &character->equipment_bonus_1709);
    ApplyModifierBlock(&character->bonus_1770, &character->unknown_16a2);
    if (character->in_party != 0) {
        ApplyModifierBlock(&character->bonus_1770, &g_status_685170.party_modifiers_22e3);
    }
}

/* Rebuild one character's equipment bonus block from its worn items, then its
   derived block from the equipment, persistent and party blocks, and
   recompute the derived stats. The standalone form character creation runs. */
// FUNCTION: WIZ8 0x0050e540
void RebuildEquipmentAndDerivedStats(W8Character* character)
{
    memset(&character->equipment_bonus_1709, 0, sizeof(W8GameplayModifierBlock));
    AccumulateEquipmentModifiers(character, &character->equipment_bonus_1709);

    memset(&character->bonus_1770, 0, sizeof(W8GameplayModifierBlock));
    ApplyModifierBlock(&character->bonus_1770, &character->equipment_bonus_1709);
    ApplyModifierBlock(&character->bonus_1770, &character->unknown_16a2);
    if (character->in_party != 0) {
        ApplyModifierBlock(&character->bonus_1770, &g_status_685170.party_modifiers_22e3);
    }
    RecalculateCharacterDerivedStats(character);
}
