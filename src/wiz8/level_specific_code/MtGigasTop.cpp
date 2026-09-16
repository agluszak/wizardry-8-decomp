#include "wiz8/level_specific_code/MtGigasTop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/utility.h"
#include "wiz8/dice.h"
#include "wiz8/fact_state.h"
#include "wiz8/layouts/game_status.h"

/* Level Specific Code\MtGigasTop.cpp (level 0x0f).

   Attribution evidence: InitializeLevelMasterFunctions004D6C50 registers all
   five callbacks under case 0x0f, the level the Levels.cpp table names
   "MtGigasTop"; the block sits between the MtGigasOuter (0x0e) callbacks and
   the Monastery2 (9) anchors. */

/* Activation callback on _VOC_EWAXXCANNON1: hands the cannon NPC (kind 0x58)
   the item the player is holding, if any, as script argument -1. */
// FUNCTION: WIZ8 0x004DC6B0
bool OnEwaxxCannon1Activated(Trigger* trigger)
{
    W8NpcState* npc = GetNpcStateByKind(0x58);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    return true;
}

/* Activation callback on EwaxxLanding: same held-item handoff to the landing
   NPC (kind 0x7f). */
// FUNCTION: WIZ8 0x004DC6E0
bool OnEwaxxLandingActivated(Trigger* trigger)
{
    W8NpcState* npc = GetNpcStateByKind(0x7f);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    return true;
}

/* Activation callback on catchCord: records fact 0x177 once the party grabs
   the catch cord. */
// FUNCTION: WIZ8 0x004DC710
bool OnCatchCordActivated(Trigger* trigger)
{
    SetFact(0x177, 1, 0);
    return true;
}

/* Activation callback on _VOC_EWAXXTOPDOOR2: hands the top-door NPC (kind
   0x60) the held item, if any, as script argument -1. */
// FUNCTION: WIZ8 0x004DC730
bool OnEwaxxTopDoor2Activated(Trigger* trigger)
{
    W8NpcState* npc = GetNpcStateByKind(0x60);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    return true;
}

/* Activation callback on painActivatorTrigger: rolls 1d40+30 health damage
   against the party; the trigger stays armed (returns false). */
// FUNCTION: WIZ8 0x004DC770
bool OnPainActivatorActivated(Trigger* trigger)
{
    W8Dice dice;

    SetDice(&dice, 1, 0x28, 0x1e);
    ApplyRolledHealthChangeToParty(&dice, 0, 1);
    return false;
}
