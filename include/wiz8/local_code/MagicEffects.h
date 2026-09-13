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
                                   int condition_id, int realm, unsigned int minimum_roll,
                                   int extra_damage, int damage, char announce, char arg_9,
                                   int duration);
/* Unrecovered attack-resolution dependencies retained as direct ABI declarations. */
char Function5520D0(W8CombatSlot* target, int realm, unsigned int minimum_roll,
                    int condition_id); /* 0x005520D0 */
char Function551EB0(W8CombatSlot* target, int condition_id, int realm, unsigned int minimum_roll,
                    int extra_damage, int damage, int source_character, int duration,
                    char arg_9);                 /* 0x00551EB0 */
void Function54BA00(W8SpellEffectEntry* effect); /* 0x0054BA00 */
void Function54C930(W8SpellEffectEntry* effect); /* 0x0054C930 */

void Function552530(void); /* 0x00552530 */
