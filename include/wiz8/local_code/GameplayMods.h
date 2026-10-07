#pragma once

#include "wiz8/gameplay_modifiers.h"

/* Local Code\Gameplay Mods.cpp. */

struct W8Character;
struct W8Enchantment;

void RebuildPartyEffectBlock(void);

/* Fold the character's equipment, condition and party modifier
   blocks into the derived bonus block and then recompute the derived stats. */
void RebuildEquipmentAndDerivedStats(W8Character* character);
/* The party-slot counterpart of RebuildEquipmentAndDerivedStats,
   run wherever worn equipment changes. */
void RebuildEquipmentAndDerivedStatsForSlot(int party_slot);
/* Rebuild the character's condition/enchantment modifier block at
   0x16a2 from its live status sources, then the derived block and stats. */
void RebuildConditionsAndDerivedStats(int party_slot);
/* The monster-side rebuild - refill the monster's modifier block
   from its conditions, enchantments and effect slots, then refresh its
   derived attributes and regeneration rates. */
void RebuildMonsterDerivedStats(int location_id);

/* The unit's block-fold helpers. Each folds its source into the shared
   modifier block; the party-wide block is their usual target. */
void ApplyConditionModifiers(W8Character* character, const unsigned int* condition_turns,
                             int condition_argument, W8GameplayModifierBlock* target);
void ApplyEnchantmentModifiers(const W8Enchantment* enchantments, W8GameplayModifierBlock* target);
void ApplyPartyEffectSlots(const W8EffectSlot* source, W8GameplayModifierBlock* target);
void ApplyCombatEffectSlots(const W8EffectSlot* source, W8GameplayModifierBlock* target);
void ApplyModifierBlock(W8GameplayModifierBlock* target, const W8GameplayModifierBlock* source);
void AccumulateEquipmentModifiers(W8Character* character, W8GameplayModifierBlock* equipment_bonus);
void RebuildCharacterModifierBlock(W8Character* character);
