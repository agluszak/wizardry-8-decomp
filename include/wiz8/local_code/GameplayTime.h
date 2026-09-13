#pragma once

struct W8Character;
struct W8MonsterInfo;

void Function502010(int elapsed);
/* 0x00503990 */
void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3);
/* 0x00502B50: rebuild the per-realm stamina and spell regeneration rates from
   the pool ceilings and the three conditioned-rate flags. */
void RebuildCharacterRegenRates00502B50(W8Character* character);
