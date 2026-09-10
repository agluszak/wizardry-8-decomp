#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"

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
char MonsterHostility00546F80(W8MonsterInfo* first, W8MonsterInfo* second)
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

/* Whether a spell id can be aimed by monster AI: inside the spell table, not
   one of the two self-only kinds, and carrying a middle target type. */
// FUNCTION: WIZ8 0x005474B0
unsigned char MonsterCanAimSpell005474B0(int spell_id)
{
    if (spell_id > 0x95) {
        srAssertFail(
            "iType < SPELL_COUNT",
            "C:\\Projects\\Wizardry 8\\Local Code\\Combat Hostility.cpp",
            0x1c2, 0);
    }
    if (spell_id != 3 && spell_id != 0x29) {
        int target_type = GetSpellTargetType(spell_id, 0);
        if (target_type > 2 && target_type < 8) {
            return 1;
        }
        return 0;
    }
    return 0;
}
