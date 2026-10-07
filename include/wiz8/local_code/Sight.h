#pragma once

#include "surrender/srMath.h"

struct W8MonsterInfo;
struct W8MonsterGroup;
struct W8VisibilityRecord;

void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
void RefreshAllSight(void);
/* Put the sight subsystem back to its starting state. */
void ResetSight(void);
void RefreshOutwardSightForAllMonsters(void);
void RefreshInwardSightForAllMonsters(void);
void RefreshMonsterSight(W8MonsterInfo* monster_info);
void ResetAndRefreshAllSight(void);
unsigned int AgeAllMonsterSight(void);
void UpdateMonsterSight(W8MonsterInfo* monster_info, bool direction, bool use_bounds);

/* Monster-to-monster sight after line of sight is already clear. */
bool CanMonsterSeeMonster(W8MonsterInfo* source, W8MonsterInfo* target, W8VisibilityRecord* record);

/* Shared perception primitive for monster/player and player/monster checks.
   penalty_source is party minimum level or monster missile value; twice it
   comes off perception_attribute in the base factor. penalty_modifier applies
   a -10 or -5 penalty per unit depending on
   skip_field_of_view, and sight_override forces full range. */
float ComputeSightThreshold(srVector3T<float> observer_position, srVector3T<float> target_position,
                            float observer_yaw, unsigned int perception_attribute,
                            unsigned char ranged_bonus, bool blinded, bool extended_sight_active,
                            int penalty_source, int penalty_modifier, int skip_field_of_view,
                            unsigned char sight_override, float distance);

bool MonsterGroupHasVisibleThreat(W8MonsterGroup* group);
bool IsSightRangeOverridden(void);
bool MonsterHasProjectileOrigin(const W8MonsterInfo* monster);
char GetMonsterSpellSightIndex(const W8MonsterInfo* monster);

extern const float g_sight_default;

bool IsVisibleUnderConditions(const W8MonsterInfo* monster, const W8VisibilityRecord* row,
                              int kind);
/* Whether one group's leader member can see the other's: far-clip distance
   first, then the outward sight record and a line-of-sight trace. */
bool MonsterGroupCanSeeGroup(W8MonsterGroup* source, W8MonsterGroup* target);
W8VisibilityRecord* FindMonToMonVisibility(W8MonsterInfo* source, W8MonsterInfo* target);
