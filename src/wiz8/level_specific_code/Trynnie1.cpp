#include "wiz8/level_specific_code/Trynnie1.h"
#include "wiz8/fact_state.h"
#include "wiz8/location_variables.h"
#include "wiz8/layouts/item_instance.h"

/* Level Specific Code\Trynnie1.cpp (level 0x1a).

   Attribution evidence: 0x004DA850 is directly attributed to this TU, and
   InitializeLevelMasterFunctions004D6C50 runs 0x004D9D30 under case 0x1a, the
   Trynnie1 block that wires the URN trigger callbacks. The intervening gap
   functions are Trynnie helpers pending their own evidence. */

void Function4DA850(void);

/* Fact 0x229 records that the Trynnie2 group was wiped out; mirror it into the
   Trynnie2Killed location variable the first time this level sees it, so the
   world-side check has a stable flag to read. */
// FUNCTION: WIZ8 0x004D9D30
void EnsureTrynnie2KilledVar004D9D30(void)
{
    if (GetFact(0x229) == 1) {
        if (GetLocationVarIDByName("Trynnie2Killed") == -1) {
            Function4DA850();
            CreateLocationVar("Trynnie2Killed", 1);
        }
    }
}

/* Item ids 0x1b3 and 0x1c3 route the use-item action down the origin/equip
   path instead of activating, the same as a non-usable equip class. */
// FUNCTION: WIZ8 0x004DA0F0
char IsSpecialItemId004DA0F0(W8ItemInstance* item)
{
    if (item->item_id != 0x1b3 && item->item_id != 0x1c3) {
        return 0;
    }
    return 1;
}
