#include "wiz8/level_specific_code/Camp.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/fact_state.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"

/* Level Specific Code for Camp.LVL (level 0x10) — the Rapax away camp.

   Attribution evidence: this TU sits in the unanchored gap between the
   Trynnie2.cpp and Trynnie1.cpp intervals (0x004DA610..0x004DA650), after the
   RapaxMainFloor/RapaxUpperFloor clusters; the level-0x10 block in
   InitializeLevelMasterFunctions registers prisondoor06/04/03. The
   original file name is not anchored by an assertion path string. */

/* "prisondoor06": clear fact 0x1e9. */
// FUNCTION: WIZ8 0x004DA610
bool CampPrisonDoor06(Trigger* pTrigger)
{
    SetFact(W8_FACT_RAPAX_QUEEN_LOCKED_UP, 0, false);
    return true;
}

/* "prisondoor04": clear fact 0x7d. */
// FUNCTION: WIZ8 0x004DA630
bool CampPrisonDoor04(Trigger* pTrigger)
{
    SetFact(W8_FACT_RODAN_LOCKED_IN_CAGE, 0, false);
    return true;
}

/* "prisondoor03": clear fact 0x3e. */
// FUNCTION: WIZ8 0x004DA650
bool CampPrisonDoor03(Trigger* pTrigger)
{
    SetFact(W8_FACT_DRAZIC_LOCKED_IN_CAGE, 0, false);
    return true;
}
