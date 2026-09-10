#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/ModalDialogBase.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/cursor.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"

/*
 * Local Screens\RCSCommon.cpp.
 *
 * Two panels the review-character screens share. Each owns a Controls object
 * and one widget inside it, and each has the same pair of bodies: one that
 * redraws the panel and one that tears both down.
 */

extern Controls* g_level_up_panel_0069c3c4;
extern Controls* g_dismiss_panel_0069c3c8;
extern W8TextControl* g_level_up_button_0069c3c0;
extern W8TextControl* g_dismiss_button_0069c400;
// GLOBAL: WIZ8 0x0069c3c4
Controls* g_level_up_panel_0069c3c4;
// GLOBAL: WIZ8 0x0069c3c8
Controls* g_dismiss_panel_0069c3c8;
// GLOBAL: WIZ8 0x0069c3c0
W8TextControl* g_level_up_button_0069c3c0;
// GLOBAL: WIZ8 0x0069c400
W8TextControl* g_dismiss_button_0069c400;

extern unsigned char g_camp_open_00683f9b;

extern void DisplayCampDialog(W8DialogBase* dialog);
extern void DismissSelectedPartyCharacter(void);
void ShowDismissCharacterDialog(void);
void OnDismissCharacterDialogClosed(W8DialogBase* dialog);

