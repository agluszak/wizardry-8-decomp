#include "wiz8/layouts/game_status.h"
#include "wiz8/integer_constants.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSPortraitCombat.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/level_specific_code/Trynnie2.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/fonts.h"
#include "wiz8/xstatus.h"

#define MGSUSEITEMSELECT_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSUseItemSelect.cpp"

// GLOBAL: WIZ8 0x0069B984
static int g_use_item_select_flags;
// GLOBAL: WIZ8 0x0069b988
W8MainUiMode g_use_item_select_return_mode;
// GLOBAL: WIZ8 0x0069B98C
static int g_use_item_select_mode;

// GLOBAL: WIZ8 0x0069B950
W8TextControl* g_use_item_select_scroll_buttons[3];
// GLOBAL: WIZ8 0x0069B95C
static int g_selected_use_item_line;
// GLOBAL: WIZ8 0x0069B960
W8TextControl* g_use_item_select_controls[9];
// GLOBAL: WIZ8 0x0069B9A0
W8ItemInstance* g_use_item_selected;
// GLOBAL: WIZ8 0x0069B9A4
W8ItemInstance* g_use_item_select_override_item;
// GLOBAL: WIZ8 0x0069B990
static int g_use_item_list_count;
// GLOBAL: WIZ8 0x0069B994
Controls* g_use_item_select_panels[3];
// GLOBAL: WIZ8 0x0069B9A8
static int g_use_item_cursor_x;
// GLOBAL: WIZ8 0x0069B9AC
static int g_use_item_cursor_y;
// GLOBAL: WIZ8 0x0069B9B0
static int g_use_item_owner_index;
// GLOBAL: WIZ8 0x0069B9B4
static W8ItemInstance* g_use_item_list[0x15e];
// GLOBAL: WIZ8 0x0069BF2C
static W8ItemInstance* g_use_item_detail_item;
// GLOBAL: WIZ8 0x0069BF30
int g_saved_target_cursor;
// GLOBAL: WIZ8 0x0069BF34
static int g_use_item_hover_row;
/* 0x0069BF38: held while CommitSelectedSpellTarget runs for a use-item
   commit; CloseUseItemSelectView early-outs on it so the commit's side
   effects cannot tear the view down mid-call. */
// GLOBAL: WIZ8 0x0069BF38
static bool g_use_item_commit_active;
// GLOBAL: WIZ8 0x0064C7DC
static wchar_t g_format_s_paren_question[] = L"%s (?)";

void UpdateUseItemScrollButtons(void);                                  /* 0x0059D070 */
static void RebuildUseItemSelectList(int mode, W8ItemInstance* select); /* 0x0059D230 */
static bool AppendUseItemListEntry(W8ItemInstance* item, W8ItemInstance* select,
                                   unsigned char pass); /* 0x0059D450 */
bool IsUseItemFilteredOut(W8ItemInstance* item);        /* 0x0059D6B0 */
static void UseItemSelectScrollUp(void);                /* 0x0059D790 */
static void UseItemSelectScrollDown(void);              /* 0x0059D7E0 */
static void UseItemSelectFilterToggle(void);            /* 0x0059D830 */
static void UseItemSelectAssayButton(void);             /* 0x0059D860 */
static void CreateUseItemSelectControls(void);          /* 0x0059C300 */

/* Build the use-item view chrome: three Controls panels, the two scroll
   buttons plus the caption label in g_use_item_select_scroll_buttons, and the
   nine g_use_item_select_controls rows. */
