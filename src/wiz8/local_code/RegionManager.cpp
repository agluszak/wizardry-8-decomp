#include "wiz8/regions.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/cursor.h"
#include "wiz8/screen_state.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/utility.h"
#include "wiz8/sr_api.h"
#include "input.h"
#include "timer.h"

#include <new>
#include <wchar.h>

extern int g_help_box_width;                                /* 0x006548A0 */
// GLOBAL: WIZ8 0x006548a0
int g_help_box_width;
extern int g_help_box_height;                               /* 0x00654ACC */
// GLOBAL: WIZ8 0x00654acc
int g_help_box_height;
extern void SetHelpBoxText(void* text);                     /* 0x00429290 */
extern void PlaceHelpBox(int x, int y);                     /* 0x00429210 */

enum { W8_SCREEN_WIDTH = 640, W8_SCREEN_HEIGHT = 480, W8_HELP_MARGIN = 2 };
enum { W8_REGION_MODE_MASK = 0xf };

/* The retail catalog contains 51 statically declared sets and 313 statically
   declared regions.  Dynamically constructed controls append after that
   catalog; region zero is the template copied into each appended record. */
// GLOBAL: WIZ8 0x00617b18
unsigned int g_region_set_count = 51;
// GLOBAL: WIZ8 0x0061f238
W8RegionSet g_region_sets[300] = {
    { 0, 0, 0 },
    { 0, 1, 6 }
};
// GLOBAL: WIZ8 0x00617b1c
unsigned int g_region_count = 313;
// GLOBAL: WIZ8 0x00620048
W8Region g_regions[1500] = {
    { W8_REGION_RECTANGLE, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 },
    { W8_REGION_RECTANGLE, 174, 138, 467, 182, MainMenuIntroduction, 0, 0, 0, -1, 0 },
    { W8_REGION_RECTANGLE, 140, 187, 501, 231, MainMenuNewGame, 0, 0, 0, -1, 0 },
    { W8_REGION_RECTANGLE, 204, 235, 436, 279, MainMenuLoadGame, 0, 0, 0, -1, 0 },
    { W8_REGION_RECTANGLE, 239, 284, 403, 328, MainMenuCredits, 0, 0, 0, -1, 0 },
    { W8_REGION_RECTANGLE, 234, 335, 408, 379, MainMenuOptions, 0, 0, 0, -1, 0 },
    { W8_REGION_RECTANGLE, 279, 423, 364, 467, MainMenuExit, 0, 0, 0, -1, 0 }
};
unsigned int g_current_region_index;
wchar_t* g_default_help_text;
unsigned int g_captured_region_index;
unsigned int g_hover_region_index;
unsigned int g_region_help_force_enabled;

// GLOBAL: WIZ8 0x00689B32
unsigned char g_flag_689b32;

extern unsigned short gfAltState;
extern unsigned short gfCtrlState;
extern unsigned short gfShiftState;

// FUNCTION: WIZ8 0x004f27a0
void SetRegionHelpDelay(int delay_ms)
{
    if (delay_ms == 0) {
        delay_ms = g_settings_6850c8.tooltip_delay_ms;
    }
    g_region_help_delay = delay_ms;
}

// FUNCTION: WIZ8 0x004f1220
void ReleasePointer689B40(void)
{
    if (g_default_help_text != 0) {
        delete[] g_default_help_text;
    }
}

/* Dispatch one mouse-position event through the enabled region sets. A forced
   modal region bypasses hit testing; otherwise the first containing region
   receives leave/enter transitions, hover help timing, and the ordinary
   position callback as one transaction. */
