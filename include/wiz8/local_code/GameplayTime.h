#pragma once

struct W8Character;
struct W8MonsterInfo;

void Function502010(int elapsed); /* 0x00502010 */
void AgeMonsterSight(W8MonsterInfo* monster_info, unsigned int minutes, int arg_3);
/* 0x00502B50: rebuild the per-realm stamina and spell regeneration rates from
   the pool ceilings and the three conditioned-rate flags. */
void RebuildCharacterRegenRates00502B50(W8Character* character);
/* 0x00502C50: the monster counterpart - hit points and stamina only, plus the
   modifier block's regen channels. */
void RebuildMonsterRegenRates00502C50(W8MonsterInfo* monster_info);
/* 0x00502650: advance the surprise fade/hold/resolve sequence while
   fSurprisePossible is set. */
void UpdateSurpriseMode(void);
void Function502790(void);          /* 0x00502790 */
void Function5025F0(void);          /* 0x005025F0 */
void Function5029E0(void);          /* 0x005029E0 */
void Function502860(void);          /* 0x00502860: end surprise and post the notice */
unsigned char Function56B6F0(void); /* 0x0056B6F0: surprise fade tick */
void Function56B5F0(void);          /* 0x0056B5F0: start the opposing surprise fade */
