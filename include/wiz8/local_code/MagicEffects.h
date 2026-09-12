#pragma once

#include "surrender/srMath.h"

struct W8CombatSlot;
struct W8SpellEffectEntry;
struct W8Character;
struct W8EffectSlot;
struct W8MonsterInfo;

/* Local Code\Magic Effects.cpp. IsScreenBusy at 0x00554540 follows the
   unit's assertion-backed hull (0x00553910) and precedes Formation & Facing's
   at 0x005545F0, so it lives with its defining unit. */

unsigned char IsScreenBusy(void);

void ApplyEffectToTarget(int* result, W8CombatSlot* target, int arg_3, int arg_4); /* 0x00552250 */
void Function54BA00(W8SpellEffectEntry* effect);                                   /* 0x0054BA00 */
void Function54C930(W8SpellEffectEntry* effect);                                   /* 0x0054C930 */

void Function5526F0(W8EffectSlot* slots, const int* args); /* 0x005526F0 */
void ClearEffectSlot(W8MonsterInfo* monster_info, W8EffectSlot* slot);
void ResetPartyEffectBlock(W8EffectSlot* slot);

void Function552530(void); /* 0x00552530 */
void RecalculateCharacterResistances(W8Character* character);
