#pragma once

#include "wiz8/layouts/targeting.h"
#include "wiz8/layouts/gameplay_databases.h"

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;
struct W8Character;

/* Local Code\Combat Range.cpp: the party's own world position and the trace
   wrapper that decides whether a line of sight counts as unobstructed. */
unsigned char TraceModeRejectsNoHit0051B3F0(int mode);

unsigned char CanReachTarget(int party_slot, int kind, W8MonsterInfo* monster_info,
                             W8TargetingContext context, int arg_5);
/* How many formation rows between `party_slot` and the monster block a short
   reach: zero when they share a row, otherwise occupants ahead of the monster
   and (when the gap is exactly two rows) the front rank. */
char CountRowsBetween(int party_slot, W8MonsterInfo* monster_info); /* 0x0051AEC0 */
unsigned char IsCurrentTargetInRange(int party_slot, int action, W8TargetingContext context);
/* 0x005194E0: whether `party_slot` may aim at `monster_info` under mode `arg_2`. */
unsigned char CanPartyMemberAimAtMonster(int party_slot, int action, W8MonsterInfo* monster_info,
                                         int failure_event, int notify_failure);
/* Whether the front rank stands between two formation positions. */
bool FrontRankScreens(unsigned int from_position, unsigned int to_position); /* 0x0051B000 */
/* Whether the monster's attack `attack` reaches the character in `party_slot`,
   or the monster `target`, given what it can see and how far away they stand. */
unsigned char MonsterAttackReachesCharacter(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                            unsigned int attack, int party_slot); /* 0x0051A2F0 */
unsigned char MonsterAttackReachesMonster(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                          unsigned int attack,
                                          W8MonsterInfo* target); /* 0x0051A510 */
/* Whether the monster's attack `attack` reaches anyone at all; `hostile_only`
   counts only those it is hostile to. */
unsigned char MonsterAttackReachesAnyone(W8MonsterInfo* monster_info, unsigned int attack,
                                         char hostile_only); /* 0x00519C00 */
/* The sight-condition slot a range band needs the observer's sight flags
   checked under: zero inside long range, the current condition beyond it. */
unsigned char RangeCategoryUsesSightCondition(const W8MonsterInfo* monster,
                                              W8RangeCategory range_category); /* 0x00519BE0 */
unsigned char MonsterActionReachesTarget(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                                         int attack, W8CombatSlot* target);
/* The furthest range band the monster can act at: its attacks first, then its
   castable spells. `skip_capability_checks` (callers pass 1 for reach/info)
   bypasses prefer-ranged/flee/usability gates; otherwise those gates apply.
   `out_sight` receives the sight-condition slot the band's target needs. */
W8RangeCategory GetMonsterBestRangeCategory(W8MonsterInfo* monster_info,
                                            char skip_capability_checks,
                                            int* out_sight); /* 0x0051A840 */
/* 0x00519AC0: the range category the weapon in `hand` attacks at; `hand` of 2
   asks for the better of the two. */
int GetCharAttackRange(W8Character* character, unsigned int hand);
/* 0x005199F0: the range category the slot's chosen action works at. */
int GetCharActionRange(int party_slot, int hand, W8TargetingContext context);
bool IsSlotInRangeOfGroup(int party_slot, int group_id, W8TargetingContext context,
                          char notify); /* 0x00519920 */
float MonsterChooseTarget(W8MonsterInfo* monster_info, int* out, int kind);
float GetRangeConstant5EC35C(void);
float GetRangeConstant5EC360(void); /* 0x0051B300 */
bool AnyoneStandsAhead(unsigned char position);
void InitializeMonsterRangeCapabilities(W8MonsterInfo* monster_info,
                                        const W8MonsterRecord* record); /* 0x0051B420 */
W8RangeCategory GetBestMonsterAttackRange(const W8MonsterRecord* record, char close_quarters_only);
float CalcRangeDistance(W8RangeCategory range_category);
/* Same band steps as CalcRangeDistance, then add the party navigator's
   movement value_0b0 (camera/party radius offset used by the world cursor). */
float CalcRangeDistanceFromParty0051AB50(W8RangeCategory range_category);
/* 0x0051A730: the range category one monster action works at. */
W8RangeCategory GetMonsterActionRangeCategory(const W8MonsterInfo* monster_info,
                                              const W8MonsterRecord* record, unsigned int attack);
