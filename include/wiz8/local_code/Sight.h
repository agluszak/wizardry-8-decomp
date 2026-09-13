#pragma once

#include "surrender/srMath.h"

struct W8MonsterInfo;
struct W8VisibilityRecord;

void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
void RefreshAllSight(void);
void RefreshOutwardSightForAllMonsters(void);
void RefreshMonsterSight(W8MonsterInfo* monster_info);
void ResetAndRefreshAllSight005060C0(void);
unsigned int AgeAllMonsterSight(void);
void UpdateMonsterSight(W8MonsterInfo* monster_info, int direction, int use_bounds);

/* Monster-to-monster sight after line of sight is already clear. */
unsigned char CanMonsterSeeMonster(W8MonsterInfo* source, W8MonsterInfo* target,
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

bool IsSightRangeOverridden(void);
bool GetSightCondition37A(const W8MonsterInfo* monster);

extern float g_sight_default_005ec254;

bool IsVisibleUnderConditions(const W8MonsterInfo* monster, const W8VisibilityRecord* row,
                              int kind); /* 0x00504B00 */
W8VisibilityRecord* FindMonToMonVisibility(W8MonsterInfo* source,
                                           W8MonsterInfo* target); /* 0x00504C20 */
