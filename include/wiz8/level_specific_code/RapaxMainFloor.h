#pragma once

class Trigger;

/* Level 0x12 (RapaxMainFloor) activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50. */
bool RapaxMainFloorAltarBox004DA3C0(Trigger* pTrigger);
bool RapaxMainFloorPlatform004DA460(Trigger* pTrigger);
bool RapaxMainFloorPlatform01004DA590(Trigger* pTrigger);
bool RapaxMainFloorPlatform02004DA5D0(Trigger* pTrigger);
