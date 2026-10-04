#include "wiz8/learned_spells.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/gameplay_databases.h"

/* Original translation unit is not established by the ItemManager/Magic source anchors. */
// FUNCTION: WIZ8 0x004F9600
void BuildLearnedSpellState(W8LearnedSpellState* scratch, W8Character* character)
{
    int spell_id;
    int realm;
    int count;

    for (realm = 0; realm < 6; ++realm) {
        character->skill_unlocks[0x1c + realm] = 0;
    }
    scratch->learned_total = 0;
    for (spell_id = 0; spell_id < 0x72; ++spell_id) {
        if (character->spell_learned[spell_id] == 1 || character->spell_learned[spell_id] == 2) {
            realm = g_spell_records[spell_id].realm;
            count = character->skill_unlocks[0x1c + realm];
            scratch->spell_ids_by_realm[realm][count] = spell_id;
            character->skill_unlocks[0x1c + realm] = count + 1;
            ++scratch->learned_total;
        }
    }
    for (realm = 0; realm < 6; ++realm) {
        scratch->scroll[realm] = 0;
    }
}
