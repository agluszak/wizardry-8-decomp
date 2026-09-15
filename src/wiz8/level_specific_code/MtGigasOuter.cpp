#include "wiz8/level_specific_code/MtGigasOuter.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/Trigger.hpp"

/* Level Specific Code\MtGigasOuter.cpp (level 0x0e).

   Attribution evidence: 0x004DC080 is this TU's hard lower bound and
   InitializeLevelMasterFunctions004D6C50 runs 0x004DBE70 under case 0x0e,
   alongside the _VOC_EWAXXLIFT1 trigger wiring. */

/* When the FlagPosition location variable exists, feed its value to the gate
   helper; values at or above 1000 stop there. Otherwise the "flag" trigger
   runs. */
// FUNCTION: WIZ8 0x004DBE70
void ProcessFlagPosition004DBE70(void)
{
    if (GetLocationVarIDByName("FlagPosition") != -1) {
        int value = GetLocationVarValueByName("FlagPosition");
        Function4DC080(value);
        if (value >= 1000) {
            return;
        }
    }
    Trigger* trigger = FindTriggerByName("flag");
    trigger->Run(-1);
}
