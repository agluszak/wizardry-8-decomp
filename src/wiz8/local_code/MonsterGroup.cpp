#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/xstatus.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/regions.h"
#include "wiz8/startup_world.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_code/character_events.h"

#include <wchar.h>
#include <string.h>

#include <stdlib.h>

static const char MONSTER_GROUP_CPP[] = "C:\\Projects\\Wizardry 8\\Local Code\\MonsterGroup.cpp";

/* Group list indices above this select the encounter list instead, biased by
   exactly this much - the same split the monster list uses. */
enum { W8_ENCOUNTER_GROUP_INDEX_BIAS = 10000 };

/* A group of one is named in the singular; any other count uses the plural
   form, which is the second entry of each name set. */
enum { W8_MONSTER_GROUP_SINGULAR = 1, W8_MONSTER_NAME_STRIDE = 24 };

/* A member counts as active while its highest condition is below HOSTILE and
   it is not under the control state the group excludes. */
enum { W8_MONSTER_CONTROL_EXCLUDED = 1 };
enum { W8_MONSTER_GROUP_ALLY_COUNT = 4 };

// FUNCTION: WIZ8 0x00510cc0
bool MoveMonsterGroupToPosition(W8MonsterGroup* group, const srVector3T<float>* position, float yaw,
                                bool proximity_check, bool include_allies, bool flatten_y,
                                bool alternate_radius)
{
    if (group->flag_28 == 0) {
        return false;
    }
    W8MonsterInfo* leader = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x67c, MONSTER_GROUP_CPP, group->value_9f, 1));
    float radius = alternate_radius ? leader->monster->movement_0c0.alternate_radius_0b4
                                    : leader->monster->radius_084;
    int location_ids[45];
    srVector3T<float> positions[45];
    unsigned int count = group->member_count;
    unsigned int index;
    for (index = 0; index < count; ++index) {
        location_ids[index] = IListGetAt(group->monsters, index);
    }
    if (include_allies) {
        for (int ally_index = 0; ally_index < 4; ++ally_index) {
            if (group->allied_group_ids[ally_index] != 0) {
                W8MonsterGroup* ally = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                    0x692, MONSTER_GROUP_CPP, group->allied_group_ids[ally_index], 1));
                W8MonsterInfo* ally_leader = MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(0x693, MONSTER_GROUP_CPP, ally->value_9f, 1));
                float ally_radius = alternate_radius
                                        ? ally_leader->monster->movement_0c0.alternate_radius_0b4
                                        : ally_leader->monster->radius_084;
                if (radius < ally_radius) {
                    radius = ally_radius;
                }
                for (index = 0; index < ally->member_count; ++index) {
                    location_ids[count++] = IListGetAt(ally->monsters, index);
                }
            }
        }
    }
    if (proximity_check) {
        for (index = 0; index < count; ++index) {
            g_octree_6598a4->UnregisterLocationObjects(static_cast<short>(location_ids[index]));
        }
    }
    srVector3T<float> target = *position;
    unsigned int found;
    if (alternate_radius) {
        found = g_octree_6598a4->FindNavigatorPosition(
            &target, yaw, radius + radius, count, positions, proximity_check, flatten_y, 0, 5, 1);
    } else {
        found = g_octree_6598a4->FindScatterPositions00437980(
            &target, yaw, radius + radius, count, positions, proximity_check, flatten_y);
    }
    if (found == 0) {
        return false;
    }
    if (proximity_check) {
        while (found < count) {
            --count;
            RemoveMonster(
                MonsterGetIndexByLocationID(0x6c9, MONSTER_GROUP_CPP, location_ids[count], 1), 1);
        }
    }
    target = positions[0];
    unsigned int position_index = 0;
    for (index = 0; index < count; ++index) {
        int location_id = location_ids[index];
        W8MonsterInfo* member = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x6d5, MONSTER_GROUP_CPP, location_id, 1));
        member->monster->SetAngles004538F0(yaw);
        member->monster->position_dirty_09c = 1;
        if (location_id == group->value_9f) {
            member->monster->SetPositionInternal00453590(&target);
        } else {
            if (position_index < found - 1) {
                ++position_index;
            }
            member->monster->SetPositionInternal00453590(&positions[position_index]);
        }
        srVector3T<float> placed = member->monster->GetPosition();
        g_octree_6598a4->UpdateMonsterLocation(static_cast<unsigned short>(location_id), &placed);
    }
    return true;
}

/* A group is done dying only when every live script record either has no
   engine Monster or reports the Monster death cycle.  Save-list location IDs
   are deliberately resolved through the canonical lookup path rather than
   treated as list indices. */
// FUNCTION: WIZ8 0x00511850
bool MonsterGroupAllMembersDying00511850(W8MonsterGroup* monster_group)
{
    unsigned int index;
    unsigned int monster_list_index;
    int location_id;
    W8MonsterInfo* monster_info;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup", "C:\\Projects\\Wizardry 8\\Local Code\\MonsterGroup.cpp",
                     0x82b, 0);
    }
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        location_id = IListGetAt(monster_group->monsters, index);
        monster_list_index = MonsterGetIndexByLocationID(
            0x830, "C:\\Projects\\Wizardry 8\\Local Code\\MonsterGroup.cpp", location_id, 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info != 0 && !monster_info->monster->IsDying()) {
            return 0;
        }
    }
    return 1;
}

/* How a monster group starts out disposed towards the party.
 
   A record flagged at +0xD0 answers from its NPC data instead of its faction:
   the group's first member is looked up, its NPC record fetched, and that
   record's own scale mapped onto this one. The two scales are not the same and
   the mapping is not the identity, which is why both are named separately.
 
   Otherwise the faction decides. The unaligned faction defers to the record's
   hostility range - -1 means neutral, zero means hostile, and anything else is
   a proximity threshold that still starts neutral - and the party's own faction
   is always friendly. Every other faction goes through the disposition table.
 
   Each of the three multi-way tests is a switch rather than a comparison chain:
   the original emits the dec/je ladder VC6 produces for small dense cases. */
