#pragma once

class Trigger;

/* Level 4 (Cosmic Circle) setup and activation callbacks, registered by
   InitializeLevelMasterFunctions. The always-false callback is also
   installed on Arnika's "ARN11" trigger. */
bool CosmicCircleReturnFalse(Trigger* pTrigger);
bool CosmicCircleTriggerPlane1Hedra(Trigger* pTrigger);
void CosmicCircleSetup(void);
