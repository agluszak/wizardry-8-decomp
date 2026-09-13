#pragma once

#include "surrender/srMath.h"
#include "wiz8/dice.h"

struct W8CombatSlot;
struct W8TargetSource;
struct W8SpellEffectEntry;

#pragma pack(push, 1)

/* One spell effect definition, 0x30 bytes. A missile carries its own copy at
   0x1fc. The radius at 0x00 bounds an area effect (0 for a single target),
   the dice at 0x04 are rolled for the effect's size, the three values at
   0x20 through 0x2c combine into its duration, and the percentage at 0x24
   scales both. */
struct W8SpellEffectDefinition {
    float radius;     /* 0x00: an area effect reaches this far; 0 is single-target */
    W8Dice magnitude; /* 0x04 */
    /* 0x08: the percentage chance of each condition the effect can inflict,
       rolled by ApplyEffectConditions. */
    unsigned char condition_chances[0x10];
    int power_level;        /* 0x18 */
    int value_1c;           /* 0x1c */
    int duration_scale;     /* 0x20 */
    unsigned int percent;   /* 0x24 */
    int duration_base;      /* 0x28 */
    int duration_per_power; /* 0x2c */
};

#pragma pack(pop)

static_assert(sizeof(W8SpellEffectDefinition) == 0x30, "W8SpellEffectDefinition_must_be_0x30");

unsigned int RollEffectMagnitude(W8SpellEffectDefinition* definition); /* 0x00551A20 */
unsigned int RollEffectDuration(W8SpellEffectDefinition* definition);  /* 0x005519C0 */

/* Local Code\Magic Effects.cpp. IsScreenBusy at 0x00554540 follows the
   unit's assertion-backed hull (0x00553910) and precedes Formation & Facing's
   at 0x005545F0, so it lives with its defining unit. */

unsigned char IsScreenBusy(void);

void ApplyEffectToTarget(unsigned int* result, W8CombatSlot* target, int arg_3,
                         int arg_4);               /* 0x00552250 */
void AnnounceEffectResisted(W8CombatSlot* target); /* 0x00552070 */
void ApplyEffectAndAnnounce(unsigned int* result, W8CombatSlot* target, int arg_3,
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
