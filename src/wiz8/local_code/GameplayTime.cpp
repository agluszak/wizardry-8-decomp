#include "wiz8/character.h"
#include "wiz8/local_code/GameplayTime.h"

/*
 * Local Code\GameplayTime.cpp.
 *
 * The character's out-of-combat regeneration rates, derived from the pool
 * ceilings.
 */

/* 0x00502B50: the hit-point, stamina and per-realm spell regeneration rates
   are one tick of the pool ceiling's share, twenty points of base and a
   twelveth of a minute each; the modifier block's three doubled-cost flags
   raise their rate by half. */
// FUNCTION: WIZ8 0x00502b50
void RebuildCharacterRegenRates00502B50(W8Character* character)
{
    float rate;
    int realm;

    rate = ((float)character->hp_max * 0.4f + 20.0f) * 0.0041666669f;
    character->health_regen_rate_0b69 = rate;
    if (character->bonus_1770.flag_42 != 0) {
        character->health_regen_rate_0b69 = rate * 1.5f;
    }

    rate = ((float)character->stamina_max * 0.9f + 20.0f) * 0.0041666669f;
    character->stamina_regen_rate_0b71 = rate;
    if (character->bonus_1770.flag_43 != 0) {
        character->stamina_regen_rate_0b71 = rate * 1.5f;
    }

    for (realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        if (character->sp_max[realm] == 0) {
            character->spell_regen_rates_0b79[realm * 2] = 0.0f;
            continue;
        }
        rate = ((float)character->sp_max[realm] * 0.65f + 20.0f) * 0.0041666669f;
        character->spell_regen_rates_0b79[realm * 2] = rate;
        if (character->bonus_1770.flag_44 != 0) {
            character->spell_regen_rates_0b79[realm * 2] = rate * 1.5f;
        }
    }
}