// FUNCTION: WIZ8 0x004f1360
unsigned int UpdateRegionMousePosition(int x, int y)
{
    W8RegionMouseEvent event;
    unsigned int set_index;
    unsigned int region_index;

    event.event.time = GetClock();
    event.event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.event.reason = MOUSE_POS;
    event.mouse_position =
        (static_cast<unsigned int>(static_cast<unsigned short>(y)) << 16) |
        static_cast<unsigned short>(x);

    if (g_captured_region_index != 0) {
        W8Region* forced = &g_regions[g_captured_region_index];
        forced->callback(&event.event, forced);
        return g_current_region_index;
    }

    for (set_index = 0; set_index < g_region_set_count; ++set_index) {
        W8RegionSet* set = &g_region_sets[set_index];
        if (set->enabled != 1 || set->first_region > set->last_region) {
            continue;
        }
        for (region_index = set->first_region;
             region_index <= set->last_region; ++region_index) {
            if (!RegionContainsPoint(region_index,
                                     static_cast<unsigned short>(x),
                                     static_cast<unsigned short>(y))) {
                continue;
            }

            W8Region* region = &g_regions[region_index];
            unsigned int previous_index = g_hover_region_index;
            g_current_region_index = region_index;
            if (previous_index != 0 && previous_index != region_index) {
                W8Region* previous = &g_regions[previous_index];
                previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
                previous->callback(&event.event, previous);
                if ((previous->flags & W8_REGION_HELP_SHOWN) != 0) {
                    ReleaseScreenTransitionObjects();
                    previous->flags &= ~W8_REGION_HELP_SHOWN;
                }
                PlayButtonSound(1);
                g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
                previous->flags &= ~W8_REGION_MOUSE_STATE_MASK;
                g_region_help_force_enabled = 0;
            }
            if (previous_index != region_index) {
                region->flags |= W8_REGION_MOUSE_ENTER;
                SetRegionHelpText(
                    FormatWideString(L"Region %d", region_index));
            }
            region->callback(&event.event, region);
            if (g_current_region_index != previous_index) {
                if (region->help_enabled != 0 &&
                    (g_settings_6850c8.tooltips_enabled != 0 ||
                     g_region_help_force_enabled != 0)) {
                    g_region_help_clock =
                        SetCountdownClock(g_region_help_delay);
                }
                PlayButtonSound(0);
            }
            region->flags &= ~W8_REGION_MOUSE_TRANSITION_MASK;
            g_hover_region_index = g_current_region_index;
            return g_current_region_index;
        }
    }

    g_current_region_index = 0;
    if (g_hover_region_index != 0) {
        unsigned int previous_index = g_hover_region_index;
        W8Region* previous = &g_regions[previous_index];
        previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
        previous->callback(&event.event, previous);
        if ((previous->flags & W8_REGION_HELP_SHOWN) != 0) {
            ReleaseScreenTransitionObjects();
            previous->flags &= ~W8_REGION_HELP_SHOWN;
        }
        PlayButtonSound(1);
        g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
        g_region_help_force_enabled = 0;
        previous->flags &= ~W8_REGION_MOUSE_STATE_MASK;
    }
    g_hover_region_index = g_current_region_index;
    return g_current_region_index;
}

/* Find the first enabled region containing the mouse position.  Moving to a
   different region also sends the old region its leave transition and drops
   any help box it still owns. */
