#pragma once

#include "wiz8/layouts/targeting.h"

extern const wchar_t g_combat_log_format[]; /* 0x00617664 */
/* 0x0068506C: a friendly NPC's combat-entry script notice is still owed. */

void ChooseAction(int party_slot, int action, int detail, const W8ActionDetailBlock* data, int a,
                  int b); /* 0x004E7CC0 */
void ChooseCombatAction(int party_slot, int context, int* out_kind, int* out_action,
                        W8CombatSlot** out_target,
                        W8ActionDetailBlock** out_detail); /* 0x004E77B0 */
void SetCharacterCombatAction(int party_slot, int action_kind, int action_detail,
                              const W8ActionDetailBlock* data, int notify); /* 0x004E8000 */
void EndCombat(int mode);                                                   /* 0x004EA310 */
void BeginCombatExecution(void);
void AssignCombatPhases(void);
void UpdateCombat(void);                            /* 0x004E8EA0 */
void SwitchCharacterTo(int party_slot, int action); /* 0x004ED390 */
void ApplyCombatEndEffects(void);                   /* 0x004EA1F0 */
bool CombatHasContinuingEffects(void);              /* 0x004ED550 */
/* 0x004ED460: whether continuous-combat stance may advance past the pending
   NPC-script / engagement gate. */
bool CombatMayAdvanceContinuously(void);
bool QueueNpcCombatScript(void); /* 0x004ED710 */
/* 0x004E9490: the combat turn scheduler - picks the next party slot or
   monster whose action phase arrived, starts party-movement phases, arms the
   action pacing clock, and rolls the round counter forward until someone
   can act or the round ends. */
void ScheduleCombatActor(void);
int CheckCombatEnd(unsigned int arg_1); /* 0x004E9F90 */
void AdvanceCombatRound(void);          /* 0x004E9B20 */
void RollCombatSurprise(char arg_1);    /* 0x004ECF50 */
/* 0x004E7090: enter combat mode; queues a friendly NPC's combat-entry script
   notice when one is still owed and declines while dialogue or a script event
   defers it. */
unsigned char StartCombat(int surprise);
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
/* 0x004EA5C0: run one party slot's committed combat action - commit the
   chosen block, check conditions that can redirect or cancel it, dispatch on
   the action kind, and charge the fatigue. */
void ExecuteCharacterAction(int party_slot);
/* 0x004EAE20: run one monster's committed combat action - the monster-side
   counterpart of ExecuteCharacterAction. */
void ExecuteMonsterAction(W8MonsterInfo* monster_info, W8MonsterRecord* record);
/* 0x004EAC90: pick the hand the slot's attack lands with this round and
   derive the row's next action phase from the stored attack values. */
void ComputeCharacterActionPhase(int party_slot);
/* 0x004EB8C0: set one monster's turn up, once - phase and speed by action. */
void SetUpMonsterTurn(W8MonsterInfo* monster_info);
/* 0x004EC1E0: the condition interrupt that fires before the actor's action;
   returns the interrupt kind, or -1 when none fires. */
int GetConditionInterrupt(W8TargetSource* source);
/* 0x004EB980: the monster's breathe/special-attack action step. */
char MonsterFleeAction(W8MonsterInfo* monster_info, W8MonsterRecord* record);
/* 0x004EBA70: run the monster's committed special attack against its marker
   lists and every monster hostile to it; returns the action outcome code. */
int ExecuteMonsterSpecialAttack(W8MonsterInfo* monster_info, W8MonsterRecord* record);
/* 0x004EBCE0: the character's breathe/special-attack action step. */
char CreateCharacterBreathEffect(int party_slot);
/* 0x004EBFE0: the character's committed breath attack against the marker
   lists plus every hostile monster; returns the action outcome code. */
int ExecuteCharacterSpecialAttack(int party_slot);
void ApplyPartyCombatAction(int party_slot, int action, int detail, const W8ActionDetailBlock* data,
                            int arg_5, int notify); /* 0x004E7EE0 */
int IsPartyEngaged(void);                           /* 0x004E7E70 */
int GetEngagementCount(void);                       /* 0x004ED2B0 */
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
/* 0x004EC900: face the monster toward whatever its combat slot targets before
   the attack starts; `alternate` picks the immediate versus animated turn. */
void OrientMonsterTowardTarget(W8MonsterInfo* monster_info, char alternate);
void AimMonsterBreathAtTarget(W8MonsterInfo* monster_info);
void SetSlotAction(int party_slot, int action_kind, int action_detail);
bool CanCharReBreathe(int party_slot);
unsigned char TryPanicWoundedCharacter(const W8CombatSlot* target); /* 0x004ECE00 */
short GetCombatActionProgress(int* out_total);                      /* 0x004EC610 */
