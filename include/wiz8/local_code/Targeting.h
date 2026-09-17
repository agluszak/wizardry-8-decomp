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

/* Notice colours for combat text: 8 for a party-side source, 9 for a monster
   and 12 for anything else. The target form colours a character target by
   its marching-order slot instead. */
char GetSourceNoticeColor(const W8TargetSource* source); /* 0x0053C320 */
char GetTargetNoticeColor(const W8TargetSource* source,
                          const W8CombatSlot* target); /* 0x0053C3F0 */

extern int g_picked_monster;
extern int g_picked_group;
/* ABS 0x0068408B: the shared combat slot the context-2 outputs name. */
extern W8CombatSlot g_shared_target_0068408b;
/* ABS 0x006840AB: one action-detail block following the shared slot; the
   eight bytes fit exactly before g_picked_monster. Context 2 hands out
   both addresses and stores the selected use-item value in the block. */
extern W8ActionDetailBlock g_shared_action_detail_006840ab;

unsigned char GetFactionFlag(char faction);
void AimByKind(int actor, W8TargetKind kind, W8TargetingContext context);
void SetMonsterCombatTarget(W8MonsterInfo* monster_info, int location_id);
unsigned char MonsterTargetMatchesSpell(W8MonsterInfo* monster_info, int spell_id);
W8CombatSlot* GetTargetBlockForContext(int party_slot, W8TargetingContext context);
void ClearTargetMarker(void);
void RefreshTargetMarker(void);
void RefreshAllPartyTargets(void);
unsigned char RepickActionTarget(int party_slot, W8TargetingContext context, int arg);
void AimAtTarget(int actor, W8CombatSlot* target, W8TargetingContext context); /* 0x005387F0 */
void AimAtPlace(int actor);                                                    /* 0x00538710 */
void RefreshCombatTargetHighlights(int party_slot, W8CombatSlot* target);
void HighlightSpellTargetsAtCachedPosition(void);
/* Pick an attack fallback, allowing the character's alternate weapon set when
   the ordinary group selection has no usable monster. */
int ChooseFallbackMonsterTarget0053C990(int party_slot, int group_id, W8TargetingContext context);
void SetFactionFlag(char faction, unsigned char flag);
unsigned char ShowMonsterTargetMarker(W8MonsterInfo* monster_info);
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, W8TargetingContext context);
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target);
void ClearPartySlotMonsterHighlights(unsigned int party_slot);
void SetTargetToCharacter(int character_slot, W8TargetingContext context);

void SetTargetingMode(int state);
void SetMonsterHighlight(int party_slot, int location_id, char on);
char HighlightMonsterAsTarget(int location_id, int party_slot, char highlight);
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

/* 0x0053C630: fill the slot's point from where its target is. */
unsigned char ResolveTargetPoint(W8CombatSlot* target, char sight_probe);
void AimCombatSlotAtParty(W8CombatSlot* combat_slot, int hostile);
void ApplyTarget(W8CombatSlot* target, W8TargetingContext context); /* 0x00538E00 */
/* 0x0053C490: whether an actor aiming at `target` should drop that aim now
   that `target` has been (re)applied. Out-of-combat contexts always clear;
   otherwise the actor and the applied target must agree on hostility with the
   action's enemy-aimed flag. */
unsigned char ShouldClearAimForAppliedTarget(W8TargetSource* source, W8CombatSlot* target,
                                             W8TargetingContext context,
                                             unsigned char action_targets_enemies);
bool IsTargetStillPresent(const W8CombatSlot* target);
bool IsTargetSourceInRangeOfGroup(const W8TargetSource* source, W8MonsterGroup* group,
                                  W8TargetingContext context);
void NoteTargetChosen(const W8TargetSource* source, const W8CombatSlot* target);

unsigned char CanTargetMonster(int party_slot, int location_id, int allow_single_target,
                               int reason); /* 0x00536AD0 */
void ClearTargetingMode(int party_slot);
void Function53B050(int party_slot); /* 0x0053B050 */
/* 0x00537270: whether the slot's current target satisfies the spell's
   needed-target kind. */
unsigned char IsSpellTargetOfNeededKind(int party_slot, int spell_id);
/* 0x0053AF40: select the party slot the spell-casting view is casting for. */
void SelectSpellCastingPartySlot(int party_slot);
/* 0x0053A440: set the targeting filter for the spell being aimed. */
void ConfigureSpellTargetFilter(int target_type, unsigned int needed_kind);
/* 0x0053A830: commit the chosen spell target. */
void CommitSelectedSpellTarget(void);
void RefreshMonsterTargetCounts005398D0(void);
unsigned char AnyMonsterVisible0053A1D0(void);
void UpdateTargetMarkerHighlight0053B1D0(void);

class W8Monster;
/* 0x0053A060: whether `monster` sits within `max_distance` of `position` and
   still projects on screen. */
bool IsMonsterVisibleWithinDistance0053A060(W8Monster* monster, const srVector3T<float>* position,
                                            float max_distance);
void RefreshSpellTargetHighlightsAtRange(void);
void PopulateTargetMarkerForCurrentAction(const srVector3T<float>* position,
                                          W8GrowableVector<int>* marker_vector, int enabled);
W8TargetingContext GetCombatActionContext0053BC90(int party_slot); /* 0x0053BC90 */
void ReconcilePartyEquipmentAfterCombat0053CD60(void);             /* 0x0053CD60 */
unsigned char
TargetIsInPlay(int party_slot, int value,
               W8TargetingContext context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT); /* 0x00536F60 */

void ClearAllMonsterHighlights(void); /* 0x0053AE00 */
/* Combat action-selection helpers used across the combat units. */
bool CanPartySlotParticipate(int party_slot); /* 0x0053C270 */
W8TargetingContext GetValidatedTargetingContext(int party_slot,
                                                W8TargetingContext context); /* 0x0053BBD0 */
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source);     /* 0x0053A9D0 */
/* 0x00537380: the target kind the slot's current action needs in the effective
   targeting context, with the main-screen selection state folded in. */
int GetTargetNeededForCurrentAction(int party_slot);
/* 0x00537540: after the selected character's action changes, drop back to no
   targeting when its recorded target still fits, or enter the mode the action
   now needs. */
void RevalidateSelectedTarget(int party_slot);
/* 0x00537D20: the cycle-target command - advance the pick to the next
   targetable group or monster and commit it. */
void CycleToNextTarget(int party_slot);
unsigned char SlotHasAnyValidTarget(int party_slot); /* 0x0053CDF0 */
/* 0x00538140: every monster the slot's action could aim at, angular order,
   stepping from the currently picked monster. */
int PickNextTargetableMonster(int party_slot);
/* 0x0053C2C0: whether the slot holds a dead character still reachable for a
   targeting mode that admits one. */
char IsDeadCharacterTargetable(int party_slot);

/* 0x005360B0: the faction table index for a name, -1 when none matches. */
char FindFactionByName(const char* name);