// FUNCTION: WIZ8 0x00511250
unsigned char MonsterGroupCalcDefaultDisposition(W8MonsterGroup* monster_group)
{
    W8MonsterRecord* record;
    W8NpcState* npc_record;
    unsigned char disposition = W8_DISPOSITION_NEUTRAL;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3bd, 0);
    }
    record = MonsterDBFromSpecies(monster_group->monster_id);
    if ((record->flags_0d0 & 1) != 0) {
        npc_record = FindNpcBindingForMonster(MonsterGetIndexByLocationID(
            0x75a, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, 0), 1));
        if (npc_record == 0) {
            srAssertFail("FALSE", MONSTER_GROUP_CPP, 0x75d,
                         FormatString("MonsterGroupCalcDefaultDisposition: Monster species %d(%ls) "
                                      "NPC data not found",
                                      monster_group->monster_id, record));
        } else {
            switch (GetNpcDispositionBand(npc_record)) {
            case 0:
                disposition = W8_DISPOSITION_FRIENDLY;
                break;
            case 1:
                break;
            case 2:
                disposition = W8_DISPOSITION_HOSTILE;
                break;
            default:
                srAssertFail("FALSE", MONSTER_GROUP_CPP, 0x769, 0);
                break;
            }
        }
    } else {
        switch (record->faction_id_25f) {
        case W8_FACTION_UNALIGNED:
            if (record->hostility_radius_25b != -1) {
                disposition = record->hostility_radius_25b == 0;
            }
            break;
        case W8_FACTION_PARTY:
            disposition = W8_DISPOSITION_FRIENDLY;
            break;
        default:
            switch (GetFactionDisposition(static_cast<signed char>(record->faction_id_25f))) {
            case W8_FACTION_HOSTILE:
                disposition = W8_DISPOSITION_HOSTILE;
                break;
            case W8_FACTION_NEUTRAL:
                break;
            case W8_FACTION_FRIENDLY:
                disposition = W8_DISPOSITION_FRIENDLY;
                break;
            default:
                srAssertFail("FALSE", MONSTER_GROUP_CPP, 0x78d, 0);
                break;
            }
            break;
        }
    }
    return disposition;
}

/* Out-of-combat disposition refresh for a loaded group that still has a live
   member. Unaligned non-NPC neutrals that carry a finite hostility radius and
   can see the party within that scaled distance are promoted to hostile first.
   Then the default disposition is reapplied unless the group's value_cb stamp
   is still inside the intelligence-squared cooldown and the faction band has
   not moved since that stamp. */
// FUNCTION: WIZ8 0x005113A0
void RefreshMonsterGroupHostility005113A0(W8MonsterGroup* monster_group)
{
    W8MonsterRecord* record;
    W8MonsterInfo* monster_info;
    unsigned int index;
    int cooldown;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3bd, 0);
    }
    record = MonsterDBFromSpecies(monster_group->monster_id);
    if (monster_group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_GROUP_CPP, 0x82b, 0);
    }
    index = 0;
    if (ILLength(monster_group->monsters) == 0) {
        return;
    }
    while (true) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x830, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, index), 1));
        if (monster_info != 0 && monster_info->monster->IsDying() == 0) {
            break;
        }
        index = index + 1;
        if (index < ILLength(monster_group->monsters)) {
            continue;
        }
        return;
    }
    if (monster_group->ubDisposition != DISP_HOSTILE && (record->flags_0d0 & 1) == 0 &&
        record->faction_id_25f == 0 && record->hostility_radius_25b != 0 &&
        record->hostility_radius_25b != -1 &&
        GetGroupNearestDistance(monster_group) <=
            record->hostility_radius_25b * g_world_scale_005ebc40 &&
        MonsterGroupHasVisibleTarget(monster_group, 1, 3, 0) != 0) {
        SetMonsterGroupHostility(monster_group, DISP_HOSTILE, 0);
    }
    cooldown = IntegerPower(record->attribute_values_d1[1], 2) * 0x3c;
    if (monster_group->value_cb != 0 &&
        static_cast<unsigned int>(g_status_685170.world_clock - monster_group->value_cb) <=
            static_cast<unsigned int>(cooldown)) {
        if (record->faction_id_25f == 0) {
            return;
        }
        if (static_cast<unsigned int>(monster_group->value_cb) >=
            static_cast<unsigned int>(GetFactionValue(static_cast<char>(record->faction_id_25f)))) {
            return;
        }
    }
    SetMonsterGroupHostility(monster_group, MonsterGroupCalcDefaultDisposition(monster_group), 0);
    monster_group->value_cb = g_status_685170.world_clock;
}

/* The group at one list index. Indices from 10000 to 19999 select the encounter
   list, biased by 10000; anything else selects the loaded group list. An index
   past the end of its list answers null quietly, while an index inside it that
   the list nonetheless fails to produce is a bug and says so.
 
   The diagnostic in the encounter branch reports the loaded list rather than
   the encounter list it actually read. Preserved as found. */
// FUNCTION: WIZ8 0x005101b0
W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int group_list_index)
{
    W8MonsterGroup* result;
    const char* detail;
    int line;

    if (group_list_index < W8_ENCOUNTER_GROUP_INDEX_BIAS ||
        group_list_index >= 2 * W8_ENCOUNTER_GROUP_INDEX_BIAS) {
        if (group_list_index >= PLLength(gXStatus.plsMonsterGroupList)) {
            return 0;
        }
        result = (W8MonsterGroup*)PLGet(gXStatus.plsMonsterGroupList, group_list_index);
        if (result != 0) {
            return result;
        }
        detail = FormatString("GroupInfo: ERROR - PLGet failed, index %d, pList %d",
                              group_list_index, gXStatus.plsMonsterGroupList);
        line = 0x3dc;
    } else {
        if (group_list_index - W8_ENCOUNTER_GROUP_INDEX_BIAS >=
            PLLength(gXStatus.plsMonsterGroupEncounterList)) {
            return 0;
        }
        result = (W8MonsterGroup*)PLGet(gXStatus.plsMonsterGroupEncounterList,
                                        group_list_index - W8_ENCOUNTER_GROUP_INDEX_BIAS);
        if (result != 0) {
            return result;
        }
        detail = FormatString("GroupInfo: ERROR - PLGet failed, index %d, pList %d",
                              group_list_index, gXStatus.plsMonsterGroupList);
        line = 0x3d1;
    }
    srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, line, detail);
    return 0;
}

/* The list index of the group carrying one id, searching the loaded groups and
   then the encounters, whose answers are biased by 10000 the way the lookup
   above expects. Not finding it is only an error when the caller says so, and
   the caller's own file and line are threaded through for the message. */
// FUNCTION: WIZ8 0x005100b0
unsigned int GetMonsterGroupIndexByID(int caller_line, const char* caller_file, int group_id,
                                      unsigned char assert_on_failure)
{
    unsigned int index;
    W8MonsterGroup* group;

    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        group = GetMonsterGroupByListIndex(index);
        if (group->group_id == group_id) {
            return index;
        }
    }

    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupEncounterList); ++index) {
        group = (W8MonsterGroup*)PLGet(gXStatus.plsMonsterGroupEncounterList, index);
        if (group->group_id == group_id) {
            return index + W8_ENCOUNTER_GROUP_INDEX_BIAS;
        }
    }

    if (assert_on_failure != 0) {
        srAssertFail("FALSE", MONSTER_GROUP_CPP, 0x3b2,
                     FormatString("GroupIndex: ID %d not found (%s line %d)", group_id, caller_file,
                                  caller_line));
    }
    return 0xffffffff;
}

/* The database record behind a group's species. The assertion is the same one
   the disposition calculation opens with, at the same source line. */
