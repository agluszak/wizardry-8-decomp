#pragma once

#include "wiz8/monster_actions.h"
#include "wiz8/sight_state.h"

/* Disposition matches plus two wildcard selectors used by visibility scans. */
enum W8VisibleTargetFilter {
    W8_VISIBLE_TARGET_NEUTRAL = 0,
    W8_VISIBLE_TARGET_HOSTILE = 1,
    W8_VISIBLE_TARGET_FRIENDLY = 2,
    W8_VISIBLE_TARGET_ANY = 3,
    W8_VISIBLE_TARGET_NON_NEUTRAL = 4
};

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;
struct W8MonsterGroup;
template <class T> class W8GrowableVector;

/* The timed sight service: clears the sight-dirty flag, refreshes what
   monsters can see, and reruns group detection in combat when allowed. */
void UpdateMonsterSight(void);
/* The per-frame monster group pass; `staggered` spreads the maintenance a
   fifth of the list at a time with a full-distance pass every 20 frames. */
void UpdateMonsterGroups(bool staggered);
/* The group's party-sight aggregate DoMonsterRTAI reads as its alert level. */
W8SightState GetMonsterGroupPartySightState(W8MonsterGroup* monster_group);
/* The real-time AI decision pass; `engage` calls may trigger the ambush and
   group-alert paths a routine tick cannot. */
void DoMonsterRTAI(W8MonsterInfo* monster_info, bool engage);
/* The orders-driven half of DoMonsterRTAI: investigate a fresh heard noise or
   let the monster's scripted order_mode pick the next mode. `decision`
   receives the mode; nonzero return means applying it is worthwhile. */
bool ChooseMonsterRTAIMode(W8MonsterInfo* monster_info, unsigned char* decision);
void ApplyMonsterRTAIDecision(W8MonsterInfo* monster_info, unsigned char decision);
/* 125000, the cap on how far a monster will walk to investigate a
   heard noise. */
extern int g_noise_investigate_radius_cap;
/* Whether a hostile group still has a member able to engage the party: a
   visible target to advance on, a usable attack or spell, a way to flee, or
   a short-range attack with a path to the party. */
bool MonsterGroupCanEngage(W8MonsterGroup* monster_group);
/* Whether the monster has a living target it can see; `party_only` skips the
   monster scan, `hostility` selects the class (three and four are wildcards),
   and `within_reach` also requires the target inside engagement range. */
bool MonsterHasVisibleTarget(W8MonsterInfo* monster_info, bool party_only,
                             W8VisibleTargetFilter hostility, bool within_reach);
float GetGroupNearestDistance(W8MonsterGroup* group);

/* Two dwords per special attack kind. */
extern int g_special_attack_table[32][2];

bool AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id);
/* Whether the combat slot accepts `spell_id` from this caster. */
bool MonsterSpellTargetOK(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
/* Whether the spell's area effect would catch a disposition-neutral monster,
   which vetoes it - the AI does not turn neutrals hostile by accident. */
bool SpellAreaHitsNeutralMonster(W8MonsterInfo* monster_info, int spell_id,
                                 W8CombatSlot* combat_slot);
/* Whether the spell's markers catch at least one party member when cast at
   `slot`. */
bool MonsterSpellHasPartyTarget(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* slot);
/* Whether any live member of the group has a visible target; the arguments
   forward to MonsterHasVisibleTarget. */
bool MonsterGroupHasVisibleTarget(W8MonsterGroup* monster_group, bool party_only,
                                  W8VisibleTargetFilter hostility, bool within_reach);
/* The out-of-combat sweep: refreshes sight, alerts same-faction groups of
   groups already fighting, and enters combat for the groups that should. */
void CheckMonsterGroupsEnterCombat(void);
/* The in-combat sweep: drops combat for groups with nothing left to fight,
   and for every group at once once nobody is still engaged. */
void CheckMonsterGroupsLeaveCombat(void);
/* Whether a group's members still count as fighting; propagates the answer
   to the group and its allies through the engagement-state pair. */
void UpdateMonsterGroupEngagement(void);
bool IsMonsterActionUsable(W8MonsterInfo* monster_info);
/* Decide what one monster does this round: give up, hold, flee, cast, attack
   or move, then fall back to holding when the choice cannot be carried out. */
void UpdateMonsterAI(W8MonsterInfo* monster_info);
/* The percentage chance the monster advances on the party this round. */
unsigned int MonsterAdvanceChance(W8MonsterInfo* monster_info, W8MonsterRecord* record);
/* Whether the monster can flee at all: it has a flee chance, a flee
   animation, enough of its stat left, and somewhere to run. */
bool CanMonsterFlee(W8MonsterInfo* monster_info, W8MonsterRecord* record, bool exclude_special);
/* Pick the direction a fleeing monster runs; returns whether a heading was
   found. */
bool AimFleeingMonster(W8MonsterInfo* monster_info, const W8MonsterRecord* record);
/* Whether the monster may cast `spell_id` now; `needs_target` also demands
   something to aim it at. */
bool IsSpellUsableByMonster(W8MonsterInfo* monster_info, int spell_id, bool needs_target);
/* Whether a monster can aim the spell it wants to cast - area
   target types aim at the world, the rest pass the slot check. */
bool CanMonsterAimSpell(W8MonsterInfo* monster_info, int spell_id);
/* Whether the control spell's visual anchor sits in range of the
   monster, falling back on where the party stands. */
short IsMonsterControlPointInRange(W8MonsterInfo* monster_info);
/* Which of the monster's ten spells to cast, weighted by the spell table. */
int ChooseMonsterSpell(W8MonsterInfo* monster_info, W8MonsterRecord* record);
/* Whether the monster sees no living enemy: a set threat flag answers at
   once, a hostile party member in play counts, and `party_only` zero also
   scans the monsters it is hostile to that it can see. */
bool MonsterHasNoVisibleEnemy(W8MonsterInfo* monster_info, bool party_only);
/* Fill `targets` with the combat slots `spell_id` may be cast at by this
   monster; the monster's action fields carry the spell while the probe runs
   and are restored after. */
void CollectMonsterSpellTargets(W8MonsterInfo* monster_info, int spell_id,
                                W8GrowableVector<W8CombatSlot>* targets);
/* Whether at least half of the group's live in-combat members the caster is
   hostile to accept the spell. */
bool MonsterGroupHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id,
                                       W8MonsterGroup* target_group);
/* Whether at least half of the party members the caster is hostile to accept
   the spell. */
bool PartyHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id);
/* Whether a live hostile monster already fighting stands nearer to one of the
   group's members than to the party - reinforcement keeps the group in or
   brings it into combat. */
bool MonsterGroupHasReinforcement(W8MonsterGroup* monster_group);
/* Whether a group outside combat should join it: neutral groups need a
   reinforcement, hostile ones a member with a visible target inside the
   leader's reach or a rendered member near the party. */
bool ShouldMonsterGroupEnterCombat(W8MonsterGroup* monster_group);
/* Refill the monster's pending-action queue with everything it may do this
   round: cooperative and special actions first, then one attack entry per
   attack-mode bit for every target the attack can reach. `target_locked`
   restricts the sweep to the stored target, `attack_locked` to the committed
   attack index. */
void BuildMonsterActionQueue(W8MonsterInfo* monster_info, bool target_locked, bool attack_locked);
/* Build the monster's list of possible actions and take one of them at
   random into its action fields and target. */
bool ChooseRandomMonsterAction(W8MonsterInfo* monster_info, bool target_locked, bool attack_locked,
                               bool set_attack_rate);
void UpdateAllMonsterAI(void);
