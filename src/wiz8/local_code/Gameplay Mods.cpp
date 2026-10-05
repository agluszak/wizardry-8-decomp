#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/character_skills.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayTime.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/utility.h"

#include <string.h>

#define GAMEPLAY_MODS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Gameplay Mods.cpp"

/*
 * Local Code\Gameplay Mods.cpp.
 *
 * The party-wide effect block and the per-character bonus blocks it folds
 * into.
 */

/* Clear the party modifier block, fold the party's effect slots, the combat
   effect run and each character's own equipment and condition blocks into
   their derived modifiers, then drive the sky node from the stored light
   byte. */
// FUNCTION: WIZ8 0x0050E700
void RebuildPartyEffectBlock(void)
{
    memset(&g_status.party_modifiers, 0, sizeof(W8GameplayModifierBlock));
    ApplyPartyEffectSlots(g_status.effect_slots, &g_status.party_modifiers);
    if (gXStatus.fCombatMode != 0) {
        unsigned char value = g_status.party_modifiers.armor_matchup;

        for (int index = 0; index < 9; ++index) {
            W8EffectSlot* slot = &g_combat_state->effect_slots[index];
            if (slot->active != 0 && slot->effect_id == 0x31) {
                value -= slot->amount;
                g_status.party_modifiers.armor_matchup = value;
            }
        }
        ApplyCombatEffectSlots(g_combat_state->effect_slots0, &g_status.party_modifiers);
    }
    int active = 0;
    unsigned int slot_byte = 0;
    while (slot_byte <= 0x82f) {
        W8Character* character = &g_status.buffers.Char[active];
        if (g_status.buffers.XChar[active].fOccupied != 0 && character->hp_current != 0 &&
            character->highest_condition == W8_CONDITION_NONE &&
            CharacterHasTrait(character, W8_TRAIT_CAMP_RECOVERY_BONUS) != 0) {
            break;
        }
        slot_byte += 0x106;
        ++active;
    }
    if (slot_byte < 0x830) {
        g_status.party_modifiers.boost_health_regen = 1;
        g_status.party_modifiers.boost_stamina_regen = 1;
        g_status.party_modifiers.boost_spell_regen = 1;
    }
    for (int party_slot = 0; party_slot < 8; ++party_slot) {
        if (g_status.buffers.XChar[party_slot].fOccupied != 0) {
            W8Character* character = &g_status.buffers.Char[party_slot];
            RebuildCharacterModifierBlock(character);
            RecalculateCharacterDerivedStats(character);
        }
    }
    if (g_status.party_modifiers.light == 0) {
        SetSkyNodeVisible(0);
        return;
    }
    SetSkyNodeVisible(1);
    SetCameraLightIntensity(g_status.party_modifiers.light + g_environment_near_scale);
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
        int item_id = character->EquippedItem[slot].iItemNo;
        if (slot == 8 || slot == 9 || item_id == -1) {
            continue;
        }
        const W8ItemDatabaseRecord* record = &g_item_records[item_id];
        if (slot != 6 && slot != 7) {
            equipment_bonus->damage_bonus += record->attack_damage_bonus;
            equipment_bonus->hit_bonus += record->attack_hit_bonus;
        }
        equipment_bonus->health_regen_adjustment += record->health_regen_bonus;
        equipment_bonus->stamina_regen_adjustment += record->stamina_regen_bonus;
        equipment_bonus->spell_regen_adjustment += record->spell_regen_bonus;
        if (record->modifier_0b1_index != -1) {
            equipment_bonus->skill_bonus[record->modifier_0b1_index] +=
                record->modifier_0b1_value;
        }
        if (record->modifier_0b3_index != -1) {
            equipment_bonus->attribute_adjustments[record->modifier_0b3_index] +=
                record->modifier_0b3_value;
        }
        for (index = 0; index < 6; ++index) {
            resistance_totals[index] += record->resistance_bonus[index];
        }
    }

    for (index = 0; index < 6; ++index) {
        int value = resistance_totals[index];
        if (value < -0x7c) {
            value = -0x7d;
        } else if (value > 0x7c) {
            value = 0x7d;
        }
        equipment_bonus->resistance_bonus[index] = static_cast<signed char>(value);
    }
}

