#pragma once

class Trigger;

/* Level 0x0f (MtGigasTop) activation callbacks, registered by
   InitializeLevelMasterFunctions under case 0x0f. */
bool OnEwaxxCannon1Activated(Trigger* trigger);
bool OnEwaxxLandingActivated(Trigger* trigger);
bool OnCatchCordActivated(Trigger* trigger);
bool OnEwaxxTopDoor2Activated(Trigger* trigger);
bool OnPainActivatorActivated(Trigger* trigger);
