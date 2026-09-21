#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/level_specific_code/Trynnie2.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/xstatus.h"

#define MGSUSEITEMSELECT_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSUseItemSelect.cpp"

// GLOBAL: WIZ8 0x0069b998
Controls* g_panel_69b998;
// GLOBAL: WIZ8 0x0069b988
W8MainUiMode g_value_69b988;

// GLOBAL: WIZ8 0x0069B950
W8TextControl* g_use_item_select_scroll_buttons[3];
// GLOBAL: WIZ8 0x0069B95C
int g_selected_use_item_line_0069b95c;
// GLOBAL: WIZ8 0x0069B960
W8TextControl* g_use_item_select_controls[9];
// GLOBAL: WIZ8 0x0069B9A0
W8ItemInstance* g_value_69b9a0;
// GLOBAL: WIZ8 0x0069B9A4
W8ItemInstance* g_value_69b9a4;
// GLOBAL: WIZ8 0x0069B990
int g_use_item_list_count_0069b990;
// GLOBAL: WIZ8 0x0069B9A8
int g_use_item_cursor_x_0069b9a8;
// GLOBAL: WIZ8 0x0069B9AC
int g_use_item_cursor_y_0069b9ac;
// GLOBAL: WIZ8 0x0069B9B0
int g_use_item_owner_index_0069b9b0;
// GLOBAL: WIZ8 0x0069B9B4
W8ItemInstance* g_use_item_list_0069b9b4[0x15e];
// GLOBAL: WIZ8 0x0069BF30
int g_saved_target_cursor_0069bf30;
// GLOBAL: WIZ8 0x0069BF34
int g_use_item_hover_row_0069bf34;
/* 0x0069BF38: held while CommitSelectedSpellTarget runs for a use-item
   commit; CloseUseItemSelectView early-outs on it so the commit's side
   effects cannot tear the view down mid-call. */
// GLOBAL: WIZ8 0x0069BF38
bool g_use_item_commit_active_0069bf38;
// GLOBAL: WIZ8 0x0064C7DC
const wchar_t g_format_s_paren_question_0064c7dc[] = L"%s (?)";

void UpdateUseItemScrollButtons0059D070(void);                           /* 0x0059D070 */
void RebuildUseItemSelectList0059D230(int mode, W8ItemInstance* select); /* 0x0059D230 */
bool AppendUseItemListEntry0059D450(W8ItemInstance* item, W8ItemInstance* select,
                                    unsigned char pass); /* 0x0059D450 */
bool IsUseItemFilteredOut0059D6B0(W8ItemInstance* item); /* 0x0059D6B0 */
void UseItemSelectScrollUp0059D790(void);                /* 0x0059D790 */
void UseItemSelectScrollDown0059D7E0(void);              /* 0x0059D7E0 */
void UseItemSelectFilterToggle0059D830(void);            /* 0x0059D830 */
void UseItemSelectAssayButton0059D860(void);             /* 0x0059D860 */
void CreateUseItemSelectControls0059C300(void);          /* 0x0059C300 */

/* Build the use-item view chrome: three Controls panels, the two scroll
   buttons plus the caption label in g_use_item_select_scroll_buttons, and the
   nine g_use_item_select_controls rows. */
