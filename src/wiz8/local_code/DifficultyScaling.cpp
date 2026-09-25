#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/game_status.h"

/* The character-side difficulty scaler: on easy a party character's value
   grows to seven fifths and on hard it shrinks to three fifths; a turncoated
   character fights for the monsters, so the scaling flips. Normal difficulty
   leaves the value alone. */
// FUNCTION: WIZ8 0x0055cc00
void ScaleValueForCharacterDifficulty(int party_slot, int* value)
{
    if (g_status.buffers.Char[party_slot].uiCondition[W8_CONDITION_TURNCOAT] > 0) {
        switch (g_settings.difficulty) {
        case 0:
            *value = (*value * 3 * 20) / 100;
            break;
        case 2:
            *value = (*value * 7 * 20) / 100;
            break;
        }
    } else {
        switch (g_settings.difficulty) {
        case 0:
            *value = (*value * 7 * 20) / 100;
            break;
        case 2:
            *value = (*value * 3 * 20) / 100;
            break;
        }
    }
}
