#include "wiz8/level_specific_code/RapaxUpperFloor.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/cursor.h"
#include "wiz8/fact_state.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/string_database.h"
#include "surrender/srMath.h"

/* Level Specific Code for RapaxUpperFloor.LVL (level 0x13) — the Rapax
   castle upper floor treasure vault.

   Attribution evidence: this TU sits in the unanchored gap between the
   Trynnie2.cpp and Trynnie1.cpp intervals (0x004DA110..0x004DA2E0); its
   string block (NP_SlipItem, TMakeTreasure*, TDoor1, TdoorOpen at
   0x00613118..0x00613150) is emitted between the same TUs' literals. The
   level-0x13 block in InitializeLevelMasterFunctions registers the
   AirBox and DoorDone triggers. The original file name is not anchored by an
   assertion path string. */

/* "AirBox": while the cursor holds an item, placing one of the four vault
   items (0x1cb/0x1cd/0x1cc/0x1ce) converts it to a treasure item, drops it at
   the NP_SlipItem entity and runs the TDoor1 trigger. When TdoorOpen is
   already set the treasure is deferred through the TMakeTreasure/
   TMakeTreasureNumber location vars for DoorDone to deliver. */
// FUNCTION: WIZ8 0x004DA110
bool RapaxUpperFloorAirBox(Trigger* pTrigger)
{
    srVector3T<float> entity_position;
    srVector3T<float> position;
    W8WorldItem* world_item;
    Trigger* door;
    int item_id;
    int var_id;

    if (g_status_685170.item_in_cursor != 0) {
        item_id = GetItemInHand();
        if (item_id == 0x1cb || item_id == 0x1cd || item_id == 0x1cc || item_id == 0x1ce) {
            SetFact(0x236, 1, 0);
            SetFactionDispositionBand(0x12, 0);
            ClearHeldItemDisplay();
            if (item_id == 0x1cb) {
                item_id = 0x1c7;
            } else if (item_id == 0x1cd) {
                item_id = 0x11f;
            } else {
                item_id = (item_id == 0x1cc) ? 0x1ca : 0x51;
            }
            var_id = GetLocationVarIDByName("TdoorOpen");
            if (var_id == -1 || GetLocationVarValueByName("TdoorOpen") == 0) {
                if (FindEntityByName("NP_SlipItem", &entity_position, 0, 0)) {
                    position = entity_position;
                    world_item = SpawnItem(item_id, &position, 3, 1);
                    if (world_item != 0) {
                        ActivateItem(world_item);
                    }
                }
                door = FindTriggerByName("TDoor1");
                if (door == 0) {
                    return true;
                }
                door->Run(-1);
                return true;
            }
            door = FindTriggerByName("TDoor1");
            if (door != 0) {
                door->Run(-1);
            }
            var_id = GetLocationVarIDByName("TMakeTreasure");
            if (var_id == -1) {
                CreateLocationVar("TMakeTreasure", 1);
            } else {
                SetTriggerVariableByName("TMakeTreasure", 1);
            }
            var_id = GetLocationVarIDByName("TMakeTreasureNumber");
            if (var_id == -1) {
                CreateLocationVar("TMakeTreasureNumber", item_id);
                return true;
            }
            SetTriggerVariableByName("TMakeTreasureNumber", item_id);
            return true;
        }
        ShowString(gppStringList[0x25a8 / 4]);
    }
    ShowString(gppStringList[0x2590 / 4]);
    return true;
}

/* "DoorDone": when TMakeTreasure is armed and TdoorOpen is still closed,
   deliver the stashed TMakeTreasureNumber item at NP_SlipItem and run TDoor1.
   Returns 0 once the treasure is delivered. */
// FUNCTION: WIZ8 0x004DA2E0
bool RapaxUpperFloorDoorDone(Trigger* pTrigger)
{
    srVector3T<float> entity_position;
    srVector3T<float> position;
    W8WorldItem* world_item;
    Trigger* door;
    int item_id;

    if (GetLocationVarIDByName("TMakeTreasure") != -1) {
        if (GetLocationVarValueByName("TMakeTreasure") != 0) {
            if (GetLocationVarIDByName("TdoorOpen") != -1) {
                if (GetLocationVarValueByName("TdoorOpen") == 0) {
                    item_id = GetLocationVarValueByName("TMakeTreasureNumber");
                    if (FindEntityByName("NP_SlipItem", &entity_position, 0, 0)) {
                        position = entity_position;
                        world_item = SpawnItem(item_id, &position, 3, 1);
                        if (world_item != 0) {
                            ActivateItem(world_item);
                        }
                    }
                    door = FindTriggerByName("TDoor1");
                    if (door != 0) {
                        door->Run(-1);
                    }
                    return false;
                }
            }
        }
    }
    return true;
}
