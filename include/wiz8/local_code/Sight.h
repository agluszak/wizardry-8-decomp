#pragma once

struct W8MonsterInfo;

void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
void RefreshAllSight(void);
void RefreshOutwardSightForAllMonsters(void);
void RefreshMonsterSight(W8MonsterInfo* monster_info);
unsigned int AgeAllMonsterSight(void);

bool IsSightRangeOverridden(void);

extern float g_sight_default_005ec254;