// FUNCTION: WIZ8 0x0059C300
void CreateUseItemSelectControls0059C300(void)
{
    Controls* panel;
    Controls** panel_iter;

    g_use_item_select_panels_69b994[0] = new Controls(0x17, 0x166, 0xa4, 0x1c2, 0x97, 0, 0);
    g_use_item_select_panels_69b994[1] = new Controls(0xa4, 0x166, 0x1dc, 0x1c2, 0x97, 0, 1);
    g_use_item_select_panels_69b994[2] = new Controls(0x1dc, 0x166, 0x269, 0x1c2, 0x97, 0, 2);
    panel = g_use_item_select_panels_69b994[0];
    g_use_item_select_scroll_buttons[0] =
        new W8TextControl(panel, 0x9c, 0x24, 0x1d, 0x44, 0x3d, 0x98, 0, 0, 2, 1, 4, 3);
    g_use_item_select_scroll_buttons[1] =
        new W8TextControl(panel, 0x9d, 0x48, 0x1d, 0x68, 0x3d, 0x98, 0, 5, 7, 6, 9, 8);
    g_use_item_select_scroll_buttons[2] =
        new W8TextControl(panel, 0xffffffff, 4, 4, 0x89, 0x13, -1, -1, -1, -1, -1, -1, -1);
    g_use_item_select_scroll_buttons[0]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_scroll_buttons[1]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_scroll_buttons[0]->m_primaryActivationCallback =
        UseItemSelectScrollUp0059D790;
    g_use_item_select_scroll_buttons[1]->m_primaryActivationCallback =
        UseItemSelectScrollDown0059D7E0;
    g_use_item_select_scroll_buttons[2]->m_textBuffer.SetLayoutMode(
        g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C);
    g_use_item_select_scroll_buttons[2]->m_textBuffer.SetText(gppStringList[0x77a], g_font_683660);
    panel = g_use_item_select_panels_69b994[2];
    g_use_item_select_controls[0] =
        new W8TextControl(panel, 0x9e, 5, 5, 0x31, 0x39, -1, -1, -1, -1, -1, -1, -1);
    g_use_item_select_controls[0]->m_textBuffer.SetLayoutMode(g_W8TextBufferLayoutMask005ED55C |
                                                              g_W8TextBufferLayoutMask005ED550);
    g_use_item_select_controls[0]->AddLayoutFlags(g_W8TextControlMask005ED594);
    g_use_item_select_controls[1] =
        new W8TextControl(panel, 0xffffffff, 0x36, 6, 0x44, 0x14, 0x1ab, 0, 0, 1, 2, 4, 3);
    g_use_item_select_controls[2] =
        new W8TextControl(panel, 0xffffffff, 0x47, 6, 0x55, 0x14, 0x1ab, 0, 5, 6, 7, 9, 8);
    g_use_item_select_controls[3] =
        new W8TextControl(panel, 0x9f, 0x58, 6, 0x66, 0x14, 0x1ab, 0, 0xa, 0xb, 0xc, 0xe, 0xd);
    g_use_item_select_controls[4] = new W8TextControl(panel, 0xffffffff, 0x36, 0x17, 0x44, 0x25,
                                                      0x1ab, 0, 0xf, 0x10, 0x11, 0x13, 0x12);
    g_use_item_select_controls[5] = new W8TextControl(panel, 0xffffffff, 0x47, 0x17, 0x55, 0x25,
                                                      0x1ab, 0, 0x14, 0x15, 0x16, 0x18, 0x17);
    g_use_item_select_controls[6] = new W8TextControl(panel, 0xffffffff, 0x58, 0x17, 0x66, 0x25,
                                                      0x1ab, 0, 0x19, 0x1a, 0x1b, 0x1d, 0x1c);
    g_use_item_select_controls[7] = new W8TextControl(panel, 0xffffffff, 0x35, 0x27, 0x56, 0x39,
                                                      0x1aa, 0, 0x1e, 0x22, 0x1f, 0x20, 0x21);
    g_use_item_select_controls[8] =
        new W8TextControl(panel, 0xa0, 0x6c, 6, 0x87, 0x21, 0x8e, 0, 4, -1, 5, 6, 7);
    g_use_item_select_controls[2]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_controls[3]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_controls[4]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_controls[5]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_controls[6]->AddLayoutFlags(g_W8TextControlMask005ED578);
    g_use_item_select_controls[0]->m_primaryActivationCallback = UseItemSelectAssayButton0059D860;
    g_use_item_select_controls[0]->m_secondaryActivationCallback = UseItemSelectAssayButton0059D860;
    g_use_item_select_controls[1]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[2]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[3]->m_primaryActivationCallback = UseItemSelectFilterToggle0059D830;
    g_use_item_select_controls[4]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[5]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[6]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[8]->m_primaryActivationCallback = CloseUseItemSelection0059D950;
    for (panel_iter = g_use_item_select_panels_69b994;
         panel_iter < g_use_item_select_panels_69b994 + 3; panel_iter++) {
        (*panel_iter)->SetEnabled(1);
    }
}

// FUNCTION: WIZ8 0x0059C930
unsigned char OpenUseItemSelectView(int slot)
{
    W8TextControl** control;
    W8MainUiMode mode;

    g_use_item_commit_active_0069bf38 = 0;
    UpdateScreenOverlays(0);
    gXStatus.fItemSelectMode = 1;
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
    if (gXStatus.fNpcDialogueMode != 0) {
        EndNpcDialogueSession0056E800(0);
    }
    CloseMainGameOverlays();
    mode = g_settings_6850c8.main_ui_mode;
    if (mode == W8_MAIN_UI_MODE_RADAR) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, 0);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
    g_value_69b988 = mode;
    RegionSetEnable(0x14);
    EnableRegionInput(0x52);
    EnableRegionInput(0x53);
    EnableRegionInput(0x54);
    EnableRegionInput(0x55);
    g_level_block->action_panel_visible = 1;
    DisableRegionInput(0x59);
    DisableRegionInput(0x56);
    DisableRegionInput(0x57);
    DisableRegionInput(0x58);
    RegionSetEnable(0x1a);
    SelectTextBox(2);
    ResetEditorStatusLine0058AA20(-1);
    g_level_block->flag_271 = 0;
    CreateUseItemSelectControls0059C300();
    g_use_item_select_mode_0069b98c = -1;
    g_use_item_select_flags_0069b984 = 0;
    g_use_item_cursor_x_0069b9a8 = -1;
    g_use_item_cursor_y_0069b9ac = -1;
    g_use_item_hover_row_0069bf34 = -1;
    g_value_69b9a0 = 0;
    g_value_69b9a4 = 0;
    g_use_item_detail_item_0069bf2c = 0;
    for (control = g_use_item_select_controls + 1; control <= g_use_item_select_controls + 6;
         control++) {
        (*control)->SetEnabled(0);
    }
    memset(g_use_item_list_0069b9b4, 0, sizeof(g_use_item_list_0069b9b4));
    g_use_item_select_controls[7]->SetEnabled(0);
    RequestRedraw(0x200);
    RequestRedraw(0x100);
    RequestRedraw(0x1000);
    g_use_item_owner_index_0069b9b0 = -1;
    RefreshUseItemSelectionForSlot0059CC40(slot);
    SelectSpellCastingPartySlot(slot);
    PauseMainGameWorld();
    return 1;
}

