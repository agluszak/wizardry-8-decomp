#pragma once

class Trigger;

/* Level 0x13 (RapaxUpperFloor) activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50. */
bool RapaxUpperFloorAirBox004DA110(Trigger* pTrigger);
bool RapaxUpperFloorDoorDone004DA2E0(Trigger* pTrigger);
