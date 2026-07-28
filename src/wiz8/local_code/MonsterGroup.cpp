#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <wchar.h>

static const char MONSTER_GROUP_CPP[] =
    "C:\\Projects\\Wizardry 8\\Local Code\\MonsterGroup.cpp";

extern int Function50A440(unsigned int monster_list_index);  /* 0x0050A440 */

/* Group list indices above this select the encounter list instead, biased by
   exactly this much - the same split the monster list uses. */
enum { W8_ENCOUNTER_GROUP_INDEX_BIAS = 10000 };
extern signed char Function50A500(int npc_record);           /* 0x0050A500 */
extern void Function565420(void);                            /* 0x00565420 */
extern void Function510590(W8MonsterGroup* monster_group);   /* 0x00510590 */
extern void Function454C80(void);                            /* 0x00454C80 */
extern void Function538DB0(int group_id, int value);         /* 0x00538DB0 */
extern unsigned char Function547510(void);                   /* 0x00547510 */
extern void Function452630(int value);                       /* 0x00452630 */
extern void Function48C670(W8MonsterGroup* monster_group);   /* 0x0048C670 */
extern void MonsterInfoLeaveCombat(W8MonsterInfo* monster_info);
extern void Function4E3C70(W8MonsterInfo* monster_info);     /* 0x004E3C70 */
extern void Function4C5730(W8Monster* monster, W8Position* position); /* 0x004C5730 */
extern unsigned char g_flag_683f94;
extern W8Character* g_all_characters;
extern unsigned char g_alternate_name_slot;
extern W8WideChar g_monster_name_buffer[];
enum { W8_MONSTER_RECORD_ALTERNATE_NAME = 0x18d };

/* A group of one is named in the singular; any other count uses the plural
   form, which is the second entry of each name set. */
enum { W8_MONSTER_GROUP_SINGULAR = 1, W8_MONSTER_NAME_STRIDE = 24 };

extern void ActivateMonster(W8MonsterInfo* monster_info, int mode);
extern unsigned char RemoveMonster(unsigned int monster_list_index,
                                  unsigned char destroy_monster);

/* A member counts as active while its state byte is below this and it is not
   under the control state the group excludes. */
