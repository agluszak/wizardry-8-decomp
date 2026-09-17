#pragma once

class Trigger;

/* Level 0x0c (MtGigas1) activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50. */
void MtGigas1Setup004DBAB0(void);
bool MtGigas1Lift1004DBB50(Trigger* pTrigger);
bool MtGigas1Lift2004DBB90(Trigger* pTrigger);
bool MtGigas1PressurePlate004DBBD0(Trigger* pTrigger);
bool MtGigas1MudWall004DBE30(Trigger* pTrigger);
