#pragma once

struct W8MonsterInfo;
struct W8VisibilityRecord;

void ReleaseMonToMonVisibilityList(W8MonsterInfo* monster_info);
void RefreshAllSight(void);
void RefreshOutwardSightForAllMonsters(void);
void RefreshMonsterSight(W8MonsterInfo* monster_info);
void ResetAndRefreshAllSight005060C0(void);
unsigned int AgeAllMonsterSight(void);
void UpdateMonsterSight(W8MonsterInfo* monster_info, int direction, int use_bounds);

/* The two sight producers still unrecovered keep address names. */
unsigned char Function5058A0(
    W8MonsterInfo* source, W8MonsterInfo* target,
    W8VisibilityRecord* record);             /* 0x005058A0 */
float Function505A40(
    float subject_x, float subject_y, float subject_z,
    float target_x, float target_y, float target_z,
    float yaw, unsigned int attribute, int missile_bonus,
    unsigned char blind, unsigned char spell_kind, int arg_12, int arg_13,
    int arg_14, int arg_15, float distance); /* 0x00505A40 */

bool IsSightRangeOverridden(void);
bool GetSightCondition37A(const W8MonsterInfo* monster);

extern float g_sight_default_005ec254;

/* 0x00683FC5: cleared to mute the sight notices and set when the level wants
   them; the NPC manager's event pass reads it too. */
extern unsigned char g_sight_messages_enabled_00683fc5;

bool IsVisibleUnderConditions(
    const W8MonsterInfo* monster, const W8VisibilityRecord* row,
    int kind); /* 0x00504B00 */
W8VisibilityRecord* FindMonToMonVisibility(
    W8MonsterInfo* source, W8MonsterInfo* target); /* 0x00504C20 */
