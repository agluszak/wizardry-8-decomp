#pragma once

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

unsigned char CanTargetPartySlot(int party_slot, const W8CombatSlot* target);
unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target);
unsigned char Function5458A0(int party_slot);
unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target);
unsigned char RateMonsterAttack(W8MonsterInfo* monster_info, int target, unsigned int attack,
                                int arg_4, int arg_5);
bool CanAnyHandReachTarget(int party_slot); /* 0x00545910 */
bool CanCharacterAttack(int party_slot);    /* 0x00545850 */
