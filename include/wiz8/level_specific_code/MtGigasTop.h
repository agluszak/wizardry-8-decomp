#pragma once

class Trigger;

/* Level 0x0f (MtGigasTop) activation callbacks, registered by
   InitializeLevelMasterFunctions004D6C50 under case 0x0f. */
bool OnEwaxxCannon1Activated(Trigger* trigger);
bool OnEwaxxLandingActivated(Trigger* trigger);
bool OnCatchCordActivated(Trigger* trigger);
bool OnEwaxxTopDoor2Activated(Trigger* trigger);
bool OnPainActivatorActivated(Trigger* trigger);
