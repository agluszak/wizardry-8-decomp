#pragma once

class Trigger;

/* Level 0x12 (RapaxMainFloor) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
bool RapaxMainFloorAltarBox(Trigger* pTrigger);
bool RapaxMainFloorPlatform(Trigger* pTrigger);
bool RapaxMainFloorPlatform01(Trigger* pTrigger);
bool RapaxMainFloorPlatform02(Trigger* pTrigger);
