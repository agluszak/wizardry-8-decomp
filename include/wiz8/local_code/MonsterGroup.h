#ifndef WIZ8_LOCAL_CODE_MONSTER_GROUP_H
#define WIZ8_LOCAL_CODE_MONSTER_GROUP_H

#include "wiz8/geometry.h"
#include "wiz8/local_code/Factions.h"

struct W8IList;
struct W8MonsterRecord;
struct W8MonsterGroup;
struct W8MonsterInfo;

/* The dispositions a group or member carries, spelled by the MONSTERS.SLF
   parser and the 0x00530f10 assertion. */
enum { DISP_NEUTRAL = 0, DISP_HOSTILE = 1, DISP_FRIENDLY = 2 };

bool DestroyMonsterGroup(W8MonsterGroup* monster_group, W8MonsterInfo* monster_info);

#pragma pack(push, 1)
/* The stride is the record LoadMonsterGroup allocates, zeroes and reads whole,
   and which its own assertion spells sizeof(*pMonsterGroup). Only the fields
   that loader establishes are named; the rest stays opaque. */
struct W8MonsterGroup {
    int group_id;              /* 0x00: GroupIndex ID lookup key */
    unsigned int member_count; /* 0x04: decremented when members leave; live-group
                                  tests compare it unsigned (JBE at 0x00510B47) */
    struct W8IList* monsters;  /* 0x08: fresh IList per live group */
    /* Refreshed together by the targeting visibility pass. */
    int visible_member_count;    /* 0x0c */
    int selectable_member_count; /* 0x10 */
    int active_member_count;     /* 0x14: recomputed from member conditions */
    int monster_id;              /* 0x18 */
    /* 0x1c: the mean of the live members' positions, recomputed on demand. */
    srVector3T<float> centre;
    /* 0x28: set once the group's members are activated and positioned in the
       world; targeting and the AI walk skip groups without it. Cleared at
       load/init. */
    bool members_active_28;
    /* 0x29: in-combat flag, named by the MonsterGroup.cpp:492 assertion.
       Cleared after the record loads and again when the group leaves
       combat. */
    unsigned char fInCombat;
    /* 0x2a: the group's disposition, the value SetMonsterGroupDisposition
       writes and each member's ubDisposition copies. At DISP_HOSTILE the group
       is live regardless of the global gate at 0x00547510; anything else has
       to pass that gate as well. */
    W8Disposition ubDisposition;
    unsigned char unknown_2b;
    /* 0x2c: selects which of the record's two name sets a member is displayed
       under. Group creation presets it for alternate-name records, and the
       wandering-group detection pass sets it once the party spots the group. */
    bool alternate_name_2c;
    /* 0x2d..0x9a: per-group state blob; [0] is the monster-awareness grant
       latch (MonsterManager fills each member's awareness once), [0x6d] is
       the MIPE-written tail byte. */
    unsigned char group_state_2d[0x6e];
    /* 0x9b: which member of the group the party currently has picked out,
       by location id, and -1 when none - which is how the group loads. Cycling
       through the group's targetable members reads it to know where it is and
       writes back where it got to. */
    int highlighted_member;
    int leader_id_9f; /* 0x9f: a member location id; RemoveMonster
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
    /* 0xc3: set by MonGen when the group is spawned as an active encounter;
       cleared by UnregisterActiveEncounterGroup, which also raises
       encounter_ended_d3. */
    bool encounter_registered_c3;
    /* 0xc4 is a saved-record version: at 2 and above the loader reads one more
       byte, and below 3 it clears flag_ca that older saves never wrote. */
    unsigned int version; /* 0xc4 */
    /* 0xc8: group engagement state; 0xc9 ticks spent in it (reset on change,
       the AI checks < 3 for "just engaged"). */
    unsigned char engagement_c8;
    unsigned char engagement_ticks_c9;
    /* 0xca: set when a script/NPC pass forces the group neutral; cleared on
       load and when hostility is recomputed. Older saves never wrote it. */
    bool forced_neutral_ca;
    /* 0xcb: world_clock stamp of the last hostility application; the
       faction-reaction pass refuses to reapply inside the record cooldown. */
    int hostility_set_at_cb;
    /* 0xcf: when this group was last budgeted. UpdateRandomEncounterBudget
       advances it by the elapsed time and the culling pass measures against it. */
    int spawn_time;
    /* 0xd3: raised as encounter_registered_c3 is cleared - the group was an
       active encounter once. */
    bool encounter_ended_d3;
    unsigned char unknown_d4[0x57];
}; /* 0x12b */
#pragma pack(pop)

unsigned int GetMonsterGroupIndexByID(int caller_line, const char* caller_file, int group_id,
                                      unsigned char assert_on_failure);
W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int group_list_index);
/* The group's flag at 0xc8, looked up by group id. */
unsigned char GetMonsterGroupEngagementState(int group_id); /* 0x00511CB0 */
unsigned char ApplyToMonsterGroupLeader(W8MonsterGroup* monster_group,
                                        const srVector3T<float>* position,
                                        char follow_leader); /* 0x0050FBA0 */
/* Whether the group is loaded, in combat, and still has members; hostile
   groups are live on that alone, others also need CombatAllowsLiveGroups. */
bool IsMonsterGroupLive(W8MonsterGroup* monster_group); /* 0x00510B30 */
/* The Nth live combat group in plsMonsterGroupList order; null when none. */
W8MonsterGroup* GetLiveMonsterGroupAtIndex(int index); /* 0x00510AC0 */
/* Channel-12 notices summarizing a group's name, count, and visibility. */
void ShowMonsterGroupInfoNotice(int group_id); /* 0x00511670 */
/* Whether the group has a member placed and rendered in the world; a nonzero
   second argument also demands the member's party-threat flag. */
