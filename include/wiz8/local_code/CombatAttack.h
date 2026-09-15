#pragma once

#include "wiz8/dice.h"

struct W8CombatSlot;
struct W8MonsterInfo;
struct W8SpellEffectDefinition;
struct W8SpellEffectResult;
struct W8TargetSource;
class W8Missile;

/* The five character hit locations, in the order gubLocalACPercent and the
   per-location armour classes use them. */
enum { W8_PC_HIT_LOCATIONS = 5 };
/* The seven monster hit locations and the six body types that name them. */
enum { W8_MONSTER_HIT_LOCATIONS = 7, W8_MONSTER_BODY_TYPES = 6 };

/* 0x0061E7B0: gppStringList indices naming each character hit location,
   paired with a second form the missile hit does not use. */
extern const unsigned short g_pc_hit_location_labels[W8_PC_HIT_LOCATIONS + 1][2];
/* 0x0061EA24: gppStringList indices naming each monster hit location for
   each body type. */
extern const unsigned short g_monster_hit_location_labels[W8_MONSTER_HIT_LOCATIONS]
                                                         [W8_MONSTER_BODY_TYPES];

/* The 0x30-byte attack record Combat Attack.cpp clears and
   FireMissileSourceToTarget consumes. The recovered filler is the Monster.cpp
   stack local built from a database attack and the monster's missile bonus. */
struct W8MissileAttackBlock {
    int unknown_00;
    int missile_value_04;
    unsigned char missile_values_08[0x10];
    int monster_value_18;
    int missile_value_1c;
    unsigned char unknown_20[0x10];
};

static_assert(sizeof(W8MissileAttackBlock) == 0x30, "W8MissileAttackBlock_size_must_be_0x30");

void ClearAttackBlock(W8MissileAttackBlock* block); /* 0x00543260 */
void FireMissileSourceToTarget(int missile_type, W8TargetSource* source, W8CombatSlot* target,
                               W8MissileAttackBlock* attack, unsigned char use_default_accuracy,
                               unsigned int range_category, int accuracy); /* 0x00544630 */

/* 0x00545090: a physical missile reached its combat target - announce the
   hit, roll penetration against the target's armour and apply the damage.
   `deflected` reports the target's missile-deflection roll succeeded. */
void ResolveMissileHit(W8Missile* missile, bool deflected);
/* 0x00544D30: a spell missile reached its combat target - play its impact
   sound and apply the carried spell effect to the target or the area. */
void ResolveSpellMissileHit(W8Missile* missile);
/* 0x00542FC0: the armour class a hit at one location must beat, for the
   attack mode used. */
int TargetArmorClassAtLocation(W8CombatSlot* target, int attack_mode, int hit_location);
/* 0x00543270: roll the effect definition's condition chances against the
   target and apply the ones that take, reporting into `result` when one is
   given. */
void ApplyEffectConditions(W8TargetSource* source, W8CombatSlot* target,
                           W8SpellEffectDefinition* definition, unsigned char announce,
                           unsigned char verbose, W8SpellEffectResult* result);

struct W8MonsterRecord;
struct W8MonsterAttack;

int CalculateMonsterMissileAccuracy(W8MonsterInfo* monster_info, const W8MonsterAttack* attack,
                                    int attack_mode, int flags);

/* Whether a character could attack what `target` names - a party member who
   is in play and not screened by the front rank, or a monster who is engaged,
   alive, targetable and within the character's reach. */
unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target); /* 0x00545C20 */
/* Whether the character can berserk - has the fighter's ability (trait
   W8_TRAIT_BERSERK), a hand that can reach, and a primary hand that fights at
   short range or closer. The attack sub-menu entry it gates is "Berserk". */
unsigned char CanCharacterBerserk(int party_slot); /* 0x005458A0 */
/* Whether a monster would press an attack on what `target` names: a hostile it
   can reach that outranks it, and that is either below forty percent health or
   out of formation. */
unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info,
                                 W8CombatSlot* target); /* 0x00545CF0 */
/* Whether the monster is in a state to attack at all: in the world, in combat,
   alive, below the deactivation threshold, flagged as attacking by its record
   and carrying a first attack. */
bool CanMonsterAttack(W8MonsterInfo* monster_info); /* 0x00545BD0 */

/* What RateMonsterAttack reports for one of a monster's three attacks: zero
   when the attack can be made, otherwise why not. */
enum {
    W8_MONSTER_ATTACK_USABLE = 0,
    W8_MONSTER_ATTACK_NOT_USABLE = 1,
    W8_MONSTER_ATTACK_OUT_OF_REACH = 3
};

/* An attack the record does not carry, or carries with bad data, is not
   usable; otherwise the attack is usable when it can reach someone, judged as
   though the monster were idle. `hostile_only` narrows the sweep to enemies.
   RateMonsterBestAttack answers zero as soon as any attack is usable, one for a
   motionless monster, and otherwise the highest reason it saw. */
unsigned char RateMonsterAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                unsigned int attack, int unused, int hostile_only); /* 0x0053D4B0 */
unsigned char RateMonsterBestAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                    int hostile_only); /* 0x0053D450 */
bool CanAnyHandReachTarget(int party_slot);            /* 0x00545910 */
bool CanCharacterAttack(int party_slot);               /* 0x00545850 */
struct W8Character;

void GetCharacterHandDamageDice(const W8Character* character, int hand, W8Dice* dice);
int GetCharacterHandDamageBonus(const W8Character* character, int hand);
int CalcRangeCategoryToTarget(const W8Character* character, int hand);
int GetHandAttackValue(int party_slot, unsigned int hand);
int NormalizeAttackMode(int attack_mode);
unsigned int ChooseAttackMode(unsigned int attack_modes);
