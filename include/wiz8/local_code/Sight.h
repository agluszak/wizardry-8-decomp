#pragma once

struct W8MonsterInfo;
struct W8SightConditions;

void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
void RefreshAllSight(void);
void RefreshOutwardSightForAllMonsters(void);
void RefreshMonsterSight(W8MonsterInfo* monster_info);
void ResetAndRefreshAllSight005060C0(void);
unsigned int AgeAllMonsterSight(void);
void UpdateMonsterSight(W8MonsterInfo* monster_info, int direction, int arg_3);

bool IsSightRangeOverridden(void);
bool GetSightCondition37A(const W8SightConditions* conditions);

extern float g_sight_default_005ec254;
