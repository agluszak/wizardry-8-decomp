#pragma once

struct W8CombatSlot;
struct W8MonsterInfo;

unsigned char CanTargetPartySlot(int party_slot, const W8CombatSlot* target);
unsigned char CharacterHasAttackOn(int party_slot, W8CombatSlot* target);
unsigned char Function5458A0(int party_slot);
unsigned char MonsterHasAttackOn(W8MonsterInfo* monster_info, W8CombatSlot* target);
unsigned char RateMonsterAttack(
    W8MonsterInfo* monster_info, int target, unsigned int attack, int arg_4, int arg_5);
bool CanAnyHandReachTarget(int party_slot); /* 0x00545910 */
bool CanCharacterAttack(int party_slot);    /* 0x00545850 */
