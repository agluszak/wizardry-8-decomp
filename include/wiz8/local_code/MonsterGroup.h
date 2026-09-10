#include "wiz8/text_types.h"
#ifndef WIZ8_LOCAL_CODE_MONSTER_GROUP_H
#define WIZ8_LOCAL_CODE_MONSTER_GROUP_H

#include "wiz8/geometry.h"

struct W8IList;
struct W8MonsterRecord;
struct W8MonsterGroup;

unsigned char DestroyMonsterGroup(W8MonsterGroup* monster_group, int value);

#pragma pack(push, 1)
/* The stride is the record LoadMonsterGroup allocates, zeroes and reads whole,
   and which its own assertion spells sizeof(*pMonsterGroup). Only the fields
   that loader establishes are named; the rest stays opaque. */
typedef struct W8MonsterGroup {
    int group_id;                         /* 0x00: GroupIndex ID lookup key */
    int member_count;                     /* 0x04: decremented when members leave */
    struct W8IList* monsters;             /* 0x08: fresh IList per live group */
    unsigned char unknown_0c[8];
    int active_member_count;              /* 0x14: recomputed from member conditions */
    int monster_id;                       /* 0x18 */
    /* 0x1c: the mean of the live members' positions, recomputed on demand. */
    srVector3T<float> centre;
    unsigned char flag_28;                /* 0x28: cleared after the record loads */
    /* 0x29: fInCombat, named by the MonsterGroup.cpp:492 assertion. Cleared
       after the record loads and again when the group leaves combat. */
    unsigned char flag_29;
    /* 0x2a: at one the group is live regardless of the global gate at
       0x00547510; anything else has to pass that gate as well. */
    unsigned char flag_2a;
    unsigned char unknown_2b;
    /* 0x2c: selects which of the record's two name sets a member is displayed
       under. GetMonsterName reads it and nothing recovered yet writes it. */
    unsigned char flag_2c;
    unsigned char unknown_2d[0x6e];
    /* 0x9b: which member of the group the party currently has picked out,
       by location id, and -1 when none - which is how the group loads. Cycling
       through the group's targetable members reads it to know where it is and
       writes back where it got to. */
    int highlighted_member;
    int value_9f;                         /* 0x9f: a member location id; RemoveMonster
                                             compares it against the departing
                                             member's before renotifying */
    /* 0xa3: the group this one follows. Walking it is how a member request is
       redirected to the group that actually leads the formation, and a zero
       ends the walk. */
    int leader_group_id;
    /* 0xa7: up to four allied group ids. The notification pass walks all four
       unconditionally and skips the zero entries, so the array is fixed-size
       rather than terminated. */
    int allied_group_ids[4];
    /* 0xb7: copied verbatim onto every member's live Monster when the formation
       is re-applied. Unaligned inside this packed record, which is why the copy
       comes out as twelve byte moves rather than three dword ones. */
    srVector3T<float> formation;
    unsigned char flag_c3;                /* 0xc3: gates the trailing notification */
    /* 0xc4 is a saved-record version: at 2 and above the loader reads one more
       byte, and below 3 it clears flag_ca that older saves never wrote. */
    unsigned int version;                 /* 0xc4 */
    unsigned char unknown_c8[2];
    unsigned char flag_ca;                /* 0xca */
    int value_cb;                         /* 0xcb: cleared by the per-turn reset */
    /* 0xcf: when this group was last budgeted. UpdateRandomEncounterBudget
       advances it by the elapsed time and the culling pass measures against it. */
    int spawn_time;
    unsigned char flag_d3;                /* 0xd3: raised as flag_c3 is cleared */
    unsigned char unknown_d4[0x57];
} W8MonsterGroup;                         /* 0x12b */
#pragma pack(pop)

unsigned int GetMonsterGroupIndexByID(    int caller_line,
    const char* caller_file,
    int group_id,
    unsigned char assert_on_failure);
W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int group_list_index);
void Function510CC0(
    W8MonsterGroup* group, srVector3T<float>* position, float yaw, int a, int b,
    int c, int d); /* 0x00510CC0 */
/* Place a monster group relative to the party camera: with flag clear the
   group moves straight to the camera position, and with flag set it picks a
   point at the requested distance on a random angle around the camera yaw,
   widened to the largest allied-group radius. Answers the movement call's
   result so callers can branch on success. */
unsigned char PositionMonsterGroupNearCamera00511050(
    W8MonsterGroup* group, float distance, float yaw, unsigned char flag); /* 0x00511050 */
void RecountActiveMonsterGroupMembers(W8MonsterGroup* monster_group);
W8MonsterGroup* FindFirstMonsterByID(int monster_id);
W8MonsterGroup* FindNextExistingMonsterByID(
    int monster_id, W8MonsterGroup* previous);
W8MonsterGroup* CreateGroup(
    unsigned int monster_id,
    unsigned int count,
    const srVector3T<float>* position,
    unsigned char flag_1,
    unsigned char flag_2,
    unsigned char flag_3);


void ResetMonsterGroupTurnState(void);
void Function5115B0(void);

void DespawnMonsterGroup(W8MonsterGroup* monster_group);
void ActivateGroupMembers(W8MonsterGroup* monster_group, int mode);
W8WideChar* GetMonsterGroupName(W8MonsterGroup* monster_group);
void RefreshMonsterGroupAndAllies(W8MonsterGroup* monster_group);

void Function48C750(W8MonsterGroup* group);
W8MonsterRecord* GetMonsterGroupRecord(W8MonsterGroup* group);
void Function510590(W8MonsterGroup* monster_group);
void Function48C670(W8MonsterGroup* monster_group);
void Function50FD40(W8MonsterGroup* monster_group, int value);
void Function547570(W8MonsterGroup* monster_group, unsigned char flag,
                           int value);
void Function50F720(W8MonsterGroup* monster_group);


void Function5103E0(W8MonsterGroup* monster_group);

#endif
