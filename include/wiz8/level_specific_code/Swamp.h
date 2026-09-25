#pragma once

class Trigger;

/* Level 0x18 (Swamp) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
bool SwampOilPool(Trigger* pTrigger);
bool SwampGasPlane(Trigger* pTrigger);
bool SwampFirePlane(Trigger* pTrigger);
bool SwampOnelid(Trigger* pTrigger);
