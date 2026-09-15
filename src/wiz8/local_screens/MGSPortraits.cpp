#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"

// GLOBAL: WIZ8 0x0069B940
Controls* g_panel_69b940;

// GLOBAL: WIZ8 0x0069B920
W8TextControl* g_portrait_controls_0069b920[8];

// The condition-buttons panel and its eight buttons, created together by
// CreateConditionButtons. The asserts there name them gpConditionButtonsPanel
// and gpConditionButtons[uiSlot].
// GLOBAL: WIZ8 0x0069B900
W8ConditionButton* g_condition_buttons_0069b900[8];
// GLOBAL: WIZ8 0x0069B944
Controls* g_condition_buttons_panel_0069b944;

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