// FUNCTION: WIZ8 0x005b6d20
bool CanSelectRcsPartySlot(int ui_slot)
{
    if (!g_party_slot_rows[ui_slot].occupied) {
        srAssertFail("gStatus.XChar[uiSlot].fOccupied",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x9ac, 0);
    }

    W8Character* character = &g_party_characters[ui_slot];
    if (character->condition_turns[19] != 0) {
        return false;
    }
    if (character->condition_turns[14] != 0) {
        return false;
    }
    if (character->condition_turns[11] != 0) {
        return false;
    }
    if (character->condition_turns[13] != 0) {
        return false;
    }
    if (g_in_combat_00683f94) {
        if (!g_combat_state->flag_a50) {
            return false;
        }
        if (g_party_slot_rows[ui_slot].pending_action != 9) {
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005b6df0
void DrawRcsText(const wchar_t* text, int left, int top, int width,
                 unsigned int layout_mode)
{
    W8ControlsRect bounds = {left, top, left + width, top + 12};
    W8TextBuffer buffer(&bounds, text, g_font_683660, layout_mode, 4);
    buffer.RenderToTarget(0, 0, -14);
}

// FUNCTION: WIZ8 0x005b6e90
void DrawRcsBoldText(const wchar_t* text, int left, int top, int width,
                     unsigned int layout_mode)
{
    W8ControlsRect bounds = {left, top, left + width, top + 12};
    W8TextBuffer buffer(
        &bounds, text, g_wiz_text_bold_font_683664, layout_mode, 4);
    buffer.RenderToTarget(0, 0, -14);
}

// FUNCTION: WIZ8 0x005b6f30
void DrawTallRcsText(const wchar_t* text, int left, int top, int width,
                     unsigned int layout_mode)
{
    W8ControlsRect bounds = {left, top, left + width, top + 18};
    W8TextBuffer buffer(&bounds, text, g_font_683660, layout_mode, 4);
    buffer.RenderToTarget(0, 0, -14);
}

// FUNCTION: WIZ8 0x005b6630
void OpenLevelUpCharacterScreen(void)
{
    if (!g_party_slot_rows[g_rcs_mode_0064cbe8].occupied) {
        srAssertFail("fCHAR_OCCUPIED(giReviewCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x888, 0);
    }
    g_current_screen_state.parameter_2 = g_rcs_mode_0064cbe8;
    g_current_screen_state.parameter_3 = g_value_0069c0f8;
    g_pending_screen_state.parameter_3 = &g_party_characters[g_rcs_mode_0064cbe8];
    g_pending_screen_state.mode = 2;
    SetPendingScreenState(W8_SCREEN_CHARACTER);
}

// FUNCTION: WIZ8 0x005b6400
void CreateRcsLevelUpPanel(void)
{
    g_level_up_panel_0069c3c4 = 0;
    g_level_up_button_0069c3c0 = 0;

    g_level_up_panel_0069c3c4 =
        new Controls(0xe9, 0x3f, 0xfb, 0x51, -1, 0, -1);
    if (g_level_up_panel_0069c3c4 == 0) {
        srAssertFail("gpLevelUpPanel",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x84e, 0);
    }

    g_level_up_button_0069c3c0 = new W8TextControl(
        g_level_up_panel_0069c3c4, 0xe7, 0, 0, 0x12, 0x12,
        0xa7, 0, 0, 2, 1, 4, 3);
    if (g_level_up_button_0069c3c0 == 0) {
        srAssertFail("gpLevelUpButton",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x852, 0);
    }
    g_level_up_button_0069c3c0->m_primaryActivationCallback =
        OpenLevelUpCharacterScreen;
    g_level_up_panel_0069c3c4->SetEnabled(1);
    g_level_up_button_0069c3c0->SetActive(0);
}
/* Ask the first panel to redraw all of itself. A null rectangle is how
   Controls::Invalidate spells "the whole area", so these are not a separate
   one-argument redraw slot - they are the panel class Local Code\Controls.cpp
   models, reached through its second vtable slot. */
// FUNCTION: WIZ8 0x005b6590
void RedrawRcsLevelUpPanel(void)
{
    g_level_up_panel_0069c3c4->Invalidate(0);
}

/* Redraw the second. */
// FUNCTION: WIZ8 0x005b68d0
void RedrawRcsDismissPanel(void)
{
    g_dismiss_panel_0069c3c8->Invalidate(0);
}

/* Tear the first panel down, then destroy its separate widget. */
// FUNCTION: WIZ8 0x005b6540
void DestroyRcsLevelUpPanel(void)
{
    Controls* panel = g_level_up_panel_0069c3c4;

    if (panel != 0) {
        delete panel;
        g_level_up_panel_0069c3c4 = 0;
    }
    if (g_level_up_button_0069c3c0 != 0) {
        delete g_level_up_button_0069c3c0;
        g_level_up_button_0069c3c0 = 0;
    }
}

// FUNCTION: WIZ8 0x005b65a0
void UpdateRcsLevelUpPanel(void)
{
    bool enabled = IsCharacterReadyToAdvance(g_rcs_mode_0064cbe8);
    if (!enabled || g_in_combat_00683f94 ||
        (!g_party_slot_rows[g_rcs_mode_0064cbe8].flag_105 &&
         g_status_685170.game_started) ||
        g_camp_open_00683f9b) {
        if (g_level_up_button_0069c3c0->m_active) {
            g_level_up_button_0069c3c0->SetActive(0);
        }
    }
    else if (!g_level_up_button_0069c3c0->m_active) {
        g_level_up_button_0069c3c0->SetActive(1);
        g_level_up_panel_0069c3c4->Invalidate(0);
    }
    g_level_up_panel_0069c3c4->Redraw();
}

// FUNCTION: WIZ8 0x005b6740
void CreateRcsDismissPanel(void)
{
    g_dismiss_panel_0069c3c8 = 0;
    g_dismiss_button_0069c400 = 0;

    g_dismiss_panel_0069c3c8 =
        new Controls(0xa7, 0x41, 0xbe, 0x51, -1, 0, -1);
    if (g_dismiss_panel_0069c3c8 == 0) {
        srAssertFail("gpDismissPanel",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x8cf, 0);
    }

    g_dismiss_button_0069c400 = new W8TextControl(
        g_dismiss_panel_0069c3c8, 0xe8, 0, 0, 0x17, 0x10,
        0x113, 0, 0, 2, 1, 2, 3);
    if (g_dismiss_button_0069c400 == 0) {
        srAssertFail("gpDismissButton",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x8d3, 0);
    }
    g_dismiss_button_0069c400->m_primaryActivationCallback =
        ShowDismissCharacterDialog;
    g_dismiss_panel_0069c3c8->SetEnabled(1);
    g_dismiss_button_0069c400->SetActive(0);
}

// FUNCTION: WIZ8 0x005b6950
void ShowDismissCharacterDialog(void)
{
    if (!g_party_slot_rows[g_rcs_mode_0064cbe8].occupied) {
        srAssertFail("fCHAR_OCCUPIED(giReviewCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x90a, 0);
    }

    W8ModalDialogBase* dialog =
        static_cast<W8ModalDialogBase*>(CreateDialogByKind(1));
    dialog->SetClientExtent(0xfa, 200);

    W8Character* character = &g_party_characters[g_rcs_mode_0064cbe8];
    const wchar_t* format;
    if (character->condition_turns[19] == 0) {
        if (character->condition_turns[W8_CONDITION_EQUIPMENT_UNLOCKED] == 0) {
            format = gppStringList[0x92d];
        }
        else {
            format = gppStringList[0x92e];
        }
    }
    else {
        format = gppStringList[0x92f];
    }
    dialog->SetMessage(
        FormatWideString(format, character->name), 1, 0x32, 1, 1, 1, 1, 0,
        0x15e);
    SetDialogDestroyCallback(dialog, OnDismissCharacterDialogClosed);
    DisplayCampDialog(dialog);
}

// FUNCTION: WIZ8 0x005b6a60
void OnDismissCharacterDialogClosed(W8DialogBase* base)
{
    if (GetDialogResult(base) &&
        g_party_slot_rows[g_rcs_mode_0064cbe8].animation_0fa != -1) {
        g_value_006840be = static_cast<unsigned short>(g_rcs_mode_0064cbe8);
        DismissSelectedPartyCharacter();
    }
}

/* The same for the second panel and its widget. */
// FUNCTION: WIZ8 0x005b6880
void DestroyRcsDismissPanel(void)
{
    Controls* panel = g_dismiss_panel_0069c3c8;

    if (panel != 0) {
        delete panel;
        g_dismiss_panel_0069c3c8 = 0;
    }
    if (g_dismiss_button_0069c400 != 0) {
        delete g_dismiss_button_0069c400;
        g_dismiss_button_0069c400 = 0;
    }
}

/* Bring the second panel up to date. Its widget is available only in the two
   leading modes, out of combat and out of camp; enabling it also invalidates the
   panel, disabling it does not. Either way the panel is then updated. */
// FUNCTION: WIZ8 0x005b68e0
void UpdateRcsDismissPanel(void)
{
    if ((g_rcs_mode_0064cbe8 == 0 || g_rcs_mode_0064cbe8 == 1) &&
        gXStatus.fCombatMode == 0 && gXStatus.fCampMode == 0) {
        if (g_dismiss_button_0069c400->m_active == 0) {
            g_dismiss_button_0069c400->SetActive(1);
            g_dismiss_panel_0069c3c8->Invalidate(0);
        }
    }
    else if (g_dismiss_button_0069c400->m_active != 0) {
        g_dismiss_button_0069c400->SetActive(0);
    }
    g_dismiss_panel_0069c3c8->Redraw();
}

/* The camp-screen panel owners: one Controls object and the two fifteen-entry
   text-control rows and three-control row the teardown walks. */

// GLOBAL: WIZ8 0x0069c2ec
Controls* g_panel_69c2ec;
// GLOBAL: WIZ8 0x0069c344
W8TextControl* g_panel_controls_69c344[15];
// GLOBAL: WIZ8 0x0069c384
W8TextControl* g_panel_controls_69c384[15];
// GLOBAL: WIZ8 0x0069c2f8
W8TextControl* g_panel_controls_69c2f8[3];

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

// FUNCTION: WIZ8 0x005B2580
void Function5B2580(void)
{
    Controls* panel = g_panel_69c2ec;
    if (panel != 0) {
        delete panel;
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
    InvalidateRegion(0xd6, 0x3c, 0x1ab, 0x12f, 0);
    Function56AAB0();
}

// SYNTHETIC: WIZ8 0x005b1b90
// W8GrowableVector<W8CharacterPageEntry*>::`scalar deleting destructor'
