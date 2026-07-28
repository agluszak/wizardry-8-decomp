#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <new>
#include <wchar.h>

extern int g_region_help_clock;
extern unsigned int g_active_region_index;
extern wchar_t* g_region_help_text;
extern unsigned int g_forced_region_index;
extern int g_region_help_delay;
extern unsigned char g_region_help_force_enabled;
extern unsigned char g_flag_6850d4;
extern unsigned short g_word_6850ed;
extern void HideRegionHelp(void);                           /* 0x00429770 */
extern unsigned int g_dword_689b50;
extern void* g_default_help_text;                           /* 0x00689B40 */
extern int g_help_box_width;                                /* 0x006548A0 */
extern int g_help_box_height;                               /* 0x00654ACC */
extern void SetHelpBoxText(void* text);                     /* 0x00429290 */
extern void GetHelpBoxAnchor(W8ScreenPoint* anchor);        /* 0x004284F0 */
extern void PlaceHelpBox(int x, int y);                     /* 0x00429210 */

enum { W8_SCREEN_WIDTH = 640, W8_SCREEN_HEIGHT = 480, W8_HELP_MARGIN = 2 };
enum { W8_REGION_MODE_MASK = 0xf, W8_REGION_HELP_SHOWN = 0x200 };

/* The complete screen-lifecycle initializer owns the catalog.  Its reviewed
   main-menu path addresses set one, so retain that bounded prefix while
   wiz8-a69 restores all records and their exact region ranges. */
unsigned int g_region_set_count = 2;
W8RegionSet g_region_sets[300];
unsigned int g_region_count = 1500;
W8Region g_regions[1500];
unsigned int g_hot_region_689b3c;
unsigned int g_hot_region_689b44;
unsigned int g_hot_region_689b4c;
unsigned short g_dword_689b48;
unsigned int g_dword_689b50;
unsigned short g_word_6850ed;

/* Raises the help box for one region, taking a stale one down first. The
   selected text comes either from the region's indexed notice entry or the
   shared fallback, and the final position is clamped inside the 640x480
   screen before the region records ownership of the box. */
// FUNCTION: WIZ8 0x004F2650
void ShowRegionHelp(unsigned int region_index)
{
    W8Region* region;
    unsigned int mode;
    void* text;
    W8ScreenPoint anchor;
    int width;
    int height;

    if (g_flag_6850d4 == 0 && g_dword_689b50 == 0) {
        return;
    }
    region = &g_regions[region_index];
    mode = region->flags & W8_REGION_MODE_MASK;
    if (mode != 1 && mode != 2 && (region->flags & W8_REGION_HELP_SHOWN) != 0) {
        HideRegionHelp();
        region->flags &= ~W8_REGION_HELP_SHOWN;
    }
    if ((region->flags & W8_REGION_HELP_SHOWN) != 0) {
        return;
    }
    if (region->help_text_id == -1) {
        text = g_default_help_text;
        if (text == 0) {
            return;
        }
    } else {
        text = g_notices[region->help_text_id];
    }
    SetHelpBoxText(text);
    width = g_help_box_width + W8_HELP_MARGIN;
    height = g_help_box_height + W8_HELP_MARGIN;
    GetHelpBoxAnchor(&anchor);
    anchor.y -= height;
    if (anchor.x < 0) {
        anchor.x = W8_HELP_MARGIN;
    }
    if (anchor.x + width > W8_SCREEN_WIDTH - 1) {
        anchor.x = W8_SCREEN_WIDTH - width;
    }
    if (anchor.y < 0) {
        anchor.y = W8_HELP_MARGIN;
    }
    if (anchor.y + height > W8_SCREEN_HEIGHT - 1) {
        anchor.y = W8_SCREEN_HEIGHT - height;
    }
    PlaceHelpBox(anchor.x, anchor.y);
    region->flags |= W8_REGION_HELP_SHOWN;
}

