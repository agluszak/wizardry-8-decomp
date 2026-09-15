#pragma once

class Trigger;

/* Level 8 (Monastery1) helpers, run by InitializeLevelMasterFunctions004D6C50
   under case 8. */
void ClearTextForBarTrigger004DC8D0(void);
bool OnRoachTriggerActivated(Trigger* trigger);
bool OnSpiderTriggerActivated(Trigger* trigger);
bool OnBarTriggerActivated(Trigger* trigger);
bool OnCoffinlideActivated(Trigger* trigger);
bool OnCoffinlidgActivated(Trigger* trigger);
bool OnWheelStarActivated(Trigger* trigger);
