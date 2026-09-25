#pragma once

class Trigger;

/* Level 0x24 (SavantTower) activation callbacks, registered by the case-0x24
   block of InitializeLevelMasterFunctions. */
void SavantTowerSyncButtonBlocker(void);          /* 0x004E0510 */
bool SavantTowerShape(Trigger* pTrigger);         /* 0x004E0560 */
bool SavantTowerButtonBlocker(Trigger* pTrigger); /* 0x004E05A0 */