// FUNCTION: WIZ8 0x00510180
W8MonsterRecord* MonsterGroupGetRecord(W8MonsterGroup* monster_group)
{
    if (monster_group == 0) {
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3bd, 0);
    }
    return MonsterDBFromSpecies(monster_group->monster_id);
}

/* Recounts how many of a group's members are still active and, only if that
   changed, publishes the new count. The list length is re-read every iteration
   because removing a member during the walk is possible. */
// FUNCTION: WIZ8 0x00510350
void RecountActiveMonsterGroupMembers(W8MonsterGroup* monster_group)
{
    unsigned int index;
    int active = 0;
    W8MonsterInfo* monster_info;

    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x412, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, index), 1));
        if (monster_info->highest_condition < W8_CONDITION_HOSTILE &&
            monster_info->control_state != W8_MONSTER_CONTROL_EXCLUDED) {
            ++active;
        }
    }
    if (active != monster_group->active_member_count) {
        monster_group->active_member_count = active;
        RequestRedrawParty();
    }
}

/* Destroys every member of a group, back to front so that the shrinking list
   does not move an entry past the cursor. It stops at the first removal that
   fails and reports that, which is why the loop is a do/while on the result
   rather than a counted walk.
 
   The despawn below compiles this same walk five more times over, at the same
   source line, so it is written once as an inline and called from both. */
static __inline unsigned char RemoveAllGroupMembersInline(W8MonsterGroup* monster_group)
{
    unsigned int index;
    unsigned char removed;
    int location_id;

    index = ILLength(monster_group->monsters);
    do {
        --index;
        if (static_cast<int>(index) < 0) {
            return 1;
        }
        /* The list read lands in a local before either constant is pushed:
           written as a nested call, VC6 pushes both `1`s ahead of it and eight
           bytes come out in the wrong order at every site this inlines into. */
        location_id = IListGetAt(monster_group->monsters, index);
        removed = 1;
        removed = RemoveMonster(
            MonsterGetIndexByLocationID(0x119, MONSTER_GROUP_CPP, location_id, 1), removed);
    } while (removed != 0);
    return 0;
}

// FUNCTION: WIZ8 0x0050f5d0
unsigned char RemoveAllGroupMembers(W8MonsterGroup* monster_group)
{
    return RemoveAllGroupMembersInline(monster_group);
}

/* Brings every member of a group into the world. Front to back, and the list
   length is re-read each time because activation can add to it. */
// FUNCTION: WIZ8 0x0050f6a0
void ActivateGroupMembers(W8MonsterGroup* monster_group, int mode)
{
    unsigned int index;

    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        ActivateMonster(
            MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x146, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, index), 1)),
            mode);
    }
}

// FUNCTION: WIZ8 0x00510590
void RefreshMonsterGroup(W8MonsterGroup* monster_group)
{
    W8Monster* leader;
    float leader_radius;
    unsigned int index;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_GROUP_CPP, 0x478, 0);
    }
    if (monster_group->leader_group_id == 0) {
        leader = GetMonsterByLocationID(monster_group->value_9f);
        static_cast<W8Navigator*>(leader)->LinkGroupNavigator00452BD0(0, 0.0, 0);
    } else {
        W8MonsterGroup* leader_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x480, MONSTER_GROUP_CPP, monster_group->leader_group_id, 1));
        leader = GetMonsterByLocationID(leader_group->value_9f);
    }
    if (leader == 0) {
        srAssertFail("pLeader", MONSTER_GROUP_CPP, 0x482, 0);
    }
    leader->GetAnimationRadius(&leader_radius);
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        W8Monster* member = GetMonsterByLocationID(IListGetAt(monster_group->monsters, index));

        if (member != leader) {
            float member_radius;
            member->GetAnimationRadius(&member_radius);
            static_cast<W8Navigator*>(member)->LinkGroupNavigator00452BD0(
                static_cast<W8Navigator*>(leader), leader_radius + leader_radius + member_radius,
                0);
        }
    }
}

/* Refreshes a group and every group allied to it, then the lead member's live
   monster. All four ally slots are walked and the empty ones skipped, so the
   array is fixed-size rather than terminated.
 
   The link repair below compiles the same walk again at the same source line,
   so it is written once as an inline and called from both. */
static __inline void RefreshMonsterGroupAndAlliesInline(W8MonsterGroup* monster_group)
{
    int index;

    RefreshMonsterGroup(monster_group);
    for (index = 0; index < W8_MONSTER_GROUP_ALLY_COUNT; ++index) {
        if (monster_group->allied_group_ids[index] != 0) {
            RefreshMonsterGroup(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                0x4a8, MONSTER_GROUP_CPP, monster_group->allied_group_ids[index], 1)));
        }
    }
    GetMonsterByLocationID(monster_group->value_9f)->PropagateGroupPosition();
}

// FUNCTION: WIZ8 0x005106d0
void RefreshMonsterGroupAndAllies(W8MonsterGroup* monster_group)
{
    RefreshMonsterGroupAndAlliesInline(monster_group);
}

/* Detaches a group from whatever is tracking it and marks it no longer loaded. */
// FUNCTION: WIZ8 0x0050f700
void DetachMonsterGroup(W8MonsterGroup* monster_group)
{
    SetTargetToGroup(monster_group->group_id, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
    monster_group->flag_28 = 0;
}

/* Whether a group is live: loaded, still flagged, and with members left. A
   group flagged at +0x2A is live on that alone; any other group also has to
   pass the global gate at 0x00547510. */
// FUNCTION: WIZ8 0x00510b30
bool IsMonsterGroupLive(W8MonsterGroup* monster_group)
{
    if (monster_group->flag_28 != 0 && monster_group->fInCombat != 0 &&
        monster_group->member_count > 0) {
        if (monster_group->ubDisposition != 1 && CombatAllowsLiveGroups() == 0) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* Walk plsMonsterGroupList and return the Nth live combat group. The live
   filter is the same as IsMonsterGroupLive; retail inlines those tests. */
// FUNCTION: WIZ8 0x00510ac0
W8MonsterGroup* GetLiveMonsterGroupAtIndex(int index)
{
    W8MonsterGroup* group = 0;
    unsigned int group_list_index = 0;
    unsigned int count = PLLength(gXStatus.plsMonsterGroupList);

    if (count != 0) {
        do {
            group = GetMonsterGroupByListIndex(group_list_index);
            if (group->flag_28 != 0 && group->fInCombat != 0 && group->member_count > 0 &&
                (group->ubDisposition == 1 || CombatAllowsLiveGroups() != 0)) {
                if (index == 0) {
                    return group;
                }
                --index;
            }
            ++group_list_index;
            count = PLLength(gXStatus.plsMonsterGroupList);
        } while (group_list_index < count);
    }
    return group;
}

/* Follows a group's leader chain to the group that actually leads the formation
   and acts on its lead member. A broken link - a leader id that resolves to no
   group - stops the walk and reports success anyway, as does a null group,
   which is why every path returns one. */
// FUNCTION: WIZ8 0x0050fba0
unsigned char ApplyToMonsterGroupLeader(W8MonsterGroup* monster_group,
                                        const srVector3T<float>* position, char follow_leader)
{
    if (monster_group != 0) {
        while (follow_leader != 0 && monster_group->leader_group_id != 0) {
            monster_group = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                0x21b, MONSTER_GROUP_CPP, monster_group->leader_group_id, 1));
            if (monster_group == 0) {
                return 1;
            }
        }
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x21e, MONSTER_GROUP_CPP, monster_group->value_9f, 1));
        monster_info->monster->ConfigureMovementToPosition00452630(position);
    }
    return 1;
}

