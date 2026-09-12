#pragma once

#include "wiz8/layouts/targeting.h"

struct W8CombatSlot;
struct W8MonsterInfo;
struct W8MonsterGroup;
template <class T> class W8GrowableVector;

void ResetCombatSlot(W8CombatSlot* slot);

/* Answer which of the real contexts applies right now. Kept after the enum so
   its result type is the domain rather than a plain int. */
W8TargetingContext GetCurrentTargetingContext(int party_slot);

unsigned char TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
unsigned char TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);

extern int g_target_state_6840b3;
extern int g_picked_group_006840b7;
/* ABS 0x0068408B: the shared combat slot the context-2 outputs name. */
extern W8CombatSlot g_shared_target_0068408b;
/* ABS 0x006840AB: one action-detail block following the shared slot; the
   eight bytes fit exactly before g_target_state_6840b3. Context 2 hands out
   both addresses and stores the selected use-item value in the block. */
extern W8ActionDetailBlock g_shared_action_detail_006840ab;
void ResetTargetingState(void);

unsigned char GetFactionFlag(char faction);
void AimByKind(int actor, W8TargetKind kind, W8TargetingContext context);
void SetMonsterCombatTarget(W8MonsterInfo* monster_info, int location_id);
unsigned char MonsterTargetMatchesSpell(W8MonsterInfo* monster_info, int spell_id);
W8CombatSlot* GetTargetBlockForContext(int party_slot, W8TargetingContext context);
void ClearTargetMarker(void);
void RefreshTargetMarker(void);
void RefreshAllPartyTargets0053BF80(void);
unsigned char Function536570(int party_slot, W8TargetingContext context, int arg);
/* 0x005387F0 */
void AimAtTarget(int actor, W8CombatSlot* target, W8TargetingContext context);
void RefreshCombatTargetHighlights(int party_slot, W8CombatSlot* target);
void HighlightSpellTargetsAtCachedPosition(void);
int Function53C990(int party_slot, int group_id, int arg);
void SetFactionFlag(char faction, unsigned char flag);
unsigned char ShowMonsterTargetMarker(W8MonsterInfo* monster_info);
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, W8TargetingContext context);
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target);
void ClearPartySlotMonsterHighlights(unsigned int party_slot);
void SetTargetToCharacter(int character_slot, W8TargetingContext context);

void SetTargetingMode(int state);
void UpdateAllMonsterHighlights(int party_slot, int location_id);
/* Cursor-table index for SetTargetCursor, including slots 10..12. Not a
   W8TargetingContext. Retail mangles the return as `H`. */
int GetTargetingCursorForState(int alternate);
bool ActionNeedsExplicitTarget(int party_slot);
unsigned int GetActionSpellLikeId(int party_slot, W8TargetingContext context);
void ResetTargetSource(W8TargetSource* source);
void SetTargetSourceToMonster(const W8MonsterInfo* monster_info, W8TargetSource* source);
W8TargetingContext ResolveTargetingContext(int party_slot, W8TargetingContext context);
char TargetMatchesNeeded(W8CombatSlot* target, int needed);
unsigned char SpellHasAnyValidTarget(int party_slot, int spell_id, unsigned char normalize);
void SetTargetToMonster(int monster_id, W8TargetingContext context);
void SetTargetToGroup(int group_id, W8TargetingContext context);

bool ClearMonsterCombatSlot(W8MonsterInfo* monster_info);

unsigned char Function53C630(W8CombatSlot* slot, int arg_2);
void AimCombatSlotAtParty(W8CombatSlot* combat_slot, int hostile);
void ApplyTarget(W8CombatSlot* target, W8TargetingContext context);
bool IsTargetStillPresent(const W8CombatSlot* target);
bool IsTargetSourceInRangeOfGroup(const W8TargetSource* source, W8MonsterGroup* group,
                                  W8TargetingContext context);
void NoteTargetChosen(const W8TargetSource* source, const W8CombatSlot* target);

unsigned char CanTargetMonster(int party_slot, int location_id, int allow_single_target,
                               int reason); /* 0x00536AD0 */
void ClearTargetingMode0053B050(int party_slot);
unsigned char Function536F60(int party_slot, int value,
                             W8TargetingContext context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
void Function5398D0(void);
unsigned char Function53A1D0(void);
void Function53B1D0(void);
void RefreshSpellTargetHighlightsAtRange(void);
void PopulateTargetMarkerForCurrentAction(const srVector3T<float>* position,
                                          W8GrowableVector<int>* marker_vector, int enabled);
unsigned char TargetIsInPlay(int party_slot, int arg_2, int arg_3); /* 0x00536F60 */

void Function53AE00(void); /* 0x0053AE00 */
void Function53CD60(void); /* 0x0053CD60 */
/* Combat action-selection helpers used across the combat units. */
bool CanPartySlotParticipate(int party_slot);      /* 0x0053C270 */
W8TargetingContext Function53BC90(int party_slot); /* 0x0053BC90 */
W8TargetingContext GetValidatedTargetingContext(int party_slot,
                                                W8TargetingContext context); /* 0x0053BBD0 */
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source);     /* 0x0053A9D0 */
void Function537540(int party_slot);                                         /* 0x00537540 */