/* Add one modifier block into another: the byte fields sum, the six flags
   latch, and the light and max-combined bytes keep the larger source. */
// FUNCTION: WIZ8 0x0050f090
void ApplyModifierBlock(W8GameplayModifierBlock* target, const W8GameplayModifierBlock* source)
{
    unsigned int index;

    target->damage_bonus += source->damage_bonus;
    target->hit_bonus += source->hit_bonus;
    target->attack_bonus += source->attack_bonus;
    target->damage_percent += source->damage_percent;
    target->armor_class_adjustment += source->armor_class_adjustment;
    target->armor_flat += source->armor_flat;
    target->armor_matchup += source->armor_matchup;
    target->damage_reduction_adjustment += source->damage_reduction_adjustment;
    target->resistance_bonus_all += source->resistance_bonus_all;
    target->damage_per_minute += source->damage_per_minute;
    target->health_regen_adjustment += source->health_regen_adjustment;
    target->stamina_regen_adjustment += source->stamina_regen_adjustment;
    target->spell_regen_adjustment += source->spell_regen_adjustment;
    for (index = 0; index < 7; ++index) {
        target->attribute_adjustments[index] += source->attribute_adjustments[index];
    }
    for (index = 0; index < 0x29; ++index) {
        target->skill_bonus[index] += source->skill_bonus[index];
    }
    for (index = 0; index < 6; ++index) {
        target->resistance_bonus[index] += source->resistance_bonus[index];
    }
    if (source->boost_health_regen != 0) {
        target->boost_health_regen = 1;
    }
    if (source->boost_stamina_regen != 0) {
        target->boost_stamina_regen = 1;
    }
    if (source->boost_spell_regen != 0) {
        target->boost_spell_regen = 1;
    }
    if (source->out_of_formation != 0) {
        target->out_of_formation = 1;
    }
    if (source->detect_secrets != 0) {
        target->detect_secrets = 1;
    }
    if (source->sight_override != 0) {
        target->sight_override = 1;
    }
    if (target->light < source->light) {
        target->light = source->light;
    }
    if (target->value < source->value) {
        target->value = source->value;
    }
    if (target->missile_deflect_chance < source->missile_deflect_chance) {
        target->missile_deflect_chance = source->missile_deflect_chance;
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
            case 0x14:
                adjusted = static_cast<unsigned char>((slot->amount + 1) / 2);
                AdjustByteByPercent(&adjusted, percent);
                target->hit_bonus += adjusted;
                break;
            case 0x20:
                adjusted = static_cast<unsigned char>((slot->amount + 1) / 2);
                AdjustByteByPercent(&adjusted, percent);
                target->armor_matchup += adjusted;
                break;
            case 0x28:
                adjusted = static_cast<unsigned char>(slot->amount * 4 + 7);
                AdjustByteByPercent(&adjusted, percent);
                target->resistance_bonus_all += adjusted;
                break;
            case 0x11:
                target->detect_secrets = 1;
                break;
            case 8:
                AdjustByteByPercent(&adjusted, percent);
                target->light = adjusted << 1;
                break;
            case 0x21:
                AdjustByteByPercent(&adjusted, percent);
                target->value = adjusted;
                break;
            case 0x1a:
                adjusted = static_cast<unsigned char>((slot->amount + 5) * 5);
                AdjustByteByPercent(&adjusted, percent);
                target->missile_deflect_chance = adjusted;
                break;
            case 0x2d:
                target->sight_override = 1;
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
                target->armor_flat += adjusted;
                target->hit_bonus += adjusted;
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
   condition and party blocks without rerunning the derived stats. */
// FUNCTION: WIZ8 0x0050f030
void RebuildCharacterModifierBlock(W8Character* character)
{
    memset(&character->bonus, 0, sizeof(W8GameplayModifierBlock));
    ApplyModifierBlock(&character->bonus, &character->equipment_bonus);
    ApplyModifierBlock(&character->bonus, &character->condition_modifiers);
    if (character->fInParty != 0) {
        ApplyModifierBlock(&character->bonus, &g_status.party_modifiers);
    }
}

/* Rebuild one character's equipment bonus block from its worn items, then its
   derived block from the equipment, condition and party blocks, and
   recompute the derived stats. The standalone form character creation runs. */
// FUNCTION: WIZ8 0x0050e540
void RebuildEquipmentAndDerivedStats(W8Character* character)
{
    memset(&character->equipment_bonus, 0, sizeof(W8GameplayModifierBlock));
    AccumulateEquipmentModifiers(character, &character->equipment_bonus);
    RebuildCharacterModifierBlock(character);
    RecalculateCharacterDerivedStats(character);
}

/* The party-slot counterpart of RebuildEquipmentAndDerivedStats: refill the
   member's equipment bonus block from its worn items, then its derived block
   and stats. */
// FUNCTION: WIZ8 0x0050e5c0
void RebuildEquipmentAndDerivedStatsForSlot(int party_slot)
{
    RebuildEquipmentAndDerivedStats(&g_status.buffers.Char[party_slot]);
}

/* Rebuild one party member's condition/enchantment modifier block from the
   live condition durations, the enchantment slots and the bound-NPC penalty,
   then its derived block and stats. Every condition and enchantment change
   funnels through it. */
// FUNCTION: WIZ8 0x0050e650
void RebuildConditionsAndDerivedStats(int party_slot)
{
    W8Character* character = &g_status.buffers.Char[party_slot];

    memset(&character->condition_modifiers, 0, sizeof(W8GameplayModifierBlock));
    ApplyConditionModifiers(character, character->uiCondition, character->condition_argument,
                            &character->condition_modifiers);
    ApplyEnchantmentModifiers(character->enchantments, &character->condition_modifiers);
    ApplyBoundNpcPenalty(character, &character->condition_modifiers);
    RebuildCharacterModifierBlock(character);
    RecalculateCharacterDerivedStats(character);
}

/* The monster-side rebuild: refill the monster's modifier block from its
   condition durations, enchantments and effect slots, then its armor byte's
   combat adjustment and the combat effect run, and finally the derived
   attributes and regeneration rates. */
// FUNCTION: WIZ8 0x0050e8c0
void RebuildMonsterDerivedStats(int location_id)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0xa6, GAMEPLAY_MODS_CPP, location_id, 1));
    W8GameplayModifierBlock* modifiers = &monster_info->modifiers;

    memset(modifiers, 0, sizeof(W8GameplayModifierBlock));
    ApplyConditionModifiers(0, monster_info->uiCondition, monster_info->condition_argument,
                            modifiers);
    ApplyEnchantmentModifiers(monster_info->enchantments, modifiers);
    ApplyPartyEffectSlots(monster_info->effect_slots, modifiers);
    if (monster_info->fInCombat != 0) {
        W8EffectSlot* slot = monster_info->pCombat->combat_effects;
        for (int index = 9; index != 0; --index, ++slot) {
            if (slot->active != 0 && slot->effect_id == 0x31) {
                modifiers->armor_matchup -= slot->amount;
            }
        }
        ApplyCombatEffectSlots(monster_info->pCombat->combat_effects_2, modifiers);
    }
    ConvertMonsterAttributes(monster_info);
    RebuildMonsterRegenRates(monster_info);
}

