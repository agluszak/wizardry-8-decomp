#pragma once

#include "wiz8/layouts/targeting.h"
#include "wiz8/layouts/gameplay_databases.h"

template <class T> class srVector3T;

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;
struct W8Character;
class W8Monster;

/* Local Code\Combat Range.cpp: the party's own world position and the trace
   wrapper that decides whether a line of sight counts as unobstructed. */
unsigned char TraceModeRejectsNoHit(int mode);

/* How many formation rows between `party_slot` and the monster block a short
   reach: zero when they share a row, otherwise occupants ahead of the monster
   and (when the gap is exactly two rows) the front rank. */
char CountRowsBetween(int party_slot, W8MonsterInfo* monster_info); /* 0x0051AEC0 */
/* 0x005194E0: whether `party_slot` may aim at `monster_info`: live threat, the
   resolved action's range category (less the rows between in combat), the
   info's aim flag for the action's ranged-ness, and the band distance. A miss
   queues the slot's complaint event when `notify_failure` asks. */
bool CanPartyMemberAimAtMonster(int party_slot, int hand, W8MonsterInfo* monster_info, int context,
                                char notify_failure);
/* Whether the front rank stands between two formation positions. */
bool FrontRankScreens(unsigned int from_position, unsigned int to_position); /* 0x0051B000 */
/* Whether the monster's attack `attack` reaches the character in `party_slot`,
   or the monster `target`, given what it can see and how far away they stand. */
bool MonsterAttackReachesCharacter(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                   unsigned int attack, int party_slot); /* 0x0051A2F0 */
bool MonsterAttackReachesMonster(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                 unsigned int attack, W8MonsterInfo* target); /* 0x0051A510 */
/* Whether the monster's attack `attack` reaches anyone at all; `hostile_only`
   counts only those it is hostile to. */
unsigned char MonsterAttackReachesAnyone(W8MonsterInfo* monster_info, unsigned int attack,
                                         char hostile_only); /* 0x00519C00 */
/* The base missile speed a range category grants `source`, in world units. */
float CalcRangeDistance(int range_category, W8TargetSource* source); /* 0x0051AA30 */
/* The sight-condition slot a range band needs the observer's sight flags
   checked under: zero inside long range, the current condition beyond it. */
bool RangeCategoryUsesSightCondition(const W8MonsterInfo* monster,
                                     W8RangeCategory range_category); /* 0x00519BE0 */
unsigned char MonsterActionReachesTarget(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                         unsigned int attack,
                                         W8CombatSlot* target); /* 0x00519F80 */
/* The location id of the nearest live, in-combat group member the monster can
   see under check `kind`, or -1 when none qualify. */
int FindNearestVisibleGroupMonster(W8MonsterInfo* monster_info, int group_id,
                                   int kind); /* 0x0051AD60 */
/* The furthest range band the monster can act at: its attacks first, then its
   castable spells. `skip_capability_checks` (callers pass 1 for reach/info)
   bypasses prefer-ranged/flee/usability gates; otherwise those gates apply.
   `out_sight` receives the sight-condition slot the band's target needs. */
W8RangeCategory GetMonsterBestRangeCategory(W8MonsterInfo* monster_info,
                                            char skip_capability_checks,
                                            int* out_sight); /* 0x0051A840 */
/* 0x00519AC0: the range category the weapon in `hand` attacks at; `hand` of 2
   asks for the better of the two. */
int GetCharAttackRange(const W8Character* character, unsigned int hand);
/* 0x005199F0: the range category the slot's chosen action works at. */
int GetCharActionRange(int party_slot, int hand, W8TargetingContext context);
bool IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context,
                          char notify); /* 0x00519920 */
float MonsterChooseTarget(W8MonsterInfo* monster_info, W8CombatSlot* out, int kind);
float GetRangeConstant5EC35C(void);
float GetRangeConstant5EC360(void); /* 0x0051B300 */
bool AnyoneStandsAhead(unsigned char position);
void InitializeMonsterRangeCapabilities(W8MonsterInfo* monster_info,
                                        const W8MonsterRecord* record); /* 0x0051B420 */
W8RangeCategory GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only);
float CalcRangeDistance(W8RangeCategory range_category);
/* Same band steps as CalcRangeDistance, then add the party navigator's
   movement collision_radius_0b0 (camera/party radius offset used by the world cursor). */
float CalcRangeDistanceFromParty(W8RangeCategory range_category);
/* 0x0051A730: the range category one monster action works at. */
W8RangeCategory GetMonsterActionRangeCategory(const W8MonsterInfo* monster_info,
                                              const W8MonsterRecord* record, unsigned int attack);
/* 0x00519BA0: the furthest range category any of the character's hands can
   reach at. */
W8RangeCategory GetBestHandRangeCategory(const W8Character* character);
/* 0x00519180: whether the slot's chosen action in `context` still reaches its
   selected target - character range and front-rank screen, monster aim, place
   distance and line of sight, or group reach. `hand` selects the attack side;
   2 asks the range helpers to use the better hand. */
bool CharacterActionReachesTarget(int party_slot, int hand, W8TargetingContext context);
/* 0x005197C0: the slot-vs-slot form the target-list builder uses: whether the
   slot's chosen action in `context` can strike `target_slot`. */
char CharacterActionReachesSlot(int party_slot, int hand, int target_slot, int context);
/* 0x0051B0A0: collect the party slots the slot could reach and strike under
   `relationship`, and pick one at random; -1 when none qualify. */
int PickReachableSlotByDisposition(int party_slot, char relationship);
/* 0x0051B320: write `out` the offset from the monster's feet to the origin of
   its attack for `kind` - 1 the projectile muzzle, 3 the spell hand. */
void GetMonsterAttackSourceOffset(W8Monster* monster, int kind, srVector3T<float>* out);
/* 0x0051B4E0: whether `source`'s queued action still reaches `target` -
   characters check their own current target, monsters check `target`. */
char SourceActionReachesTarget(W8TargetSource* source, W8CombatSlot* target);
/* 0x00518E30: whether the slot has any attack of `category` that reaches a
   valid target in the scanned groups for `hand`; the condition interrupt uses
   it to tell usable attacks from merely reachable ones. `flag` == 1 skips the
   monster scan. */
char CanPartySlotAttackAnyTarget(int party_slot, int category, int flag, char hand);
