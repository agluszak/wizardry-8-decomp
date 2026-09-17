#pragma once

class Trigger;

/* Level 4 (Cosmic Circle) setup and activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50. The always-false callback is also
   installed on Arnika's "ARN11" trigger. */
bool CosmicCircleReturnFalse004D9AC0(Trigger* pTrigger);
bool CosmicCircleTriggerPlane1Hedra004D9AD0(Trigger* pTrigger);
void CosmicCircleSetup004D9B40(void);