/* Fold the live conditions into the modifier block: each running condition id
   carries its own fixed penalty set, condition seven also adds the carried
   argument (poison strength) to the block's regen channel, and condition 0x13
   scales its hit adjustment off the bound monster when the record still
   resolves on the saved level. */
// FUNCTION: WIZ8 0x0050eac0
void ApplyConditionModifiers(W8Character* character, const unsigned int* condition_turns,
                             int condition_argument, W8GameplayModifierBlock* target)
{
    unsigned int i;

    for (unsigned int index = 0; index < W8_CONDITION_COUNT; ++index) {
        if (condition_turns[index] == 0) {
            continue;
        }
        switch (index) {
        case 3:
            target->hit_bonus -= 2;
            target->armor_class_adjustment -= 2;
            break;
        case 4:
            target->hit_bonus -= 5;
            target->armor_class_adjustment -= 4;
            break;
        case 5:
            target->attribute_adjustments[W8_ATTRIBUTE_SPEED] -= 0x32;
            break;
        case 6:
            target->hit_bonus -= 3;
            target->armor_class_adjustment -= 2;
            break;
        case W8_CONDITION_POISONED:
            target->hit_bonus -= 2;
            target->armor_class_adjustment -= 2;
            target->damage_per_minute += condition_argument;
            break;
        case 9:
            target->hit_bonus -= 5;
            for (i = 0; i < 7; ++i) {
                target->attribute_adjustments[i] -= 0x14;
            }
            for (i = 0; i < 0x29; ++i) {
                target->skill_bonus[i] -= 0x14;
            }
            break;
        case 0xb:
            target->attribute_adjustments[W8_ATTRIBUTE_INTELLIGENCE] -= 0x32;
            break;
        case 0xc:
            if (character == 0 || CharacterHasTrait(character, W8_TRAIT_EFFECTIVE_WHILE_BLIND) == 0) {
                target->attribute_adjustments[W8_ATTRIBUTE_SENSES] -= 0x32;
                target->out_of_formation = 1;
            } else {
                target->attribute_adjustments[W8_ATTRIBUTE_SENSES] +=
                    static_cast<signed char>(ScaleValueByProfessionLevel(character, W8_TRAIT_EFFECTIVE_WHILE_BLIND, 50.0f)) -
                    0x32;
            }
            break;
        case 0xe:
            target->attribute_adjustments[W8_ATTRIBUTE_DEXTERITY] -= 0x32;
            /* fall through */
        case W8_CONDITION_ASLEEP:
        case 0x10:
        case W8_CONDITION_UNCONSCIOUS:
            target->out_of_formation = 1;
            break;
        case 0x13:
            if (GetConditionRecordFlag(CharacterPointerToPartySlot(character), 1) != 0) {
                W8MonsterInfo* bound;
                if (character->conditions[1].level_acquired == g_status.current_level &&
                    (bound = MonsterInfoFromID(0x16b, GAMEPLAY_MODS_CPP,
                                               character->conditions[1].source_monster,
                                               1)) != 0) {
                    W8MonsterRecord* monster = GetMonsterDataForInfo(bound);
                    target->health_regen_adjustment += -1 - (monster->effective_level >> 1);
                } else {
                    target->health_regen_adjustment += -5;
                }
            }
            break;
        }
    }
}

