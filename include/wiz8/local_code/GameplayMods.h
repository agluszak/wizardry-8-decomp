#pragma once

#include "wiz8/gameplay_modifiers.h"

/* Local Code\Gameplay Mods.cpp. */

struct W8Character;
struct W8Enchantment;

void RebuildPartyEffectBlock0050E700(void);

/* 0x0050E540: fold the character's equipment, condition and party modifier
   blocks into the derived bonus block and then recompute the derived stats. */
void RebuildEquipmentAndDerivedStats(W8Character* character);
/* 0x0050E5C0: the party-slot counterpart of RebuildEquipmentAndDerivedStats,
   run wherever worn equipment changes. */
void RebuildEquipmentAndDerivedStatsForSlot(int party_slot);
/* 0x0050E650: rebuild the character's condition/enchantment modifier block at
   0x16a2 from its live status sources, then the derived block and stats. */
void RebuildConditionsAndDerivedStats(int party_slot);
/* 0x0050E8C0: the monster-side rebuild - refill the monster's modifier block
   from its conditions, enchantments and effect slots, then refresh its
   derived attributes and regeneration rates. */
void RebuildMonsterDerivedStats(int location_id);

/* The unit's block-fold helpers. Each folds its source into the shared
   modifier block; the party-wide block is their usual target. */
void ApplyConditionModifiers(W8Character* character, const int* condition_turns,
                             int condition_argument,
                             W8GameplayModifierBlock* target); /* 0x0050EAC0 */
void ApplyEnchantmentModifiers(const W8Enchantment* enchantments,
                               W8GameplayModifierBlock* target); /* 0x0050ECC0 */
void ApplyPartyEffectSlots(const W8EffectSlot* source,
                           W8GameplayModifierBlock* target); /* 0x0050EDC0 */
void ApplyCombatEffectSlots(const W8EffectSlot* source,
                            W8GameplayModifierBlock* target); /* 0x0050EF50 */
void ApplyModifierBlock(W8GameplayModifierBlock* target,
                        const W8GameplayModifierBlock* source); /* 0x0050F090 */
