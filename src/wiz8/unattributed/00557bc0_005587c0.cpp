#include "wiz8/local_code/CharGeneration.h"

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