// FUNCTION: WIZ8 0x0059CAC0
void CloseUseItemSelectView(void)
{
    W8TextControl** control;
    Controls** panel;

    if (g_use_item_commit_active_0069bf38 == 0) {
        RegionSetDisable(0x1a);
        DisableRegionInput(0x52);
        DisableRegionInput(0x53);
        DisableRegionInput(0x54);
        DisableRegionInput(0x55);
        RegionSetDisable(0x14);
        g_level_block->action_panel_visible = 0;
        SetTargetingMode(0);
        ResetEditorStatusLine0058AA20(-1);
        g_level_block->flag_271 = 1;
        SelectTextBox(gXStatus.fCombatMode != 0);
        for (control = g_use_item_select_scroll_buttons;
             control < g_use_item_select_scroll_buttons + 3; control++) {
            if (*control != 0) {
                delete *control;
            }
        }
        for (control = g_use_item_select_controls; control < g_use_item_select_controls + 9;
             control++) {
            if (*control != 0) {
                delete *control;
            }
        }
        for (panel = g_use_item_select_panels_69b994; panel < g_use_item_select_panels_69b994 + 3;
             panel++) {
            if (*panel != 0) {
                delete *panel;
            }
        }
        gXStatus.fItemSelectMode = 0;
        ApplyMainGameModeFlag(g_value_69b988, 1);
        RequestRedraw(0x200);
        RequestRedraw(0x100);
        RequestRedraw(0x1000);
        ResumeMainGameWorld();
        gXStatus.item_drag_active = 0;
        gXStatus.dragged_item = 0;
        gXStatus.dragged_item_origin = 0xff;
        gXStatus.dragged_character_slot = -1;
        if (gXStatus.fLockInteract != 0 && IsScreenTransitionPending() == 0) {
            OpenLockInteraction00587510(0);
            return;
        }
        if (gXStatus.fTrapInteract != 0 && IsScreenTransitionPending() == 0) {
            OpenTrapInteraction0058A470(0);
            return;
        }
        if (gXStatus.fCampMode != 0 && g_pending_screen_state.id != 6) {
            SyncDialogueNpcState00577260();
        }
    }
}

/* Rebuild the list for the character the open use-item view now targets. A
   dragged item switches the list to that item's usable/unusable passes; a
   recorded item on the party slot re-selects its line; otherwise the current
   mode is rebuilt or the scroll buttons resynced for an idle list. */