// FUNCTION: WIZ8 0x0059C300
static void CreateUseItemSelectControls(void)
{
    Controls* panel;
    Controls** panel_iter;

    g_use_item_select_panels[0] = new Controls(0x17, 0x166, 0xa4, 0x1c2, 0x97, 0, 0);
    g_use_item_select_panels[1] = new Controls(0xa4, 0x166, 0x1dc, 0x1c2, 0x97, 0, 1);
    g_use_item_select_panels[2] = new Controls(0x1dc, 0x166, 0x269, 0x1c2, 0x97, 0, 2);
    panel = g_use_item_select_panels[0];
    g_use_item_select_scroll_buttons[0] =
        new W8TextControl(panel, 0x9c, 0x24, 0x1d, 0x44, 0x3d, 0x98, 0, 0, 2, 1, 4, 3);
    g_use_item_select_scroll_buttons[1] =
        new W8TextControl(panel, 0x9d, 0x48, 0x1d, 0x68, 0x3d, 0x98, 0, 5, 7, 6, 9, 8);
    g_use_item_select_scroll_buttons[2] =
        new W8TextControl(panel, 0xffffffff, 4, 4, 0x89, 0x13, -1, -1, -1, -1, -1, -1, -1);
    g_use_item_select_scroll_buttons[0]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_scroll_buttons[1]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_scroll_buttons[0]->m_primaryActivationCallback = UseItemSelectScrollUp;
    g_use_item_select_scroll_buttons[1]->m_primaryActivationCallback = UseItemSelectScrollDown;
    g_use_item_select_scroll_buttons[2]->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignMiddle |
                                                                    g_W8TextBufferAlignCenter);
    g_use_item_select_scroll_buttons[2]->m_textBuffer.SetText(gppStringList[0x77a],
                                                              g_wiz_text_font_secondary);
    panel = g_use_item_select_panels[2];
    g_use_item_select_controls[0] =
        new W8TextControl(panel, 0x9e, 5, 5, 0x31, 0x39, -1, -1, -1, -1, -1, -1, -1);
    g_use_item_select_controls[0]->m_textBuffer.SetLayoutMode(g_W8TextBufferAlignBottom |
                                                              g_W8TextBufferAlignRight);
    g_use_item_select_controls[0]->AddLayoutFlags(g_W8TextControlLayoutImageAtOrigin);
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
    g_use_item_select_controls[2]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_controls[3]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_controls[4]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_controls[5]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_controls[6]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_use_item_select_controls[0]->m_primaryActivationCallback = UseItemSelectAssayButton;
    g_use_item_select_controls[0]->m_secondaryActivationCallback = UseItemSelectAssayButton;
    g_use_item_select_controls[1]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[2]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[3]->m_primaryActivationCallback = UseItemSelectFilterToggle;
    g_use_item_select_controls[4]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[5]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[6]->m_primaryActivationCallback = NoOp;
    g_use_item_select_controls[8]->m_primaryActivationCallback = CloseUseItemSelection;
    for (panel_iter = g_use_item_select_panels; panel_iter < g_use_item_select_panels + 3;
         panel_iter++) {
        (*panel_iter)->SetEnabled(true);
    }
}

// FUNCTION: WIZ8 0x0059C930
unsigned char OpenUseItemSelectView(int slot)
{
    W8TextControl** control;
    W8MainUiMode mode;

    g_use_item_commit_active = false;
    UpdateScreenOverlays(0);
    gXStatus.fItemSelectMode = true;
    if (g_level_block->combat_end_notification != -1) {
        DestroySubMenuControls();
    }
    CloseNpcDialogueIfActive();
    CloseMainGameOverlays();
    mode = g_settings.main_ui_mode;
    if (mode == W8_MAIN_UI_MODE_RADAR) {
        ApplyMainGameModeFlag(W8_MAIN_UI_MODE_FORMATION, false);
    } else {
        SetViewportMode(GetMainGameViewportMode());
    }
    g_use_item_select_return_mode = mode;
    RestoreSpellCastingRegions();
    RegionSetEnable(0x1a);
    SelectTextBox(2);
    ResetEditorStatusLine(-1);
    g_level_block->text_box_visible = false;
    CreateUseItemSelectControls();
    g_use_item_select_mode = -1;
    g_use_item_select_flags = 0;
    g_use_item_cursor_x = -1;
    g_use_item_cursor_y = -1;
    g_use_item_hover_row = -1;
    g_use_item_selected = 0;
    g_use_item_select_override_item = 0;
    g_use_item_detail_item = 0;
    for (control = g_use_item_select_controls + 1; control <= g_use_item_select_controls + 6;
         control++) {
        (*control)->SetEnabled(false);
    }
    memset(g_use_item_list, 0, sizeof(g_use_item_list));
    g_use_item_select_controls[7]->SetEnabled(false);
    RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    RequestRedrawCombatBar();
    RequestRedraw(W8_MAIN_REDRAW_SUBMENU_BUTTONS);
    g_use_item_owner_index = -1;
    RefreshUseItemSelectionForSlot(slot);
    SelectSpellCastingPartySlot(slot);
    PauseMainGameWorld();
    return 1;
}

// FUNCTION: WIZ8 0x0059CAC0
void CloseUseItemSelectView(void)
{
    W8TextControl** control;
    Controls** panel;

    if (!g_use_item_commit_active) {
        RegionSetDisable(0x1a);
        DisableRegionInput(0x52);
        DisableRegionInput(0x53);
        DisableRegionInput(0x54);
        DisableRegionInput(0x55);
        RegionSetDisable(0x14);
        g_level_block->action_panel_visible = 0;
        SetTargetingMode(W8_TARGET_NEED_NONE);
        ResetEditorStatusLine(-1);
        g_level_block->text_box_visible = true;
        SelectTextBox(gXStatus.fCombatMode);
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
        for (panel = g_use_item_select_panels; panel < g_use_item_select_panels + 3; panel++) {
            if (*panel != 0) {
                delete *panel;
            }
        }
        gXStatus.fItemSelectMode = false;
        ApplyMainGameModeFlag(g_use_item_select_return_mode, true);
        RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
        RequestRedrawCombatBar();
        RequestRedraw(W8_MAIN_REDRAW_SUBMENU_BUTTONS);
        ResumeMainGameWorld();
        gXStatus.item_drag_active = false;
        gXStatus.dragged_item = 0;
        gXStatus.dragged_item_origin = W8_ITEM_ORIGIN_NONE;
        gXStatus.dragged_character_slot = -1;
        if (gXStatus.fLockInteract && !IsScreenTransitionPending()) {
            OpenLockInteraction(0);
            return;
        }
        if (gXStatus.fTrapInteract && !IsScreenTransitionPending()) {
            OpenTrapInteraction(0);
            return;
        }
        if (gXStatus.fCampMode && g_pending_screen_state.id != W8_SCREEN_CAMP) {
            SyncDialogueNpcState();
        }
    }
}

