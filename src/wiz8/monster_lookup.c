#include "gameplay_boundaries.h"

// FUNCTION: WIZ8 0x00510B60
W8MonsterGroup* FindFirstMonsterByID(int monster_id)
{
    unsigned int index = 0;

    if (PListGetCount(g_monster_group_species_list) > 0) {
        do {
            W8MonsterGroup* group = GetMonsterGroupByID(index);
            if (group->monster_id == monster_id) {
                return group;
            }
            ++index;
        } while (index < PListGetCount(g_monster_group_species_list));
    }

    index = 0;
    if (PListGetCount(g_monster_group_encounter_list) > 0) {
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