/* Re-applies every loaded group's formation onto its lead member's live
   Monster. One typed assignment: the source is unaligned inside a packed
   record, which is what makes VC6 emit it as twelve byte moves with every load
   hoisted ahead of the stores. */
// FUNCTION: WIZ8 0x00510830
void ReapplyMonsterGroupFormations(void)
{
    unsigned int group_list_index;
    W8MonsterGroup* monster_group;
    W8MonsterInfo* monster_info;

    for (group_list_index = 0; group_list_index < PLLength(gXStatus.plsMonsterGroupList);
         ++group_list_index) {
        monster_group = GetMonsterGroupByListIndex(group_list_index);
        monster_info = MonsterInfoFromID(0x4ef, MONSTER_GROUP_CPP, monster_group->value_9f, 1);
        monster_info->monster->formation = monster_group->formation;
    }
}

/* Sets a group's formation and pushes it straight onto every member's live
   Monster, so the group record and the members never disagree. */
// FUNCTION: WIZ8 0x0050ff40
void SetMonsterGroupFormation(W8MonsterGroup* monster_group, const srVector3T<float>* formation)
{
    unsigned int count;
    int index;
    W8Monster* monster;

    if (monster_group == 0) {
        return;
    }
    monster_group->formation = *formation;
    count = ILLength(monster_group->monsters);
    for (index = 0; index < static_cast<int>(count); ++index) {
        monster = MonsterGetScriptPartByLocationIndex(
                      MonsterGetIndexByLocationID(0x34f, MONSTER_GROUP_CPP,
                                                  IListGetAt(monster_group->monsters, index), 1))
                      ->monster;
        monster->formation = *formation;
    }
}

/* Retires a group and its allies from the encounter budget. The leader chain is
   followed first, so calling this on any member of a formation retires the
   whole formation; the retire itself is idempotent, gated on the flag it
   clears. All four ally slots are walked and the empty ones skipped. */
// FUNCTION: WIZ8 0x00510a10
void RetireMonsterGroupAndAllies(W8MonsterGroup* monster_group)
{
    int index;
    W8MonsterGroup* ally;

    while (monster_group->leader_group_id != 0) {
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x553, MONSTER_GROUP_CPP, monster_group->leader_group_id, 1));
    }
    if (monster_group->flag_c3 != 0) {
        DetachMonsterGroup(monster_group);
        monster_group->flag_c3 = 0;
        monster_group->flag_d3 = 1;
        for (index = 0; index < W8_MONSTER_GROUP_ALLY_COUNT; ++index) {
            if (monster_group->allied_group_ids[index] != 0) {
                ally = GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                    0x564, MONSTER_GROUP_CPP, monster_group->allied_group_ids[index], 1));
                DetachMonsterGroup(ally);
                ally->flag_c3 = 0;
                ally->flag_d3 = 1;
            }
        }
    }
}

/* The display name for a whole group, which is GetMonsterName's shape one level
   up: the same special-cased record id, the same choice between the record's
   two name sets, but the variant comes from the member count rather than a
   caller - a group of exactly one is named in the singular.
 
   Its opening assertion is followed immediately by MonsterGroupGetRecord's own. */
// FUNCTION: WIZ8 0x00510280
wchar_t* GetMonsterGroupName(W8MonsterGroup* monster_group)
{
    W8MonsterRecord* record;
    unsigned int name_form;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3eb, 0);
    }
    record = MonsterGroupGetRecord(monster_group);
    name_form = monster_group->member_count != W8_MONSTER_GROUP_SINGULAR;
    if (record->record_id_187 == W8_MONSTER_RECORD_ALTERNATE_NAME) {
        swprintf(g_status_685170.monster_name_buffer_2453, L"Al-%s",
                 g_status_685170.buffers.characters[g_status_685170.alternate_name_slot_247f].name);
        return g_status_685170.monster_name_buffer_2453;
    }
    if (monster_group->flag_2c != 0) {
        return record->name_00 + name_form * W8_MONSTER_NAME_STRIDE;
    }
    return record->name_60 + name_form * W8_MONSTER_NAME_STRIDE;
}

/* Takes a whole group out of combat: every member leaves individually, the
   group's own fInCombat is lowered, and the lead member is marked. The member
   list length is re-read each iteration because leaving combat can change it.
   Both assertions name what they guard - the global combat mode and the group's
   own fInCombat, which is what gives +0x29 its name. */
// FUNCTION: WIZ8 0x0050fad0
void MonsterGroupLeaveCombat(W8MonsterGroup* monster_group)
{
    unsigned int index;
    W8MonsterInfo* lead;

    if (gXStatus.fCombatMode == 0) {
        srAssertFail("gXStatus.fCombatMode", MONSTER_GROUP_CPP, 0x1eb, 0);
    }
    if (monster_group->fInCombat == 0) {
        srAssertFail("pMonsterGroup->fInCombat", MONSTER_GROUP_CPP, 0x1ec, 0);
    }
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        MonsterInfoLeaveCombat(MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x1f1, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, index), 1)));
    }
    monster_group->fInCombat = 0;
    RequestRedrawParty();
    lead = MonsterInfoFromID(0x1fd, MONSTER_GROUP_CPP, monster_group->value_9f, 1);
    lead->flag_255 |= 0x80;
}

/* Removes a group and everything allied to it from the world. Each ally is
   emptied and its slot cleared before the group itself is emptied, so the
   group's own members are the last to go and an ally cannot be reached twice. */
// FUNCTION: WIZ8 0x00510930
void DespawnMonsterGroup(W8MonsterGroup* monster_group)
{
    int index;

    for (index = 0; index < W8_MONSTER_GROUP_ALLY_COUNT; ++index) {
        if (monster_group->allied_group_ids[index] != 0) {
            RemoveAllGroupMembersInline(GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(
                0x536, MONSTER_GROUP_CPP, monster_group->allied_group_ids[index], 1)));
            monster_group->allied_group_ids[index] = 0;
        }
    }
    RemoveAllGroupMembersInline(monster_group);
}

/* Brings a freshly loaded group's members into the world and marks the group
   loaded. Members that are already active are left alone. The sentinel at +0x9B
   is reset first, so a reload does not inherit the previous run's value. */
