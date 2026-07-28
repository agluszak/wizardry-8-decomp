#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

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
   rather than a counted walk. */
// FUNCTION: WIZ8 0x0050F5D0
unsigned char RemoveAllGroupMembers(W8MonsterGroup* monster_group)
{
    unsigned int index;
    unsigned char removed;

    index = PListGetCount((W8PList*)monster_group->monsters);
    do {
        --index;
        if (static_cast<int>(index) < 0) {
            return 1;
        }
        /* The destroy flag is a variable the original sets before the index
           lookup, not a constant pushed at the call: that is what puts both
           `push 1` after the lookup rather than before it. */
        removed = 1;
        removed = RemoveMonster(
            MonsterGetIndexByLocationID(
                0x119,
                MONSTER_GROUP_CPP,
                IListGetAt(monster_group->monsters, index),
                1),
            removed);
    } while (removed != 0);
    return 0;
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
   array is fixed-size rather than terminated. */
// FUNCTION: WIZ8 0x005106D0
void RefreshMonsterGroupAndAllies(W8MonsterGroup* monster_group)
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
