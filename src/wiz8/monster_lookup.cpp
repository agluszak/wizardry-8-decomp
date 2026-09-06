#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"

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
