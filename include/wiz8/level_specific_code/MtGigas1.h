#pragma once

class Trigger;

/* Level 0x0c (MtGigas1) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
void MtGigas1Setup(void);
bool MtGigas1Lift1(Trigger* pTrigger);
bool MtGigas1Lift2(Trigger* pTrigger);
bool MtGigas1PressurePlate(Trigger* pTrigger);
bool MtGigas1MudWall(Trigger* pTrigger);
