/* Local Code\Game Difficulty.cpp - difficulty scaling for combat values.

   Retail retains no path string for this unit; the official demo carries a
   "E:\Wizardry 8\Local Code\Game Difficulty.cpp" anchor at demo 0x005624F0,
   matching retail 0x0055CCB0. The unit sits in the retail span between
   chunk.cpp (0x0055CB90) and InputMapper.cpp (0x0055D800); the neighbouring
   character-side scaler has no proven owner and stays in
   DifficultyScaling.cpp. */

#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Configuration.h"

/* The monster-side counterpart: a hostile monster scales like a turncoated
   character and a friendly one like a party character. */
// FUNCTION: WIZ8 0x0055ccb0
void ScaleValueForMonsterDifficulty(W8MonsterInfo* monster_info, int* value)
{
    if (monster_info->ubDisposition == DISP_HOSTILE) {
        switch (g_settings.difficulty) {
        case 0:
            *value = (*value * 3 * 20) / 100;
            break;
        case 2:
            *value = (*value * 7 * 20) / 100;
            break;
        }
    } else if (monster_info->ubDisposition == DISP_FRIENDLY) {
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
