#pragma once

#include "wiz8/gameplay_modifiers.h"

/* Local Code\Gameplay Mods.cpp. */

void RebuildPartyEffectBlock0050E700(void);

/* The unit's unrecovered block helpers. Each folds its source into the
   shared modifier block; the party-wide block is their target. */
void ApplyPartyEffectSlots(
    const W8EffectSlot* source, W8GameplayModifierBlock* target); /* 0x0050EDC0 */
void ApplyCombatEffectSlots(
    const W8EffectSlot* source, W8GameplayModifierBlock* target); /* 0x0050EF50 */
void ApplyModifierBlock(
    W8GameplayModifierBlock* target,
    const W8GameplayModifierBlock* source); /* 0x0050F090 */
