#include "wiz8/level_specific_code/Monastery1.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/float_constants.h"
#include "surrender/srCamera.h"

/* Level Specific Code\Monastery1.cpp (level 8).

   Attribution evidence: InitializeLevelMasterFunctions004D6C50 registers
   0x004DC8D0 under case 8, the Monastery1 block that also wires the
   roach_trigger, spider_trigger, Bartrigger and Coffinlide callbacks. The
   function itself is bar-trigger housekeeping for that same set. */

// FUNCTION: WIZ8 0x004DC8D0
void ClearTextForBarTrigger004DC8D0(void)
{
    Trigger* trigger = FindTriggerByName("Bartrigger");
    if (trigger != 0 && trigger->state_index == 1) {
        trigger = FindTriggerByName("Textforbar");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~0x100u;
        }
    }
}

/* Activation callback on roach_trigger: the first activation drops three
   roaches (kind 0x138) 1000 units above each roach location and records
   RoachesSpawned so later activations do nothing. */
// FUNCTION: WIZ8 0x004DC910
bool OnRoachTriggerActivated(Trigger* trigger)
{
    srVector3T<float> positions[3];

    if (GetLocationVarIDByName("RoachesSpawned") != -1) {
        return false;
    }
    if (!FindEntityByName("location_roach01", &positions[0], 0, 0)) {
        return false;
    }
    if (!FindEntityByName("location_roach02", &positions[1], 0, 0)) {
        return false;
    }
    if (!FindEntityByName("location_roach03", &positions[2], 0, 0)) {
        return false;
    }
    positions[0].z += g_float_005ebc64;
    positions[1].z += g_float_005ebc64;
    positions[2].z += g_float_005ebc64;
    SpawnMonsters(0x138, 3, &positions[0], 1, 0, 0, 0);
    SpawnMonsters(0x138, 3, &positions[1], 1, 0, 0, 0);
    SpawnMonsters(0x138, 3, &positions[2], 1, 0, 0, 0);
    CreateLocationVar("RoachesSpawned", 1);
    return true;
}

/* Activation callback on spider_trigger: every activation spawns six spiders
   (kind 0x19c) at the spiderman entity when it exists. */
// FUNCTION: WIZ8 0x004DCA20
bool OnSpiderTriggerActivated(Trigger* trigger)
{
    srVector3T<float> position;

    if (FindEntityByName("spiderman", &position, 0, 0)) {
        SpawnMonsters(0x19c, 6, &position, 1, 0, 0, 0);
    }
    return true;
}

/* Activation callback on Bartrigger: clears the armed bit of the Textforbar
   trigger so the bar text stops repeating. */
// FUNCTION: WIZ8 0x004DCAF0
bool OnBarTriggerActivated(Trigger* trigger)
{
    Trigger* textTrigger = FindTriggerByName("Textforbar");

    if (textTrigger != 0) {
        textTrigger->flags_0a0 &= ~0x100u;
    }
    return true;
}

/* Activation callback on Coffinlide: the first activation spawns the head
   (kind 0x12d) at the Head entity and records HeadSpawned. */
// FUNCTION: WIZ8 0x004DCA60
bool OnCoffinlideActivated(Trigger* trigger)
{
    srVector3T<float> position;

    if (GetLocationVarIDByName("HeadSpawned") == -1 && FindEntityByName("Head", &position, 0, 0)) {
        SpawnMonsters(0x12d, 1, &position, 1, 1, 0, 0);
        CreateLocationVar("HeadSpawned", 1);
    }
    return true;
}

/* Activation callback on Coffinlidg: casts spell 0x2a at power 3 onto the
   camera position. */
// FUNCTION: WIZ8 0x004DCAC0
bool OnCoffinlidgActivated(Trigger* trigger)
{
    srVector3T<float> position;

    position = GetWorld()->camera->getLocation();
    PointCastSpell(position, 0x2a, 3);
    return true;
}

/* Activation callback on wheel_star: consumes the held star key (item 0x24c);
   anything else in hand plays level message 0x22 and leaves the trigger
   armed. */
// FUNCTION: WIZ8 0x004DCB10
bool OnWheelStarActivated(Trigger* trigger)
{
    if (GetItemInHand() != 0x24c) {
        ShowLevelMessage004D9960(0x22);
        g_flag_00606994 = 1;
        return false;
    }
    RemovePartyItemByID005215D0(GetItemInHand(), 0);
    return true;
}