// FUNCTION: WIZ8 0x0050f630
void LoadMonsterGroupMembers(W8MonsterGroup* monster_group)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    monster_group->highlighted_member = -1;
    for (index = 0; index < ILLength(monster_group->monsters); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x12d, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, index), 1));
        if (monster_info->fActive == 0) {
            ActivateMonsterInWorld(monster_info);
        }
    }
    monster_group->flag_28 = 1;
}

/* Clears the per-turn scratch on every loaded group and every monster entry.
   Both walks re-read their list length each iteration. */
// FUNCTION: WIZ8 0x00511530
void ResetMonsterGroupTurnState(void)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        GetMonsterGroupByListIndex(index)->value_cb = 0;
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        monster_info->player_visibility.last_seen_clock_0c = 0;
        monster_info->party_threat.last_seen_clock_08 = 0;
    }
}

/* On a level's first visit, rebind every group monster's script from the name
   of the script it already carries, so reloaded monsters resume their
   level-local script objects. */
// FUNCTION: WIZ8 0x005115B0
void RebindMonsterGroupScripts(void)
{
    for (unsigned int index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        W8MonsterGroup* group = GetMonsterGroupByListIndex(index);
        W8MonsterInfo* info = MonsterInfoFromID(0x7f3, MONSTER_GROUP_CPP, group->value_9f, 1);
        if (info->monster->script_238 != 0) {
            char script_name[256];
            strcpy(script_name, info->monster->script_238->getName());
            info->monster->SetScript004C7F10(script_name, 1);
        }
    }
}

/* Writes a control state onto every live member of the group; the Lure
   effect uses it to flip a whole out-of-combat group at once. */
// FUNCTION: WIZ8 0x005117D0
void SetMonsterGroupControlState(W8MonsterGroup* monster_group, int control_state)
{
    for (unsigned int index = 0; index < ILLength(monster_group->monsters); ++index) {
        int location_id = IListGetAt(monster_group->monsters, index);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x818, MONSTER_GROUP_CPP, location_id, 1));
        if (info != 0 && info->monster->IsDying() == 0) {
            SetMonsterControlState(info, control_state);
        }
    }
}

/* Nonzero when the group - or, recursing, one of its allied groups - has a
   member whose highest condition is in the 0x0d..0x11 incapacitated band. */
// FUNCTION: WIZ8 0x00511D40
bool MonsterGroupHasIncapacitatedMember(int group_id)
{
    W8MonsterGroup* group =
        GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(0x961, MONSTER_GROUP_CPP, group_id, 1));
    unsigned int index;
    for (index = 0; index < ILLength(group->monsters); ++index) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x967, MONSTER_GROUP_CPP, IListGetAt(group->monsters, index), 1));
        if (info->highest_condition > 0xd && info->highest_condition < 0x12) {
            return true;
        }
    }
    for (index = 0; index < 4; ++index) {
        if (group->allied_group_ids[index] != 0 &&
            MonsterGroupHasIncapacitatedMember(group->allied_group_ids[index])) {
            return true;
        }
    }
    return false;
}

/* The mean position of a group's members, recomputed only while the group is
   loaded and cached on the group itself; an unloaded group answers with
   whatever it last held. The out-parameter is optional, so the same call both
   refreshes the cache and reads it.
 
   The divisor is the member count captured before the walk, not re-read after
   it, which is what makes a member added during the walk skew the average
   rather than divide by the wrong count. Preserved as found. */
// FUNCTION: WIZ8 0x0050ffd0
void GetMonsterGroupCentre(W8MonsterGroup* monster_group, srVector3T<float>* centre)
{
    unsigned int count;
    unsigned int index;
    srVector3T<float> position;
    float divisor;

    if (monster_group == 0) {
        return;
    }
    if (monster_group->flag_28 != 0) {
        monster_group->centre.SetZero();
        count = ILLength(monster_group->monsters);
        for (index = 0; index < count; ++index) {
            MonsterGetLocation(
                MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(0x379, MONSTER_GROUP_CPP,
                                                IListGetAt(monster_group->monsters, index), 1))
                    ->monster,
                &position);
            monster_group->centre += position;
        }
        divisor = static_cast<float>(count);
        monster_group->centre /= divisor;
    }
    if (centre != 0) {
        *centre = monster_group->centre;
    }
}

/* Repairs every loaded group's formation links and refreshes the ones that now
   lead. A group that names itself as its own leader is broken: the self-link is
   cut and any ally slot pointing back at it is cleared too. Every group that
   ends up leading - because it never had a leader, or because the repair just
   removed one - is then refreshed along with its allies.
 
   The list length is read once, before the walk, unlike the other passes over
   this list. Preserved as found. */
// FUNCTION: WIZ8 0x00510740
void RepairMonsterGroupLeaderLinks(void)
{
    unsigned int count;
    unsigned int group_list_index;
    W8MonsterGroup* monster_group;
    int ally;

    count = PLLength(gXStatus.plsMonsterGroupList);
    for (group_list_index = 0; group_list_index < count; ++group_list_index) {
        monster_group = GetMonsterGroupByListIndex(group_list_index);
        if (monster_group->leader_group_id == monster_group->group_id) {
            monster_group->leader_group_id = 0;
            for (ally = 0; ally < W8_MONSTER_GROUP_ALLY_COUNT; ++ally) {
                if (monster_group->allied_group_ids[ally] == monster_group->group_id) {
                    monster_group->allied_group_ids[ally] = 0;
                }
            }
        }
        if (monster_group->leader_group_id == 0) {
            RefreshMonsterGroupAndAlliesInline(monster_group);
        }
    }
}

/* Detaches a group from whatever leader it currently has: the leader's ally slot
   pointing back at it is cleared and its own leader link cut. Answers the group
   the id resolved to, which the callers then re-lay-out.
 
   Written as an inline because 0x0050F4A0 and 0x0050FC20 both compile it, at the
   same two source lines. */
static __inline W8MonsterGroup* UnlinkMonsterGroupFromLeaderInline(W8MonsterGroup* monster_group)
{
    W8MonsterGroup* current;
    W8MonsterGroup* previous_leader;
    int group_id = monster_group->group_id;
    int ally;

    current =
        GetMonsterGroupByListIndex(GetMonsterGroupIndexByID(0x303, MONSTER_GROUP_CPP, group_id, 1));
    if (current != 0 && current->leader_group_id != 0) {
        previous_leader = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x309, MONSTER_GROUP_CPP, current->leader_group_id, 1));
        if (previous_leader != 0) {
            for (ally = 0; ally < W8_MONSTER_GROUP_ALLY_COUNT; ++ally) {
                if (previous_leader->allied_group_ids[ally] == group_id) {
                    previous_leader->allied_group_ids[ally] = 0;
                }
            }
        }
        current->leader_group_id = 0;
    }
    return current;
}

