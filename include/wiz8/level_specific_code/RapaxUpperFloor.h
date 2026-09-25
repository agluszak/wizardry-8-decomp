#pragma once

class Trigger;

/* Level 0x13 (RapaxUpperFloor) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
bool RapaxUpperFloorAirBox(Trigger* pTrigger);
bool RapaxUpperFloorDoorDone(Trigger* pTrigger);
