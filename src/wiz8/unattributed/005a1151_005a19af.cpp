#include "wiz8/regions.h"

/* Address quarantine 005a1151-005a19af; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x005A19A0
void DisableRegionSet1C(void)
{
    RegionSetDisable(0x1c);
}
