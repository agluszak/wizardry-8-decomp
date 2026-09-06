#include "wiz8/regions.h"

extern "C" unsigned int g_region_set_69c528;

/* Address quarantine 005c4341-005c87af; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x005C5D70
void DisableRegionSet69C528(void)
{
    RegionSetDisable(g_region_set_69c528);
}
