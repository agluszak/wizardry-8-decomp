#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>

// FUNCTION: WIZ8 0x00510B60
W8MonsterGroup* FindFirstMonsterByID(int monster_id)
{
    unsigned int index;
    W8MonsterGroup* group;

    for (index = 0; index < PListGetCount(g_monster_group_species_list); ++index) {
        group = GetMonsterGroupByID(index);
        if (group->monster_id == monster_id) {
            goto found;
        }
    }
    for (index = 0; index < PListGetCount(g_monster_group_encounter_list); ++index) {
        group = (W8MonsterGroup*)PListGetAt(g_monster_group_encounter_list, index);
        if (group->monster_id == monster_id) {
            goto found;
        }
    }
    group = 0;

found:
    return group;
}

// FUNCTION: WIZ8 0x00510BF0
W8MonsterGroup* FindNextExistingMonsterByID(int monster_id, W8MonsterGroup* previous)
{
    /* One variable carries both the PListIndexOf result and the loop index; the
       original keeps them in the same register and steps it with a plain
       increment rather than computing index = position + 1 separately. */
    int index = 0;
    W8MonsterGroup* group;

    if (previous != 0) {
        index = PListIndexOf(g_monster_group_species_list, previous);
        if (index >= (int)PListGetCount(g_monster_group_species_list) - 1) {
            goto reset_encounter;
        }
        if (index == -1) {
            index = PListIndexOf(g_monster_group_encounter_list, previous);
            if (index == -1) {
                group = 0;
                goto done;
            }
            ++index;
            goto search_encounter;
        }
        ++index;
    }
    for (; (unsigned int)index < PListGetCount(g_monster_group_species_list); ++index) {
        group = GetMonsterGroupByID((unsigned int)index);
        if (group->monster_id == monster_id) {
            goto done;
        }
    }

reset_encounter:
    index = 0;

search_encounter:
    for (; (unsigned int)index < PListGetCount(g_monster_group_encounter_list); ++index) {
        group = (W8MonsterGroup*)PListGetAt(g_monster_group_encounter_list, (unsigned int)index);
        if (group->monster_id == monster_id) {
            goto done;
        }
    }
    group = 0;

done:
    return group;
}

#define MAX_MONSTERS_IN_DATABASE 1000

// FUNCTION: WIZ8 0x004E57C0
W8MonsterRecord* GetMonsterDataByID(unsigned int monster_species)
{
    W8MonsterRecord* record;

    if (monster_species >= MAX_MONSTERS_IN_DATABASE) {
        srAssertFail(
            "uiMonsterSpecies < MAX_MONSTERS_IN_DATABASE",
            "C:\\Projects\\Wizardry 8\\Local Code\\MonsterManager.cpp",
            0x5f3,
            0);
    }
    record = g_monster_record_cache[monster_species];
    if (record == 0) {
        record = (W8MonsterRecord*)malloc(sizeof(W8MonsterRecord));
        if (record == 0) {
            return 0;
        }
        if (!LoadMonsterDatabaseRecord(monster_species, record)) {
            free(record);
            return 0;
        }
        g_monster_record_cache[monster_species] = record;
    }
    return record;
}
