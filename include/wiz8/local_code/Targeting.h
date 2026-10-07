#pragma once

#include "wiz8/layouts/targeting.h"
#include "wiz8/layouts/gameplay_databases.h"

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
char GetSourceNoticeColor(const W8TargetSource* source);
char GetTargetNoticeColor(const W8TargetSource* source, const W8CombatSlot* target);

unsigned char GetFactionFlag(signed char faction);
/* One faction's last band-change world-clock stamp. */
int GetFactionValue(signed char faction);
void AimByKind(int actor, W8TargetKind kind, W8TargetingContext context);
void SetMonsterCombatTarget(W8MonsterInfo* monster_info, int location_id);
bool MonsterTargetMatchesSpell(W8MonsterInfo* monster_info, int spell_id);
W8CombatSlot* GetTargetBlockForContext(int party_slot, W8TargetingContext context);
void ClearTargetMarker(void);
void RefreshTargetMarker(void);
void RefreshAllPartyTargets(void);
bool RepickActionTarget(int party_slot, W8TargetingContext context, int arg);
void AimAtTarget(int actor, W8CombatSlot* target, W8TargetingContext context);
void AimAtCharacter(int actor, int character_slot, W8TargetingContext context);
void AimAtCharacterIndirect(int actor, int character_slot, W8TargetingContext context);
void AimAtMonsterLocation(int party_slot, int location_id, bool allow_single_target);
void AimAtPlace(int actor);
void AimAtGroundTarget(int party_slot);
/* Raise or clear per-monster highlight bits for a party slot. */
void UpdateSlotMonsterHighlights(int party_slot, bool enable);
void RefreshCombatTargetHighlights(int party_slot, W8CombatSlot* target);
void HighlightSpellTargetsAtCachedPosition(void);
/* Pick an attack fallback, allowing the character's alternate weapon set when
   the ordinary group selection has no usable monster. */
int ChooseFallbackMonsterTarget(int party_slot, int group_id, W8TargetingContext context);
void SetFactionFlag(signed char faction, bool flag);
unsigned char ShowMonsterTargetMarker(W8MonsterInfo* monster_info);
bool IsSpellTargetStillValidIn(int party_slot, int spell_id, W8TargetingContext context);
void ClearTargetHighlights(int party_slot, const W8CombatSlot* target);
void ClearPartySlotMonsterHighlights(unsigned int party_slot);
void SetTargetToCharacter(int character_slot, bool in_combat);

void SetTargetingMode(W8TargetNeed state);
void SetMonsterHighlight(int party_slot, int location_id, bool on);
void SetGroupHighlight(int party_slot, int group_id, bool on);
/* Location id of the nearest hovered live monster, or -1. */
int PickNearestMonsterUnderCursor(int cursor_x, int cursor_y);
char HighlightMonsterAsTarget(int location_id, int party_slot, char highlight);
void UpdateAllMonsterHighlights(int party_slot, int location_id);
/* Cursor-table index for SetTargetCursor, including slots 10..12. Not a
   W8TargetingContext. */
int GetTargetingCursorForState(int alternate);
bool ActionNeedsExplicitTarget(int party_slot);
unsigned int GetActionSpellLikeId(int party_slot, W8TargetingContext context);
void ResetTargetSource(W8TargetSource* source);
void SetTargetSourceToMonster(const W8MonsterInfo* monster_info, W8TargetSource* source);
W8TargetingContext ResolveTargetingContext(int party_slot, W8TargetingContext context);
bool TargetMatchesNeeded(W8CombatSlot* target, W8TargetNeed needed);
bool SpellHasAnyValidTarget(int party_slot, int spell_id, bool normalize);
void SetTargetToMonster(int monster_id, bool in_combat);
void SetTargetToGroup(int group_id, bool in_combat);

bool ClearMonsterCombatSlot(W8MonsterInfo* monster_info);

/* Fill the slot's point from where its target is. */
bool ResolveTargetPoint(W8CombatSlot* target, bool sight_probe);
void AimCombatSlotAtParty(W8CombatSlot* combat_slot, int hostile);
void ApplyTarget(W8CombatSlot* target, bool in_combat);
/* Whether an actor aiming at `target` should drop that aim now
   that `target` has been (re)applied. An out-of-combat application always clears;
   otherwise the actor and the applied target must agree on hostility with the
   action's enemy-aimed flag. */
bool ShouldClearAimForAppliedTarget(W8TargetSource* source, W8CombatSlot* target, bool in_combat,
                                    bool action_targets_enemies);
bool IsTargetStillPresent(const W8CombatSlot* target);
bool IsTargetSourceInRangeOfGroup(const W8TargetSource* source, W8MonsterGroup* group,
                                  W8TargetingContext context);

bool CanTargetMonster(int party_slot, int location_id, bool allow_single_target,
                      bool notify_failure);
bool CanTargetMonsterGroup(int party_slot, W8MonsterGroup* group);
void ClearSlotTargeting(int party_slot);
/* Whether the slot's current target satisfies the spell's
   needed-target kind. */