// FUNCTION: WIZ8 0x0059CC40
void RefreshUseItemSelectionForSlot0059CC40(int party_slot)
{
    W8ItemInstance* item;
    int mode;

    if (!IsPartySlotEligible00524A10(party_slot)) {
        CloseUseItemSelectView();
        return;
    }
    g_use_item_owner_index_0069b9b0 = party_slot;
    if (gXStatus.item_drag_active) {
        if (g_status_685170.item_in_cursor == 0) {
            if (gXStatus.dragged_item_origin == 2) {
                if (gXStatus.fCombatMode) {
                    srAssertFail("!gXStatus.fCombatMode", MGSUSEITEMSELECT_CPP, 0x1f0, 0);
                }
                g_use_item_select_scroll_buttons[1]->EnableSecondaryState(0);
                g_use_item_select_scroll_buttons[0]->DisableSecondaryState(0);
                mode = 1;
            } else {
                g_use_item_select_scroll_buttons[1]->DisableSecondaryState(0);
                g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
                mode = 0;
            }
            RebuildUseItemSelectList0059D230(mode, gXStatus.dragged_item);
        } else {
            ResetEditorStatusLine0058AA20(2);
            g_use_item_select_controls[3]->SetEnabled(1);
            if (static_cast<unsigned char>(g_use_item_select_controls[3]->m_stateFlags &
                                           g_W8TextControlMask005ED570) == 0) {
                g_use_item_select_controls[3]->EnableSecondaryState(1);
                g_use_item_select_controls[3]->Invalidate(0);
            }
            g_use_item_select_flags_0069b984 |= 1;
            g_use_item_list_count_0069b990 = 0;
            g_selected_use_item_line_0069b95c = -1;
            AppendUseItemListEntry0059D450(&g_status_685170.item_in_hand_235b,
                                           &g_status_685170.item_in_hand_235b, 0);
            AppendUseItemListEntry0059D450(&g_status_685170.item_in_hand_235b,
                                           &g_status_685170.item_in_hand_235b, 1);
        }
        if (g_selected_use_item_line_0069b95c == -1) {
            srAssertFail("giReuseItemLineNumber != -1", MGSUSEITEMSELECT_CPP, 0x1fe, 0);
        }
        ScrollTextBoxTo(g_selected_use_item_line_0069b95c);
        gXStatus.item_drag_active = 0;
        return;
    }
    if (CanPartySlotUseRecordedItem(party_slot)) {
        gXStatus.dragged_item = 0;
        item = FindCharacterItemAt(party_slot,
                                   g_status_685170.buffers.party_rows[party_slot].item_origin,
                                   g_status_685170.buffers.party_rows[party_slot].item_slot);
        if (CanUseItemForAction(g_use_item_owner_index_0069b9b0, item)) {
            if (g_status_685170.buffers.party_rows[party_slot].item_origin == 2) {
                if (gXStatus.fCombatMode) {
                    srAssertFail("!gXStatus.fCombatMode", MGSUSEITEMSELECT_CPP, 0x216, 0);
                }
                g_use_item_select_scroll_buttons[1]->EnableSecondaryState(0);
                g_use_item_select_scroll_buttons[0]->DisableSecondaryState(0);
                mode = 1;
            } else {
                g_use_item_select_scroll_buttons[1]->DisableSecondaryState(0);
                g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
                mode = 0;
            }
            RebuildUseItemSelectList0059D230(mode, item);
            if (g_selected_use_item_line_0069b95c == -1) {
                srAssertFail("giReuseItemLineNumber != -1", MGSUSEITEMSELECT_CPP, 0x221, 0);
            }
            ScrollTextBoxTo(g_selected_use_item_line_0069b95c);
            SelectTextSlot1D8(g_selected_use_item_line_0069b95c, 2);
            RedrawTextBox();
            return;
        }
    }
    gXStatus.dragged_item = 0;
    if (g_use_item_select_mode_0069b98c != -1) {
        RebuildUseItemSelectList0059D230(g_use_item_select_mode_0069b98c, 0);
        return;
    }
    g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
    if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                       g_W8TextControlMask005ED570) != 0) {
            g_use_item_select_scroll_buttons[1]->DisableSecondaryState(0);
            g_use_item_select_scroll_buttons[1]->Invalidate(0);
        }
        RebuildUseItemSelectList0059D230(0, 0);
        return;
    }
    g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x0059CF30
void SetValue69B988(W8MainUiMode value)
{
    g_value_69b988 = value;
}
// FUNCTION: WIZ8 0x0059CF40
void RedrawPanel69B998(void)
{
    g_panel_69b998->Invalidate(0);
}

// FUNCTION: WIZ8 0x0059D790
void UseItemSelectScrollUp0059D790(void)
{
    if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                       g_W8TextControlMask005ED570) != 0) {
            g_use_item_select_scroll_buttons[1]->DisableSecondaryState(0);
            g_use_item_select_scroll_buttons[1]->Invalidate(0);
        }
        RebuildUseItemSelectList0059D230(0, 0);
        return;
    }
    g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x0059D7E0
void UseItemSelectScrollDown0059D7E0(void)
{
    if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                   g_W8TextControlMask005ED570) != 0) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                       g_W8TextControlMask005ED570) != 0) {
            g_use_item_select_scroll_buttons[0]->DisableSecondaryState(0);
            g_use_item_select_scroll_buttons[0]->Invalidate(0);
        }
        RebuildUseItemSelectList0059D230(1, 0);
        return;
    }
    g_use_item_select_scroll_buttons[1]->EnableSecondaryState(0);
}

// FUNCTION: WIZ8 0x0059D830
void UseItemSelectFilterToggle0059D830(void)
{
    if (static_cast<unsigned char>(g_use_item_select_controls[3]->m_stateFlags &
                                   g_W8TextControlMask005ED570) == 0) {
        g_use_item_select_controls[3]->EnableSecondaryState(1);
        g_use_item_select_controls[3]->Invalidate(0);
    }
    g_use_item_select_flags_0069b984 |= 1;
}

// FUNCTION: WIZ8 0x0059D860
void UseItemSelectAssayButton0059D860(void)
{
    if (g_use_item_detail_item_0069bf2c != 0 &&
        g_use_item_select_controls[0]->m_imageObject != -1) {
        OpenUseItemAssayDialog59D880(g_use_item_detail_item_0069bf2c);
    }
}

void Function59D230(int mode, int arg); /* 0x0059D230: rebuild the list for a select mode */

