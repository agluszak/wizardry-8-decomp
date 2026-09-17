#pragma once

class Trigger;

/* Level 0x0d (MtGigas2) activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50. */
void MtGigas2Setup004DB200(void);
bool MtGigas2Train004DB380(Trigger* pTrigger);
bool MtGigas2RedWire004DB420(Trigger* pTrigger);
bool MtGigas2BlueWire004DB460(Trigger* pTrigger);
bool MtGigas2YellowWire004DB4A0(Trigger* pTrigger);
bool MtGigas2Lift3004DB650(Trigger* pTrigger);
bool MtGigas2TopDoor1004DB690(Trigger* pTrigger);
bool MtGigas2Officer1004DB6D0(Trigger* pTrigger);
bool MtGigas2Officer2004DB770(Trigger* pTrigger);
bool MtGigas2LaserAlarm004DB810(Trigger* pTrigger);
bool MtGigas2WiringMalfunction004DBA70(Trigger* pTrigger);
bool MtGigas2AccessHatch004DBA90(Trigger* pTrigger);