/* Fold the enchantment slots into the modifier block: slot five raises one
   attribute by ten per power, slot six raises them all by five per power and
   slot seven raises the damage-reduction and armor bytes by eight and one per
   power, each scaled by the slot's percentage. */
// FUNCTION: WIZ8 0x0050ecc0
void ApplyEnchantmentModifiers(const W8Enchantment* enchantments, W8GameplayModifierBlock* target)
{
    unsigned char amount;
    unsigned int i;

    for (unsigned int index = 0; index < 8; ++index) {
        const W8Enchantment* slot = &enchantments[index];
        if (slot->turns == 0) {
            continue;
        }
        switch (index) {
        case 5:
            amount = static_cast<unsigned char>(slot->power * 10);
            AdjustByteByPercent(&amount, slot->percent);
            target->attribute_adjustments[W8_ATTRIBUTE_SPEED] += amount;
            break;
        case 6:
            amount = static_cast<unsigned char>(slot->power * 5);
            AdjustByteByPercent(&amount, slot->percent);
            for (i = 0; i < 7; ++i) {
                target->attribute_adjustments[i] += amount;
            }
            break;
        case 7:
            amount = static_cast<unsigned char>(slot->power << 3);
            AdjustByteByPercent(&amount, slot->percent);
            target->damage_reduction_adjustment += amount;
            amount = static_cast<unsigned char>(slot->power);
            AdjustByteByPercent(&amount, slot->percent);
            target->armor_matchup += amount;
            break;
        }
    }
}