/* Commit the pending use-item action: while the selected item is still usable
   by the owner and its recorded target suits it, commit the target and aim
   the item use. Spell 0x17 keeps the view open for the follow-up pick;
   anything else closes it. */
// FUNCTION: WIZ8 0x0059D180
void CommitSelectedItemUse(void)
{
    W8Character* character;

    if (g_value_69b9a0 != 0 &&
        CanUseItemForAction(g_status_685170.selected_character, g_value_69b9a0) &&
        IsItemTargetOfNeededKind(g_status_685170.selected_character, g_value_69b9a0)) {
        character = &g_status_685170.buffers.characters[g_status_685170.selected_character];
        if (g_value_69b9a0 != 0) {
            g_use_item_commit_active_0069bf38 = 1;
            CommitSelectedSpellTarget();
            g_use_item_commit_active_0069bf38 = 0;
            AimItemUseAtCurrentTarget0051DB60(character, g_value_69b9a0);
            if (g_value_69b9a0 != 0 && g_value_69b9a0->item_id != -1 &&
                GetItemSpell(g_value_69b9a0) == 0x17) {
                return;
            }
            CloseUseItemSelectView();
        }
    }
}

/* Reset the detail control and rebuild the use-item list for a mode:
   -1 clears the view state and disables the action controls, 0 lists the
   owning character's equipment then backpack, and any other mode lists the
   party item pool. Each mode appends entries in two passes (usable first,
   then the rest) through AppendUseItemListEntry0059D450; select re-highlights
   the line holding that item. */
// FUNCTION: WIZ8 0x0059D230
void RebuildUseItemSelectList0059D230(int mode, W8ItemInstance* select)
{
    W8Character* character;
    unsigned char pass;
    unsigned int i;
    int slot;

    slot = g_status_685170.selected_character;
    ResetEditorStatusLine0058AA20(2);
    ClearTextSlot1E8(2);
    g_value_69b9a0 = 0;
    g_use_item_detail_item_0069bf2c = 0;
    g_use_item_select_controls[0]->m_imageObject = -1;
    g_use_item_select_controls[0]->m_measured_w = -1;
    g_use_item_select_controls[0]->m_measured_h = -1;
    g_use_item_select_controls[0]->m_imageFrame = -1;
    g_use_item_select_controls[0]->m_normalSprite = -1;
    g_use_item_select_controls[0]->m_pressedSprite = -1;
    g_use_item_select_controls[0]->m_textBuffer.SetText(g_wchar_0068ee58, 0);
    g_use_item_select_controls[0]->Invalidate(1);
    SelectSpellCastingPartySlot(g_status_685170.selected_character);
    g_use_item_select_mode_0069b98c = mode;
    if (mode == -1) {
        for (i = 1; i <= 6; i++) {
            g_use_item_select_controls[i]->SetEnabled(0);
        }
        return;
    }
    g_use_item_select_controls[3]->SetEnabled(1);
    if (static_cast<unsigned char>(g_use_item_select_controls[3]->m_stateFlags &
                                   g_W8TextControlMask005ED570) == 0) {
        g_use_item_select_controls[3]->EnableSecondaryState(1);
        g_use_item_select_controls[3]->Invalidate(0);
    }
    g_use_item_select_flags_0069b984 |= 1;
    g_use_item_list_count_0069b990 = 0;
    g_selected_use_item_line_0069b95c = -1;
    pass = 0;
    if (mode != 0) {
        do {
            for (i = 0; i < static_cast<unsigned int>(g_status_685170.party_item_count_1791); i++) {
                if (!AppendUseItemListEntry0059D450(&g_status_685170.party_item_pool_0021[i],
                                                    select, pass)) {
                    return;
                }
            }
            ScrollTextBoxTo(0);
            pass++;
        } while (pass < 2);
        g_use_item_select_panels_69b994[1]->Invalidate(0);
        return;
    }
    character = &g_status_685170.buffers.characters[slot];
    do {
        for (i = 0; i < 12; i++) {
            if (!AppendUseItemListEntry0059D450(&character->equipment[i], select, pass)) {
                return;
            }
        }
        for (i = 0; i < 8; i++) {
            if (!AppendUseItemListEntry0059D450(&character->backpack[i], select, pass)) {
                return;
            }
        }
        pass++;
    } while (pass < 2);
    ScrollTextBoxTo(0);
    g_use_item_select_panels_69b994[1]->Invalidate(0);
}

/* Append one item to the use-item list under the active filter. Pass 0 takes
   items the owner can use; pass 1 takes the rest outside camp/lock/trap
   contexts (where only the usable pass runs). Entries store the item, select
   it when it matches select, and show a display name with stack/charge counts
   or the unidentified "(?)". Returns false when the list is full. */
