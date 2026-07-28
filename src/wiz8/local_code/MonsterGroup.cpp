#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

static const char MONSTER_GROUP_CPP[] =
    "C:\\Projects\\Wizardry 8\\Local Code\\MonsterGroup.cpp";

extern int Function50A440(unsigned int monster_list_index);  /* 0x0050A440 */

/* Group list indices above this select the encounter list instead, biased by
   exactly this much - the same split the monster list uses. */
enum { W8_ENCOUNTER_GROUP_INDEX_BIAS = 10000 };
extern signed char Function50A500(int npc_record);           /* 0x0050A500 */

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
