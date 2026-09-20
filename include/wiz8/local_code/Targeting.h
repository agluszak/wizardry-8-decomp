#pragma once

#include "wiz8/layouts/targeting.h"

struct W8CombatSlot;
struct W8MonsterInfo;
struct W8MonsterGroup;
template <class T> class srVector3T;
template <class T> class W8GrowableVector;

void ResetCombatSlot(W8CombatSlot* slot);

/* Answer which of the real contexts applies right now. Kept after the enum so
   its result type is the domain rather than a plain int. */
W8TargetingContext GetCurrentTargetingContext(int party_slot);

bool TargetSourceIsCharacter(const W8TargetSource* source, int allow_indirect);
bool TargetSourceIsMonster(const W8TargetSource* source, int allow_indirect);

/* Notice colours for combat text: 8 for a party-side source, 9 for a monster
   and 12 for anything else. The target form colours a character target by
   its marching-order slot instead. */
char GetSourceNoticeColor(const W8TargetSource* source); /* 0x0053C320 */
char GetTargetNoticeColor(const W8TargetSource* source,
                          const W8CombatSlot* target); /* 0x0053C3F0 */

/* ABS 0x0068407F: cached world-space target point refreshed by the targeting
   refresh path. */
/* ABS 0x0068408B: the shared combat slot the context-2 outputs name. */
/* ABS 0x006840AB: one action-detail block following the shared slot; the
   eight bytes fit exactly before gXStatus.picked_monster. Context 2 hands out
   both addresses and stores the selected use-item value in the block. */

unsigned char GetFactionFlag(char faction);
/* One faction's last band-change world-clock stamp. */
int GetFactionValue(char faction); /* 0x005360f0 */
void AimByKind(int actor, W8TargetKind kind, W8TargetingContext context);
void SetMonsterCombatTarget(W8MonsterInfo* monster_info, int location_id);
bool MonsterTargetMatchesSpell(W8MonsterInfo* monster_info, int spell_id);
W8CombatSlot* GetTargetBlockForContext(int party_slot, W8TargetingContext context);
void ClearTargetMarker(void);
void RefreshTargetMarker(void);
void RefreshAllPartyTargets(void);
bool RepickActionTarget(int party_slot, W8TargetingContext context, int arg);
void AimAtTarget(int actor, W8CombatSlot* target, W8TargetingContext context);  /* 0x005387F0 */
void AimAtCharacter(int actor, int character_slot, W8TargetingContext context); /* 0x00538670 */
void AimAtCharacterIndirect(int actor, int character_slot,
                            W8TargetingContext context); /* 0x005386C0 */
void AimAtMonsterLocation00537950(int party_slot, int location_id,
                                  int allow_single_target); /* 0x00537950 */
void AimAtPlace(int actor);                                 /* 0x00538710 */
void AimAtGroundTarget00538770(int party_slot);             /* 0x00538770 */
/* 0x0053C130: raise or clear per-monster highlight bits for a party slot. */
void UpdateSlotMonsterHighlights0053C130(int party_slot, char enable);
void RefreshCombatTargetHighlights(int party_slot, W8CombatSlot* target);
void HighlightSpellTargetsAtCachedPosition(void);
/* Pick an attack fallback, allowing the character's alternate weapon set when
   the ordinary group selection has no usable monster. */
int ChooseFallbackMonsterTarget0053C990(int party_slot, int group_id, W8TargetingContext context);
void SetFactionFlag(char faction, unsigned char flag);
bool ShowMonsterTargetMarker(W8MonsterInfo* monster_info);
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, W8TargetingContext context);
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target);
void ClearPartySlotMonsterHighlights(unsigned int party_slot);
void SetTargetToCharacter(int character_slot, W8TargetingContext context);

void SetTargetingMode(int state);
void SetMonsterHighlight(int party_slot, int location_id, char on);
void SetGroupHighlight(int party_slot, int group_id, char on);
/* Location id of the nearest hovered live monster, or -1. */
int PickNearestMonsterUnderCursor005396D0(int cursor_x, int cursor_y); /* 0x005396D0 */
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
bool SpellHasAnyValidTarget(int party_slot, int spell_id, unsigned char normalize);
void SetTargetToMonster(int monster_id, W8TargetingContext context);
void SetTargetToGroup(int group_id, W8TargetingContext context);

bool ClearMonsterCombatSlot(W8MonsterInfo* monster_info);

/* 0x0053C630: fill the slot's point from where its target is. */
bool ResolveTargetPoint(W8CombatSlot* target, char sight_probe);
void AimCombatSlotAtParty(W8CombatSlot* combat_slot, int hostile);
void ApplyTarget(W8CombatSlot* target, W8TargetingContext context); /* 0x00538E00 */
/* 0x0053C490: whether an actor aiming at `target` should drop that aim now
   that `target` has been (re)applied. Out-of-combat contexts always clear;
   otherwise the actor and the applied target must agree on hostility with the
   action's enemy-aimed flag. */
bool ShouldClearAimForAppliedTarget(W8TargetSource* source, W8CombatSlot* target,
                                    W8TargetingContext context,
                                    unsigned char action_targets_enemies);
bool IsTargetStillPresent(const W8CombatSlot* target);
bool IsTargetSourceInRangeOfGroup(const W8TargetSource* source, W8MonsterGroup* group,
                                  W8TargetingContext context);
void NoteTargetChosen(const W8TargetSource* source, const W8CombatSlot* target);

bool CanTargetMonster(int party_slot, int location_id, int allow_single_target,
                      int reason);                                 /* 0x00536AD0 */