// FUNCTION: WIZ8 0x0059D450
bool AppendUseItemListEntry0059D450(W8ItemInstance* item, W8ItemInstance* select,
                                    unsigned char pass)
{
    wchar_t* text;
    unsigned int color;
    short count;
    bool charged;

    if (item->item_id == -1) {
        return true;
    }
    if (IsUseItemFilteredOut0059D6B0(item)) {
        return true;
    }
    if (pass != 0 || !CanUseItemForAction(g_use_item_owner_index_0069b9b0, item)) {
        if (gXStatus.fCampMode) {
            return true;
        }
        if (gXStatus.fLockInteract) {
            return true;
        }
        if (gXStatus.fTrapInteract) {
            return true;
        }
        if (pass != 1) {
            return true;
        }
        if (CanUseItemForAction(g_use_item_owner_index_0069b9b0, item)) {
            return true;
        }
        if (g_use_item_list_count_0069b990 + 1U > 0x15e) {
            g_selected_use_item_line_0069b95c = g_use_item_list_count_0069b990;
            return false;
        }
        if (select != 0 && select == item) {
            g_selected_use_item_line_0069b95c = g_use_item_list_count_0069b990;
        }
        g_use_item_list_0069b9b4[g_use_item_list_count_0069b990] = item;
        if (g_item_records[item->item_id].equip_class != 0xd ||
            g_status_685170.buffers.characters[g_use_item_owner_index_0069b9b0]
                    .condition_turns[8] == 0) {
            color = 0;
        } else {
            color = 4;
        }
        ShowNotice(color, FormatItemDisplayName(item, 1), 2, 0xffffffff, false);
        g_use_item_list_count_0069b990 = g_use_item_list_count_0069b990 + 1;
        return true;
    }
    if (g_use_item_list_count_0069b990 + 1U > 0x15e) {
        g_selected_use_item_line_0069b95c = g_use_item_list_count_0069b990;
        return false;
    }
    if (select != 0 && select == item) {
        g_selected_use_item_line_0069b95c = g_use_item_list_count_0069b990;
    }
    g_use_item_list_0069b9b4[g_use_item_list_count_0069b990] = item;
    charged = false;
    switch (g_item_records[item->item_id].quantity_kind) {
    case 1:
        count = item->stack_count;
        break;
    case 2:
    case 3:
        if (item->identified == 0) {
            ShowNotice(0xf,
                       FormatWideString(g_format_s_paren_question_0064c7dc,
                                        FormatItemDisplayName(item, 0)),
                       2, 0xffffffff, false);
            g_use_item_list_count_0069b990 = g_use_item_list_count_0069b990 + 1;
            return true;
        }
        charged = true;
        count = item->uses_or_charges;
        break;
    case 4:
        charged = true;
        count = item->uses_or_charges;
        break;
    default:
        ShowNotice(0xf, FormatItemDisplayName(item, 0), 2, 0xffffffff, false);
        g_use_item_list_count_0069b990 = g_use_item_list_count_0069b990 + 1;
        return true;
    }
    if (count == -1) {
        text = FormatWideString(g_format_s_paren_question_0064c7dc, FormatItemDisplayName(item, 0));
    } else if (count > 1 || charged) {
        text = FormatWideString(g_format_s_paren_d_0061a700, FormatItemDisplayName(item, 0), count);
    } else {
        text = FormatItemDisplayName(item, 0);
    }
    ShowNotice(0xf, text, 2, 0xffffffff, false);
    g_use_item_list_count_0069b990 = g_use_item_list_count_0069b990 + 1;
    return true;
}

/* Filter predicate for AppendUseItemListEntry0059D450: while the select flags
   are set, entries the character cannot activate or cast from are dropped,
   except usable-class items stay unless the slot's bound NPC wants the
   item. */
// FUNCTION: WIZ8 0x0059D6B0
bool IsUseItemFilteredOut0059D6B0(W8ItemInstance* item)
{
    W8NpcState* npc;

    if (g_use_item_select_flags_0069b984 == 0) {
        return false;
    }
    if (item->item_id == -1) {
        return true;
    }
    if (!CanCharacterActivateItem(
            &g_status_685170.buffers.characters[g_status_685170.selected_character], item)) {
        if (CanCastFromItem(&g_status_685170.buffers.characters[g_status_685170.selected_character],
                            item)) {
            return false;
        }
        if (!IsUsableItemClass00522A00(item)) {
            return true;
        }
        if ((g_status_685170.selected_character == 0 || g_status_685170.selected_character == 1) &&
            (npc =
                 GetNpcState(g_status_685170.buffers.party_rows[g_status_685170.selected_character]
                                 .animation_0fa),
             npc != 0) &&
            NpcWantsItem0050DC50(npc, item)) {
            return true;
        }
    }
    return false;
}

/* After the cursor item is dropped mid-select, rebuild the list under the
   saved select mode so the rows reflect the new state, then repaint. */
// FUNCTION: WIZ8 0x0059D690
void RefreshUseItemSelection(void)
{
    if (g_use_item_select_mode_0069b98c != -1) {
        RebuildUseItemSelectList0059D230(g_use_item_select_mode_0069b98c, 0);
        RequestRedraw(0x200);
    }
}

