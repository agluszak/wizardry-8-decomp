#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "input.h"

// GLOBAL: WIZ8 0x0069B940
Controls* g_panel_69b940; /* gpLevelButtonsPanel */

// GLOBAL: WIZ8 0x0069B920
W8TextControl* g_portrait_controls_0069b920[8]; /* gpLevelButtons[uiSlot] */

// The condition-buttons panel and its eight buttons, created together by
// CreateConditionButtons. The asserts there name them gpConditionButtonsPanel
// and gpConditionButtons[uiSlot].
// GLOBAL: WIZ8 0x0069B900
W8ConditionButton* g_condition_buttons_0069b900[8];
// GLOBAL: WIZ8 0x0069B944
Controls* g_condition_buttons_panel_0069b944;
/* Party slot the level-up portrait button opens; -1 while idle. */
// GLOBAL: WIZ8 0x0069B948
int giLevelUpChar;

void OnLevelButtonActivate(void);

/* Toggle numeric hit-point display on the party portraits and invalidate all
   eight slot masks so the new mode repaints everywhere. */
// FUNCTION: WIZ8 0x0059AA30
void ToggleNumericHitPoints(void)
{
    g_settings_6850c8.numeric_hit_points = g_settings_6850c8.numeric_hit_points == 0;
    for (unsigned int slot = 0; slot < 8; slot++) {
        RequestRedraw(1u << slot);
    }
}

/* The slot's anchor positions for the keyboard menu and portrait band: the
   panel corner, the band's two x edges (their order swaps with the column),
   the grid row and the column pixel. 'adjust' applies the compact-display
   shift used when the party display is a single column. */
// FUNCTION: WIZ8 0x0059AA60
void GetPartySlotMenuAnchor(int party_slot, int* menu_x, int* menu_y, int* band_menu_edge,
                            int* band_portrait_edge, int* grid_row, int* column_x, int adjust)
{
    switch (party_slot) {
    case 0:
        *grid_row = 1;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0x12;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 1:
        *grid_row = 2;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0x12;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    case 2:
        *grid_row = 5;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0x67;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 3:
        *grid_row = 6;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0x67;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    case 4:
        *grid_row = 9;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0xbc;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 5:
        *grid_row = 10;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0xbc;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    case 6:
        *grid_row = 0xd;
        *column_x = 0;
        *menu_x = 0;
        *menu_y = 0x111;
        *band_menu_edge = *menu_x + 2;
        *band_portrait_edge = *menu_x + 0x69;
        break;
    case 7:
        *grid_row = 0xe;
        *column_x = 0x200;
        *menu_x = 0x200;
        *menu_y = 0x111;
        *band_menu_edge = *menu_x + 0x6b;
        *band_portrait_edge = *menu_x;
        break;
    default:
        break;
    }
    if (adjust != 0 && g_settings_6850c8.main_ui_mode != W8_MAIN_UI_MODE_PORTRAITS &&
        static_cast<unsigned int>(g_settings_6850c8.main_ui_mode) <=
            static_cast<unsigned int>(W8_MAIN_UI_MODE_RADAR)) {
        if ((party_slot & 1) == 0) {
            *menu_x -= 0x69;
            *band_menu_edge = -1;
            *band_portrait_edge -= 0x69;
            --*grid_row;
            *column_x = 0;
        } else {
            *menu_x = 0x269;
            *band_menu_edge = -1;
            *band_portrait_edge = 0x269;
            ++*grid_row;
            *column_x = 0x269;
        }
    }
}

