#pragma once

class Trigger;

/* Level 6 (MartensBluff2) master function and activation callbacks,
   registered by the case-6 block of InitializeLevelMasterFunctions. */
void MartensBluff2Setup(void);
bool TriggerArrowTrap(Trigger* pTrigger);
bool MartensBluff2Spikeballtrigger(Trigger* pTrigger);
void MartensBluff2Spikeball(int command);
void MartensBluff2MonsterCrusher(int command);
void MartensBluff2IdolGas(int command);
void MartensBluff2IdolGasVictim(void);
bool MartensBluff2DoorBolt(Trigger* pTrigger);
bool MartensBluff2DummyLever(Trigger* pTrigger);
bool MartensBluff2Dummy(Trigger* pTrigger);
bool MartensBluff2PerfumeBox(Trigger* pTrigger);
bool MartensBluff2SquisherControls(Trigger* pTrigger);
bool MartensBluff2DoorControls(Trigger* pTrigger);
bool MartensBluff2StoneIdol(Trigger* pTrigger);
bool MartensBluff2BlueFlowers(Trigger* pTrigger);
