#include "wiz8/level_specific_code/SavantTower.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/location_variables.h"
#include "wiz8/layouts/world.h"

/* Level Specific Code\SavantTower.cpp (level 0x24, the Savant's tower on
   Ascension Peak).

   Attribution evidence: the case-0x24 block of
   InitializeLevelMasterFunctions004D6C50 registers the Triangle/Circle/
   Square/Star shape triggers and ButtonBlocker against these callbacks, and
   the retail string pool places "DeactivatingBomb" in its own contribution
   between Ascension.cpp's and the ConnectiveTissue Liche block's. */

/* The ButtonBlocker prop tracks fact 0x156. */
// FUNCTION: WIZ8 0x004E0510
void SavantTowerSyncButtonBlocker004E0510(void)
{
    W8Prop* prop = FindPropByName(g_world, "ButtonBlocker");
    if (prop != 0) {
        if (GetFact(0x156) != 0) {
            prop->SetSetting6C(1);
        } else {
            prop->SetSetting6C(0);
        }
    }
}

/* The shape callback (Triangle/Circle/Square/Star): on the first step latch
   DeactivatingBomb and queue the NPC-0x46 script notice. */
// FUNCTION: WIZ8 0x004E0560
bool SavantTowerShape004E0560(Trigger* pTrigger)
{
    if (GetLocationVarIDByName("DeactivatingBomb") == -1) {
        CreateLocationVar("DeactivatingBomb", 1);
        QueueNpcScriptNotice(GetNpcStateByKind(0x46), 0, 0, 0, 0);
    }
    return true;
}

/* The ButtonBlocker callback: pin the blocker prop, queue the NPC-0x46
   script notice and latch fact 0x156. */
// FUNCTION: WIZ8 0x004E05A0
bool SavantTowerButtonBlocker004E05A0(Trigger* pTrigger)
{
    W8Prop* blocker = FindPropByName(g_world, "ButtonBlocker");

    if (blocker != 0) {
        blocker->SetSetting6C(1);
    }
    QueueNpcScriptNotice(GetNpcStateByKind(0x46), 0, 1, 0, 0);
    SetFact(0x156, 1, 0);
    return true;
}