bool MonsterGroupHasRenderableMember(W8MonsterGroup* monster_group,
                                     char require_threat); /* 0x00511B40 */
/* Write `state` into the group's engagement byte and propagate it to its four
   allied groups; while the byte is set, each call ticks the counter beside
   it. The record kinds the special encounter ids carry ignore a set. */
void SetMonsterGroupEngagementState(int group_id, unsigned char state); /* 0x00511BE0 */
bool MoveMonsterGroupToPosition(W8MonsterGroup* group, const srVector3T<float>* position, float yaw,
                                bool proximity_check, bool include_allies, bool flatten_y,
                                bool alternate_radius); /* 0x00510CC0 */
/* Place a monster group relative to the party camera: with flag clear the
   group moves straight to the camera position, and with flag set it picks a
   point at the requested distance on a random angle around the camera yaw,
   widened to the largest allied-group radius. Answers the movement call's
   result so callers can branch on success. */
unsigned char PositionMonsterGroupNearCamera00511050(W8MonsterGroup* group, float distance,
                                                     float yaw,
                                                     unsigned char flag); /* 0x00511050 */
void RecountActiveMonsterGroupMembers(W8MonsterGroup* monster_group);
/* 0x0050FFD0: refresh the group's cached centre; a null centre out-pointer
   keeps only the cache update, which is how SpawnMonsters uses it. */
void GetMonsterGroupCentre(W8MonsterGroup* monster_group, srVector3T<float>* centre);
W8MonsterGroup* FindFirstMonsterByID(int monster_id);
W8MonsterGroup* FindNextExistingMonsterByID(int monster_id, W8MonsterGroup* previous);
unsigned char GiveBirthToMonster(W8MonsterGroup* monster_group); /* 0x00511990 */
W8MonsterGroup* CreateGroup(unsigned int monster_id, unsigned int count,
                            const srVector3T<float>* position, unsigned char flag_1,
                            unsigned char flag_2, unsigned char flag_3);

void ResetMonsterGroupTurnState(void);
void RebindMonsterGroupScripts(void);

void DespawnMonsterGroup(W8MonsterGroup* monster_group);
void ActivateGroupMembers(W8MonsterGroup* monster_group, int mode);
wchar_t* GetMonsterGroupName(W8MonsterGroup* monster_group);
void RefreshMonsterGroupAndAllies(W8MonsterGroup* monster_group);

W8MonsterRecord* MonsterGroupGetRecord(W8MonsterGroup* group);
void RefreshMonsterGroup(W8MonsterGroup* monster_group);
void DetachMonsterGroup(W8MonsterGroup* monster_group);

unsigned char RemoveAllGroupMembers(W8MonsterGroup* monster_group); /* 0x0050F5D0 */
/* MonsterGroup.cpp: respawns a same-sized group of a different monster id
   beside the source group, deactivating the old members as each replacement
   activates; NULL on failure. */
W8MonsterGroup* ReplaceMonsterGroupSpecies00511A40(W8MonsterGroup* group,
                                                   unsigned int monster_id); /* 0x00511A40 */
/* 0x00511CE0: mark every member's navigator position dirty (or clean). */
void SetMonsterGroupNavigatorDirty(W8MonsterGroup* monster_group, unsigned char flag);
bool MonsterGroupAllMembersDying00511850(W8MonsterGroup* monster_group); /* 0x00511850 */
void LoadMonsterGroupMembers(W8MonsterGroup* monster_group);             /* 0x0050F630 */
/* Out-of-combat refresh: proximity hostility for unaligned neutrals, then
   default disposition on the intelligence-squared cooldown. */
void RefreshMonsterGroupHostility005113A0(W8MonsterGroup* monster_group); /* 0x005113A0 */
void MonsterGroupEnterCombat(W8MonsterGroup* monster_group);              /* 0x0050F720 */
/* Re-elect the group's leader member: the live member carrying the highest
   navigator leadership_rank_008 takes over leader_id_9f, else the first member does, and
   the outgoing leader's script and heard-noise state move across. */
void ElectGroupLeaderMember(W8MonsterGroup* monster_group); /* 0x005103E0 */
/* Re-elect the allied leader group when this group's leader falls: the allied
   group whose members hold the highest navigator leadership_rank_008 leads the rest,
   and the fallen leader's script and heard-noise state carry to the new
   leader's MonsterInfo. */
void ElectAlliedLeaderGroup(W8MonsterGroup* monster_group,
                            W8MonsterInfo* leader_info); /* 0x0050FD40 */
/* Marks every live member of the group and of its allied groups for removal. */
void MarkMonsterGroupForRemoval(int group_id); /* 0x005118E0 */
/* 0x005117D0: write the control state onto every live member of the group. */
void SetMonsterGroupControlState(W8MonsterGroup* monster_group, int control_state);
/* Nonzero when the group - or one of its allied groups - has a member whose
   highest condition is in the 0x0d..0x11 incapacitated band. */
bool MonsterGroupHasIncapacitatedMember(int group_id); /* 0x00511D40 */
/* 0x00510A10: retire a group and its allies from the encounter budget,
   following the leader chain first. */
void RetireMonsterGroupAndAllies(W8MonsterGroup* monster_group);

void MonsterGroupLeaveCombat(W8MonsterGroup* monster_group); /* 0x0050FAD0 */

void SetMonsterGroupFormation(W8MonsterGroup* monster_group,
                              const srVector3T<float>* formation); /* 0x0050FF40 */
void ReapplyMonsterGroupFormations(void);                          /* 0x00510830 */
void RepairMonsterGroupLeaderLinks(void);                          /* 0x00510740 */
void ApplyDefaultMonsterGroupSounds(void);                         /* 0x005108C0 */

#endif