/* Right release on the use-item text box: open the assay dialog for the item
   on the clicked row. The target cursor is saved so the destroy callback can
   restore it. */
// FUNCTION: WIZ8 0x0059D880
void OpenUseItemAssayDialog59D880(W8ItemInstance* item)
{
    W8AssayDialog* dialog;

    g_saved_target_cursor_0069bf30 = gXStatus.iCurrentCursor;
    dialog = new W8AssayDialog(
        item, &g_status_685170.buffers.characters[g_use_item_owner_index_0069b9b0]);
    dialog->SetText(&g_wchar_00689b34);
    dialog->SetOrigin(g_info_dialog_x_005ef958, 0x48);
    dialog->m_destroy_callback = RestoreTargetCursor59D930;
    OpenModal(dialog);
}

/* Destroy callback for the item-assay and spell-info dialogs: put back the
   cursor that was current before the dialog opened and repaint everything. */
// FUNCTION: WIZ8 0x0059D930
void RestoreTargetCursor59D930(W8DialogBase*)
{
    SetTargetCursor(g_saved_target_cursor_0069bf30);
    RequestRedraw(-1);
}

// FUNCTION: WIZ8 0x0059D950
void CloseUseItemSelection0059D950(void)
{
    CloseUseItemSelectView();
    ClearTargetingMode(g_status_685170.selected_character);
}

/* Scroll up/down buttons for the use-item list (catalog callback_ids 0 and 1). */
// FUNCTION: WIZ8 0x0059D970
unsigned char UseItemSelectScrollRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_REPEAT:
        g_use_item_select_scroll_buttons[region->callback_id]->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case LEFT_BUTTON_UP:
        g_use_item_select_scroll_buttons[region->callback_id]->OnLeftButtonUp(0);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        }
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_use_item_select_scroll_buttons[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_use_item_select_scroll_buttons[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
        break;
    }
    return 0;
}

/* Use-item select action/icon controls (catalog callback_ids 0, 3, 8). */
// FUNCTION: WIZ8 0x0059DA30
unsigned char UseItemSelectControlRegionEvent(const InputAtom* event, W8Region* region)
{
    int us_event = event->usEvent;

    if (us_event <= RIGHT_BUTTON_DOWN) {
        if (us_event == RIGHT_BUTTON_DOWN) {
            g_use_item_select_controls[region->callback_id]->OnRightButtonDown(0);
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_DOWN) {
            if (us_event == LEFT_BUTTON_UP) {
                g_use_item_select_controls[region->callback_id]->OnLeftButtonUp(0);
                if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
                    return 1;
                }
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
                return 1;
            }
            if (us_event != LEFT_BUTTON_REPEAT) {
                return 0;
            }
        }
        g_use_item_select_controls[region->callback_id]->OnLeftButtonDown(0);
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    }
    if (us_event == RIGHT_BUTTON_UP) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            g_use_item_select_controls[region->callback_id]->OnRightButtonUp(0);
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        }
        return 1;
    }
    if (us_event == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_use_item_select_controls[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_use_item_select_controls[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

/* Use-item text-box body region event: button presses only arm the held bits
   and all action happens on release - left release commits the slot's line,
   right release opens the assay dialog for the item under the cursor. Motion
   keeps g_use_item_cursor_x/y as the last cursor point and tracks the hovered
   row into g_use_item_hover_row_0069bf34. */
// FUNCTION: WIZ8 0x0059DB40
unsigned char UseItemSelectTextBoxRegionEvent(const InputAtom* event, W8Region* region)
{
    unsigned short y;
    int row;
    int slot;
    int us_event = event->usEvent;

    switch (us_event) {
    case LEFT_BUTTON_UP:
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
        slot = GetTextSlot1D8(2);
        if (slot == -1) {
            return 1;
        }
        SelectUseItemLine0059DDC0(slot);
        return 1;
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            ClearTextSlot1D8(2);
            g_use_item_hover_row_0069bf34 = -1;
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_use_item_cursor_x_0069b9a8 = static_cast<unsigned short>(event->uiParam);
            g_use_item_cursor_y_0069b9ac = static_cast<unsigned short>(event->uiParam >> 16);
            return 1;
        }
        if (static_cast<unsigned short>(event->uiParam) == g_use_item_cursor_x_0069b9a8 &&
            static_cast<int>(event->uiParam >> 16) == g_use_item_cursor_y_0069b9ac) {
            return 1;
        }
        g_use_item_cursor_x_0069b9a8 = static_cast<unsigned short>(event->uiParam);
        g_use_item_cursor_y_0069b9ac = static_cast<unsigned short>(event->uiParam >> 16);
        y = static_cast<unsigned short>(event->uiParam >> 16);
        if (g_level_block->text_box_top <= y && y <= g_level_block->text_box_bottom) {
            row = (y - g_level_block->text_box_top) / 0xb;
            if (row != g_use_item_hover_row_0069bf34) {
                ClearTextSlot1D8(2);
                if (row < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
                    SelectTextSlot1D8(g_level_block->text_lines[2] + row, 2);
                }
                RedrawTextBox();
            }
            g_use_item_hover_row_0069bf34 = row;
        }
        return 1;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) == 0) {
            return 1;
        }
        region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        y = static_cast<unsigned short>(event->uiParam >> 16);
        if (y < g_level_block->text_box_top || g_level_block->text_box_bottom < y) {
            return 1;
        }
        row = (y - g_level_block->text_box_top) / 0xb;
        if (row < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2]) &&
            g_use_item_list_0069b9b4[g_level_block->text_lines[2] + row] != 0) {
            OpenUseItemAssayDialog59D880(
                g_use_item_list_0069b9b4[g_level_block->text_lines[2] + row]);
        }
        return 1;
    }
    return 0;
}

