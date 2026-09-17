#pragma once

class Trigger;

/* Level 0x18 (Swamp) activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50. */
bool SwampOilPool004DA960(Trigger* pTrigger);
bool SwampGasPlane004DA9B0(Trigger* pTrigger);
bool SwampFirePlane004DAA10(Trigger* pTrigger);
bool SwampOnelid004DADF0(Trigger* pTrigger);
