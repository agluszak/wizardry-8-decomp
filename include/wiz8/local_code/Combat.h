#pragma once

#include "wiz8/layouts/targeting.h"

extern const wchar_t g_combat_log_format_00617664[]; /* 0x00617664 */

void ChooseAction(int party_slot, int action, int detail, const void* data, int a,
                  int b); /* 0x004E7CC0 */
void ChooseCombatAction(int party_slot, int context, int* out_kind, int* out_action,
                        W8CombatSlot** out_target,
                        W8ActionDetailBlock** out_detail); /* 0x004E77B0 */
void SetCharacterCombatAction(int party_slot, int action_kind, int action_detail, int arg_4,
                              const void* data, int notify); /* 0x004E8000 */
void EndCombat004EA310(int mode);                            /* 0x004EA310 */
void Function4E8370(void);                                   /* 0x004E8370 */
unsigned char IsSlotActionChosen(int party_slot, int context, int arg_3, int arg_4);
void SwitchCharacterTo(int party_slot, int action); /* 0x004ED390 */
void Function4EA1F0(void);                          /* 0x004EA1F0 */
struct W8TargetSource;
struct W8CombatSlot;
union W8ActionDetailBlock;
struct W8ItemInstance;
struct W8Character;
struct W8MonsterInfo;
struct W8CombatCharacterRow;

void CatchUpCombatActor(W8CombatCharacterRow* row); /* 0x004ECEB0 */
bool AnyCharacterEngaged(void);                     /* 0x004E7CA0 */
/* 0x004F96F0: whether the character knows any spell whose realm's sp_left
   still covers its spell_point_cost. */
bool CharacterHasCastableSpell(W8Character* character);
void PointCameraAtCombatTarget(W8TargetSource* source, W8CombatSlot* target); /* 0x004ECC80 */
void Function4EA5C0(int party_slot);                                          /* 0x004EA5C0 */
void ApplyPartyCombatAction(int party_slot, int action, int detail, const void* data, int arg_5,
                            int notify); /* 0x004E7EE0 */
int IsPartyEngaged(void);                /* 0x004E7E70 */
void RecordCharacterDeath(int party_slot);
void DropCharacterFromRound(int party_slot);
/* 0x004E79A0: whether one party slot may switch to the given targeting
   context, in the two forms the target-refresh pass asks. */
bool CharacterCanSwitchTo(int party_slot, W8TargetingContext context, int arg_3, int arg_4);
unsigned char TryCharacterAction(int party_slot, int action, char commit);
void NotifyNearbyMonsters(int what);
void CombatLog(const char* format, ...);
void BeginCombatRound(void);
void EndMonsterTurn(W8MonsterInfo* monster_info);
void EndMonsterAttack(W8MonsterInfo* monster_info); /* 0x004EB7F0 */
void SetSlotAction(int party_slot, int action_kind, int action_detail);
bool CanCharReBreathe(int party_slot);
unsigned char CharacterHasCondition(const W8Character* character, int condition);