// FUNCTION: WIZ8 0x0059BAD0
void ReleasePortraitControls(void)
{
    RegionSetDisable(5);
    W8TextControl** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(0);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
    Controls* panel = g_panel_69b940;
    if (panel != 0) {
        delete panel;
        g_panel_69b940 = 0;
    }
    control = g_portrait_controls_0069b920;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

// FUNCTION: WIZ8 0x0059BF70
void ReleaseConditionButtons(void)
{
    Controls* panel = g_condition_buttons_panel_0069b944;
    if (panel != 0) {
        delete panel;
        g_condition_buttons_panel_0069b944 = 0;
    }
    W8ConditionButton** control = g_condition_buttons_0069b900;
    do {
        if (*control != 0) {
            delete *control;
            *control = 0;
        }
        ++control;
    } while (control < g_condition_buttons_0069b900 + 8);
}

// FUNCTION: WIZ8 0x0059BB40
void DisablePortraitControls0059BB40(void)
{
    RegionSetDisable(5);
    W8TextControl** control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(0);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

/* Hide the condition-button region set and clear any open condition highlight. */
// FUNCTION: WIZ8 0x0059C030
void DisableConditionButtons0059C030(void)
{
    RegionSetDisable(6);
    g_condition_buttons_panel_0069b944->SetEnabled(false);
    if (g_level_block->condition_highlight_party_slot != -1) {
        g_level_block->condition_highlight_party_slot = -1;
        DismissHighlightOverlay();
        RequestRedraw(0x8000);
        RequestRedraw(0xff);
    }
}

/* Show the condition-button region set for a non-normal layout. */
// FUNCTION: WIZ8 0x0059BFC0
void EnableConditionButtons0059BFC0(void)
{
    W8ConditionButton** control;

    if (g_settings_6850c8.main_ui_mode == W8_MAIN_UI_MODE_PORTRAITS) {
        srAssertFail("gConfig.uiCurrentLayout != LAYOUT_NORMAL",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0xa0c, 0);
    }
    RegionSetEnable(6);
    g_condition_buttons_panel_0069b944->SetEnabled(true);
    control = g_condition_buttons_0069b900;
    do {
        (*control)->SetEnabled(true);
        ++control;
    } while (control < g_condition_buttons_0069b900 + 8);
    g_condition_buttons_panel_0069b944->Invalidate(0);
}

// FUNCTION: WIZ8 0x0059BB70
void EnablePortraitAdvanceRegions0059BB70(void)
{
    RegionSetEnable(5);
    unsigned int state_offset = 0;
    int party_slot = 0;
    do {
        if (!IsCharacterReadyToAdvance(party_slot) ||
            g_status_685170.buffers.party_rows[party_slot].flag_103 == 0) {
            DisableRegionInput(party_slot + 0x12);
        } else {
            EnableRegionInput(party_slot + 0x12);
        }
        state_offset += 0x106;
        ++party_slot;
    } while (state_offset < 0x830);
}

// FUNCTION: WIZ8 0x0059BBD0
void InvalidatePortraitControl0059BBD0(unsigned int party_slot)
{
    if (party_slot < 8 && g_status_685170.buffers.party_rows[party_slot].occupied) {
        g_portrait_controls_0069b920[party_slot]->Invalidate(0);
    }
}

// FUNCTION: WIZ8 0x0059BC00
void RedrawPanel69B940(void)
{
    g_panel_69b940->Invalidate(0);
}

/* Open the character screen for giLevelUpChar when that portrait button fires. */
// FUNCTION: WIZ8 0x0059BCA0
void OnLevelButtonActivate(void)
{
    int slot = giLevelUpChar;

    if (slot == -1 || slot >= 8) {
        return;
    }
    if (g_status_685170.buffers.party_rows[slot].occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(giLevelUpChar)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x988, 0);
    }
    g_pending_screen_state.parameter_3 = &g_status_685170.buffers.characters[slot];
    g_pending_screen_state.mode = 2;
    SetPendingScreenState(W8_SCREEN_CHARACTER);
}

/* The eight level-up portrait buttons sit in two columns on gpLevelButtonsPanel,
   one row per party pair. CreateConditionButtons mirrors this layout with a
   different region base and icon set. */
// FUNCTION: WIZ8 0x0059B940
void CreateLevelButtons(void)
{
    unsigned int uiSlot;
    unsigned int column_x;
    int row_y;
    int count;
    W8TextControl** control;

    g_panel_69b940 = 0;
    control = g_portrait_controls_0069b920;
    for (count = 8; count != 0; --count) {
        *control = 0;
        ++control;
    }

    g_panel_69b940 = new Controls(0, 0, 0x280, 0x1e0, -1, 0, -1);
    if (g_panel_69b940 == 0) {
        srAssertFail("gpLevelButtonsPanel",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x8e0, 0);
    }

    uiSlot = 0;
    control = g_portrait_controls_0069b920;
    do {
        column_x = (uiSlot & 1) != 0 ? 0x23b : 0;
        row_y = (uiSlot >> 1) * 0x55;
        W8TextControl* button =
            new W8TextControl(g_panel_69b940, uiSlot + 0x12, column_x + 0x19, row_y + 0x46,
                              column_x + 0x2b, row_y + 0x58, 0xa7, 0, 0, 2, 1, 4, 3);
        *control = button;
        button->m_primaryActivationCallback = OnLevelButtonActivate;
        if (*control == 0) {
            srAssertFail("gpLevelButtons[uiSlot]",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x8f7, 0);
        }
        ++control;
        ++uiSlot;
    } while (control < g_portrait_controls_0069b920 + 8);

    giLevelUpChar = -1;
    g_panel_69b940->SetEnabled(true);
    control = g_portrait_controls_0069b920;
    do {
        (*control)->SetActive(false);
        ++control;
    } while (control < g_portrait_controls_0069b920 + 8);
}

// SYNTHETIC: WIZ8 0x005991A0
// W8ConditionButton::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005991C0
W8ConditionButton::~W8ConditionButton() {}

/* The base text control draws the frame; the derived pass overlays the slot's
   condition icon when the widget is active and either the redraw is full or
   the widget was already dirty. The icon is m_image_object_bc plus the
   per-condition offset the selector byte chooses. */
// FUNCTION: WIZ8 0x00599210
void W8ConditionButton::Redraw(int full_redraw)
{
    bool dirty = m_dirty;
    int left;
    int top;

    W8TextControl::Redraw(full_redraw);
    if (!m_active) {
        return;
    }
    if (m_pPanel == 0) {
        return;
    }
    if (full_redraw == 0 && !dirty) {
        return;
    }
    GetTextOrigin(&left, &top);
    left += 2;
    top += 2;
    if (m_condition_b8 == 0) {
        DrawCatalogImageAndInvalidate(-14, m_image_object_bc + 0xb6, 0, 0, left, top, 2, 0);
    } else if (m_condition_b8 == 1) {
        DrawCatalogImageAndInvalidate(-14, m_image_object_bc + 0xc9, 0, 0, left, top, 2, 0);
    }
}

/* Leaving the button drops the hovered-slot tracking; if a hover was showing,
   its highlight overlay comes down with it. */
// FUNCTION: WIZ8 0x005992E0
void W8ConditionButton::OnMouseLeave(int event)
{
    W8TextControl::OnMouseLeave(event);
    if (g_level_block->condition_highlight_party_slot != -1) {
        g_level_block->condition_highlight_party_slot = -1;
        DismissHighlightOverlay();
        RequestRedraw(0x8000);
        RequestRedraw(0xff);
    }
}

// FUNCTION: WIZ8 0x00599330
void W8ConditionButton::OnLeftButtonDown(int event)
{
    W8TextControl::OnLeftButtonDown(event);
    g_level_block->condition_highlight_party_slot = m_ui_slot_c0;
    RequestRedraw(0x8000);
}

// FUNCTION: WIZ8 0x00599360
void W8ConditionButton::OnLeftButtonUp(int event)
{
    W8TextControl::OnLeftButtonUp(event);
    g_level_block->condition_highlight_party_slot = -1;
    DismissHighlightOverlay();
    RequestRedraw(0x8000);
    RequestRedraw(0xff);
}

/* The eight buttons sit in two columns inside their own panel, one row per
   party pair. Each tracks one party slot through the region set and the
   recorded ui_slot. */
// FUNCTION: WIZ8 0x0059BDB0
void CreateConditionButtons(void)
{
    unsigned int uiSlot;
    unsigned int column_x;
    int row_y;
    int count;
    W8ConditionButton** control;

    g_condition_buttons_panel_0069b944 = 0;
    control = g_condition_buttons_0069b900;
    for (count = 8; count != 0; --count) {
        *control = 0;
        ++control;
    }

    g_condition_buttons_panel_0069b944 = new Controls(0, 0, 0x280, 0x1e0, -1, 0, -1);
    if (g_condition_buttons_panel_0069b944 == 0) {
        srAssertFail("gpConditionButtonsPanel",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x9db, 0);
    }

    uiSlot = 0;
    control = g_condition_buttons_0069b900;
    do {
        column_x = (uiSlot & 1) != 0 ? 0x23f : 0;
        row_y = (uiSlot >> 1) * 0x55;
        W8ConditionButton* button = new W8ConditionButton(
            g_condition_buttons_panel_0069b944, uiSlot + 0x1a, column_x + 0x17, row_y + 0x13,
            column_x + 0x2a, row_y + 0x26, 0xa8, 0, 0, 0, 1, 1, -1, uiSlot);
        *control = button;
        if (button == 0) {
            srAssertFail("gpConditionButtons[uiSlot]",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\MGSPortraits.cpp", 0x9ec, 0);
        }
        ++control;
        ++uiSlot;
    } while (control < g_condition_buttons_0069b900 + 8);

    RegionSetDisable(6);
    g_condition_buttons_panel_0069b944->SetEnabled(false);
    if (g_level_block->condition_highlight_party_slot != -1) {
        g_level_block->condition_highlight_party_slot = -1;
        DismissHighlightOverlay();
        RequestRedraw(0x8000);
        RequestRedraw(0xff);
    }
}

/* Forward left-button and hover events to the portrait level-up control for
   the region's callback_id slot; release records giLevelUpChar. */
// FUNCTION: WIZ8 0x0059BD20
unsigned char PortraitControlRegionEvent(const InputAtom* event, W8Region* region)
{
    W8TextControl* control = g_portrait_controls_0069b920[region->callback_id];
    if (control == 0) {
        return 0;
    }
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
        control->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            giLevelUpChar = region->callback_id;
            control->OnLeftButtonUp(0);
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            giLevelUpChar = -1;
            control->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            control->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* Forward left-button and hover events to the condition button for the
   region's callback_id slot. */
// FUNCTION: WIZ8 0x0059C260
unsigned char ConditionButtonRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned short us_event = event->usEvent;
    unsigned short slot = region->callback_id;
    if (us_event == LEFT_BUTTON_DOWN) {
        g_condition_buttons_0069b900[slot]->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
    } else {
        if (us_event != LEFT_BUTTON_UP) {
            if (us_event == MOUSE_POS) {
                if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                    g_condition_buttons_0069b900[slot]->OnMouseLeave(0);
                    return 1;
                }
                if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                    g_condition_buttons_0069b900[slot]->OnMouseEnter(0);
                    return 1;
                }
            }
            return 0;
        }
        g_condition_buttons_0069b900[slot]->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
    }
    return 1;
}
