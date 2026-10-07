

#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Configuration.h"

/* The monster-side counterpart: a hostile monster scales like a turncoated
   character and a friendly one like a party character. */
// FUNCTION: WIZ8 0x0055ccb0
void ScaleValueForMonsterDifficulty(W8MonsterInfo* monster_info, int* value)
{
    if (monster_info->ubDisposition == W8_DISPOSITION_HOSTILE) {
        switch (g_settings.difficulty) {
        case 0:
            *value = (*value * 3 * 20) / 100;
            break;
        case 2:
            *value = (*value * 7 * 20) / 100;
            break;
        default:
            break;
        }
    } else if (monster_info->ubDisposition == W8_DISPOSITION_FRIENDLY) {
        switch (g_settings.difficulty) {
        case 0:
            *value = (*value * 7 * 20) / 100;
            break;
        case 2:
            *value = (*value * 3 * 20) / 100;
            break;
        default:
            break;
        }
    }
}