/* Moves a group under a new leader, or detaches it when none is given.
 
   Linking a group to itself is refused outright. Otherwise the group is first
   unlinked from wherever it currently sits: its old leader's ally slot is
   cleared and its own leader link cut, and if it was leading a live formation
   it is retired from the encounter budget on the way out. The formation is then
   re-laid-out through whichever group the id still resolves to.
 
   With a leader given, the group takes the first free ally slot; a full leader
   refuses the link and answers zero, leaving the group detached rather than
   half-attached. Detaching instead - a null leader - hands a live group back to
   the budget through the other of the two 0x0048C6xx entry points. */
// FUNCTION: WIZ8 0x0050fc20
unsigned char LinkMonsterGroupToLeader(W8MonsterGroup* leader, W8MonsterGroup* monster_group)
{
    unsigned int slot;

    if (leader != 0 && leader->group_id == monster_group->group_id) {
        return 0;
    }
    if (monster_group->flag_c3 != 0 && monster_group->leader_group_id == 0) {
        DetachMonsterGroup(monster_group);
    }
    RefreshMonsterGroup(UnlinkMonsterGroupFromLeaderInline(monster_group));
    if (leader != 0) {
        for (slot = 0; slot < W8_MONSTER_GROUP_ALLY_COUNT; ++slot) {
            if (leader->allied_group_ids[slot] == 0) {
                leader->allied_group_ids[slot] = monster_group->group_id;
                monster_group->leader_group_id = leader->group_id;
                RefreshMonsterGroup(monster_group);
                SetMonsterGroupHostility(monster_group, leader->ubDisposition, 0);
                return 1;
            }
        }
        return 0;
    }
    if (monster_group->flag_c3 != 0) {
        ReleaseMonsterGroup(monster_group);
    }
    return 1;
}

/* Allocate one live monster group, create its members, activate them, and
   register it on the species or encounter list. */
// FUNCTION: WIZ8 0x0050F1A0
W8MonsterGroup* CreateGroup(unsigned int monster_id, unsigned int count,
                            const srVector3T<float>* position, unsigned char use_alternate_name,
                            unsigned char announce_spawn, unsigned char place_on_ground)
{
    W8MonsterGroup* group;
    W8MonsterRecord* record;
    W8PList* list;
    unsigned int index;
    unsigned int created;
    int registry_before;
    float yaw;

    if (count > 9) {
        count = 9;
    }
    group = static_cast<W8MonsterGroup*>(malloc(sizeof(W8MonsterGroup)));
    if (group == 0) {
        return 0;
    }
    memset(group, 0, sizeof(W8MonsterGroup));

    record = MonsterDBFromSpecies(monster_id);
    if (record == 0) {
        free(group);
        return 0;
    }

    do {
        group->group_id = g_status_685170.next_group_id_234a;
        g_status_685170.next_group_id_234a = g_status_685170.next_group_id_234a + 1;
    } while (group->group_id == 0);

    group->leader_group_id = 0;
    group->allied_group_ids[0] = 0;
    group->allied_group_ids[1] = 0;
    group->allied_group_ids[2] = 0;
    group->allied_group_ids[3] = 0;
    group->member_count = 0;
    group->active_member_count = 0;
    group->monster_id = monster_id;
    group->centre = *position;
    group->flag_28 = 0;
    group->fInCombat = 0;
    group->unknown_2b = 3;
    if (use_alternate_name != 0 || (record->flags_0d0 & 0x10) != 0) {
        group->flag_2c = 1;
    } else {
        group->flag_2c = 0;
    }
    group->unknown_2d[0] = 0;
    group->unknown_2d[1] = 0;
    group->unknown_2d[2] = 0xff;
    group->unknown_2d[3] = 0xff;
    group->unknown_2d[4] = 0xff;
    group->unknown_2d[5] = 0xff;
    group->spawn_time = g_status_685170.world_clock;

    group->monsters = ILCreate();
    if (group->monsters == 0) {
        free(group);
        return 0;
    }

    list = gXStatus.plsMonsterGroupList;
    if (record->unborn_26a != 0) {
        list = gXStatus.plsMonsterGroupEncounterList;
    }
    registry_before = GetUsedPageFileBytes();
    if (PLAdoptAppend(list, group) == -1) {
        free(group);
        return 0;
    }

    created = 0;
    if (count != 0) {
        do {
            if (CreateMonsterInfo(group, record, const_cast<srVector3T<float>*>(position)) == 0) {
                free(group);
                return 0;
            }
            ++created;
        } while (created < count);
    }

    group->value_9f = g_status_685170.next_monster_location_id_234e - 1;
    group->highlighted_member = -1;

    index = 0;
    while (index < ILLength(group->monsters)) {
        int location_id = IListGetAt(group->monsters, index);
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x12d, MONSTER_GROUP_CPP, location_id, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);

        if (monster_info->fActive == 0) {
            ActivateMonsterInWorld(monster_info);
        }
        ++index;
    }

    group->flag_28 = 1;
    yaw = GetCameraFacingYaw004BE5C0(const_cast<srVector3T<float>*>(position));
    MoveMonsterGroupToPosition(group, position, yaw, 0, 0, 0, 0);
    RefreshMonsterGroup(group);
    SetMonsterGroupHostility(group, MonsterGroupCalcDefaultDisposition(group), 0);

    if (announce_spawn != 0 && g_flag_689b32 != 0) {
        int registry_after = GetUsedPageFileBytes();
        const wchar_t* verb = count == 1 ? L"appears" : L"appear";
        const wchar_t* name = record->name_00;

        if (group->flag_2c == 0) {
            name += (group->member_count != 1) + 2;
        } else if (group->member_count != 1) {
            name += 1;
        }
        WriteGameLog(9, L"%d %s %s nearby! (%dK)", count, name, verb,
                     (registry_after - registry_before) >> 10);
    }

    return group;
}

/* Destroys one monster group. It is taken out of whatever is tracking it, then
   either retired from the encounter budget and torn down - if it leads - or
   simply unlinked from its leader and the formation re-laid-out. Its member
   IList is released and the record itself removed from whichever of the two
   group lists holds it, chosen by the same 10000 bias the lookups use.
 
   Failing to destroy the member list leaves the record in place and reports
   failure, so the group survives rather than being half-freed. */
