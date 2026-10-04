#pragma once

#include "wiz8/load_category.h"

void RedistributePartyEncumbrance(void);

struct W8Character;
bool RecalculateCarriedWeight(W8Character* character);
bool RecalculateCarryingCapacity(W8Character* character);
void RecalculateCharacterDerivedStats(W8Character* character);