/* Rebuild the list for the character the open use-item view now targets. A
   dragged item switches the list to that item's usable/unusable passes; a
   recorded item on the party slot re-selects its line; otherwise the current
   mode is rebuilt or the scroll buttons resynced for an idle list. */
// FUNCTION: WIZ8 0x0059CC40
void RefreshUseItemSelectionForSlot(int party_slot)
{
    W8ItemInstance* item;
    int mode;

    if (!IsPartySlotEligible(party_slot)) {
        CloseUseItemSelectView();
        return;
    }
    g_use_item_owner_index = party_slot;
    if (gXStatus.item_drag_active) {
        if (!g_status.item_in_cursor) {
            if (gXStatus.dragged_item_origin == W8_ITEM_ORIGIN_PARTY_POOL) {
                if (gXStatus.fCombatMode) {
                    srAssertFail("!gXStatus.fCombatMode", MGSUSEITEMSELECT_CPP, 0x1f0, 0);
                }
                g_use_item_select_scroll_buttons[1]->EnableSecondaryState(false);
                g_use_item_select_scroll_buttons[0]->DisableSecondaryState(false);
                mode = 1;
            } else {
                g_use_item_select_scroll_buttons[1]->DisableSecondaryState(false);
                g_use_item_select_scroll_buttons[0]->EnableSecondaryState(false);
                mode = 0;
            }
            RebuildUseItemSelectList(mode, gXStatus.dragged_item);
        } else {
            ResetEditorStatusLine(2);
            g_use_item_select_controls[3]->SetEnabled(true);
            UseItemSelectFilterToggle();
            g_use_item_list_count = 0;
            g_selected_use_item_line = -1;
            AppendUseItemListEntry(&g_status.item_in_hand, &g_status.item_in_hand, 0);
            AppendUseItemListEntry(&g_status.item_in_hand, &g_status.item_in_hand, 1);
        }
        if (g_selected_use_item_line == -1) {
            srAssertFail("giReuseItemLineNumber != -1", MGSUSEITEMSELECT_CPP, 0x1fe, 0);
        }
        ScrollTextBoxTo(g_selected_use_item_line);
        gXStatus.item_drag_active = false;
        return;
    }
    if (CanPartySlotUseRecordedItem(party_slot)) {
        gXStatus.dragged_item = 0;
        item = FindCharacterItemAt(party_slot, g_status.buffers.XChar[party_slot].item_origin,
                                   g_status.buffers.XChar[party_slot].item_slot);
        if (CanUseItemForAction(g_use_item_owner_index, item)) {
            if (g_status.buffers.XChar[party_slot].item_origin == W8_ITEM_ORIGIN_PARTY_POOL) {
                if (gXStatus.fCombatMode) {
                    srAssertFail("!gXStatus.fCombatMode", MGSUSEITEMSELECT_CPP, 0x216, 0);
                }
                g_use_item_select_scroll_buttons[1]->EnableSecondaryState(false);
                g_use_item_select_scroll_buttons[0]->DisableSecondaryState(false);
                mode = 1;
            } else {
                g_use_item_select_scroll_buttons[1]->DisableSecondaryState(false);
                g_use_item_select_scroll_buttons[0]->EnableSecondaryState(false);
                mode = 0;
            }
            RebuildUseItemSelectList(mode, item);
            if (g_selected_use_item_line == -1) {
                srAssertFail("giReuseItemLineNumber != -1", MGSUSEITEMSELECT_CPP, 0x221, 0);
            }
            ScrollTextBoxTo(g_selected_use_item_line);
            SetHoveredTextLine(g_selected_use_item_line, 2);
            RedrawTextBox();
            return;
        }
    }
    gXStatus.dragged_item = 0;
    if (g_use_item_select_mode != -1) {
        RebuildUseItemSelectList(g_use_item_select_mode, 0);
        return;
    }
    g_use_item_select_scroll_buttons[0]->EnableSecondaryState(false);
    UseItemSelectScrollUp();
}

