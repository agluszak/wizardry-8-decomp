#pragma once

struct W8MonsterInfo;
struct W8SightConditions;

/* One monster-to-monster visibility record. The producer at 0x005049C0
   allocates it as plain 0x31-byte memory; the fields below are the slots its
   two directions read and write. The two position triples are stored as
   truncated integers, which is what the producer's float casts do. */
#pragma pack(push, 1)
struct W8MonToMonVisibility {
    int about_location_id;               /* 0x00 */
    unsigned char state_04;              /* 0x04: zero, one or two */
    unsigned char sight_flags_05[4];     /* 0x05: two flag pairs plus two bytes */
    unsigned char unknown_09[2];
    unsigned char flag_0b;               /* 0x0b */
    int last_seen_clock_0c;              /* 0x0c */
    int subject_x_10;
    int subject_y_14;
    int subject_z_18;
    int target_x_1c;
    int target_y_20;
    int target_z_24;
    unsigned char line_of_sight_28;      /* 0x28 */
    unsigned char unknown_29[8];
};
#pragma pack(pop)

static_assert(sizeof(W8MonToMonVisibility) == 0x31,
              "W8MonToMonVisibility_must_be_0x31");

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
    W8MonToMonVisibility* record);           /* 0x005058A0 */
float Function505A40(
    float subject_x, float subject_y, float subject_z,
    float target_x, float target_y, float target_z,
    float yaw, unsigned int attribute, int missile_bonus,
    unsigned char blind, unsigned char spell_kind, int arg_12, int arg_13,
    int arg_14, int arg_15, float distance); /* 0x00505A40 */

bool IsSightRangeOverridden(void);
bool GetSightCondition37A(const W8SightConditions* conditions);

extern float g_sight_default_005ec254;
