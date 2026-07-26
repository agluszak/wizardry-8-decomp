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
