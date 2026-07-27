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

// FUNCTION: WIZ8 0x004F2260
void ClearRegionSetModeBits(unsigned int region_set_index)
{
    unsigned int region_index;
    W8Region* region;

    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x23f,
            0);
    }
    region_index = g_region_sets[region_set_index].first_region;
    if (region_index <= g_region_sets[region_set_index].last_region) {
        region = &g_regions[region_index];
        do {
            if (region_index >= g_region_count) {
                srAssertFail(
                    "uiRegionIndex < guiRegionCount",
                    "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
                    0x259,
                    0);
            }
            region->flags &= 0xfff3;
            ++region_index;
            ++region;
        } while (region_index <= g_region_sets[region_set_index].last_region);
    }
}

// FUNCTION: WIZ8 0x004F22F0
void SetRegionSetMode4(unsigned int region_set_index)
{
    unsigned int region_index;
    unsigned int last_region;

    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSetIndex < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x24d,
            0);
    }
    region_index = g_region_sets[region_set_index].first_region;
    if (region_index <= g_region_sets[region_set_index].last_region) {
        do {
            if (region_index >= g_region_count) {
                srAssertFail(
                    "uiRegionIndex < guiRegionCount",
                    "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
                    0x262,
                    0);
            }
            last_region = g_region_sets[region_set_index].last_region;
            g_regions[region_index].flags =
                (g_regions[region_index].flags & 0xfff3) | 4;
            ++region_index;
        } while (region_index <= last_region);
    }
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

// FUNCTION: WIZ8 0x004F2420
void SetRegionBounds(unsigned int region_index, unsigned short x1, unsigned short y1,
                     unsigned short x2, unsigned short y2)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x27d,
            0);
    }
    g_regions[region_index].x1 = x1;
    g_regions[region_index].y1 = y1;
    g_regions[region_index].x2 = x2;
    g_regions[region_index].y2 = y2;
}

// FUNCTION: WIZ8 0x004F2490
bool RegionContainsPoint(unsigned int region_index, unsigned short x, unsigned short y)
{
    W8Region* region;

    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x2a4,
            0);
    }
    region = &g_regions[region_index];
    switch (region->flags & 0x0f) {
    case 1:
        if (x >= region->x1 && x <= region->x2 &&
            y >= region->y1 && y <= region->y2) {
            return true;
        }
        break;
    case 2: {
        short delta_x = x - region->x1;
        short delta_y = y - region->y1;
        if (delta_x * delta_x + delta_y * delta_y <= region->x2 * region->x2) {
            return true;
        }
        break;
    }
    }
    return false;
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

// FUNCTION: WIZ8 0x004F2880
unsigned int CreateRegionSet(void)
{
    unsigned int region_set_index = g_region_set_count++;

    if (g_region_set_count > 300) {
        srAssertFail(
            "guiRegsetCount <= REGSET_LIMIT",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x487,
            0);
    }
    g_region_sets[region_set_index].enabled = 0;
    g_region_sets[region_set_index].first_region = g_region_count;
    g_region_sets[region_set_index].last_region = 0;
    return region_set_index;
}