bool CanTargetMonsterGroup(int party_slot, W8MonsterGroup* group); /* 0x00536D60 */
void ClearTargetingMode(int party_slot);
void ClearSlotTargeting0053B050(int party_slot); /* 0x0053B050 */
/* 0x00537270: whether the slot's current target satisfies the spell's
   needed-target kind. */
bool IsSpellTargetOfNeededKind(int party_slot, int spell_id);
bool IsItemTargetOfNeededKind(int party_slot, const W8ItemInstance* item); /* 0x005372B0 */
/* 0x0053AF40: select the party slot the spell-casting view is casting for. */
void SelectSpellCastingPartySlot(int party_slot);
/* 0x0053A440: set the targeting filter for the spell being aimed. */
void ConfigureSpellTargetFilter(int target_type, unsigned int needed_kind);
/* 0x0053A830: commit the chosen spell target. */
void CommitSelectedSpellTarget(void);
bool IsItemTargetOfNeededKind(int party_slot, const W8ItemInstance* item);
void RefreshMonsterTargetCounts005398D0(void);
bool AnyMonsterVisible0053A1D0(void);
void UpdateTargetMarkerHighlight0053B1D0(void);
/* 0x00539E70: fill `found` with the location ids of monsters within `radius`
   of `centre` that are visible from `eye` (unless highlighting is on, which
   takes them without the sight check and tints instead). */
void CollectMonstersWithinRadius(const srVector3T<float>* centre, const srVector3T<float>* eye,
                                 W8GrowableVector<int>* found, float radius, char side,
                                 char highlighting);

class W8Monster;
/* 0x0053A060: whether `monster` is within `max_distance` of the player and
   still projects on screen. `position` is unused by retail. */
bool IsMonsterVisibleWithinDistance0053A060(W8Monster* monster, const srVector3T<float>* position,
                                            float max_distance);
/* 0x00538510: pick the next targetable member of `group` and paint it with the
   `color` highlight (0 clears, 1 green, 2 red). */
void HighlightPickedGroupMember00538510(int party_slot, W8MonsterGroup* group, int color);
/* 0x005392E0: apply a highlight tint (0 clears, 1 green, 2 red) to every member
   of the group - the debug message names it ModifyGroupColor. */
void ModifyGroupColor(int group_id, int color);
/* 0x00539B70: whether `target` lies within the combined radii of `eye` and
   `bonus`, inside the heading and elevation arcs. */
bool TargetInRangeAndArcs00539B70(const srVector3T<float>* target, float bonus,
                                  const srVector3T<float>* eye, float eye_radius, float heading,
                                  float elevation);
/* 0x00539CA0: append the location ids of live targetable monsters inside the
   aim cone to `found`, filtered by disposition (3 admits all); returns how many
   were added. */
int CollectConeMonsterTargets00539CA0(const W8TargetSource* source, const srVector3T<float>* eye,
                                      float heading, float elevation, W8GrowableVector<int>* found,
                                      unsigned char disposition, int sight_flag);
/* 0x0053A770: whether the pending item use needs a target picked. */
bool ItemUseNeedsTarget0053A770(int party_slot);
void RefreshSpellTargetHighlightsAtRange(void);
void PopulateTargetMarkerForCurrentAction(const srVector3T<float>* position,
                                          W8GrowableVector<int>* marker_vector, int enabled);
W8TargetingContext GetCombatActionContext0053BC90(int party_slot); /* 0x0053BC90 */
void ReconcilePartyEquipmentAfterCombat0053CD60(void);             /* 0x0053CD60 */
bool TargetIsInPlay(
    int party_slot, int value,
    W8TargetingContext context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT); /* 0x00536F60 */

void ClearAllMonsterHighlights(void); /* 0x0053AE00 */
/* Combat action-selection helpers used across the combat units. */
bool CanPartySlotParticipate(int party_slot); /* 0x0053C270 */
W8TargetingContext GetValidatedTargetingContext(int party_slot,
                                                W8TargetingContext context); /* 0x0053BBD0 */
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source);     /* 0x0053BE00 */
/* 0x00536A20: what the interface has to ask the player to pick for one
   action - a fixed kind for most, the spell's answer for casts and item use. */
int GetTargetNeededForAction(int action, int spell_id, const W8ActionDetailBlock* detail_block);
/* 0x00537380: the target kind the slot's current action needs in the effective
   targeting context, with the main-screen selection state folded in. */
int GetTargetNeededForCurrentAction(int party_slot);
int GetTargetNeededForItem(const W8ItemInstance* item);    /* 0x00537330 */
void TintHighlightedMonster(W8Monster* monster, int tint); /* 0x00539480 */
/* 0x00537540: after the selected character's action changes, drop back to no
   targeting when its recorded target still fits, or enter the mode the action
   now needs. */
void RevalidateSelectedTarget(int party_slot);
/* 0x00537B00: cycle the pick within a group and aim the party slot at it. */
void AimAtMonsterGroupMember(int party_slot, W8MonsterGroup* group);
/* 0x00537D20: the cycle-target command - advance the pick to the next
   targetable group or monster and commit it. */
void CycleToNextTarget(int party_slot);
bool SlotHasAnyValidTarget(int party_slot); /* 0x0053CDF0 */
/* 0x00538140: every monster the slot's action could aim at, angular order,
   stepping from the currently picked monster. */
int PickNextTargetableMonster(int party_slot);
/* 0x0053C2C0: whether the slot holds a dead character still reachable for a
   targeting mode that admits one. */
bool IsDeadCharacterTargetable(int party_slot);

/* 0x005360B0: the faction table index for a name, -1 when none matches. */
char FindFactionByName(const char* name);
void RepickInvalidCombatTargets00536400(void);
