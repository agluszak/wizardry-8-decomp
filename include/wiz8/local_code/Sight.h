#pragma once

#include "surrender/srMath.h"

struct W8MonsterInfo;
struct W8MonsterGroup;
struct W8VisibilityRecord;

void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
void RefreshAllSight(void);
/* 0x005048E0: put the sight subsystem back to its starting state. */
void ResetSight(void);
void RefreshOutwardSightForAllMonsters(void);
void RefreshInwardSightForAllMonsters(void);
void RefreshMonsterSight(W8MonsterInfo* monster_info);
void ResetAndRefreshAllSight(void);
unsigned int AgeAllMonsterSight(void);
void UpdateMonsterSight(W8MonsterInfo* monster_info, int direction, int use_bounds);

/* Monster-to-monster sight after line of sight is already clear. */
bool CanMonsterSeeMonster(W8MonsterInfo* source, W8MonsterInfo* target,
                          W8VisibilityRecord* record); /* 0x005058A0 */

/* Shared perception primitive for monster/player and player/monster checks.
   Retail passes observer and target as by-value srVector3T<float> blocks.
   penalty_source is party minimum level or monster missile value;
   retail subtracts twice penalty_source from perception_attribute in the base
   factor. penalty_modifier applies a -10 or -5 penalty per unit depending on
   skip_field_of_view, and sight_override forces full range. */
float ComputeSightThreshold(srVector3T<float> observer_position, srVector3T<float> target_position,
                            float observer_yaw, unsigned int perception_attribute,
                            unsigned char ranged_bonus, unsigned char blinded,
                            unsigned char extended_sight_active, int penalty_source,
                            int penalty_modifier, int skip_field_of_view,
                            unsigned char sight_override, float distance); /* 0x00505A40 */

bool MonsterGroupHasVisibleThreat(W8MonsterGroup* group);
bool IsSightRangeOverridden(void);
bool GetSightCondition37A(const W8MonsterInfo* monster);
char GetSightCondition37CIndex(const W8MonsterInfo* monster); /* 0x00505E80 */

extern float g_sight_default_005ec254;

bool IsVisibleUnderConditions(const W8MonsterInfo* monster, const W8VisibilityRecord* row,
                              int kind); /* 0x00505DD0 */
/* Whether one group's leader member can see the other's: far-clip distance
   first, then the outward sight record and a line-of-sight trace. */
bool MonsterGroupCanSeeGroup(W8MonsterGroup* source, W8MonsterGroup* target); /* 0x00505F30 */
W8VisibilityRecord* FindMonToMonVisibility(W8MonsterInfo* source,
                                           W8MonsterInfo* target); /* 0x00505D20 */
