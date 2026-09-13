#pragma once

#include "wiz8/dice.h"

struct W8CombatSlot;
struct W8MonsterInfo;
struct W8TargetSource;

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

unsigned char CanTargetPartySlot(int party_slot, const W8CombatSlot* target);
unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target);
unsigned char Function5458A0(int party_slot);
unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target);
unsigned char RateMonsterAttack(W8MonsterInfo* monster_info, int target, unsigned int attack,
                                int arg_4, int arg_5);
bool CanAnyHandReachTarget(int party_slot); /* 0x00545910 */
bool CanCharacterAttack(int party_slot);    /* 0x00545850 */
struct W8Character;

void GetCharacterHandDamageDice(const W8Character* character, int hand, W8Dice* dice);
int GetCharacterHandDamageBonus(const W8Character* character, int hand);
int CalcRangeCategoryToTarget(const W8Character* character, int hand);
int GetHandAttackValue(int party_slot, unsigned int hand);
int NormalizeAttackMode(int attack_mode);
unsigned int ChooseAttackMode(unsigned int attack_modes);
