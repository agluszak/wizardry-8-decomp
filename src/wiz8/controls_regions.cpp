#include "gameplay_boundaries.h"
#include "sr_api.h"

/* 0x004F21E0 and 0x004F2220; descriptive names taken from their use here. */
extern "C" void RegionSetEnable(unsigned int region_set_id);
extern "C" void RegionSetDisable(unsigned int region_set_id);

/* Local Code\Controls.cpp. m_uiRegionSetId is named by the canonical assertion
   at line 399, and REGSET_NULL is zero because the body's guard is a plain
   test against zero. */
#define REGSET_NULL 0

struct W8Controls {
    unsigned char unknown_00[0x48];
    unsigned int m_uiRegionSetId;           /* 0x48 */

    void EnableRegionSet(unsigned char enable);
};

// FUNCTION: WIZ8 0x004F30F0
void W8Controls::EnableRegionSet(unsigned char enable)
{
    if (m_uiRegionSetId == REGSET_NULL) {
        srAssertFail(
            "m_uiRegionSetId != REGSET_NULL",
            "C:\\Projects\\Wizardry 8\\Local Code\\Controls.cpp",
            0x18f,
            0);
    }
    if (enable) {
        RegionSetEnable(m_uiRegionSetId);
    } else {
        RegionSetDisable(m_uiRegionSetId);
    }
}
