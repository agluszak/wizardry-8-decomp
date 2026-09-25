#include "wiz8/local_screens/MGSSpellIcons.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/regions.h"
#include "wiz8/xstatus.h"
#include "wiz8/engine_code/3dapi.h"

// GLOBAL: WIZ8 0x0069c260
unsigned int g_effect_icon_help_duration_69c260;

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

#define MGSSPELLICONS_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSSpellIcons.cpp"

void RebuildCombatEffectHudRows(void);

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

/* Drop the live spell-icon rows and rebuild them from the current party spell
   slots, then enable and redraw the top strip when any icons remain. */
// FUNCTION: WIZ8 0x005AEBE0
void RefreshSpellIconHudRows(void)
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
    RebuildSpellIconHudRows();
    g_spell_icon_strip_69c2b0->SetEnabled(true);
    if (g_spell_icon_count_69c25c != 0) {
        g_spell_icon_strip_69c2b0->Redraw();
    }
}

/* Walk the twelve party effect slots and create one top-strip text control per
   active effect whose visual table entry is not BAD_INDEX. */
// FUNCTION: WIZ8 0x005AEC70
void RebuildSpellIconHudRows(void)
{
    int left;
    int right;
    int icon;
    int slot;
    W8TextControl* control;
    W8EffectSlot* effect;

    left = 0xd1;
    right = 0xe3;
    g_spell_icon_count_69c25c = 0;
    for (slot = 0; slot < 12; ++slot) {
        effect = &g_status_685170.effect_slots_17af[slot];
        if (effect->active != 0) {
            icon = g_effect_visual_table[effect->effect_id][0];
            if (icon == -1) {
                ReportAssertion("iSpellIcon != BAD_INDEX", MGSSPELLICONS_CPP, 0x108);
            }
            control = new W8TextControl(g_spell_icon_strip_69c2b0, 0xd5 - g_spell_icon_count_69c25c,
                                        left, 0, right, 0x12, icon, 0, 0, -1, -1, -1, -1);
            g_spell_icon_rows_69c264[g_spell_icon_count_69c25c] = control;
            control->Invalidate(0);
            EnableRegionInput(0xd5 - g_spell_icon_count_69c25c);
            g_spell_icon_count_69c25c = g_spell_icon_count_69c25c + 1;
            right = left - 1;
            left = left - 0x13;
        }
    }
}

/* Tear down combat-effect rows, rebuild them while combat is up, then enable
   and redraw the flanking panels. Portrait-refresh pending on slots 0/1 hides
   the matching panel. */
// FUNCTION: WIZ8 0x005AEF30
void RefreshCombatEffectHud(void)
{
    DestroyCombatEffectHudRows();
    if (gXStatus.fCombatMode == 0) {
        return;
    }
    RebuildCombatEffectHudRows();
    g_combat_effect_left_panel_69c2bc->SetEnabled(true);
    g_combat_effect_right_panel_69c2b8->SetEnabled(true);
    if (g_combat_effect_left_count_69c2b4 != 0) {
        g_combat_effect_left_panel_69c2bc->Redraw();
    }
    if (g_combat_effect_right_count_69c2ac != 0) {
        g_combat_effect_right_panel_69c2b8->Redraw();
    }
    if (g_level_block->portrait_refresh_pending[0] != 0) {
        g_combat_effect_left_panel_69c2bc->SetEnabled(false);
    }
    if (g_level_block->portrait_refresh_pending[1] != 0) {
        g_combat_effect_right_panel_69c2b8->SetEnabled(false);
    }
}

/* Rebuild both combat-effect panels: nine left slots walk right from x=0,
   six right slots walk left from x=0x69, one text control per active effect. */
