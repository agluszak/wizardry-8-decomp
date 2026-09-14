#pragma once

#include "surrender/srMath.h"
#include "wiz8/dice.h"

struct W8CombatSlot;
struct W8MonsterInfo;
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

/* 0x00552250: shrink a rolled magnitude by the share of it the target's
   resistance in the realm turns aside; a permanent magnitude is left alone. */
void ReduceMagnitudeByResistance(unsigned int* magnitude, W8CombatSlot* target, int realm,
                                 int power_level);
void AnnounceEffectResisted(W8CombatSlot* target); /* 0x00552070 */
void ApplyEffectAndAnnounce(unsigned int* result, W8CombatSlot* target, int realm,
                            int power_level); /* 0x00552340 */
/* 0x00551BA0 sits in the unresolved gap before the unit's assertion hull;
   GroupAttacks.cpp's call sites need the declaration. Its own assertion names
   Magic Effects.cpp, so the gap is attributed to this unit. */
char ResolveAttackOnTarget00551BA0(const W8TargetSource* source, W8CombatSlot* target,
                                   int condition_id, int realm, unsigned int power_level,
                                   int argument, int magnitude, char announce_resistance,
                                   char announce_condition, int duration);
/* 0x005520D0: the saving throw against a condition. A dead target is beyond
   reach and counts as resisting. */
char TargetResistsCondition(W8CombatSlot* target, int realm, unsigned int power_level,
                            int condition_id);
/* 0x00551EB0: land a condition whose saving throw failed. `source_character`
   is part of the call but nothing in the body reads it. */
char InflictConditionOnTarget(W8CombatSlot* target, int condition_id, int realm,
                              unsigned int power_level, int argument, unsigned int magnitude,
                              int source_character, int duration, char announce);
/* 0x0055CC00/0x0055CCB0 sit in the unattributed gap between chunk.cpp and
   PC Item.cpp; the saving throw is their only recovered caller. Each scales the
   value by seven fifths or three fifths on the easy and hard settings and leaves
   it alone on normal; which way round depends on the character's condition
   thirteen or the monster's allegiance flag. */
void ScaleValueForCharacterDifficulty(int party_slot, int* value);
void ScaleValueForMonsterDifficulty(W8MonsterInfo* monster_info, int* value);
void ProcessSpellEffectTargets(W8SpellEffectEntry* effect); /* 0x0054BA00 */
void FinishSpellEffectTargets(W8SpellEffectEntry* effect);  /* 0x0054C930 */

/* 0x0060CFFC: eight bytes per effect id; the leading dword names the monster
   visual resource, -1 means the effect has none. */
extern const int g_effect_visual_table[149][2];

void ResetCombatEffects(void); /* 0x00552530 */