// FUNCTION: WIZ8 0x004f16f0
unsigned int FindRegionAtPoint(unsigned short x, unsigned short y)
{
    W8RegionMouseEvent event;
    unsigned int set_index;
    unsigned int region_index;

    event.event.time = GetClock();
    event.event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.event.reason = MOUSE_POS;
    event.mouse_position =
        (static_cast<unsigned int>(y) << 16) | x;

    if (g_captured_region_index != 0) {
        return g_captured_region_index;
    }

    for (set_index = 0; set_index < g_region_set_count; ++set_index) {
        W8RegionSet* set = &g_region_sets[set_index];
        if (set->enabled != 1 || set->first_region > set->last_region) {
            continue;
        }
        for (region_index = set->first_region;
             region_index <= set->last_region; ++region_index) {
            if (!RegionContainsPoint(region_index, x, y)) {
                continue;
            }
            if (g_hover_region_index != 0 &&
                g_hover_region_index != region_index) {
                W8Region* previous = &g_regions[g_hover_region_index];
                previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
                previous->callback(&event.event, previous);
                if ((previous->flags & W8_REGION_HELP_SHOWN) != 0) {
                    ReleaseScreenTransitionObjects();
                    previous->flags &= ~W8_REGION_HELP_SHOWN;
                }
                g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
                previous->flags &= ~W8_REGION_MOUSE_STATE_MASK;
                g_region_help_force_enabled = 0;
                g_hover_region_index = 0;
                g_current_region_index = 0;
            }
            return region_index;
        }
    }

    if (g_hover_region_index != 0 &&
        (g_regions[g_hover_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        unsigned int previous_index = g_hover_region_index;
        ReleaseScreenTransitionObjects();
        g_regions[previous_index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    return 0;
}

/* Route one queued input atom to the forced region, the current hot region,
   or the first enabled region under the event's mouse position. */
// FUNCTION: WIZ8 0x004f1910
unsigned char DispatchRegionInput(const InputAtom* event)
{
    unsigned int region_index = g_captured_region_index;
    unsigned int set_index;
    int sound_id = -1;
    unsigned short x = static_cast<unsigned short>(event->uiParam) +
                       g_cursor_hotspot_x_6596bc;
    unsigned short y = static_cast<unsigned short>(event->uiParam >> 16) +
                       g_cursor_hotspot_y_6596c0;

    if (region_index != 0) {
        goto dispatch;
    }

    region_index = g_current_region_index;
    if (region_index != 0 && RegionContainsPoint(region_index, x, y)) {
        goto dispatch;
    }

    for (set_index = 0; set_index < g_region_set_count; ++set_index) {
        W8RegionSet* set = &g_region_sets[set_index];
        if (set->enabled != 1 || set->first_region > set->last_region) {
            continue;
        }
        for (region_index = set->first_region;
             region_index <= set->last_region; ++region_index) {
            if (RegionContainsPoint(region_index, x, y)) {
                goto dispatch;
            }
        }
    }
    return 0;

dispatch:
    W8Region* region = &g_regions[region_index];
    if (region->help_enabled != 0 &&
        (g_settings_6850c8.tooltips_enabled != 0 ||
         g_region_help_force_enabled != 0) &&
        event->usEvent != MOUSE_POS) {
        if ((region->flags & W8_REGION_HELP_SHOWN) != 0) {
            ReleaseScreenTransitionObjects();
            region->flags &= ~W8_REGION_HELP_SHOWN;
        }
        if (region->help_enabled != 0 &&
            (g_settings_6850c8.tooltips_enabled != 0 ||
             g_region_help_force_enabled != 0)) {
            g_region_help_clock = SetCountdownClock(g_region_help_delay);
        }
    }

    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case RIGHT_BUTTON_DOWN:
        sound_id = 2;
        break;
    case LEFT_BUTTON_UP:
        if ((g_regions[g_current_region_index].flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            sound_id = 3;
        }
        break;
    case RIGHT_BUTTON_UP:
        if ((g_regions[g_current_region_index].flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            sound_id = 3;
        }
        break;
    }

    unsigned char handled = region->callback(
        reinterpret_cast<const W8RegionEvent*>(event), region);
    if (sound_id != -1) {
        PlayButtonSound(sound_id);
    }
    return handled;
}

/* Raises the help box for one region, taking a stale one down first. The
   selected text comes either from the region's indexed notice entry or the
   shared fallback, and the final position is clamped inside the 640x480
   screen before the region records ownership of the box. */
// FUNCTION: WIZ8 0x004f2650
void ShowRegionHelp(unsigned int region_index)
{
    W8Region* region;
    unsigned int mode;
    void* text;
    W8ScreenPoint anchor;
    int width;
    int height;

    if (g_settings_6850c8.tooltips_enabled == 0 && g_region_help_force_enabled == 0) {
        return;
    }
    region = &g_regions[region_index];
    mode = region->flags & W8_REGION_MODE_MASK;
    if (mode != 1 && mode != 2 && (region->flags & W8_REGION_HELP_SHOWN) != 0) {
        ReleaseScreenTransitionObjects();
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
        text = gppStringList[region->help_text_id];
    }
    SetHelpBoxText(text);
    width = g_help_box_width + W8_HELP_MARGIN;
    height = g_help_box_height + W8_HELP_MARGIN;
    GetScreenPoint004284F0(&anchor);
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

/* Force one region to the front of modal dispatch. If another region owned
   that role, send its ordinary mouse-leave callback first and release any
   transition object it still owns. */
// FUNCTION: WIZ8 0x004f2040
void ActivateDialogRegion(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x1d6,
            0);
    }

    g_captured_region_index = region_index;
    g_current_region_index = region_index;
    if (g_hover_region_index == region_index) {
        return;
    }

    if (g_hover_region_index != 0) {
        W8RegionEvent event;
        event.time = GetClock();
        event.modifiers = gfAltState | gfCtrlState | gfShiftState;
        event.reason = MOUSE_POS;

        W8Region* previous = &g_regions[g_hover_region_index];
        previous->flags = (previous->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
        previous->callback(&event, previous);
        unsigned int previous_index = g_hover_region_index;
        if ((g_regions[previous_index].flags & W8_REGION_HELP_SHOWN) != 0) {
            ReleaseScreenTransitionObjects();
            g_regions[previous_index].flags &= ~W8_REGION_HELP_SHOWN;
        }
        g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
        g_regions[g_hover_region_index].flags &= ~W8_REGION_MOUSE_STATE_MASK;
        g_region_help_force_enabled = 0;
        g_hover_region_index = 0;
    }
    g_regions[g_captured_region_index].flags &= ~W8_REGION_MOUSE_STATE_MASK;
}

// FUNCTION: WIZ8 0x004f21b0
unsigned char ClearActiveRegionIfMatches(unsigned int region_index)
{
    if (g_captured_region_index == region_index) {
        g_captured_region_index = 0;
        g_current_region_index = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004f21d0
unsigned int GetForcedRegion(void)
{
    return g_captured_region_index;
}

// FUNCTION: WIZ8 0x004f21e0
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

// FUNCTION: WIZ8 0x004f2220
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

// FUNCTION: WIZ8 0x004f2260
void EnableRegionSetInput(unsigned int region_set_index)
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
            region->flags &= ~W8_REGION_INPUT_MODE_MASK;
            ++region_index;
            ++region;
        } while (region_index <= g_region_sets[region_set_index].last_region);
    }
}

// FUNCTION: WIZ8 0x004f22f0
void DisableRegionSetInput(unsigned int region_set_index)
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
                (g_regions[region_index].flags & ~W8_REGION_INPUT_MODE_MASK) |
                W8_REGION_INPUT_DISABLED;
            ++region_index;
        } while (region_index <= last_region);
    }
}

// FUNCTION: WIZ8 0x004f2380
void EnableRegionInput(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x259,
            0);
    }
    g_regions[region_index].flags &= ~W8_REGION_INPUT_MODE_MASK;
}

