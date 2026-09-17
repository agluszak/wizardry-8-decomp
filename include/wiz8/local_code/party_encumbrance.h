#pragma once

/* W8Character::load_category. The inventory UI exposes these five bands as
   white (none), blue (light), green (medium), yellow (heavy), and red
   (extreme); the recovered thresholds are <50, <70, <85, <=100, >100 percent
   of carrying capacity. */
enum W8LoadCategory {
    W8_LOAD_NONE = 0,
    W8_LOAD_LIGHT = 1,
    W8_LOAD_MEDIUM = 2,
    W8_LOAD_HEAVY = 3,
    W8_LOAD_EXTREME = 4
};

void RedistributePartyEncumbrance(void);

struct W8Character;
bool RecalculateCarriedWeight(W8Character* character);
bool RecalculateCarryingCapacity004EDC10(W8Character* character);
void RecalculateCharacterDerivedStats(W8Character* character);
