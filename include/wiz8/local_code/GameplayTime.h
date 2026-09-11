#pragma once

struct W8Character;

void Function502010(int elapsed);
/* 0x00502B50: rebuild the per-realm stamina and spell regeneration rates from
   the pool ceilings and the three conditioned-rate flags. */
void RebuildCharacterRegenRates00502B50(W8Character* character);
