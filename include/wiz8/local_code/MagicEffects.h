#pragma once

#include "surrender/srMath.h"

struct W8CombatSlot;
struct W8TargetSource;
struct W8SpellEffectEntry;

/* Local Code\Magic Effects.cpp. IsScreenBusy at 0x00554540 follows the
   unit's assertion-backed hull (0x00553910) and precedes Formation & Facing's
   at 0x005545F0, so it lives with its defining unit. */

unsigned char IsScreenBusy(void);

void ApplyEffectToTarget(int* result, W8CombatSlot* target, int arg_3, int arg_4); /* 0x00552250 */
void AnnounceEffectResisted(W8CombatSlot* target);                                 /* 0x00552070 */
void ApplyEffectAndAnnounce(int* result, W8CombatSlot* target, int arg_3,
                            int arg_4); /* 0x00552340 */
/* 0x00551BA0 sits in the unresolved gap before the unit's assertion hull;
   GroupAttacks.cpp's call sites need the declaration. Its own assertion names
   Magic Effects.cpp, so the gap is attributed to this unit. */
char ResolveAttackOnTarget00551BA0(const W8TargetSource* source, W8CombatSlot* target,
                                   int condition_id, int arg_4, unsigned int arg_5, int arg_6,
                                   int arg_7, char arg_8, char arg_9, int arg_10);
void Function54BA00(W8SpellEffectEntry* effect); /* 0x0054BA00 */
void Function54C930(W8SpellEffectEntry* effect); /* 0x0054C930 */

void Function552530(void); /* 0x00552530 */