// FUNCTION: WIZ8 0x004F21B0
unsigned char ClearActiveRegionIfMatches(unsigned int region_index)
{
    if (g_forced_region_index == region_index) {
        g_forced_region_index = 0;
        g_active_region_index = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004F21D0
unsigned int GetForcedRegion(void)
{
    return g_forced_region_index;
}

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

// FUNCTION: WIZ8 0x004F25A0
void UpdateRegionHelp(void)
{
    if (g_forced_region_index == 0) {
        if (g_active_region_index != 0 &&
            g_regions[g_active_region_index].help_enabled != 0 &&
            (g_flag_6850d4 != 0 || g_region_help_force_enabled != 0) &&
            ClockIsTicking(g_region_help_clock) == 0) {
            ShowRegionHelp(g_active_region_index);
        }
    } else if (g_regions[g_forced_region_index].help_enabled != 0 &&
               (g_flag_6850d4 != 0 || g_region_help_force_enabled != 0) &&
               ClockIsTicking(g_region_help_clock) == 0) {
        ShowRegionHelp(g_forced_region_index);
    }
}


// FUNCTION: WIZ8 0x004F2750
void SetRegionHelpText(const wchar_t* text)
{
    if (g_region_help_text != 0) {
        ::operator delete(g_region_help_text);
    }
    if (text != 0) {
        g_region_help_text = static_cast<wchar_t*>(
            ::operator new((wcslen(text) + 1) * sizeof(wchar_t)));
        wcscpy(g_region_help_text, text);
    } else {
        g_region_help_text = 0;
    }
}

// FUNCTION: WIZ8 0x004F27F0
void ResetRegionHelp(unsigned char delayed)
{
    unsigned int region_index = g_active_region_index;

    HideRegionHelp();
    g_regions[region_index].flags &= 0xfffffdff;
    if (delayed == 0) {
        ShowRegionHelp(g_active_region_index);
    } else if (g_regions[g_active_region_index].help_enabled != 0 &&
               (g_flag_6850d4 != 0 || g_region_help_force_enabled != 0)) {
        g_region_help_clock = SetCountdownClock(g_region_help_delay);
    }
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

// FUNCTION: WIZ8 0x004F28E0
void ResetRegionSet(unsigned int region_set_index)
{
    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSet < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x4a0,
            0);
    }
    g_region_sets[region_set_index].last_region = 0;
}

// FUNCTION: WIZ8 0x004F2920
unsigned int AddRegionToSet(unsigned int region_set_index)
{
    unsigned int region_index;

    if (region_set_index >= g_region_set_count) {
        srAssertFail(
            "uiRegionSet < guiRegsetCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x4b7,
            0);
    }
    if (g_region_sets[region_set_index].last_region == 0) {
        region_index = g_region_sets[region_set_index].first_region;
    } else {
        region_index = g_region_sets[region_set_index].last_region + 1;
    }
    g_region_sets[region_set_index].last_region = region_index;
    if (region_index == g_region_count) {
        ++g_region_count;
        if (g_region_count > 1500) {
            srAssertFail(
                "guiRegionCount <= REGION_LIMIT",
                "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
                0x4c7,
                0);
        }
        g_regions[region_index] = g_regions[0];
    }
    return region_index;
}

// FUNCTION: WIZ8 0x004F29C0
void SetRegionCallback(unsigned int region_index, W8RegionCallback callback,
                       unsigned short callback_id)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x4df,
            0);
    }
    g_regions[region_index].callback = callback;
    g_regions[region_index].callback_id = callback_id;
}

// FUNCTION: WIZ8 0x004F2A10
void SetRegionOwner(unsigned int region_index, void* owner)
{
    g_regions[region_index].owner = owner;
}

// FUNCTION: WIZ8 0x004F2A30
void SetRegionHelp(unsigned int region_index, unsigned char enabled, int help_text_id)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x506,
            0);
    }
    g_regions[region_index].help_enabled = enabled;
    g_regions[region_index].help_text_id = help_text_id;
}

// FUNCTION: WIZ8 0x004F2BB0
void EnableRegionHelp(unsigned int region_index)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x558,
            0);
    }
    g_regions[region_index].help_enabled = 1;
}

// FUNCTION: WIZ8 0x004F2BF0
void DisableRegionHelp(unsigned int region_index)
{
    if (region_index > g_region_count) {
        srAssertFail(
            "uiRegionIndex <= guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x55f,
            0);
    }
    g_regions[region_index].help_enabled = 0;
}

extern void Function429770(void);

/* Drops every region back to its resting state. The three tracked regions are
   released first - each only if it still carries bit 0x200 - then every region
   set is disabled and every region keeps only its low two flag bits. The three
   trackers are cleared last, and the fourth field is reseeded from the
   settings word rather than zeroed. */
// FUNCTION: WIZ8 0x004F1240
void ResetRegions(void)
{
    unsigned int index;
    W8RegionSet* set;
    W8Region* region;
    unsigned int remaining;

    index = g_hot_region_689b3c;
    if (g_hot_region_689b3c != 0 && (g_regions[g_hot_region_689b3c].flags & 0x200) != 0) {
        Function429770();
        g_regions[index].flags &= ~0x200u;
    }
    index = g_hot_region_689b4c;
    if (g_hot_region_689b4c != 0 && (g_regions[g_hot_region_689b4c].flags & 0x200) != 0) {
        Function429770();
        g_regions[index].flags &= ~0x200u;
    }
    index = g_hot_region_689b44;
    if (g_hot_region_689b44 != 0 && (g_regions[g_hot_region_689b44].flags & 0x200) != 0) {
        Function429770();
        g_regions[index].flags &= ~0x200u;
    }
    if (g_region_set_count != 0) {
        set = g_region_sets;
        remaining = g_region_set_count;
        do {
            set->enabled = 0;
            set = set + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }
    if (g_region_count != 0) {
        region = g_regions;
        remaining = g_region_count;
        do {
            region->flags &= 3;
            region = region + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }
    g_hot_region_689b3c = 0;
    g_hot_region_689b4c = 0;
    g_hot_region_689b44 = 0;
    g_dword_689b50 = 0;
    g_dword_689b48 = g_word_6850ed;
}
