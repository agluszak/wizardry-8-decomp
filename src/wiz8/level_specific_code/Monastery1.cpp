#include "wiz8/level_specific_code/Monastery1.h"
#include "wiz8/engine_code/Trigger.hpp"

/* Level Specific Code\Monastery1.cpp (level 8).

   Attribution evidence: InitializeLevelMasterFunctions004D6C50 registers
   0x004DC8D0 under case 8, the Monastery1 block that also wires the
   roach_trigger, spider_trigger, Bartrigger and Coffinlide callbacks. The
   function itself is bar-trigger housekeeping for that same set. */

// FUNCTION: WIZ8 0x004DC8D0
void ClearTextForBarTrigger004DC8D0(void)
{
    Trigger* trigger = FindTriggerByName("Bartrigger");
    if (trigger != 0 && trigger->value_0b1 == 1) {
        trigger = FindTriggerByName("Textforbar");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~0x100u;
        }
    }
}
