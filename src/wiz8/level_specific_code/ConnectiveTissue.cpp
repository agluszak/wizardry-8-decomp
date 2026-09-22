#include "wiz8/level_specific_code/ConnectiveTissue.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/location_variables.h"

/* Level Specific Code\ConnectiveTissue.cpp (level 0x1b, the Arnika-Trynton
   connector map).

   Attribution evidence: the case-0x1b block of
   InitializeLevelMasterFunctions004D6C50 registers the "Liche" trigger
   against this callback, and the retail string pool gives its
   UNDEAD01-06/LicheDead names a contiguous contribution between the
   SavantTower and Arnika translation units. Level 0x1b is the
   Arnika_Trynton map in the retail level table. */

/* The "Liche" crypt callback: without item 0x15d in hand and before the
   trigger's action has fired it only answers with level message 6. Otherwise
   it answers with message 7, marks the lich dead (1000 experience the first
   time) and enables the six UNDEAD spawn generators. */
// FUNCTION: WIZ8 0x004E05F0
bool ConnectiveTissueLiche004E05F0(Trigger* pTrigger)
{
    MonGen* generator;

    if (GetItemInHand() != 0x15d && pTrigger->action_230 == 0) {
        ShowLevelMessage004D9960(6);
        g_trigger_feedback_00606994 = 1;
        return false;
    }
    ShowLevelMessage004D9960(7);
    if (GetLocationVarIDByName("LicheDead") == -1) {
        CreateLocationVar("LicheDead", 1);
        AwardPartyExperience004EEF10(0x3e8, 0);
    }
    generator = FindMonGenByName("UNDEAD01");
    if (generator != 0) {
        generator->flags |= 1;
    }
    generator = FindMonGenByName("UNDEAD02");
    if (generator != 0) {
        generator->flags |= 1;
    }
    generator = FindMonGenByName("UNDEAD03");
    if (generator != 0) {
        generator->flags |= 1;
    }
    generator = FindMonGenByName("UNDEAD04");
    if (generator != 0) {
        generator->flags |= 1;
    }
    generator = FindMonGenByName("UNDEAD05");
    if (generator != 0) {
        generator->flags |= 1;
    }
    generator = FindMonGenByName("UNDEAD06");
    if (generator != 0) {
        generator->flags |= 1;
    }
    return true;
}