/* Wheel rotation over the use-item text box re-selects the row under the
   cursor, even when it is already the hovered row. */
// FUNCTION: WIZ8 0x0059DD30
void UseItemSelectTextBoxWheelAt(short x, unsigned short y, unsigned char flag)
{
    int row;

    if (y < g_level_block->text_box_top || g_level_block->text_box_bottom < y) {
        return;
    }
    row = (y - g_level_block->text_box_top) / 0xb;
    if (row != g_use_item_hover_row_0069bf34 || flag != 0) {
        ClearTextSlot1D8(2);
        if (row < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[2])) {
            SelectTextSlot1D8(g_level_block->text_lines[2] + row, 2);
        }
        RedrawTextBox();
    }
    g_use_item_hover_row_0069bf34 = row;
}

/* Left-click commit of a use-item list row: validate the item's spell for
   this owner, learn it directly when castable-from-item, otherwise point the
   targeting filter at the embedded spell. */
// FUNCTION: WIZ8 0x0059DDC0
void SelectUseItemLine0059DDC0(int iTextLine)
{
    int target_type;

    if (iTextLine < 0) {
        srAssertFail("iTextLine >= 0", MGSUSEITEMSELECT_CPP, 0x61d, 0);
    }
    if (iTextLine >= g_use_item_list_count_0069b990) {
        // c-style-cast-ok: the assertion text itself spells (INT32)
        srAssertFail("iTextLine < (INT32) guiNumItemsInList", MGSUSEITEMSELECT_CPP, 0x61e, 0);
    }
    SetTargetingMode(0);
    Function59DFA0(g_use_item_list_0069b9b4[iTextLine]);
    if (ValidateItemSpellUse(g_use_item_owner_index_0069b9b0, g_use_item_list_0069b9b4[iTextLine],
                             SpellCastingNoticeClosed005A02F0) != 0) {
        QueueCharacterEvent(&g_status_685170.buffers.characters[g_use_item_owner_index_0069b9b0],
                            g_character_event_kind_005ee65c, 0,
                            g_character_event_flags_mask_005ed8e4 | g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
        return;
    }
    g_value_69b9a0 = g_use_item_list_0069b9b4[iTextLine];
    if (Trynnie2UseItem004D9F60(g_value_69b9a0) != 0) {
        CloseUseItemSelectView();
        return;
    }
    if (IsUsableItemClass00522A00(g_value_69b9a0) != 0) {
        Function59E0F0();
        CloseUseItemSelectView();
        return;
    }
    if (CanCastFromItem(&g_status_685170.buffers.characters[g_status_685170.selected_character],
                        g_value_69b9a0)) {
        LearnSpellFromItem(&g_status_685170.buffers.characters[g_status_685170.selected_character],
                           g_value_69b9a0);
        CloseUseItemSelectView();
        return;
    }
    SelectTextSlot1E8(iTextLine, 2);
    ClearTextSlot1D8(2);
    RedrawTextBox();
    target_type =
        GetSpellTargetType(GetItemSpell(g_value_69b9a0),
                           ItemClassNormalizesTarget(&g_item_records[g_value_69b9a0->item_id]));
    ConfigureSpellTargetFilter(target_type, GetTargetNeededForItem(g_value_69b9a0));
}

// FUNCTION: WIZ8 0x0059E0D0
W8ItemInstance* GetSelectedOrFallbackValue0059E0D0(void)
{
    W8ItemInstance* value = g_value_69b9a4;
    if (value == 0) {
        value = g_value_69b9a0;
    }
    return value;
}

// FUNCTION: WIZ8 0x0059E0E0
void SelectCurrentUseItemLine0059E0E0(void)
{
    SelectUseItemLine0059DDC0(g_selected_use_item_line_0069b95c);
}

// FUNCTION: WIZ8 0x0059E1E0
void SetValue69B9A4(W8ItemInstance* value)
{
    g_value_69b9a4 = value;
}
