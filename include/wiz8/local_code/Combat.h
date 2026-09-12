#pragma once

#include "wiz8/layouts/targeting.h"

void ChooseAction(int party_slot, int action, int detail, const void* data, int a,
                  int b); /* 0x004E7CC0 */
void ChooseCombatAction(int party_slot, int context, int* out_kind, int* out_action,
                        W8CombatSlot** out_target,
                        W8ActionDetailBlock** out_detail); /* 0x004E77B0 */
void SetCharacterCombatAction(int party_slot, int action_kind, int action_detail, int arg_4,
                              const void* data, int notify); /* 0x004E8000 */
void EndCombat004EA310(int mode);                             /* 0x004EA310 */
unsigned char IsSlotActionChosen(int party_slot, int context, int arg_3, int arg_4);
void SwitchCharacterTo(int party_slot, int action); /* 0x004ED390 */
void Function4EA1F0(void);                          /* 0x004EA1F0 */
void Function517780(void);                          /* 0x00517780 */
void Function5A3470(void);                          /* 0x005A3470 */
struct W8TargetSource;
struct W8CombatSlot;
union W8ActionDetailBlock;
struct W8ItemInstance;
struct W8Character;
struct W8MonsterInfo;
struct W8CombatCharacterRow;

void CatchUpCombatActor(W8CombatCharacterRow* row); /* 0x004ECEB0 */
unsigned char Function4F96F0(W8Character* character);                            /* 0x004F96F0 */
void Function4ECC80(W8TargetSource* source, W8CombatSlot* target);               /* 0x004ECC80 */
void Function4EA5C0(int party_slot);                                             /* 0x004EA5C0 */
void Function4E7EE0(int party_slot, int action, int detail, const void* data, int arg_5,
                    int notify);                           /* 0x004E7EE0 */
void RecordCharacterDeath(int party_slot);
void DropCharacterFromRound(int party_slot);
/* 0x004E79A0: whether one party slot may switch to the given targeting
   context, in the two forms the target-refresh pass asks. */
unsigned char CharacterCanSwitchTo(int party_slot, W8TargetingContext context, int arg_3,
                                   int arg_4);
unsigned char TryCharacterAction(int party_slot, int action, char commit);
void NotifyNearbyMonsters(int what);
void CombatLog(const char* format, ...);
void BeginCombatRound(void);
void EndMonsterTurn(W8MonsterInfo* monster_info);
void SetSlotAction(int party_slot, int action_kind, int action_detail);
unsigned char CanCharReBreathe(int party_slot);
unsigned char CharacterHasCondition(const W8Character* character, int condition);
void Function52E5C0(int event, int a, int b, int argument); /* 0x0052E5C0 */
extern int g_special_event_0068c50c;                        /* 0x0068C50C */
int Function5A1350(void);                                   /* 0x005A1350 */
