#include "wiz8/regions.h"

extern "C" unsigned int g_region_set_69c528;
extern "C" {
// GLOBAL: WIZ8 0x0069c528
unsigned int g_region_set_69c528;
}

// FUNCTION: WIZ8 0x005C5D70
void DisableRegionSet69C528(void)
{
    RegionSetDisable(g_region_set_69c528);
}
