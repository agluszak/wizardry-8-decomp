#pragma once

class Trigger;

/* Level 0x15 (Rift1) activation callbacks, registered by
   InitializeLevelMasterFunctions. */
bool Rift1Sexspawn(Trigger* pTrigger);
bool Rift1Hotstuff(Trigger* pTrigger);
bool Rift1Fireantspawn(Trigger* pTrigger);
bool Rift1Gate(Trigger* pTrigger);
bool Rift1AshLock(Trigger* pTrigger);
bool Rift1TimeDorado(Trigger* pTrigger);
