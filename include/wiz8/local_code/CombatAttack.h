#pragma once

struct W8CombatSlot;
struct W8MonsterInfo;
struct W8TargetSource;
class W8Missile;

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

unsigned char CanTargetPartySlot(int party_slot, const W8CombatSlot* target);
unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target);
unsigned char Function5458A0(int party_slot);
unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target);
unsigned char RateMonsterAttack(W8MonsterInfo* monster_info, int target, unsigned int attack,
                                int arg_4, int arg_5);
bool CanAnyHandReachTarget(int party_slot); /* 0x00545910 */
bool CanCharacterAttack(int party_slot);    /* 0x00545850 */