// FUNCTION: WIZ8 0x005AEFC0
void RebuildCombatEffectHudRows(void)
{
    int left;
    int right;
    int icon;
    int slot;
    W8TextControl* control;
    W8EffectSlot* effect;

    g_combat_effect_left_panel_69c2bc->SetBounds(0x81, 0x14, 0x13d, 0x28);
    left = 0;
    right = 0x14;
    g_combat_effect_left_count_69c2b4 = 0;
    for (slot = 0; slot < 9; ++slot) {
        effect = &g_combat_state->effect_slots[slot];
        if (effect->active != 0) {
            icon = g_effect_visual_table[effect->effect_id][0];
            if (icon == -1) {
                ReportAssertion("iSpellIcon != BAD_INDEX", MGSSPELLICONS_CPP, 0x228);
            }
            control = new W8TextControl(g_combat_effect_left_panel_69c2bc,
                                        g_combat_effect_left_count_69c2b4 + 0xd6, left, 0, right,
                                        0x14, icon, 0, 0, -1, -1, -1, -1);
            g_combat_effect_left_rows_69c2c0[g_combat_effect_left_count_69c2b4] = control;
            control->Invalidate(0);
            EnableRegionInput(g_combat_effect_left_count_69c2b4 + 0xd6);
            g_combat_effect_left_count_69c2b4 = g_combat_effect_left_count_69c2b4 + 1;
            left = right + 1;
            right = right + 0x15;
        }
    }
    g_combat_effect_right_panel_69c2b8->SetBounds(0x182, 0x14, 0x1ff, 0x28);
    left = 0x69;
    right = 0x7d;
    g_combat_effect_right_count_69c2ac = 0;
    for (slot = 0; slot < 6; ++slot) {
        effect = &g_combat_state->effect_slots_85a[slot];
        if (effect->active != 0) {
            icon = g_effect_visual_table[effect->effect_id][0];
            if (icon == -1) {
                ReportAssertion("iSpellIcon != BAD_INDEX", MGSSPELLICONS_CPP, 0x25e);
            }
            control = new W8TextControl(g_combat_effect_right_panel_69c2b8,
                                        0xe4 - g_combat_effect_right_count_69c2ac, left, 0, right,
                                        0x14, icon, 0, 0, -1, -1, -1, -1);
            g_combat_effect_right_rows_69c294[g_combat_effect_right_count_69c2ac] = control;
            control->Invalidate(0);
            EnableRegionInput(0xe4 - g_combat_effect_right_count_69c2ac);
            g_combat_effect_right_count_69c2ac = g_combat_effect_right_count_69c2ac + 1;
            right = left - 1;
            left = left - 0x15;
        }
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

// FUNCTION: WIZ8 0x005AED90
void ShowPartyEffectIconHelp(int slot_index)
{
    W8SpellRuntimeRecord* records = g_spell_records;
    W8EffectSlot* slot = &g_status_685170.effect_slots_17af[slot_index];
    int amount = slot->amount;
    unsigned int duration = slot->duration_0d;
    int effect_id = slot->effect_id;
    wchar_t* name;
    wchar_t* detail;
    unsigned int name_len;
    unsigned int detail_len;
    wchar_t* text;

    g_effect_icon_help_duration_69c260 = duration;
    name = FormatWideString(g_format_s_spaced_colon_0064da8c, records[effect_id].display_name);
    name_len = wcslen(name);
    detail =
        FormatWideString(gppStringList[0x1e70 / 4], amount, g_effect_icon_help_duration_69c260);
    detail_len = wcslen(detail);
    text = static_cast<wchar_t*>(operator new((name_len + detail_len) * 2 + 2));
    if (text == 0) {
        srAssertFail("pText", MGSSPELLICONS_CPP, 0x15c, 0);
    }
    wcscpy(text, FormatWideString(g_format_s_colon_00648164, records[effect_id].display_name));
    wcscat(text,
           FormatWideString(gppStringList[0x1e70 / 4], amount, g_effect_icon_help_duration_69c260));
    SetRegionHelpText(text);
    operator delete(text);
}

// FUNCTION: WIZ8 0x005AEEA0
unsigned char PartyEffectIconRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int match = 0;
    int slot_index = 0;
    W8EffectSlot* slot;

    PushButtonSoundScheme(0, 1);
    slot = g_status_685170.effect_slots_17af;
    do {
        if (slot->active != 0) {
            if (match == region->callback_id) {
                break;
            }
            match = match + 1;
        }
        slot = slot + 1;
        slot_index = slot_index + 1;
    } while (slot < &g_status_685170.effect_slots_17af[12]);

    if (slot_index != 12 && event->usEvent == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            ShowPartyEffectIconHelp(slot_index);
            return 1;
        }
        if (g_effect_icon_help_duration_69c260 !=
            g_status_685170.effect_slots_17af[slot_index].duration_0d) {
            ShowPartyEffectIconHelp(slot_index);
            ResetRegionHelp(0);
        }
    }
    return 0;
}

/* Left combat-effect strip (nine slots at g_combat_state->effect_slots). */