// FUNCTION: WIZ8 0x0050f4a0
bool DestroyMonsterGroup(W8MonsterGroup* monster_group, W8MonsterInfo* monster_info)
{
    unsigned int group_list_index;
    W8PList* list;
    void* removed;

    if (monster_group->flag_28 != 0) {
        SetTargetToGroup(monster_group->group_id, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
        monster_group->flag_28 = 0;
    }
    if (monster_group->leader_group_id == 0) {
        if (monster_group->flag_c3 != 0) {
            DetachMonsterGroup(monster_group);
        }
        SetMonsterGroupMode(monster_group, monster_info);
    } else {
        RefreshMonsterGroup(UnlinkMonsterGroupFromLeaderInline(monster_group));
    }
    if (ILDestroy(monster_group->monsters) != 0) {
        group_list_index =
            GetMonsterGroupIndexByID(0xf3, MONSTER_GROUP_CPP, monster_group->group_id, 1);
        list = gXStatus.plsMonsterGroupList;
        if (group_list_index >= W8_ENCOUNTER_GROUP_INDEX_BIAS) {
            group_list_index -= W8_ENCOUNTER_GROUP_INDEX_BIAS;
            list = gXStatus.plsMonsterGroupEncounterList;
        }
        removed = PLRemoveAt(list, group_list_index);
        if (removed != 0) {
            free(removed);
            return true;
        }
    }
    return false;
}

/* Gives every live group whose lead member has no cycle-24 runtime the default
   sound set. The list length is read once up front, and the lead member is
   fetched for every group even though only the flagged ones use it. */
// FUNCTION: WIZ8 0x005108c0
void ApplyDefaultMonsterGroupSounds(void)
{
    unsigned int count;
    unsigned int group_list_index;
    W8MonsterGroup* monster_group;
    W8MonsterInfo* lead;

    count = PLLength(gXStatus.plsMonsterGroupList);
    for (group_list_index = 0; group_list_index < count; ++group_list_index) {
        monster_group = GetMonsterGroupByListIndex(group_list_index);
        lead = MonsterInfoFromID(0x501, MONSTER_GROUP_CPP, monster_group->value_9f, 1);
        if (monster_group->flag_c3 != 0 && lead->monster->script_238 == 0) {
            lead->monster->SetScript004C7F10("Default.MSF", 1);
        }
    }
}

// FUNCTION: WIZ8 0x00510b60
W8MonsterGroup* FindFirstMonsterByID(int monster_id)
{
    unsigned int index;
    W8MonsterGroup* group;

    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        group = GetMonsterGroupByListIndex(index);
        if (group->monster_id == monster_id) {
            goto found;
        }
    }
    for (index = 0; index < PLLength(gXStatus.plsMonsterGroupEncounterList); ++index) {
        group = (W8MonsterGroup*)PLGet(gXStatus.plsMonsterGroupEncounterList, index);
        if (group->monster_id == monster_id) {
            goto found;
        }
    }
    group = 0;

found:
    return group;
}

// FUNCTION: WIZ8 0x00510bf0
W8MonsterGroup* FindNextExistingMonsterByID(int monster_id, W8MonsterGroup* previous)
{
    /* One variable carries both the PListIndexOf result and the loop index; the
       original keeps them in the same register and steps it with a plain
       increment rather than computing index = position + 1 separately. */
    int index = 0;
    W8MonsterGroup* group;

    if (previous != 0) {
        index = PListIndexOf(gXStatus.plsMonsterGroupList, previous);
        if (index >= (int)PLLength(gXStatus.plsMonsterGroupList) - 1) {
            goto reset_encounter;
        }
        if (index == -1) {
            index = PListIndexOf(gXStatus.plsMonsterGroupEncounterList, previous);
            if (index == -1) {
                group = 0;
                goto done;
            }
            ++index;
            goto search_encounter;
        }
        ++index;
    }
    for (; (unsigned int)index < PLLength(gXStatus.plsMonsterGroupList); ++index) {
        group = GetMonsterGroupByListIndex((unsigned int)index);
        if (group->monster_id == monster_id) {
            goto done;
        }
    }

reset_encounter:
    index = 0;

search_encounter:
    for (; (unsigned int)index < PLLength(gXStatus.plsMonsterGroupEncounterList); ++index) {
        group = (W8MonsterGroup*)PLGet(gXStatus.plsMonsterGroupEncounterList, (unsigned int)index);
        if (group->monster_id == monster_id) {
            goto done;
        }
    }
    group = 0;

done:
    return group;
}

/* Pick the group's new leader member: the live member carrying the highest
   navigator value_008, or the first member when none qualify, then hand the
   outgoing leader's script and heard-noise state to the new leader's
   MonsterInfo. */
// FUNCTION: WIZ8 0x005103E0
void ElectGroupLeaderMember(W8MonsterGroup* monster_group)
{
    if (monster_group == 0) {
        srAssertFail("pMonsterGroup", MONSTER_GROUP_CPP, 0x438, 0);
    }
    if (ILLength(monster_group->monsters) == 0) {
        srAssertFail("ILLength(pMonsterGroup->plsMonsterIDList)", MONSTER_GROUP_CPP, 0x439, 0);
    }
    W8MonsterInfo* old_info =
        MonsterInfoFromID(0x43b, MONSTER_GROUP_CPP, monster_group->value_9f, 1);
    W8Monster* old_monster = old_info->monster;
    int leader_id = 0;
    unsigned int best = 0;
    if (ILLength(monster_group->monsters) != 0) {
        unsigned int index = 0;
        while (index < ILLength(monster_group->monsters)) {
            int member_id = IListGetAt(monster_group->monsters, index);
            W8Monster* member = GetMonsterByLocationID(member_id);
            W8MonsterInfo* member_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x443, MONSTER_GROUP_CPP, member_id, 1));
            if (member_info->highest_condition < 0xd && ((member->flags_1dc >> 9) & 1) == 0 &&
                best < static_cast<unsigned int>(member->movement_0c0.value_008)) {
                best = member->movement_0c0.value_008;
                leader_id = member_id;
            }
            ++index;
        }
        if (leader_id != 0) {
            monster_group->value_9f = leader_id;
            goto done;
        }
    }
    monster_group->value_9f = IListGetAt(monster_group->monsters, 0);
done:
    W8MonsterInfo* leader_info =
        MonsterInfoFromID(0x454, MONSTER_GROUP_CPP, monster_group->value_9f, 1);
    stScript* script = old_monster->script_238;
    W8Monster* leader_monster = leader_info->monster;
    if (script != 0 && script->getName() != 0) {
        script = old_monster->script_238;
        leader_monster->SetScript004C7F10(script != 0 ? script->getName() : 0, '\x01');
    }
    leader_info->heard_noise_radius_43 = old_info->heard_noise_radius_43;
    leader_info->heard_noise_position_37 = old_info->heard_noise_position_37;
}

/* Detach every allied group, promote the one whose members hold the highest
   navigator value_008 to lead the rest, and carry the fallen leader's script
   and heard-noise state to the new leader's MonsterInfo. */
