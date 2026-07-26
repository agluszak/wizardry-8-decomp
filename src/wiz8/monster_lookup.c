#include "gameplay_boundaries.h"
#include "sr_api.h"

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
    unsigned int index = 0;

    if (previous == 0) {
search_species:
        if (PListGetCount(g_monster_group_species_list) > index) {
            do {
                W8MonsterGroup* group = GetMonsterGroupByID(index);
                if (group->monster_id == monster_id) {
                    return group;
                }
                ++index;
            } while (index < PListGetCount(g_monster_group_species_list));
        }
    }
    else {
        int position = PListIndexOf(g_monster_group_species_list, previous);
        int species_count = PListGetCount(g_monster_group_species_list);
        if (position < species_count - 1) {
            if (position == -1) {
                position = PListIndexOf(g_monster_group_encounter_list, previous);
                if (position == -1) {
                    return 0;
                }
                index = position + 1;
                goto search_encounter;
            }
            index = position + 1;
            goto search_species;
        }
    }

    index = 0;
search_encounter:
    if (PListGetCount(g_monster_group_encounter_list) > index) {
        do {
            W8MonsterGroup* group = PListGetAt(g_monster_group_encounter_list, index);
            if (group->monster_id == monster_id) {
                return group;
            }
            ++index;
        } while (index < PListGetCount(g_monster_group_encounter_list));
    }
    return 0;
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