// FUNCTION: WIZ8 0x004f23d0
void DisableRegionInput(unsigned int region_index)
{
    if (region_index >= g_region_count) {
        srAssertFail(
            "uiRegionIndex < guiRegionCount",
            "C:\\Projects\\Wizardry 8\\Local Code\\RegionManager.cpp",
            0x262,
            0);
    }
    unsigned int flags = g_regions[region_index].flags;
    flags &= ~W8_REGION_INPUT_MODE_MASK;
    flags |= W8_REGION_INPUT_DISABLED;
    g_regions[region_index].flags = flags;
}

// FUNCTION: WIZ8 0x004f2420
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

// FUNCTION: WIZ8 0x004f2490
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
    switch (region->flags & W8_REGION_MODE_MASK) {
    case W8_REGION_RECTANGLE:
        if (x >= region->x1 && x <= region->x2 &&
            y >= region->y1 && y <= region->y2) {
            return true;
        }
        break;
    case W8_REGION_CIRCLE: {
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

// FUNCTION: WIZ8 0x004f2550
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

// FUNCTION: WIZ8 0x004f25a0
void UpdateRegionHelp(void)
{
    if (g_captured_region_index == 0) {
        if (g_current_region_index != 0 &&
            g_regions[g_current_region_index].help_enabled != 0 &&
            (g_settings_6850c8.tooltips_enabled != 0 || g_region_help_force_enabled != 0) &&
            ClockIsTicking(g_region_help_clock) == 0) {
            ShowRegionHelp(g_current_region_index);
        }
    } else if (g_regions[g_captured_region_index].help_enabled != 0 &&
               (g_settings_6850c8.tooltips_enabled != 0 || g_region_help_force_enabled != 0) &&
               ClockIsTicking(g_region_help_clock) == 0) {
        ShowRegionHelp(g_captured_region_index);
    }
}


// FUNCTION: WIZ8 0x004f2750
void SetRegionHelpText(const wchar_t* text)
{
    if (g_default_help_text != 0) {
        delete[] g_default_help_text;
    }
    if (text != 0) {
        g_default_help_text = new wchar_t[wcslen(text) + 1];
        wcscpy(g_default_help_text, text);
    } else {
        g_default_help_text = 0;
    }
}

// FUNCTION: WIZ8 0x004f27f0
void ResetRegionHelp(unsigned char delayed)
{
    unsigned int region_index = g_current_region_index;

    ReleaseScreenTransitionObjects();
    g_regions[region_index].flags &= 0xfffffdff;
    if (delayed == 0) {
        ShowRegionHelp(g_current_region_index);
    } else if (g_regions[g_current_region_index].help_enabled != 0 &&
               (g_settings_6850c8.tooltips_enabled != 0 || g_region_help_force_enabled != 0)) {
        g_region_help_clock = SetCountdownClock(g_region_help_delay);
    }
}


// FUNCTION: WIZ8 0x004f2880
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

// FUNCTION: WIZ8 0x004f28e0
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

// FUNCTION: WIZ8 0x004f2920
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

// FUNCTION: WIZ8 0x004f29c0
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

// FUNCTION: WIZ8 0x004f2a10
void SetRegionOwner(unsigned int region_index, void* owner)
{
    g_regions[region_index].owner = owner;
}

// FUNCTION: WIZ8 0x004f2a30
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

/* Send the current hot region a mouse-leave transition at the live cursor
   position, then relinquish its help and hover state. */
// FUNCTION: WIZ8 0x004f2a80
void ClearHotRegion004F2A80(void)
{
    W8ScreenPoint mouse;
    W8RegionMouseEvent event;

    GetScreenPoint004284F0(&mouse);
    event.event.time = GetClock();
    event.event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.event.reason = MOUSE_POS;
    event.mouse_position =
        (static_cast<unsigned int>(mouse.y) << 16) |
        (static_cast<unsigned int>(mouse.x) & 0xffff);

    if (g_current_region_index != 0) {
        W8Region* region = &g_regions[g_current_region_index];
        unsigned int mode = region->flags & W8_REGION_MODE_MASK;
        if (mode == 1 || mode == 2) {
            region->flags = (region->flags & 0xff0f) | W8_REGION_MOUSE_LEAVE;
            region->callback(&event.event, region);
            unsigned int region_index = g_current_region_index;
            if ((g_regions[region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
                ReleaseScreenTransitionObjects();
                g_regions[region_index].flags &= ~W8_REGION_HELP_SHOWN;
            }
            g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
            g_region_help_force_enabled = 0;
            g_regions[g_current_region_index].flags &= ~W8_REGION_MOUSE_STATE_MASK;
            g_current_region_index = 0;
        }
    }
}

// FUNCTION: WIZ8 0x004f2bb0
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

// FUNCTION: WIZ8 0x004f2bf0
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

/* Drops every region back to its resting state. The three tracked regions are
   released first - each only if it still carries bit 0x200 - then every region
   set is disabled and every region keeps only its low two flag bits. The three
   trackers are cleared last, and the fourth field is reseeded from the
   settings word rather than zeroed. */
// FUNCTION: WIZ8 0x004f1240
void ResetRegions(void)
{
    unsigned int index;
    W8RegionSet* set;
    W8Region* region;
    unsigned int remaining;

    index = g_current_region_index;
    if (g_current_region_index != 0 && (g_regions[g_current_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        ReleaseScreenTransitionObjects();
        g_regions[index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    index = g_hover_region_index;
    if (g_hover_region_index != 0 && (g_regions[g_hover_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        ReleaseScreenTransitionObjects();
        g_regions[index].flags &= ~W8_REGION_HELP_SHOWN;
    }
    index = g_captured_region_index;
    if (g_captured_region_index != 0 && (g_regions[g_captured_region_index].flags & W8_REGION_HELP_SHOWN) != 0) {
        ReleaseScreenTransitionObjects();
        g_regions[index].flags &= ~W8_REGION_HELP_SHOWN;
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
    g_current_region_index = 0;
    g_hover_region_index = 0;
    g_captured_region_index = 0;
    g_region_help_force_enabled = 0;
    g_region_help_delay = (unsigned short)g_settings_6850c8.tooltip_delay_ms;
}
