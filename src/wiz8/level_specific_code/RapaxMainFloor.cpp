#include "wiz8/level_specific_code/RapaxMainFloor.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/local_code/ItemManager.h"
#include "surrender/srMath.h"

/* Level Specific Code for RapaxMainFloor.LVL (level 0x12) — the Rapax castle
   main floor altar and platform sequence.

   Attribution evidence: this TU sits in the unanchored gap between the
   Trynnie2.cpp and Trynnie1.cpp intervals (0x004DA3C0..0x004DA5D0); its
   string block (NP_Al-Sedexus, LezboDemonAppeared, RAM03/07/11 and the cursed
   item entities at 0x0061315c..0x006131b4) is emitted between the same TUs'
   literals. The level-0x12 block in InitializeLevelMasterFunctions004D6C50
   registers AltarBox and platformtrigger/01/02. The original file name is
   not anchored by an assertion path string. */

/* "AltarBox": the first activation while the fact-0x14c gate is clear marks
   LezboDemonAppeared, spawns Al-Sedexus (0x124) at NP_Al-Sedexus and arms the
   trigger's follow-up action 0x1fc. */
// FUNCTION: WIZ8 0x004DA3C0
bool RapaxMainFloorAltarBox004DA3C0(Trigger* pTrigger)
{
    srVector3T<float> position;
    int var_id;

    if (g_status_685170.flag_2489 == 0) {
        var_id = GetLocationVarIDByName("LezboDemonAppeared");
        if (var_id != -1) {
            var_id = GetLocationVarValueByName("LezboDemonAppeared");
            if (var_id == 1) {
                return true;
            }
            SetTriggerVariableByName00444030("LezboDemonAppeared", 1);
        } else {
            CreateLocationVar("LezboDemonAppeared", 1);
        }
        if (FindEntityByName("NP_Al-Sedexus", &position, 0, 0)) {
            SpawnMonsters(0x124, 1, &position, 0, 1, 0, 0);
        }
        pTrigger->required_item_id = 0x1fc;
    }
    return true;
}

/* "platformtrigger": drop the three cursed Rapax items (0x1fd robe, 0x1fe
   helm, 0x1ff dagger) at their NP_C* entities and enable RAM03. */
// FUNCTION: WIZ8 0x004DA460
bool RapaxMainFloorPlatform004DA460(Trigger* pTrigger)
{
    srVector3T<float> entity_position;
    srVector3T<float> position;
    W8WorldItem* world_item;
    Trigger* ram_trigger;

    if (FindEntityByName("NP_CRobe", &entity_position, 0, 0)) {
        position = entity_position;
        world_item = SpawnItem(0x1fd, &position, 3, 1);
        if (world_item != 0) {
            ActivateItem(world_item);
        }
    }
    if (FindEntityByName("NP_CHelm", &entity_position, 0, 0)) {
        position = entity_position;
        world_item = SpawnItem(0x1fe, &position, 3, 1);
        if (world_item != 0) {
            ActivateItem(world_item);
        }
    }
    if (FindEntityByName("NP_CDagger", &entity_position, 0, 0)) {
        position = entity_position;
        world_item = SpawnItem(0x1ff, &position, 3, 1);
        if (world_item != 0) {
            ActivateItem(world_item);
        }
    }
    ram_trigger = FindTriggerByName("RAM03");
    if (ram_trigger != 0) {
        ram_trigger->flags_0a0 |= 0x100U;
    }
    return true;
}

/* "platformtrigger01": disable RAM03 and enable RAM07. */
// FUNCTION: WIZ8 0x004DA590
bool RapaxMainFloorPlatform01004DA590(Trigger* pTrigger)
{
    Trigger* ram_trigger;

    ram_trigger = FindTriggerByName("RAM03");
    if (ram_trigger != 0) {
        ram_trigger->flags_0a0 &= ~0x100U;
    }
    ram_trigger = FindTriggerByName("RAM07");
    if (ram_trigger != 0) {
        ram_trigger->flags_0a0 |= 0x100U;
    }
    return true;
}

/* "platformtrigger02": disable RAM07 and enable RAM11. */
// FUNCTION: WIZ8 0x004DA5D0
bool RapaxMainFloorPlatform02004DA5D0(Trigger* pTrigger)
{
    Trigger* ram_trigger;

    ram_trigger = FindTriggerByName("RAM07");
    if (ram_trigger != 0) {
        ram_trigger->flags_0a0 &= ~0x100U;
    }
    ram_trigger = FindTriggerByName("RAM11");
    if (ram_trigger != 0) {
        ram_trigger->flags_0a0 |= 0x100U;
    }
    return true;
}
