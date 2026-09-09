#include "wiz8/local_code/MonsterManager.h"

/*
 * Local Code\Combat Hostility.cpp.
 *
 * Whether two monsters count as hostile to each other: same-species
 * short-circuit, disposition-band equality, and the faction records behind
 * them.
 */

/* Species 0x224 never counts: both directions answer zero before anything
   else is read. */
enum { W8_NEUTRAL_SPECIES_224 = 0x224 };

/* Compare two monsters for hostility. Equal disposition bands answer two;
   either band clear answers zero; otherwise the faction records decide, and
   only matching non-zero factions fall through to the condition-thirteen
   presence test. */
// FUNCTION: WIZ8 0x00546F80
char Function546F80(W8MonsterInfo* first, W8MonsterInfo* second)
{
    W8MonsterRecord* first_record;
    W8MonsterRecord* second_record;
    unsigned int first_faction;
    unsigned int second_faction;

    if (first->monster_species == W8_NEUTRAL_SPECIES_224 ||
        second->monster_species == W8_NEUTRAL_SPECIES_224) {
        return 0;
    }
    if (first->flag_16 == second->flag_16) {
        return 2;
    }
    if (first->flag_16 == 0 || second->flag_16 == 0) {
        return 0;
    }
    first_record = GetMonsterDataForInfo(first);
    second_record = GetMonsterDataForInfo(second);
    first_faction = first_record->faction_id_25f;
    if (first_faction == 0) {
        return 1;
    }
    second_faction = second_record->faction_id_25f;
    if (second_faction == 0 || first_faction != second_faction) {
        return 1;
    }
    if ((first->condition_turns[13] != 0) == (second->condition_turns[13] != 0)) {
        return 0;
    }
    return 1;
}
