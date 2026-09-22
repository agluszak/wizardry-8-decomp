#pragma once

struct W8MonsterInfo;
struct W8MonsterGroup;
struct W8TargetSource;
struct W8CombatSlot;
struct W8Character;
union W8ActionDetailBlock;
template <class T> class W8GrowableVector;

/* Local Code\Combat Hostility.cpp: whether two monsters count as hostile to
   each other, and whether a spell can be aimed by monster AI. */
char MonsterHostility00546F80(W8MonsterInfo* first, W8MonsterInfo* second);
/* 0x00547310: whether a party action aims at enemies (melee kinds, or a spell /
   item-spell whose target type is an enemy band). */
unsigned char CharacterActionTargetsEnemies(W8Character* character, int action_kind,
                                            int action_detail, W8ActionDetailBlock* detail);
/* 0x00547440: the monster-side counterpart; action kinds 0 and 3 always count,
   kind 2 defers to MonsterCanAimSpell005474B0. */
unsigned char MonsterActionTargetsEnemies(int action_kind, int action_detail,
                                          unsigned int* spell_power_level);
bool MonsterCanAimSpell005474B0(int spell_id);
bool CombatAllowsLiveGroups(void);
void SetMonsterHostility(W8MonsterInfo* monster, unsigned char hostility); /* 0x005477D0 */
void RecountCombatMonsters(void);                                          /* 0x00546E70 */
void SetMonsterGroupHostility(W8MonsterGroup* group, unsigned int hostility,
                              char recurse); /* 0x00547570 */

/* 0x00547010: the disposition two party slots hold toward each other from
   their turncoat state - same side is friendly, split is hostile. */
char CharacterVsCharacterDisposition(int first, int second);
/* 0x00547080: the disposition opposite to the target source's side - a normal
   party member answers hostile, a monster answers its band flipped, and a
   turncoated target has no opposite. */
char GetOppositeDisposition(W8TargetSource* source);
/* 0x00547120: run the listed monster ids (skipping the source itself) through
   MakeTargetGroupHostile so each one's group turns on the source. */
void ProvokeListedMonsterGroups(W8TargetSource* source, W8GrowableVector<int>* monsters);
char MonsterVsCharDisposition(int character_slot, W8MonsterInfo* monster_info);
/* Give every other same-faction group that can see this one its disposition -
   one group going hostile brings the rest of its faction with it. */
void AlertSameFactionGroups(W8MonsterGroup* monster_group); /* 0x005478A0 */

/* 0x005471D0: the target's monster group turns hostile toward the source's
   side and enters combat when it was not already. */
void MakeTargetGroupHostile(W8TargetSource* source, W8CombatSlot* target);
/* 0x00547540: SetMonsterGroupHostility looked up by group id. */
void SetMonsterGroupHostilityByID(int group_id, unsigned int hostility, char recurse);
/* 0x00547CB0: write the fatigue the slot's pending pray costs, or -1 when it
   cannot be paid; the flag picks the check flavor. */
int TurnUndead(int party_slot, int* out_cost, char check);
/* 0x00547FE0: the fatigue the slot's pending turn-undead costs, zero when it
   cannot be carried out. */
int CharacterPrayAction00547FE0(int party_slot);

/* 0x0061EC14: gppStringList indices naming each monster special-attack kind,
   indexed by W8MonsterRecord::special_attack_kind_0e3; slot zero is unused. */
extern const int g_monster_special_attack_name_ids_61ec14[12];
