#include "wiz8/local_code/Controls.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_base.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/screen_state.h"
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
extern W8TextControl005ED604* g_level_up_button_0069c3c0;
extern W8TextControl005ED604* g_dismiss_button_0069c400;
extern int g_rcs_mode_0064cbe8;
extern unsigned char g_in_combat_00683f94;
extern unsigned char g_camp_open_00683f9b;
extern unsigned short g_value_006840be;
extern void* g_value_0069c0f8;
extern void SetPendingScreenState(int state);
extern void DisplayCampDialog(W8DialogBase005DC7A0* dialog);
extern void DismissSelectedPartyCharacter(void);
void ShowDismissCharacterDialog(void);
void OnDismissCharacterDialogClosed(W8DialogBase005DC7A0* dialog);

// FUNCTION: WIZ8 0x005b6630
void OpenLevelUpCharacterScreen(void)
{
    if (!g_party_slot_rows[g_rcs_mode_0064cbe8].occupied) {
        srAssertFail("fCHAR_OCCUPIED(giReviewCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSCommon.cpp",
                     0x888, 0);
    }
    g_screen_state_0068ec78.parameter_2 = g_rcs_mode_0064cbe8;
    g_screen_state_0068ec78.parameter_3 = g_value_0069c0f8;
    g_dword_68ed10.parameter_3 = &g_party_characters[g_rcs_mode_0064cbe8];
    g_dword_68ed10.mode = 2;
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

    g_level_up_button_0069c3c0 = new W8TextControl005ED604(
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
    g_level_up_button_0069c3c0->SetEnabled(0);
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
        if (g_level_up_button_0069c3c0->m_flag_5) {
            g_level_up_button_0069c3c0->SetEnabled(0);
        }
    }
    else if (!g_level_up_button_0069c3c0->m_flag_5) {
        g_level_up_button_0069c3c0->SetEnabled(1);
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

    g_dismiss_button_0069c400 = new W8TextControl005ED604(
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
    g_dismiss_button_0069c400->SetEnabled(0);
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
void OnDismissCharacterDialogClosed(W8DialogBase005DC7A0* base)
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
        if (g_dismiss_button_0069c400->m_flag_5 == 0) {
            g_dismiss_button_0069c400->SetEnabled(1);
            g_dismiss_panel_0069c3c8->Invalidate(0);
        }
    }
    else if (g_dismiss_button_0069c400->m_flag_5 != 0) {
        g_dismiss_button_0069c400->SetEnabled(0);
    }
    g_dismiss_panel_0069c3c8->Redraw();
}
