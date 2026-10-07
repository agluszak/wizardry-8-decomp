#pragma once

#include "wiz8/layouts/targeting.h"
#include "wiz8/monster_actions.h"
#include "wiz8/local_code/Factions.h"

struct W8MonsterInfo;
struct W8MonsterGroup;
struct W8TargetSource;
struct W8CombatSlot;
struct W8Character;
union W8ActionDetailBlock;
template <class T> class W8GrowableVector;

/* Local Code\Combat Hostility.cpp: whether two monsters count as hostile to
   each other, and whether a spell can be aimed by monster AI. */
W8Disposition MonsterHostility(W8MonsterInfo* first, W8MonsterInfo* second);
/* Whether a party action aims at enemies (melee kinds, or a spell /
   item-spell whose target type is an enemy band). */
bool CharacterActionTargetsEnemies(W8Character* character, W8ActionKind action_kind,
                                   int action_detail, W8ActionDetailBlock* detail);
/* The monster-side counterpart; action kinds 0 and 3 always count,
   kind 2 defers to MonsterCanAimSpell. */
bool MonsterActionTargetsEnemies(W8MonsterActionKind action_kind, int action_detail,
                                 unsigned int* spell_power_level);
bool MonsterCanAimSpell(int spell_id);
bool CombatAllowsLiveGroups(void);
void SetMonsterHostility(W8MonsterInfo* monster, W8Disposition hostility);
void RecountCombatMonsters(void);
void SetMonsterGroupHostility(W8MonsterGroup* group, unsigned int hostility, bool recurse);

/* The disposition two party slots hold toward each other from
   their turncoat state - same side is friendly, split is hostile. */
W8Disposition CharacterVsCharacterDisposition(int first, int second);
/* The disposition opposite to the target source's side - a normal
   party member answers hostile, a monster answers its band flipped, and a
   turncoated target has no opposite. */
W8Disposition GetOppositeDisposition(W8TargetSource* source);
/* Run the listed monster ids (skipping the source itself) through
   MakeTargetGroupHostile so each one's group turns on the source. */
void ProvokeListedMonsterGroups(W8TargetSource* source, W8GrowableVector<int>* monsters);
W8Disposition MonsterVsCharDisposition(int character_slot, W8MonsterInfo* monster_info);
/* Give every other same-faction group that can see this one its disposition -
   one group going hostile brings the rest of its faction with it. */
void AlertSameFactionGroups(W8MonsterGroup* monster_group);

/* The target's monster group turns hostile toward the source's
   side and enters combat when it was not already. */
void MakeTargetGroupHostile(W8TargetSource* source, W8CombatSlot* target);
/* SetMonsterGroupHostility looked up by group id. */
void SetMonsterGroupHostilityByID(int group_id, unsigned int hostility, bool recurse);
/* Write the fatigue the slot's pending pray costs, or -1 when it
   cannot be paid; the flag picks the check flavor. */
int TurnUndead(int party_slot, int* out_cost, bool check);
/* The fatigue the slot's pending turn-undead costs, zero when it
   cannot be carried out. */
int CharacterPrayAction(int party_slot);

/* GppStringList indices naming each monster special-attack kind,
   indexed by W8MonsterRecord::special_attack_kind; slot zero is unused. */
extern int g_monster_special_attack_name_ids[12];