// FUNCTION: WIZ8 0x0059CF30
void SetUseItemSelectReturnMode(W8MainUiMode value)
{
    g_use_item_select_return_mode = value;
}
// FUNCTION: WIZ8 0x0059CF40
void InvalidateUseItemSelectPanel(void)
{
    g_use_item_select_panels[1]->Invalidate(0);
}

/* fItemSelectMode per-frame update: keep the scroll buttons synced, run the
   dirty panels' redraw pass, then re-check the pending commit through the
   shared helper. */
// FUNCTION: WIZ8 0x0059CF50
void UpdateUseItemSelect(bool active)
{
    bool panel_dirty;
    int i;

    panel_dirty = false;
    UpdateUseItemScrollButtons();
    for (i = 0; i < 3; i++) {
        if (g_use_item_select_panels[i]->m_fEnabled) {
            if (active) {
                g_use_item_select_panels[i]->Invalidate(0);
            }
            if (i == 1) {
                panel_dirty = g_use_item_select_panels[1]->m_fEnabled &&
                              (g_use_item_select_panels[1]->m_fDirty ||
                               g_use_item_select_panels[1]->m_fLayoutDirty);
            }
            g_use_item_select_panels[i]->Redraw();
            if (panel_dirty) {
                RedrawTextBoxScrollChrome();
            }
        }
    }
    CommitSelectedItemUse();
}

/* Keep the scroll buttons' enabled and secondary states in step with the
   list: a single-entry list locks both, combat shows the secondary-state
   paging instead of plain enabling. */
// FUNCTION: WIZ8 0x0059D070
void UpdateUseItemScrollButtons(void)
{
    if (g_use_item_select_mode == -1 && g_use_item_list_count == 1) {
        g_use_item_select_scroll_buttons[0]->SetEnabled(false);
        g_use_item_select_scroll_buttons[1]->SetEnabled(false);
        return;
    }
    if (gXStatus.fCombatMode) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                       g_W8TextControlStateSecondary) == 0) {
            g_use_item_select_scroll_buttons[0]->EnableSecondaryState(false);
            g_use_item_select_scroll_buttons[0]->Invalidate(false);
            if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                           g_W8TextControlStateSecondary) != 0) {
                if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                               g_W8TextControlStateSecondary) != 0) {
                    g_use_item_select_scroll_buttons[1]->DisableSecondaryState(false);
                    g_use_item_select_scroll_buttons[1]->Invalidate(false);
                }
                RebuildUseItemSelectList(0, 0);
            } else {
                g_use_item_select_scroll_buttons[0]->EnableSecondaryState(false);
            }
        }
        if (g_use_item_select_scroll_buttons[1]->m_enabled) {
            g_use_item_select_scroll_buttons[1]->SetEnabled(false);
            g_use_item_select_scroll_buttons[1]->Invalidate(false);
        }
        return;
    }
    if (!g_use_item_select_scroll_buttons[1]->m_enabled) {
        g_use_item_select_scroll_buttons[1]->SetEnabled(true);
        g_use_item_select_scroll_buttons[1]->Invalidate(false);
    }
    if (!g_use_item_select_scroll_buttons[0]->m_enabled) {
        g_use_item_select_scroll_buttons[0]->SetEnabled(true);
        g_use_item_select_scroll_buttons[0]->Invalidate(false);
    }
}

// FUNCTION: WIZ8 0x0059D790
static void UseItemSelectScrollUp(void)
{
    if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                       g_W8TextControlStateSecondary) != 0) {
            g_use_item_select_scroll_buttons[1]->DisableSecondaryState(false);
            g_use_item_select_scroll_buttons[1]->Invalidate(false);
        }
        RebuildUseItemSelectList(0, 0);
        return;
    }
    g_use_item_select_scroll_buttons[0]->EnableSecondaryState(false);
}

// FUNCTION: WIZ8 0x0059D7E0
static void UseItemSelectScrollDown(void)
{
    if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                   g_W8TextControlStateSecondary) != 0) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                       g_W8TextControlStateSecondary) != 0) {
            g_use_item_select_scroll_buttons[0]->DisableSecondaryState(false);
            g_use_item_select_scroll_buttons[0]->Invalidate(false);
        }
        RebuildUseItemSelectList(1, 0);
        return;
    }
    g_use_item_select_scroll_buttons[1]->EnableSecondaryState(false);
}

// FUNCTION: WIZ8 0x0059D830
static void UseItemSelectFilterToggle(void)
{
    if (static_cast<unsigned char>(g_use_item_select_controls[3]->m_stateFlags &
                                   g_W8TextControlStateSecondary) == 0) {
        g_use_item_select_controls[3]->EnableSecondaryState(true);
        g_use_item_select_controls[3]->Invalidate(false);
    }
    g_use_item_select_flags |= 1;
}

