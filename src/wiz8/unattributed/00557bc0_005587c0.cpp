#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/character.h"

/* Address quarantine 00557bc0-005587c0; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

/* Refund every spent skill point through the per-skill commit helper. */
// FUNCTION: WIZ8 0x00557F90
void Function557F90(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;
    int spent;

    for (index = 0; index < 0x29; ++index) {
        spent = creation_state->skill_points_spent[index];
        if (spent > 0) {
            Function557BC0(character, creation_state, index, -spent);
        }
    }
}

/* Sums the realm skill costs the edited character still owes: skills the
   original holds but the edited build lacks, or the original's own set when
   it has no caster level to compare against. */
// FUNCTION: WIZ8 0x00557FD0
int Function557FD0(W8Character* original, W8Character* edited)
{
    int total = 0;

    if (GetProfessionCasterLevel(edited, -1) != 0) {
        int index;

        for (index = 0x18; index <= 0x1b; ++index) {
            if (original->skills[index].flag_00 == 0) {
                continue;
            }
            if (edited->skills[index].flag_00 != 0) {
                continue;
            }
            total += original->skill_costs_185c[index - 0x18];
        }
        return total;
    }
    if (GetProfessionCasterLevel(original, -1) < 1) {
        return 0;
    }
    {
        int index;

        for (index = 0x18; index <= 0x1b; ++index) {
            if (original->skills[index].flag_00 == 0) {
                continue;
            }
            total += original->skill_costs_185c[index - 0x18];
        }
        total += original->unknown_1860;
    }
    return total;
}
