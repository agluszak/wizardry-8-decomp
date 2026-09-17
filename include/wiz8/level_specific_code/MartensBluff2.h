#pragma once

class Trigger;

/* Level 6 (MartensBluff2) master function and activation callbacks,
   registered by the case-6 block of InitializeLevelMasterFunctions004D6C50. */
void MartensBluff2Setup004DCB50(void);
bool TriggerArrowTrap(Trigger* pTrigger);
bool MartensBluff2Spikeballtrigger004DD3E0(Trigger* pTrigger);
void MartensBluff2Spikeball004DD3F0(int command);
void MartensBluff2MonsterCrusher004DDF40(int command);
void MartensBluff2IdolGas004DE660(int command);
void MartensBluff2IdolGasVictim004DE7D0(void);
bool MartensBluff2DoorBolt004DDD30(Trigger* pTrigger);
bool MartensBluff2DummyLever004DDD50(Trigger* pTrigger);
bool MartensBluff2Dummy004DDD80(Trigger* pTrigger);
bool MartensBluff2PerfumeBox004DDDC0(Trigger* pTrigger);
bool MartensBluff2SquisherControls004DDF20(Trigger* pTrigger);
bool MartensBluff2DoorControls004DDEB0(Trigger* pTrigger);
bool MartensBluff2StoneIdol004DE520(Trigger* pTrigger);
bool MartensBluff2BlueFlowers004DE620(Trigger* pTrigger);