bool IsSpellTargetOfNeededKind(int party_slot, int spell_id);
/* Whether the slot's current target satisfies the item's
   needed-target kind. */
bool IsItemTargetOfNeededKind(int party_slot, const W8ItemInstance* item);
/* Select the party slot the spell-casting view is casting for. */
void SelectSpellCastingPartySlot(int party_slot);
/* Set the targeting filter for the spell being aimed. */
void ConfigureSpellTargetFilter(W8SpellTargetType target_type, W8TargetNeed needed_kind);
/* Commit the chosen spell target. */
void CommitSelectedSpellTarget(void);
void RefreshMonsterTargetCounts(void);
bool AnyMonsterVisible(void);
void UpdateTargetMarkerHighlight(void);
/* Fill `found` with the location ids of monsters within `radius`
   of `centre` that are visible from `eye` (unless highlighting is on, which
   takes them without the sight check and tints instead). */
void CollectMonstersWithinRadius(const srVector3T<float>* centre, const srVector3T<float>* eye,
                                 W8GrowableVector<int>* found, float radius, char side,
                                 char highlighting);

class W8Monster;
/* Whether `monster` is within `max_distance` of the player and
   still projects on screen. `position` is unused. */
bool IsMonsterVisibleWithinDistance(W8Monster* monster, const srVector3T<float>* position,
                                    float max_distance);
/* Pick the next targetable member of `group` and paint it with the
   `color` highlight (0 clears, 1 green, 2 red). */
void HighlightPickedGroupMember(int party_slot, W8MonsterGroup* group, W8TargetHighlight color);
/* Apply a highlight tint (0 clears, 1 green, 2 red) to every member
   of the group - the debug message names it ModifyGroupColor. */
void ModifyGroupColor(int group_id, W8TargetHighlight color);
/* Whether `target` lies within the combined radii of `eye` and
   `bonus`, inside the heading and elevation arcs. */
bool TargetInRangeAndArcs(const srVector3T<float>* target, float bonus,
                          const srVector3T<float>* eye, float eye_radius, float heading,
                          float elevation);
/* Append the location ids of live targetable monsters inside the
   aim cone to `found`, filtered by disposition (3 admits all); returns how many
   were added. */
int CollectConeMonsterTargets(const W8TargetSource* source, const srVector3T<float>* eye,
                              float heading, float elevation, W8GrowableVector<int>* found,
                              unsigned char disposition, int sight_flag);
/* Whether the pending item use needs a target picked. */
bool ItemUseNeedsTarget(int party_slot);
void RefreshSpellTargetHighlightsAtRange(void);
void PopulateTargetMarkerForCurrentAction(const srVector3T<float>* position,
                                          W8GrowableVector<int>* marker_vector, int enabled);
W8TargetingContext GetCombatActionContext(int party_slot);
void ReconcilePartyEquipmentAfterCombat(void);
bool TargetIsInPlay(int party_slot, int hand,
                    W8TargetingContext context = W8_TARGETING_CONTEXT_OUT_OF_COMBAT);

void ClearAllMonsterHighlights(void);
/* Combat action-selection helpers used across the combat units. */
bool CanPartySlotParticipate(int party_slot);
W8TargetingContext GetValidatedTargetingContext(int party_slot, W8TargetingContext context);
void SetTargetSourceToCharacter(int party_slot, W8TargetSource* source);
/* What the interface has to ask the player to pick for one
   action - a fixed kind for most, the spell's answer for casts and item use. */
W8TargetNeed GetTargetNeededForAction(W8ActionKind action, int spell_id,
                                      const W8ActionDetailBlock* detail_block);
/* The target kind the slot's current action needs in the effective
   targeting context, with the main-screen selection state folded in. */
W8TargetNeed GetTargetNeededForCurrentAction(int party_slot);
W8TargetNeed GetTargetNeededForItem(const W8ItemInstance* item);
void TintHighlightedMonster(W8Monster* monster, W8TargetHighlight tint);
/* After the selected character's action changes, drop back to no
   targeting when its recorded target still fits, or enter the mode the action
   now needs. */
void RevalidateSelectedTarget(int party_slot);
/* Cycle the pick within a group and aim the party slot at it. */
void AimAtMonsterGroupMember(int party_slot, W8MonsterGroup* group);
/* The cycle-target command - advance the pick to the next
   targetable group or monster and commit it. */
void CycleToNextTarget(int party_slot);
bool SlotHasAnyValidTarget(int party_slot);
/* Every monster the slot's action could aim at, angular order,
   stepping from the currently picked monster. */
int PickNextTargetableMonster(int party_slot);
/* Whether the slot holds a dead character still reachable for a
   targeting mode that admits one. */
bool IsDeadCharacterTargetable(int party_slot);

/* The faction table index for a name, -1 when none matches. */
signed char FindFactionByName(const char* name);
void RepickInvalidCombatTargets(void);