enum { W8_MONSTER_STATE_ACTIVE_LIMIT = 0xd, W8_MONSTER_CONTROL_EXCLUDED = 1 };
enum { W8_MONSTER_GROUP_ALLY_COUNT = 4 };

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
    int npc_record;
    unsigned char disposition = W8_DISPOSITION_NEUTRAL;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3bd, 0);
    }
    record = MonsterDBFromSpecies(monster_group->monster_id);
    if ((record->flags_0d0 & 1) != 0) {
        npc_record = Function50A440(MonsterGetIndexByLocationID(
            0x75a, MONSTER_GROUP_CPP, IListGetAt(monster_group->monsters, 0), 1));
        if (npc_record == 0) {
            srAssertFail(
                "FALSE",
                MONSTER_GROUP_CPP,
                0x75d,
                FormatString(
                    "MonsterGroupCalcDefaultDisposition: Monster species %d(%ls) "
                    "NPC data not found",
                    monster_group->monster_id,
                    record));
        } else {
            switch (Function50A500(npc_record)) {
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
            if (record->hostility_range_25b != -1) {
                disposition = record->hostility_range_25b == 0;
            }
            break;
        case W8_FACTION_PARTY:
            disposition = W8_DISPOSITION_FRIENDLY;
            break;
        default:
            switch (GetFactionDisposition(
                static_cast<signed char>(record->faction_id_25f))) {
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

/* The group at one list index. Indices from 10000 to 19999 select the encounter
   list, biased by 10000; anything else selects the loaded group list. An index
   past the end of its list answers null quietly, while an index inside it that
   the list nonetheless fails to produce is a bug and says so.
 
   The diagnostic in the encounter branch reports the loaded list rather than
   the encounter list it actually read. Preserved as found. */
// FUNCTION: WIZ8 0x005101B0
W8MonsterGroup* GetMonsterGroupByListIndex(unsigned int group_list_index)
{
    W8MonsterGroup* result;
    const char* detail;
    int line;

    if (group_list_index < W8_ENCOUNTER_GROUP_INDEX_BIAS ||
        group_list_index >= 2 * W8_ENCOUNTER_GROUP_INDEX_BIAS) {
        if (group_list_index >= PListGetCount(g_monster_group_list)) {
            return 0;
        }
        result = (W8MonsterGroup*)PListGetAt(g_monster_group_list, group_list_index);
        if (result != 0) {
            return result;
        }
        detail = FormatString(
            "GroupInfo: ERROR - PLGet failed, index %d, pList %d",
            group_list_index,
            g_monster_group_list);
        line = 0x3dc;
    } else {
        if (group_list_index - W8_ENCOUNTER_GROUP_INDEX_BIAS >=
            PListGetCount(g_monster_group_encounter_list)) {
            return 0;
        }
        result = (W8MonsterGroup*)PListGetAt(
            g_monster_group_encounter_list,
            group_list_index - W8_ENCOUNTER_GROUP_INDEX_BIAS);
        if (result != 0) {
            return result;
        }
        detail = FormatString(
            "GroupInfo: ERROR - PLGet failed, index %d, pList %d",
            group_list_index,
            g_monster_group_list);
        line = 0x3d1;
    }
    srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, line, detail);
    return 0;
}

/* The list index of the group carrying one id, searching the loaded groups and
   then the encounters, whose answers are biased by 10000 the way the lookup
   above expects. Not finding it is only an error when the caller says so, and
   the caller's own file and line are threaded through for the message. */
// FUNCTION: WIZ8 0x005100B0
unsigned int GetMonsterGroupIndexByID(
    int caller_line,
    const char* caller_file,
    int group_id,
    unsigned char assert_on_failure)
{
    unsigned int index;
    W8MonsterGroup* group;

    for (index = 0; index < PListGetCount(g_monster_group_list); ++index) {
        group = GetMonsterGroupByListIndex(index);
        if (group->group_id == group_id) {
            return index;
        }
    }

    for (index = 0; index < PListGetCount(g_monster_group_encounter_list); ++index) {
        group = (W8MonsterGroup*)PListGetAt(g_monster_group_encounter_list, index);
        if (group->group_id == group_id) {
            return index + W8_ENCOUNTER_GROUP_INDEX_BIAS;
        }
    }

    if (assert_on_failure != 0) {
        srAssertFail(
            "FALSE",
            MONSTER_GROUP_CPP,
            0x3b2,
            FormatString(
                "GroupIndex: ID %d not found (%s line %d)",
                group_id,
                caller_file,
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
   because removing a member during the walk is possible.
 
   The member list is an IList, but its length is taken through PListGetCount -
   the two share a layout and the original really does call the P-list one here.
   Preserved as found. */
// FUNCTION: WIZ8 0x00510350
void RecountActiveMonsterGroupMembers(W8MonsterGroup* monster_group)
{
    unsigned int index;
    int active = 0;
    W8MonsterInfo* monster_info;

    for (index = 0; index < PListGetCount((W8PList*)monster_group->monsters); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x412,
                MONSTER_GROUP_CPP,
                IListGetAt(monster_group->monsters, index),
                1));
        if (monster_info->value_107 < W8_MONSTER_STATE_ACTIVE_LIMIT &&
            monster_info->control_state != W8_MONSTER_CONTROL_EXCLUDED) {
            ++active;
        }
    }
    if (active != monster_group->active_member_count) {
        monster_group->active_member_count = active;
        Function565420();
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

    index = PListGetCount((W8PList*)monster_group->monsters);
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
            MonsterGetIndexByLocationID(
                0x119, MONSTER_GROUP_CPP, location_id, 1),
            removed);
    } while (removed != 0);
    return 0;
}

// FUNCTION: WIZ8 0x0050F5D0
unsigned char RemoveAllGroupMembers(W8MonsterGroup* monster_group)
{
    return RemoveAllGroupMembersInline(monster_group);
}

/* Brings every member of a group into the world. Front to back, and the list
   length is re-read each time because activation can add to it. */
// FUNCTION: WIZ8 0x0050F6A0
void ActivateGroupMembers(W8MonsterGroup* monster_group, int mode)
{
    unsigned int index;

    for (index = 0; index < PListGetCount((W8PList*)monster_group->monsters); ++index) {
        ActivateMonster(
            MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(
                    0x146,
                    MONSTER_GROUP_CPP,
                    IListGetAt(monster_group->monsters, index),
                    1)),
            mode);
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

    Function510590(monster_group);
    for (index = 0; index < W8_MONSTER_GROUP_ALLY_COUNT; ++index) {
        if (monster_group->allied_group_ids[index] != 0) {
            Function510590(GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(
                    0x4a8,
                    MONSTER_GROUP_CPP,
                    monster_group->allied_group_ids[index],
                    1)));
        }
    }
    GetMonsterByLocationID(monster_group->value_9f);
    Function454C80();
}

// FUNCTION: WIZ8 0x005106D0
void RefreshMonsterGroupAndAllies(W8MonsterGroup* monster_group)
{
    RefreshMonsterGroupAndAlliesInline(monster_group);
}

/* Detaches a group from whatever is tracking it and marks it no longer loaded. */
// FUNCTION: WIZ8 0x0050F700
void DetachMonsterGroup(W8MonsterGroup* monster_group)
{
    Function538DB0(monster_group->group_id, 0);
    monster_group->flag_28 = 0;
}

/* Whether a group is live: loaded, still flagged, and with members left. A
   group flagged at +0x2A is live on that alone; any other group also has to
   pass the global gate at 0x00547510. */
// FUNCTION: WIZ8 0x00510B30
unsigned char IsMonsterGroupLive(W8MonsterGroup* monster_group)
{
    if (monster_group->flag_28 != 0 && monster_group->flag_29 != 0 &&
        monster_group->member_count != 0) {
        if (monster_group->flag_2a != 1 && Function547510() == 0) {
            return 0;
        }
        return 1;
    }
    return 0;
}

/* Follows a group's leader chain to the group that actually leads the formation
   and acts on its lead member. A broken link - a leader id that resolves to no
   group - stops the walk and reports success anyway, as does a null group,
   which is why every path returns one. */
// FUNCTION: WIZ8 0x0050FBA0
unsigned char ApplyToMonsterGroupLeader(W8MonsterGroup* monster_group, int value,
                                        char follow_leader)
{
    if (monster_group != 0) {
        while (follow_leader != 0 && monster_group->leader_group_id != 0) {
            monster_group = GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(
                    0x21b, MONSTER_GROUP_CPP, monster_group->leader_group_id, 1));
            if (monster_group == 0) {
                return 1;
            }
        }
        MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x21e, MONSTER_GROUP_CPP, monster_group->value_9f, 1));
        Function452630(value);
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

    for (group_list_index = 0;
         group_list_index < PListGetCount(g_monster_group_list);
         ++group_list_index) {
        monster_group = GetMonsterGroupByListIndex(group_list_index);
        monster_info = MonsterInfoFromID(
            0x4ef, MONSTER_GROUP_CPP, monster_group->value_9f, 1);
        monster_info->monster->formation = monster_group->formation;
    }
}

/* Sets a group's formation and pushes it straight onto every member's live
   Monster, so the group record and the members never disagree. */
// FUNCTION: WIZ8 0x0050FF40
void SetMonsterGroupFormation(W8MonsterGroup* monster_group,
                              const W8MonsterFormation* formation)
{
    unsigned int count;
    int index;
    W8Monster* monster;

    if (monster_group == 0) {
        return;
    }
    monster_group->formation.value_00 = formation->value_00;
    monster_group->formation.value_04 = formation->value_04;
    monster_group->formation.value_08 = formation->value_08;
    count = PListGetCount((W8PList*)monster_group->monsters);
    for (index = 0; index < static_cast<int>(count); ++index) {
        monster = MonsterGetScriptPartByLocationIndex(
                      MonsterGetIndexByLocationID(
                          0x34f,
                          MONSTER_GROUP_CPP,
                          IListGetAt(monster_group->monsters, index),
                          1))
                      ->monster;
        monster->formation.value_00 = formation->value_00;
        monster->formation.value_04 = formation->value_04;
        monster->formation.value_08 = formation->value_08;
    }
}

/* Retires a group and its allies from the encounter budget. The leader chain is
   followed first, so calling this on any member of a formation retires the
   whole formation; the retire itself is idempotent, gated on the flag it
   clears. All four ally slots are walked and the empty ones skipped. */
// FUNCTION: WIZ8 0x00510A10
void RetireMonsterGroupAndAllies(W8MonsterGroup* monster_group)
{
    int index;
    W8MonsterGroup* ally;

    while (monster_group->leader_group_id != 0) {
        monster_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(
                0x553, MONSTER_GROUP_CPP, monster_group->leader_group_id, 1));
    }
    if (monster_group->flag_c3 != 0) {
        Function48C670(monster_group);
        monster_group->flag_c3 = 0;
        monster_group->flag_d3 = 1;
        for (index = 0; index < W8_MONSTER_GROUP_ALLY_COUNT; ++index) {
            if (monster_group->allied_group_ids[index] != 0) {
                ally = GetMonsterGroupByListIndex(
                    GetMonsterGroupIndexByID(
                        0x564,
                        MONSTER_GROUP_CPP,
                        monster_group->allied_group_ids[index],
                        1));
                Function48C670(ally);
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
 
   Its opening assertion is followed immediately by MonsterGroupGetRecord's own,
   which is that body inlined here. */
// FUNCTION: WIZ8 0x00510280
W8WideChar* GetMonsterGroupName(W8MonsterGroup* monster_group)
{
    W8MonsterRecord* record;
    unsigned int name_form;

    if (monster_group == 0) {
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3eb, 0);
        srAssertFail("pMonsterGroup != NULL", MONSTER_GROUP_CPP, 0x3bd, 0);
    }
    record = MonsterDBFromSpecies(monster_group->monster_id);
    name_form = monster_group->member_count != W8_MONSTER_GROUP_SINGULAR;
    if (record->record_id_187 == W8_MONSTER_RECORD_ALTERNATE_NAME) {
        swprintf(
            g_monster_name_buffer,
            L"Al-%s",
            g_all_characters[g_alternate_name_slot].name);
        return g_monster_name_buffer;
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
// FUNCTION: WIZ8 0x0050FAD0
void MonsterGroupLeaveCombat(int unused, W8MonsterGroup* monster_group)
{
    unsigned int index;
    W8MonsterInfo* lead;

    if (g_flag_683f94 == 0) {
        srAssertFail("gXStatus.fCombatMode", MONSTER_GROUP_CPP, 0x1eb, 0);
    }
    if (monster_group->flag_29 == 0) {
        srAssertFail("pMonsterGroup->fInCombat", MONSTER_GROUP_CPP, 0x1ec, 0);
    }
    for (index = 0; index < PListGetCount((W8PList*)monster_group->monsters); ++index) {
        MonsterInfoLeaveCombat(MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x1f1,
                MONSTER_GROUP_CPP,
                IListGetAt(monster_group->monsters, index),
                1)));
    }
    monster_group->flag_29 = 0;
    Function565420();
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
            RemoveAllGroupMembersInline(GetMonsterGroupByListIndex(
                GetMonsterGroupIndexByID(
                    0x536,
                    MONSTER_GROUP_CPP,
                    monster_group->allied_group_ids[index],
                    1)));
            monster_group->allied_group_ids[index] = 0;
        }
    }
    RemoveAllGroupMembersInline(monster_group);
}

/* Brings a freshly loaded group's members into the world and marks the group
   loaded. Members that are already active are left alone. The sentinel at +0x9B
   is reset first, so a reload does not inherit the previous run's value. */
// FUNCTION: WIZ8 0x0050F630
void LoadMonsterGroupMembers(W8MonsterGroup* monster_group)
{
    unsigned int index;
    W8MonsterInfo* monster_info;

    monster_group->value_9b = -1;
    for (index = 0; index < PListGetCount((W8PList*)monster_group->monsters); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x12d,
                MONSTER_GROUP_CPP,
                IListGetAt(monster_group->monsters, index),
                1));
        if (monster_info->flag_14 == 0) {
            Function4E3C70(monster_info);
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

    for (index = 0; index < PListGetCount(g_monster_group_list); ++index) {
        GetMonsterGroupByListIndex(index)->value_cb = 0;
    }
    for (index = 0; index < PListGetCount(g_monster_list); ++index) {
        monster_info = MonsterGetScriptPartByLocationIndex(index);
        monster_info->value_354 = 0;
        monster_info->value_28e = 0;
    }
}

/* The mean position of a group's members, recomputed only while the group is
   loaded and cached on the group itself; an unloaded group answers with
   whatever it last held. The out-parameter is optional, so the same call both
   refreshes the cache and reads it.
 
   The divisor is the member count captured before the walk, not re-read after
   it, which is what makes a member added during the walk skew the average
   rather than divide by the wrong count. Preserved as found. */
// FUNCTION: WIZ8 0x0050FFD0
void GetMonsterGroupCentre(W8MonsterGroup* monster_group, W8Position* centre)
{
    unsigned int count;
    unsigned int index;
    W8Position position;
    float divisor;

    if (monster_group == 0) {
        return;
    }
    if (monster_group->flag_28 != 0) {
        monster_group->centre.x = 0;
        monster_group->centre.y = 0;
        monster_group->centre.z = 0;
        count = PListGetCount((W8PList*)monster_group->monsters);
        for (index = 0; index < count; ++index) {
            Function4C5730(
                MonsterGetScriptPartByLocationIndex(
                    MonsterGetIndexByLocationID(
                        0x379,
                        MONSTER_GROUP_CPP,
                        IListGetAt(monster_group->monsters, index),
                        1))
                    ->monster,
                &position);
            monster_group->centre.x += position.x;
            monster_group->centre.y += position.y;
            monster_group->centre.z += position.z;
        }
        divisor = static_cast<float>(count);
        monster_group->centre.x /= divisor;
        monster_group->centre.y /= divisor;
        monster_group->centre.z /= divisor;
    }
    if (centre != 0) {
        centre->x = monster_group->centre.x;
        centre->y = monster_group->centre.y;
        centre->z = monster_group->centre.z;
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

    count = PListGetCount(g_monster_group_list);
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
