#pragma once

#include "wiz8/local_code/Factions.h"

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
char CountRowsBetween(int party_slot, W8MonsterInfo* monster_info);
/* Shorten the action range by intervening formation rows in combat. */
void CloseFormationGap(W8MonsterInfo* monster_info, int party_slot, W8RangeCategory* range);
/* Whether `party_slot` may aim at `monster_info`: live threat, the
   resolved action's range category (less the rows between in combat), the
   info's aim flag for the action's ranged-ness, and the band distance. A miss
   queues the slot's complaint event when `notify_failure` asks. */
bool CanPartyMemberAimAtMonster(int party_slot, int hand, W8MonsterInfo* monster_info,
                                W8TargetingContext context, bool notify_failure);
/* Whether the front rank stands between two formation positions. */
bool FrontRankScreens(unsigned int from_position, unsigned int to_position);
/* Whether the monster's attack `attack` reaches the character in `party_slot`,
   or the monster `target`, given what it can see and how far away they stand. */
bool MonsterAttackReachesCharacter(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                   unsigned int attack, int party_slot);
bool MonsterAttackReachesMonster(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                 unsigned int attack, W8MonsterInfo* target);
/* Whether the monster's attack reaches friendly or hostile targets. Berserk
   monsters always use friendly targets. */
bool MonsterAttackReachesAnyone(W8MonsterInfo* monster_info, unsigned int attack,
                                bool friendly_targets);
/* The base missile speed a range category grants `source`, in world units. */
float CalcRangeDistance(W8RangeCategory range_category, W8TargetSource* source);
/* The sight-condition slot a range band needs the observer's sight flags
   checked under: zero inside long range, the current condition beyond it. */
bool RangeCategoryUsesSightCondition(const W8MonsterInfo* monster, W8RangeCategory range_category);
bool MonsterActionReachesTarget(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                unsigned int attack, W8CombatSlot* target);
/* The location id of the nearest live, in-combat group member the monster can
   see under check `kind`, or -1 when none qualify. */
int FindNearestVisibleGroupMonster(W8MonsterInfo* monster_info, int group_id, int kind);
/* The furthest range band the monster can act at: its attacks first, then its
   castable spells. `skip_capability_checks` (callers pass 1 for reach/info)
   bypasses prefer-ranged/flee/usability gates; otherwise those gates apply.
   `out_sight` receives the sight-condition slot the band's target needs. */
W8RangeCategory GetMonsterBestRangeCategory(W8MonsterInfo* monster_info,
                                            bool skip_capability_checks, int* out_sight);
/* The range category the weapon in `hand` attacks at; `hand` of 2
   asks for the better of the two. */
W8RangeCategory GetCharAttackRange(const W8Character* character, unsigned int hand);
/* The range category the slot's chosen action works at. */
W8RangeCategory GetCharActionRange(int party_slot, int hand, W8TargetingContext context);
bool IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context, bool notify);
float MonsterChooseTarget(W8MonsterInfo* monster_info, W8CombatSlot* out, int kind);
float GetGroundTargetRange(void);
float GetMonsterEngagementRange(void);
bool AnyoneStandsAhead(unsigned char position);
void InitializeMonsterRangeCapabilities(W8MonsterInfo* monster_info, const W8MonsterRecord* record);
W8RangeCategory GetBestMonsterAttackRange(const W8MonsterRecord* record, bool close_quarters_only);
float CalcRangeDistance(W8RangeCategory range_category);
/* Same band steps as CalcRangeDistance, then add the party navigator's
   movement collision_radius (camera/party radius offset used by the world cursor). */
float CalcRangeDistanceFromParty(W8RangeCategory range_category);
/* The range category one monster action works at. */
W8RangeCategory GetMonsterActionRangeCategory(const W8MonsterInfo* monster_info,
                                              const W8MonsterRecord* record, unsigned int attack);
/* The furthest range category any of the character's hands can
   reach at. */
W8RangeCategory GetBestHandRangeCategory(const W8Character* character);
/* Whether the slot's chosen action in `context` still reaches its
   selected target - character range and front-rank screen, monster aim, place
   distance and line of sight, or group reach. `hand` selects the attack side;
   2 asks the range helpers to use the better hand. */
bool CharacterActionReachesTarget(int party_slot, int hand, W8TargetingContext context);
/* The slot-vs-slot form the target-list builder uses: whether the
   slot's chosen action in `context` can strike `target_slot`. */
bool CharacterActionReachesSlot(int party_slot, int hand, int target_slot,
                                W8TargetingContext context);
/* Collect the party slots the slot could reach and strike under
   `relationship`, and pick one at random; -1 when none qualify. */
int PickReachableSlotByDisposition(int party_slot, W8Disposition relationship);
/* Write `out` the offset from the monster's feet to the origin of
   its attack for `kind` - 1 the projectile muzzle, 3 the spell hand. */
void GetMonsterAttackSourceOffset(W8Monster* monster, int kind, srVector3T<float>* out);
/* Whether `source`'s queued action still reaches `target` -
   characters check their own current target, monsters check `target`. */
bool SourceActionReachesTarget(W8TargetSource* source, W8CombatSlot* target);
/* Whether the slot has any attack of `category` that reaches a
   valid target in the scanned groups for `hand`; the condition interrupt uses
   it to tell usable attacks from merely reachable ones. `flag` == 1 skips the
   monster scan. */
bool CanPartySlotAttackAnyTarget(int party_slot, W8TargetingContext category, int flag, bool hand);
