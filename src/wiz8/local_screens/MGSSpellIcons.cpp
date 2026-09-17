#include "wiz8/local_screens/MGSSpellIcons.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/regions.h"

/* Local Screens\MGSSpellIcons.cpp. The vector destructor emission at
   0x005B1B70 is the last one before the compiler's MGSSpellIcons.cpp to
   RCSCommon.cpp boundary at 0x005B1B90; the matching scalar deleting
   destructors are emitted on the RCSCommon side. The iSpellIcon assertion
   path at 0x005AEC70 names this TU. */

// GLOBAL: WIZ8 0x0069C25C
unsigned int g_spell_icon_count_69c25c;
/* Child spell-icon text controls under g_spell_icon_strip_69c2b0. */
// GLOBAL: WIZ8 0x0069C264
W8TextControl* g_spell_icon_rows_69c264[12];
/* Right-side combat-effect icon rows under g_combat_effect_right_panel_69c2b8. */
// GLOBAL: WIZ8 0x0069C294
W8TextControl* g_combat_effect_right_rows_69c294[6];
// GLOBAL: WIZ8 0x0069C2AC
unsigned int g_combat_effect_right_count_69c2ac;
// GLOBAL: WIZ8 0x0069C2B0
Controls* g_spell_icon_strip_69c2b0;
// GLOBAL: WIZ8 0x0069C2B4
unsigned int g_combat_effect_left_count_69c2b4;
// GLOBAL: WIZ8 0x0069C2B8
Controls* g_combat_effect_right_panel_69c2b8;
// GLOBAL: WIZ8 0x0069C2BC
Controls* g_combat_effect_left_panel_69c2bc;
/* Left-side combat-effect icon rows under g_combat_effect_left_panel_69c2bc. */
// GLOBAL: WIZ8 0x0069C2C0
W8TextControl* g_combat_effect_left_rows_69c2c0[9];

// TEMPLATE: WIZ8 0x005b1b70
// W8GrowableVector<W8CharacterPageEntry*>::~W8GrowableVector<W8CharacterPageEntry*>

// FUNCTION: WIZ8 0x005AE9D0
unsigned char CreateSpellIconHudControls(void)
{
    g_spell_icon_strip_69c2b0 = 0;
    g_combat_effect_left_panel_69c2bc = 0;
    g_combat_effect_right_panel_69c2b8 = 0;

    g_spell_icon_strip_69c2b0 = new Controls(0x19d, 0, 0x280, 0x12, -1, 0, 0);
    if (g_spell_icon_strip_69c2b0 == 0) {
        return 0;
    }

    g_spell_icon_count_69c25c = 0;
    g_combat_effect_left_panel_69c2bc = new Controls(0x81, 0x14, 0x13d, 0x28, -1, 0, 0);
    if (g_combat_effect_left_panel_69c2bc == 0) {
        return 0;
    }

    g_combat_effect_left_count_69c2b4 = 0;
    g_combat_effect_right_panel_69c2b8 = new Controls(0x182, 0x14, 0x1ff, 0x28, -1, 0, 0);
    if (g_combat_effect_right_panel_69c2b8 == 0) {
        return 0;
    }

    g_combat_effect_right_count_69c2ac = 0;
    return 1;
}

// FUNCTION: WIZ8 0x005AEB20
void DestroySpellIconHudControls(void)
{
    unsigned int index;

    if (g_spell_icon_count_69c25c != 0) {
        for (index = 0; index < g_spell_icon_count_69c25c; ++index) {
            g_spell_icon_strip_69c2b0->RemoveControl(g_spell_icon_rows_69c264[index]);
            if (g_spell_icon_rows_69c264[index] != 0) {
                delete g_spell_icon_rows_69c264[index];
            }
        }
    }
    for (index = 0; index < 0xc; ++index) {
        DisableRegionInput(0xd5 - index);
    }
    g_spell_icon_count_69c25c = 0;
    if (g_spell_icon_strip_69c2b0 != 0) {
        delete g_spell_icon_strip_69c2b0;
    }

    DestroyCombatEffectHudRows();
    if (g_combat_effect_left_panel_69c2bc != 0) {
        delete g_combat_effect_left_panel_69c2bc;
    }
    if (g_combat_effect_right_panel_69c2b8 != 0) {
        delete g_combat_effect_right_panel_69c2b8;
    }
}

// FUNCTION: WIZ8 0x005AF210
void DestroyCombatEffectHudRows(void)
{
    unsigned int index;

    if (g_combat_effect_left_count_69c2b4 != 0) {
        for (index = 0; index < g_combat_effect_left_count_69c2b4; ++index) {
            g_combat_effect_left_panel_69c2bc->RemoveControl(
                g_combat_effect_left_rows_69c2c0[index]);
            if (g_combat_effect_left_rows_69c2c0[index] != 0) {
                delete g_combat_effect_left_rows_69c2c0[index];
            }
        }
    }
    for (index = 0; index < 9; ++index) {
        DisableRegionInput(index + 0xd6);
    }
    g_combat_effect_left_count_69c2b4 = 0;

    if (g_combat_effect_right_count_69c2ac != 0) {
        for (index = 0; index < g_combat_effect_right_count_69c2ac; ++index) {
            g_combat_effect_right_panel_69c2b8->RemoveControl(
                g_combat_effect_right_rows_69c294[index]);
            if (g_combat_effect_right_rows_69c294[index] != 0) {
                delete g_combat_effect_right_rows_69c294[index];
            }
        }
    }
    for (index = 0; index < 6; ++index) {
        DisableRegionInput(0xe4 - index);
    }
    g_combat_effect_right_count_69c2ac = 0;
}

/* Clear and invalidate the main-game effect strip while it is up. Moved here
   from Magic Effects.cpp: the TU report proves this hull's range covers
   0x005AF2D0. */
// FUNCTION: WIZ8 0x005af2d0
void InvalidateMainGameEffectHud(void)
{
    if (g_current_screen_state.id == 7) {
        ClearSurfaceRect(0x7f, 0x14, 0x201, 0x28);
        InvalidateRegion(0x7f, 0x14, 0x201, 0x28, 0);
    }
}