// FUNCTION: WIZ8 0x005AF300
void ShowCombatLeftEffectIconHelp(int slot_index)
{
    W8SpellRuntimeRecord* records = g_spell_records;
    W8EffectSlot* slot = &g_combat_state->effect_slots[slot_index];
    int amount = slot->amount;
    unsigned int duration = slot->duration_0d;
    int effect_id = slot->effect_id;
    wchar_t* name;
    wchar_t* detail;
    unsigned int name_len;
    unsigned int detail_len;
    wchar_t* text;

    g_effect_icon_help_duration_69c260 = duration;
    name = FormatWideString(g_format_s_spaced_colon_0064da8c, records[effect_id].display_name);
    name_len = wcslen(name);
    detail = FormatWideString(gppStringList[0x1e74 / 4], amount);
    detail_len = wcslen(detail);
    text = static_cast<wchar_t*>(operator new((name_len + detail_len) * 2 + 2));
    if (text == 0) {
        srAssertFail("pText", MGSSPELLICONS_CPP, 0x2e1, 0);
    }
    wcscpy(text, FormatWideString(g_format_s_colon_00648164, records[effect_id].display_name));
    wcscat(text, FormatWideString(gppStringList[0x1e74 / 4], amount));
    SetRegionHelpText(text);
    operator delete(text);
}

// FUNCTION: WIZ8 0x005AF410
void ShowCombatRightEffectIconHelp(int slot_index)
{
    W8SpellRuntimeRecord* records = g_spell_records;
    W8EffectSlot* slot = &g_combat_state->effect_slots_85a[slot_index];
    int amount = slot->amount;
    unsigned int duration = slot->duration_0d;
    int effect_id = slot->effect_id;
    wchar_t* name;
    wchar_t* detail;
    unsigned int name_len;
    unsigned int detail_len;
    wchar_t* text;

    g_effect_icon_help_duration_69c260 = duration;
    name = FormatWideString(g_format_s_spaced_colon_0064da8c, records[effect_id].display_name);
    name_len = wcslen(name);
    detail =
        FormatWideString(gppStringList[0x1e70 / 4], amount, g_effect_icon_help_duration_69c260);
    detail_len = wcslen(detail);
    text = static_cast<wchar_t*>(operator new((name_len + detail_len) * 2 + 2));
    if (text == 0) {
        srAssertFail("pText", MGSSPELLICONS_CPP, 0x30d, 0);
    }
    wcscpy(text, FormatWideString(g_format_s_colon_00648164, records[effect_id].display_name));
    wcscat(text,
           FormatWideString(gppStringList[0x1e70 / 4], amount, g_effect_icon_help_duration_69c260));
    SetRegionHelpText(text);
    operator delete(text);
}

/* Top-row party effect icons: map callback_id onto the Nth active party
   effect slot and refresh help on enter. */

// FUNCTION: WIZ8 0x005AF530
unsigned char CombatLeftEffectIconRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int match;
    unsigned int slot_index;
    W8EffectSlot* slot;

    if (gXStatus.fCombatMode != 0) {
        PushButtonSoundScheme(0, 1);
        match = 0;
        slot_index = 0;
        slot = g_combat_state->effect_slots;
        do {
            if (slot->active != 0) {
                if (match == region->callback_id) {
                    break;
                }
                match = match + 1;
            }
            slot_index = slot_index + 1;
            slot = slot + 1;
        } while (slot_index < 9);

        if (slot_index != 9 && event->usEvent == MOUSE_POS) {
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                return 1;
            }
            if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                ShowCombatLeftEffectIconHelp(slot_index);
                return 1;
            }
            if (g_effect_icon_help_duration_69c260 !=
                g_combat_state->effect_slots[slot_index].duration_0d) {
                ShowCombatLeftEffectIconHelp(slot_index);
                ResetRegionHelp(0);
            }
        }
    }
    return 0;
}

/* Right combat-effect strip (six slots at g_combat_state->effect_slots_85a). */

// FUNCTION: WIZ8 0x005AF5E0
unsigned char CombatRightEffectIconRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned int match;
    unsigned int slot_index;
    W8EffectSlot* slot;

    if (gXStatus.fCombatMode != 0) {
        PushButtonSoundScheme(0, 1);
        match = 0;
        slot_index = 0;
        slot = g_combat_state->effect_slots_85a;
        do {
            if (slot->active != 0) {
                if (match == region->callback_id) {
                    break;
                }
                match = match + 1;
            }
            slot_index = slot_index + 1;
            slot = slot + 1;
        } while (slot_index < 6);

        if (slot_index != 6 && event->usEvent == MOUSE_POS) {
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                return 1;
            }
            if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                ShowCombatRightEffectIconHelp(slot_index);
                return 1;
            }
            if (g_effect_icon_help_duration_69c260 !=
                g_combat_state->effect_slots_85a[slot_index].duration_0d) {
                ShowCombatRightEffectIconHelp(slot_index);
                ResetRegionHelp(0);
            }
        }
    }
    return 0;
}
