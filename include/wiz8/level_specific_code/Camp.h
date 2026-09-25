#pragma once

class Trigger;

/* Level 0x10 (Camp) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
bool CampPrisonDoor06(Trigger* pTrigger);
bool CampPrisonDoor04(Trigger* pTrigger);
bool CampPrisonDoor03(Trigger* pTrigger);
