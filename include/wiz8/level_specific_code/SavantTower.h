#pragma once

class Trigger;

/* Level 0x24 (SavantTower) activation callbacks, registered by the case-0x24
   block of InitializeLevelMasterFunctions004D6C50. */
void SavantTowerSyncButtonBlocker004E0510(void);          /* 0x004E0510 */
bool SavantTowerShape004E0560(Trigger* pTrigger);         /* 0x004E0560 */
bool SavantTowerButtonBlocker004E05A0(Trigger* pTrigger); /* 0x004E05A0 */
