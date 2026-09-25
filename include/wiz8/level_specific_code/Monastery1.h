#pragma once

class Trigger;

/* Level 8 (Monastery1) helpers, run by InitializeLevelMasterFunctions
   under case 8. */
void ClearTextForBarTrigger(void);
bool OnRoachTriggerActivated(Trigger* trigger);
bool OnSpiderTriggerActivated(Trigger* trigger);
bool OnBarTriggerActivated(Trigger* trigger);
bool OnCoffinlideActivated(Trigger* trigger);
bool OnCoffinlidgActivated(Trigger* trigger);
bool OnWheelStarActivated(Trigger* trigger);