// FUNCTION: WIZ8 0x0050FD40
void ElectAlliedLeaderGroup(W8MonsterGroup* monster_group, W8MonsterInfo* leader_info)
{
    int* allies = monster_group->allied_group_ids;
    int leader_group_id = 0;
    unsigned int best = 0;
    int live_allies = 0;
    int index;
    for (index = 0; index < 4; ++index) {
        if (allies[index] != 0) {
            ++live_allies;
        }
    }
    if (live_allies == 0) {
        return;
    }
    for (index = 0; index < 4; ++index) {
        if (allies[index] != 0) {
            W8MonsterGroup* candidate = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0x2ce, MONSTER_GROUP_CPP, allies[index], 1));
            candidate->leader_group_id = 0;
            unsigned int high = 0;
            unsigned int member_index = 0;
            while (member_index < ILLength(candidate->monsters)) {
                W8Monster* member =
                    GetMonsterByLocationID(IListGetAt(candidate->monsters, member_index));
                if (member != 0) {
                    unsigned int value = static_cast<unsigned int>(member->movement_0c0.value_008);
                    if (high < value) {
                        high = value;
                    }
                }
                ++member_index;
            }
            if (best < high) {
                leader_group_id = candidate->group_id;
                best = high;
            }
        }
    }
    if (leader_group_id != 0) {
        W8MonsterGroup* leader = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x275, MONSTER_GROUP_CPP, leader_group_id, 1));
        LinkMonsterGroupToLeader(0, leader);
    }
    for (index = 0; index < 4; ++index) {
        int ally_id = allies[index];
        if (ally_id != 0 && leader_group_id != ally_id) {
            W8MonsterGroup* leader = 0;
            if (leader_group_id != 0) {
                leader = GetMonsterGroupByListIndex(
                    GetMonsterGroupIndexByID(0x271, MONSTER_GROUP_CPP, leader_group_id, 1));
            }
            W8MonsterGroup* ally = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(0x275, MONSTER_GROUP_CPP, ally_id, 1));
            LinkMonsterGroupToLeader(leader, ally);
        }
    }
    W8MonsterGroup* leader = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(0x2e4, MONSTER_GROUP_CPP, leader_group_id, 1));
    W8MonsterInfo* new_leader_info =
        MonsterInfoFromID(0x2e7, MONSTER_GROUP_CPP, leader->value_9f, 1);
    stScript* script = leader_info->monster->script_238;
    if (script != 0 && script->getName() != 0) {
        script = leader_info->monster->script_238;
        new_leader_info->monster->SetScript004C7F10(script != 0 ? script->getName() : 0, '\x01');
    }
    new_leader_info->heard_noise_radius_43 = leader_info->heard_noise_radius_43;
    new_leader_info->heard_noise_position_37 = leader_info->heard_noise_position_37;
}

/* Bring the group - or the group leading it - into combat once every gate
   clears: an untargetable, dirty or dying member, or the startup navigator
   still settling, all defer the entry. The leader's members and its allies'
   members enter together, and a second live hostile group arms the
   ambush-warning event. */
// FUNCTION: WIZ8 0x0050F720
void MonsterGroupEnterCombat(W8MonsterGroup* monster_group)
{
    while (true) {
        if (g_status_685170.value_2390 != '\0' && monster_group->ubDisposition == '\x01' &&
            gXStatus.fCombatMode == '\0') {
            return;
        }
        if (GetFlag68F105() != '\0') {
            return;
        }
        if (monster_group->fInCombat != '\0') {
            return;
        }
        if (monster_group == 0) {
            srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3bd, 0);
        }
        W8MonsterRecord* record = MonsterDBFromSpecies(monster_group->monster_id);
        if (record->untargetable_24a != '\0') {
            return;
        }
        if (monster_group == 0) {
            srAssertFail("pMonsterGroup", MONSTER_GROUP_CPP, 0x511, 0);
        }
        unsigned int index = 0;
        while (index < ILLength(monster_group->monsters)) {
            int member_id = IListGetAt(monster_group->monsters, index);
            W8MonsterInfo* member_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x516, MONSTER_GROUP_CPP, member_id, 1));
            if (member_info->monster->position_dirty_09c != '\0') {
                return;
            }
            ++index;
        }
        if (g_startup_world_659c0c->position_dirty_09c != '\0') {
            return;
        }
        if (monster_group == 0) {
            srAssertFail("pMonsterGroup", MONSTER_GROUP_CPP, 0x8c9, 0);
        }
        index = 0;
        while (index < ILLength(monster_group->monsters)) {
            int member_id = IListGetAt(monster_group->monsters, index);
            W8MonsterInfo* member_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x8ce, MONSTER_GROUP_CPP, member_id, 1));
            if (((member_info->monster->flags_1dc >> 5) & 1) != 0) {
                return;
            }
            ++index;
        }
        if (monster_group == 0) {
            srAssertFail("pMonsterGroup", MONSTER_GROUP_CPP, 0x82b, 0);
        }
        index = 0;
        if (ILLength(monster_group->monsters) == 0) {
            return;
        }
        while (true) {
            int member_id = IListGetAt(monster_group->monsters, index);
            W8MonsterInfo* member_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x830, MONSTER_GROUP_CPP, member_id, 1));
            if (member_info != 0 && member_info->monster->IsDying() == '\0') {
                break;
            }
            ++index;
            if (ILLength(monster_group->monsters) <= index) {
                return;
            }
        }
        if (monster_group->leader_group_id == 0) {
            index = 0;
            while (index < ILLength(monster_group->monsters)) {
                int member_id = IListGetAt(monster_group->monsters, index);
                W8MonsterInfo* member_info = MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(0x1b1, MONSTER_GROUP_CPP, member_id, 1));
                MonsterInfoEnterCombat(member_info);
                ++index;
            }
            monster_group->fInCombat = '\x01';
            if (monster_group->ubDisposition == '\x01' && gXStatus.fCombatMode != '\0' &&
                g_combat_state->value_004 != 0) {
                unsigned int live_groups = 0;
                unsigned int group_list_index = 0;
                while (group_list_index < PLLength(gXStatus.plsMonsterGroupList)) {
                    W8MonsterGroup* other = GetMonsterGroupByListIndex(group_list_index);
                    if (other->flag_28 != '\0' && other->fInCombat != '\0' &&
                        other->member_count != 0 &&
                        (other->ubDisposition == '\x01' || CombatAllowsLiveGroups() != '\0')) {
                        ++live_groups;
                    }
                    ++group_list_index;
                }
                if (live_groups > 1) {
                    ApplyItemEffectToRandomCharacter(g_effect_005ee60c, -1, 0,
                                                     g_effect_argument_005ed8c8);
                }
            }
            int* allies = monster_group->allied_group_ids;
            for (int ally_index = 0; ally_index < 4; ++ally_index) {
                if (allies[ally_index] != 0) {
                    W8MonsterGroup* ally = GetMonsterGroupByListIndex(
                        GetMonsterGroupIndexByID(0x1c8, MONSTER_GROUP_CPP, allies[ally_index], 1));
                    if (ally->fInCombat == '\0') {
                        index = 0;
                        while (index < ILLength(ally->monsters)) {
                            int member_id = IListGetAt(ally->monsters, index);
                            W8MonsterInfo* member_info =
                                MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                                    0x1d0, MONSTER_GROUP_CPP, member_id, 1));
                            MonsterInfoEnterCombat(member_info);
                            ++index;
                        }
                        ally->fInCombat = '\x01';
                    }
                }
            }
            RequestRedrawParty();
            SetMonsterGroupEngagementState(monster_group->group_id, 0);
            return;
        }
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(0x1aa, MONSTER_GROUP_CPP, monster_group->leader_group_id, 1));
    }
}
