#pragma once

class Trigger;

/* Level 0x0d (MtGigas2) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
void MtGigas2Setup(void);
bool MtGigas2Train(Trigger* pTrigger);
bool MtGigas2RedWire(Trigger* pTrigger);
bool MtGigas2BlueWire(Trigger* pTrigger);
bool MtGigas2YellowWire(Trigger* pTrigger);
bool MtGigas2Lift3(Trigger* pTrigger);
bool MtGigas2TopDoor1(Trigger* pTrigger);
bool MtGigas2Officer1(Trigger* pTrigger);
bool MtGigas2Officer2(Trigger* pTrigger);
bool MtGigas2LaserAlarm(Trigger* pTrigger);
bool MtGigas2WiringMalfunction(Trigger* pTrigger);
bool MtGigas2AccessHatch(Trigger* pTrigger);
