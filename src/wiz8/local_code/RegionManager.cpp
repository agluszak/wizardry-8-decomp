#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

// FUNCTION: WIZ8 0x004F21E0
void RegionSetEnable(unsigned int region_set_index)
{
    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x22b,
            0);
    }
    g_region_sets[region_set_index].enabled = 1;
}

// FUNCTION: WIZ8 0x004F2220
void RegionSetDisable(unsigned int region_set_index)
{
    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x234,
            0);
    }
    g_region_sets[region_set_index].enabled = 0;
}

// FUNCTION: WIZ8 0x004F2380
void ClearRegionModeBits(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x259,
            0);
    }
    g_regions[region_index].flags &= 0xfff3;
}

// FUNCTION: WIZ8 0x004F23D0
void SetRegionMode4(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x262,
            0);
    }
    unsigned int flags = g_regions[region_index].flags;
    flags &= 0xfff3;
    flags |= 4;
    g_regions[region_index].flags = flags;
}

// FUNCTION: WIZ8 0x004F2550
bool RegionHasFlags(unsigned int region_index, unsigned int flags)
{
    bool has_flags;

    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x2c6,
            0);
    }
    has_flags = (g_regions[region_index].flags & flags) != 0;
    return has_flags;
}
