#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/game_status.h"

/* Unresolved fragment: the character-side difficulty scaler sits at the head
   of the chunk.cpp (0x0055CB90) -> InputMapper.cpp (0x0055D800) gap, directly
   before the monster-side twin that the demo's Game Difficulty.cpp anchor
   proves. No variant match bounds this body, so its unit stays unproven. */

/* On easy a party character's value grows to seven fifths and on hard it
   shrinks to three fifths; a turncoated character fights for the monsters, so
   the scaling flips. Normal difficulty leaves the value alone. */
// FUNCTION: WIZ8 0x0055cc00
void ScaleValueForCharacterDifficulty(int party_slot, int* value)
{
    if (g_status_685170.buffers.Char[party_slot].uiCondition[W8_CONDITION_TURNCOAT] > 0) {
        switch (g_settings_6850c8.difficulty) {
        case 0:
            *value = (*value * 3 * 20) / 100;
            break;
        case 2:
            *value = (*value * 7 * 20) / 100;
            break;
        }
    } else {
        switch (g_settings_6850c8.difficulty) {
        case 0:
            *value = (*value * 7 * 20) / 100;
            break;
        case 2:
            *value = (*value * 3 * 20) / 100;
            break;
        }
    }
}