// FUNCTION: WIZ8 0x0059D860
static void UseItemSelectAssayButton(void)
{
    if (g_use_item_detail_item != 0 && g_use_item_select_controls[0]->m_imageObject != -1) {
        OpenUseItemAssayDialog(g_use_item_detail_item);
    }
}

/* Commit the pending use-item action: while the selected item is still usable
   by the owner and its recorded target suits it, commit the target and aim
   the item use. Spell 0x17 keeps the view open for the follow-up pick;
   anything else closes it. */
// FUNCTION: WIZ8 0x0059D180
void CommitSelectedItemUse(void)
{
    W8Character* character;

    if (g_use_item_selected != 0 &&
        CanUseItemForAction(g_status.selected_character, g_use_item_selected) &&
        IsItemTargetOfNeededKind(g_status.selected_character, g_use_item_selected)) {
        character = &g_status.buffers.Char[g_status.selected_character];
        if (g_use_item_selected != 0) {
            g_use_item_commit_active = true;
            CommitSelectedSpellTarget();
            g_use_item_commit_active = false;
            AimItemUseAtCurrentTarget(character, g_use_item_selected);
            if (g_use_item_selected != 0 && g_use_item_selected->iItemNo != -1 &&
                GetItemSpell(g_use_item_selected) == 0x17) {
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
   then the rest) through AppendUseItemListEntry; select re-highlights
   the line holding that item. */
// FUNCTION: WIZ8 0x0059D230
static void RebuildUseItemSelectList(int mode, W8ItemInstance* select)
{
    W8Character* character;
    unsigned char pass;
    unsigned int i;
    int slot;

    slot = g_status.selected_character;
    ResetEditorStatusLine(2);
    ClearSelectedTextLine(2);
    g_use_item_selected = 0;
    g_use_item_detail_item = 0;
    g_use_item_select_controls[0]->ClearImage();
    g_use_item_select_controls[0]->m_textBuffer.SetText(g_dialogue_empty_text, 0);
    g_use_item_select_controls[0]->Invalidate(true);
    SelectSpellCastingPartySlot(g_status.selected_character);
    g_use_item_select_mode = mode;
    if (mode == -1) {
        for (i = 1; i <= 6; i++) {
            g_use_item_select_controls[i]->SetEnabled(false);
        }
        return;
    }
    g_use_item_select_controls[3]->SetEnabled(true);
    UseItemSelectFilterToggle();
    g_use_item_list_count = 0;
    g_selected_use_item_line = -1;
    pass = 0;
    if (mode != 0) {
        do {
            for (i = 0; i < g_status.party_item_count; i++) {
                if (!AppendUseItemListEntry(&g_status.party_item_pool[i], select, pass)) {
                    return;
                }
            }
            ScrollTextBoxTo(0);
            pass++;
        } while (pass < 2);
        InvalidateUseItemSelectPanel();
        return;
    }
    character = &g_status.buffers.Char[slot];
    do {
        for (i = 0; i < 12; i++) {
            if (!AppendUseItemListEntry(&character->EquippedItem[i], select, pass)) {
                return;
            }
        }
        for (i = 0; i < 8; i++) {
            if (!AppendUseItemListEntry(&character->backpack[i], select, pass)) {
                return;
            }
        }
        pass++;
    } while (pass < 2);
    ScrollTextBoxTo(0);
    InvalidateUseItemSelectPanel();
}

/* Append one item to the use-item list under the active filter. Pass 0 takes
   items the owner can use; pass 1 takes the rest outside camp/lock/trap
   contexts (where only the usable pass runs). Entries store the item, select
   it when it matches select, and show a display name with stack/charge counts
   or the unidentified "(?)". Returns false when the list is full. */
// FUNCTION: WIZ8 0x0059D450
static bool AppendUseItemListEntry(W8ItemInstance* item, W8ItemInstance* select, unsigned char pass)
{
    wchar_t* text;
    unsigned int color;
    short count;
    bool charged;

    if (item->iItemNo == -1) {
        return true;
    }
    if (IsUseItemFilteredOut(item)) {
        return true;
    }
    if (pass != 0 || !CanUseItemForAction(g_use_item_owner_index, item)) {
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
        if (CanUseItemForAction(g_use_item_owner_index, item)) {
            return true;
        }
        if (g_use_item_list_count + 1U > 0x15e) {
            g_selected_use_item_line = g_use_item_list_count;
            return false;
        }
        if (select != 0 && select == item) {
            g_selected_use_item_line = g_use_item_list_count;
        }
        g_use_item_list[g_use_item_list_count] = item;
        if (g_item_records[item->iItemNo].equip_class != W8_ITEM_EQUIP_CLASS_INSTRUMENT ||
            g_status.buffers.Char[g_use_item_owner_index].uiCondition[W8_CONDITION_SILENCED] == 0) {
            color = 0;
        } else {
            color = 4;
        }
        ShowNotice(color, FormatItemDisplayName(item, true), 2);
        ++g_use_item_list_count;
        return true;
    }
    if (g_use_item_list_count + 1U > 0x15e) {
        g_selected_use_item_line = g_use_item_list_count;
        return false;
    }
    if (select != 0 && select == item) {
        g_selected_use_item_line = g_use_item_list_count;
    }
    g_use_item_list[g_use_item_list_count] = item;
    charged = false;
    switch (g_item_records[item->iItemNo].quantity_kind) {
    case W8_ITEM_QUANTITY_STACK:
        count = item->stack_count;
        break;
    case W8_ITEM_QUANTITY_CHARGES:
    case W8_ITEM_QUANTITY_USES:
        if (!item->identified) {
            ShowNotice(
                0xf,
                FormatWideString(g_format_s_paren_question, FormatItemDisplayName(item, false)), 2);
            ++g_use_item_list_count;
            return true;
        }
        charged = true;
        count = item->uses_or_charges;
        break;
    case W8_ITEM_QUANTITY_SHOTS:
        charged = true;
        count = item->uses_or_charges;
        break;
    default:
        ShowNotice(W8_FONT_PALETTE_TEXT_BOX, FormatItemDisplayName(item, false), 2);
        ++g_use_item_list_count;
        return true;
    }
    if (count == -1) {
        text = FormatWideString(g_format_s_paren_question, FormatItemDisplayName(item, false));
    } else if (count > 1 || charged) {
        text = FormatWideString(g_format_s_paren_d, FormatItemDisplayName(item, false), count);
    } else {
        text = FormatItemDisplayName(item, false);
    }
    ShowNotice(W8_FONT_PALETTE_TEXT_BOX, text, 2);
    ++g_use_item_list_count;
    return true;
}

/* Filter predicate for AppendUseItemListEntry: while the select flags
   are set, entries the character cannot activate or cast from are dropped,
   except usable-class items stay unless the slot's bound NPC wants the
   item. */
// FUNCTION: WIZ8 0x0059D6B0
bool IsUseItemFilteredOut(W8ItemInstance* item)
{
    W8NpcState* npc;

    if (g_use_item_select_flags == 0) {
        return false;
    }
    if (item->iItemNo == -1) {
        return true;
    }
    if (!CanCharacterActivateItem(&g_status.buffers.Char[g_status.selected_character], item)) {
        if (CanCastFromItem(&g_status.buffers.Char[g_status.selected_character], item)) {
            return false;
        }
        if (!IsUsableItemClass(item)) {
            return true;
        }
        if ((g_status.selected_character == 0 || g_status.selected_character == 1) &&
            (npc = GetNpcState(g_status.buffers.XChar[g_status.selected_character].npc_index),
             npc != 0) &&
            NpcWantsItem(npc, item)) {
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
    if (g_use_item_select_mode != -1) {
        RebuildUseItemSelectList(g_use_item_select_mode, 0);
        RequestRedraw(W8_MAIN_REDRAW_LAYOUT);
    }
}

/* Right release on the use-item text box: open the assay dialog for the item
   on the clicked row. The target cursor is saved so the destroy callback can
   restore it. */
// FUNCTION: WIZ8 0x0059D880
void OpenUseItemAssayDialog(W8ItemInstance* item)
{
    W8AssayDialog* dialog;

    g_saved_target_cursor = gXStatus.iCurrentCursor;
    dialog = new W8AssayDialog(item, &g_status.buffers.Char[g_use_item_owner_index]);
    dialog->SetText(&g_empty_wide_string);
    dialog->SetOrigin(g_info_dialog_x, 0x48);
    dialog->m_destroy_callback = RestoreTargetCursor;
    OpenModal(dialog);
}

/* Destroy callback for the item-assay and spell-info dialogs: put back the
   cursor that was current before the dialog opened and repaint everything. */
// FUNCTION: WIZ8 0x0059D930
void RestoreTargetCursor(W8DialogBase*)
{
    SetTargetCursor(g_saved_target_cursor);
    RequestRedraw(W8_MAIN_REDRAW_ALL);
}

// FUNCTION: WIZ8 0x0059D950
void CloseUseItemSelection(void)
{
    CloseUseItemSelectView();
    ClearSlotTargeting(g_status.selected_character);
}

/* Scroll up/down buttons for the use-item list (catalog callback_ids 0 and 1). */
// FUNCTION: WIZ8 0x0059D970
unsigned char UseItemSelectScrollRegionEvent(const InputAtom* event, W8Region* region)
{
    return DispatchButtonRegionEvent(event, region,
                                     g_use_item_select_scroll_buttons[region->callback_id]);
}

/* Use-item select action/icon controls (catalog callback_ids 0, 3, 8). */
// FUNCTION: WIZ8 0x0059DA30
unsigned char UseItemSelectControlRegionEvent(const InputAtom* event, W8Region* region)
{
    switch (event->usEvent) {
    case RIGHT_BUTTON_DOWN:
        g_use_item_select_controls[region->callback_id]->OnRightButtonDown(0);
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_UP:
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            g_use_item_select_controls[region->callback_id]->OnRightButtonUp(0);
            region->flags &= ~W8_REGION_RIGHT_BUTTON_HELD;
        }
        return 1;
    default:
        return DispatchButtonRegionEvent(event, region,
                                         g_use_item_select_controls[region->callback_id]);
    }
}

/* Use-item text-box body region event: button presses only arm the held bits
   and all action happens on release - left release commits the slot's line,
   right release opens the assay dialog for the item under the cursor. Motion
   keeps g_use_item_cursor_x/y as the last cursor point and tracks the hovered
   row into g_use_item_hover_row. */
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
        slot = GetHoveredTextLine(2);
        if (slot == -1) {
            return 1;
        }
        SelectUseItemLine(slot);
        return 1;
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        return 1;
    case RIGHT_BUTTON_DOWN:
        region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
        return 1;
    case MOUSE_POS:
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            ClearHoveredTextLine(2);
            g_use_item_hover_row = -1;
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_use_item_cursor_x = static_cast<unsigned short>(event->uiParam);
            g_use_item_cursor_y = static_cast<unsigned short>(event->uiParam >> 16);
            return 1;
        }
        if (static_cast<unsigned short>(event->uiParam) == g_use_item_cursor_x &&
            static_cast<int>(event->uiParam >> 16) == g_use_item_cursor_y) {
            return 1;
        }
        g_use_item_cursor_x = static_cast<unsigned short>(event->uiParam);
        g_use_item_cursor_y = static_cast<unsigned short>(event->uiParam >> 16);
        y = static_cast<unsigned short>(event->uiParam >> 16);
        if (g_level_block->text_box_top <= y && y <= g_level_block->text_box_bottom) {
            row = (y - g_level_block->text_box_top) / 0xb;
            if (row != g_use_item_hover_row) {
                ClearHoveredTextLine(2);
                if (row < static_cast<int>(g_status.text_box_lines_shown[2])) {
                    SetHoveredTextLine(g_level_block->text_lines[2] + row, 2);
                }
                RedrawTextBox();
            }
            g_use_item_hover_row = row;
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
        if (row < static_cast<int>(g_status.text_box_lines_shown[2]) &&
            g_use_item_list[g_level_block->text_lines[2] + row] != 0) {
            OpenUseItemAssayDialog(g_use_item_list[g_level_block->text_lines[2] + row]);
        }
        return 1;
    }
    return 0;
}

/* Wheel rotation over the use-item text box re-selects the row under the
   cursor, even when it is already the hovered row. */
// FUNCTION: WIZ8 0x0059DD30
void UseItemSelectTextBoxWheelAt(short x, unsigned short y, bool flag)
{
    int row;

    if (y < g_level_block->text_box_top || g_level_block->text_box_bottom < y) {
        return;
    }
    row = (y - g_level_block->text_box_top) / 0xb;
    if (row != g_use_item_hover_row || flag) {
        ClearHoveredTextLine(2);
        if (row < static_cast<int>(g_status.text_box_lines_shown[2])) {
            SetHoveredTextLine(g_level_block->text_lines[2] + row, 2);
        }
        RedrawTextBox();
    }
    g_use_item_hover_row = row;
}

/* Left-click commit of a use-item list row: validate the item's spell for
   this owner, learn it directly when castable-from-item, otherwise point the
   targeting filter at the embedded spell. */
// FUNCTION: WIZ8 0x0059DDC0
void SelectUseItemLine(int iTextLine)
{
    W8SpellTargetType target_type;

    if (iTextLine < 0) {
        srAssertFail("iTextLine >= 0", MGSUSEITEMSELECT_CPP, 0x61d, 0);
    }
    if (iTextLine >= g_use_item_list_count) {
        // c-style-cast-ok: the assertion text itself spells (INT32)
        srAssertFail("iTextLine < (INT32) guiNumItemsInList", MGSUSEITEMSELECT_CPP, 0x61e, 0);
    }
    SetTargetingMode(W8_TARGET_NEED_NONE);
    UpdateUseItemDetailPanel(g_use_item_list[iTextLine]);
    if (ValidateItemSpellUse(g_use_item_owner_index, g_use_item_list[iTextLine],
                             SpellCastingNoticeClosed)) {
        QueueCharacterEvent(&g_status.buffers.Char[g_use_item_owner_index], g_character_event_kind2,
                            0, g_character_event_flags_mask | g_character_event_no_flags,
                            g_character_event_full_volume);
        return;
    }
    g_use_item_selected = g_use_item_list[iTextLine];
    if (Trynnie2UseItem(g_use_item_selected)) {
        CloseUseItemSelectView();
        return;
    }
    if (IsUsableItemClass(g_use_item_selected)) {
        TakeUseItemIntoHand();
        CloseUseItemSelectView();
        return;
    }
    if (CanCastFromItem(&g_status.buffers.Char[g_status.selected_character], g_use_item_selected)) {
        LearnSpellFromItem(&g_status.buffers.Char[g_status.selected_character],
                           g_use_item_selected);
        CloseUseItemSelectView();
        return;
    }
    SetSelectedTextLine(iTextLine, 2);
    ClearHoveredTextLine(2);
    RedrawTextBox();
    target_type = GetSpellTargetType(
        GetItemSpell(g_use_item_selected),
        ItemClassNormalizesTarget(&g_item_records[g_use_item_selected->iItemNo]));
    ConfigureSpellTargetFilter(target_type, GetTargetNeededForItem(g_use_item_selected));
}

/* Show the item in the detail control: its catalog icon plus a quantity line -
   stack counts only past one, uses/charges always once the item is identified,
   '?' while it still is not. */
// FUNCTION: WIZ8 0x0059DFA0
void UpdateUseItemDetailPanel(W8ItemInstance* item)
{
    wchar_t text[0x20];
    const wchar_t* value;
    short count;
    bool charges = false;

    g_use_item_select_controls[0]->SetImage(
        g_item_video_objects.GetOrCreateVideoObject(item->iItemNo));
    count = 0;
    switch (g_item_records[item->iItemNo].quantity_kind) {
    case W8_ITEM_QUANTITY_STACK:
        count = item->stack_count;
        break;
    case W8_ITEM_QUANTITY_CHARGES:
    case W8_ITEM_QUANTITY_USES:
        charges = true;
        if (!item->identified) {
            count = -1;
        } else {
            count = item->uses_or_charges;
        }
        break;
    case W8_ITEM_QUANTITY_SHOTS:
        charges = true;
        count = item->uses_or_charges;
        break;
    }
    if (count == -1) {
        value = L"?";
    } else if (count > 1 || charges) {
        swprintf(text, g_format_d, count);
        value = text;
    } else {
        value = g_dialogue_empty_text;
    }
    g_use_item_select_controls[0]->m_textBuffer.SetText(value, g_wiz_text_font_secondary);
    g_use_item_select_controls[0]->Invalidate(true);
    g_use_item_detail_item = item;
}

// FUNCTION: WIZ8 0x0059E0D0
W8ItemInstance* GetSelectedOrFallbackValue(void)
{
    W8ItemInstance* value = g_use_item_select_override_item;
    if (value == 0) {
        value = g_use_item_selected;
    }
    return value;
}

// FUNCTION: WIZ8 0x0059E0E0
void SelectCurrentUseItemLine(void)
{
    SelectUseItemLine(g_selected_use_item_line);
}

/* Move the selected item into the cursor hand. When the hand is already
   holding an item that one goes back to the owner or party first; if the
   store shifted the party pool, the selection is re-anchored onto the entry
   the move slid it to before the hand copy is refreshed. */
// FUNCTION: WIZ8 0x0059E0F0
void TakeUseItemIntoHand(void)
{
    unsigned int i;
    unsigned int old_count;

    if (!g_status.item_in_cursor) {
        CopyItemInstance(&g_status.item_in_hand, g_use_item_selected, 0, true);
        return;
    }
    if (g_use_item_owner_index == -1) {
        srAssertFail("giUseItemChar != BAD_INDEX", MGSUSEITEMSELECT_CPP, 0x6c1, 0);
    }
    old_count = g_status.party_item_count;
    GiveItemToCharacterOrParty(g_use_item_owner_index, &g_status.item_in_hand, true);
    if (g_use_item_select_mode == 1 && old_count != g_status.party_item_count &&
        g_status.party_item_count != 0) {
        for (i = 0; i < g_status.party_item_count; i++) {
            if (g_use_item_selected == &g_status.party_item_pool[i]) {
                g_use_item_selected = &g_status.party_item_pool[i + 1];
                CopyItemInstance(&g_status.item_in_hand, g_use_item_selected, 0, true);
                return;
            }
        }
    }
    CopyItemInstance(&g_status.item_in_hand, g_use_item_selected, 0, true);
}

// FUNCTION: WIZ8 0x0059E1E0
void SetUseItemSelectOverrideItem(W8ItemInstance* value)
{
    g_use_item_select_override_item = value;
}
