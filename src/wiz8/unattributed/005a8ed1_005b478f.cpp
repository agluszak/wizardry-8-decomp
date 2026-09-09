#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/cursor.h"
#include "wiz8/regions.h"
#include "wiz8/sgp_video.h"
#include "wiz8/dirty_tiles.h"

#include <wchar.h>

extern "C" {
extern int g_value_69c1cc;
// GLOBAL: WIZ8 0x0064d8ac
unsigned long g_value_64d8ac = 6;
// GLOBAL: WIZ8 0x0069c1cc
int g_value_69c1cc;
extern unsigned long g_value_64d8ac;
}

/* Address quarantine 005a8ed1-005b478f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// The review-screen panel sets torn down on leave: each set is a panel with
// its button controls beside it. Their creators are not yet recovered, so the
// members keep neutral names.
// GLOBAL: WIZ8 0x0069c428
Controls* g_panel_69c428;
// GLOBAL: WIZ8 0x0069c43c
W8TextControl* g_panel_controls_69c43c[2];
// GLOBAL: WIZ8 0x0069c464
Controls* g_panel_69c464;
// GLOBAL: WIZ8 0x0069c468
W8TextControl* g_panel_controls_69c468[2];
// GLOBAL: WIZ8 0x0069c470
W8TextControl* g_panel_controls_69c470[7];
// GLOBAL: WIZ8 0x0069c48c
Controls* g_panel_69c48c;
// GLOBAL: WIZ8 0x0069c2ec
Controls* g_panel_69c2ec;
// GLOBAL: WIZ8 0x0069c344
W8TextControl* g_panel_controls_69c344[15];
// GLOBAL: WIZ8 0x0069c384
W8TextControl* g_panel_controls_69c384[15];
// GLOBAL: WIZ8 0x0069c2f8
W8TextControl* g_panel_controls_69c2f8[3];

extern void Function4257F0(int value);

/* Remember the current save name, truncating into the shared wide buffer
   with its last byte forced narrow. */
// FUNCTION: WIZ8 0x005A9E70
void Function5A9E70(void* target)
{
    wcsncpy(
        reinterpret_cast<wchar_t*>(&g_value_69c1cc),
        static_cast<const wchar_t*>(target),
        0x40);
    reinterpret_cast<char*>(&g_value_69c1cc)[0x7e] = 0;
}

// FUNCTION: WIZ8 0x005A9E90
int* GetAddress69C1CC(void)
{
    return &g_value_69c1cc;
}
// FUNCTION: WIZ8 0x005AE9C0
void SetValue64D8AC(unsigned long value)
{
    g_value_64d8ac = value;
}

/* Release the three level-runtime dialogue owners through the shared
   teardown, then clear the slots. */
// FUNCTION: WIZ8 0x005B1C00
void Function5B1C00(void)
{
    if (g_level_block->unknown_2a0 != 0) {
        Function4257F0(g_level_block->unknown_2a0);
        g_level_block->unknown_2a0 = 0;
    }
    if (g_level_block->unknown_2a4 != 0) {
        Function4257F0(g_level_block->unknown_2a4);
        g_level_block->unknown_2a4 = 0;
    }
    if (g_level_block->unknown_2a8 != 0) {
        Function4257F0(g_level_block->unknown_2a8);
        g_level_block->unknown_2a8 = 0;
    }
}

// FUNCTION: WIZ8 0x005B9220
void Function5B9220(void)
{
    Controls* panel = g_panel_69c464;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69c464 = 0;
    }
    W8TextControl** control = g_panel_controls_69c468;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c468 + 2);
}

// FUNCTION: WIZ8 0x005B9760
void Function5B9760(void)
{
    Controls* panel = g_panel_69c48c;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69c48c = 0;
    }
    W8TextControl** control = g_panel_controls_69c470;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c470 + 7);
}

// FUNCTION: WIZ8 0x005B9EA0
void Function5B9EA0(void)
{
    Controls* panel = g_panel_69c428;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69c428 = 0;
    }
    W8TextControl** control = g_panel_controls_69c43c;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c43c + 2);
}

// FUNCTION: WIZ8 0x005B2580
void Function5B2580(void)
{
    Controls* panel = g_panel_69c2ec;
    if (panel != 0) {
        panel->~Controls();
        ::operator delete(panel);
        g_panel_69c2ec = 0;
    }
    for (int index = 0; index < 15; ++index) {
        if (g_panel_controls_69c344[index] != 0) {
            delete g_panel_controls_69c344[index];
            g_panel_controls_69c344[index] = 0;
        }
        if (g_panel_controls_69c384[index] != 0) {
            delete g_panel_controls_69c384[index];
            g_panel_controls_69c384[index] = 0;
        }
    }
    W8TextControl** control = g_panel_controls_69c2f8;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_panel_controls_69c2f8 + 3);
}

// FUNCTION: WIZ8 0x005B2200
void Function5B2200(void)
{
    Function5B2580();
    g_flag_00683f9a = 0;
    UpdateHeldItemCursor();
    RegionSetDisable(0x1b);
    RequestRedraw(0x200);
    ClearSurfaceRect(0xd6, 0x3c, 0x1ab, 0x12f);
    MarkScreenRectDirty(0xd6, 0x3c, 0x1ab, 0x12f, 0);
    Function56AAB0();
}
