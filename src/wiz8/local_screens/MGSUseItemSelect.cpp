#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/fonts.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/level_specific_code/Trynnie2.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/regions.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"

#define MGSUSEITEMSELECT_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSUseItemSelect.cpp"

// GLOBAL: WIZ8 0x0069b994
Controls* g_use_item_select_panels_69b994[3];
// GLOBAL: WIZ8 0x0069b988
int g_value_69b988;

// GLOBAL: WIZ8 0x0069B950
W8TextControl* g_use_item_select_scroll_buttons[2];
// GLOBAL: WIZ8 0x0069B95C
int g_selected_use_item_line_0069b95c;
// GLOBAL: WIZ8 0x0069B960
W8TextControl* g_use_item_select_controls[9];
// GLOBAL: WIZ8 0x0069B9A0
W8ItemInstance* g_value_69b9a0;
// GLOBAL: WIZ8 0x0069B9A4
W8ItemInstance* g_value_69b9a4;
// GLOBAL: WIZ8 0x0069B98C
int g_use_item_select_mode_0069b98c;
// GLOBAL: WIZ8 0x0069B990
int g_use_item_list_count_0069b990;
// GLOBAL: WIZ8 0x0069B9A8
int g_use_item_cursor_x_0069b9a8;
// GLOBAL: WIZ8 0x0069B9AC
int g_use_item_cursor_y_0069b9ac;
// GLOBAL: WIZ8 0x0069B984
int g_use_item_select_flags_0069b984;
// GLOBAL: WIZ8 0x0069B9B0
int g_use_item_owner_index_0069b9b0;
// GLOBAL: WIZ8 0x0069B9B4
W8ItemInstance* g_use_item_list_0069b9b4[0x15e];
// GLOBAL: WIZ8 0x0069BF2C
W8ItemInstance* g_use_item_detail_item_0069bf2c;
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
void SetValue69B988(int value)
{
    g_value_69b988 = value;
}
// FUNCTION: WIZ8 0x0059CF40
void RedrawPanel69B998(void)
{
    g_use_item_select_panels_69b994[1]->Invalidate(0);
}

/* fItemSelectMode per-frame update: keep the scroll buttons synced, run the
   dirty panels' redraw pass, then re-check the pending commit — the same
   sequence CommitSelectedItemUse performs, inlined here by VC6. */
// FUNCTION: WIZ8 0x0059CF50
void UpdateUseItemSelect0059CF50(unsigned char active)
{
    W8Character* character;
    bool panel_dirty;
    int i;

    panel_dirty = false;
    UpdateUseItemScrollButtons0059D070();
    for (i = 0; i < 3; i++) {
        if (g_use_item_select_panels_69b994[i]->m_fEnabled) {
            if (active) {
                g_use_item_select_panels_69b994[i]->Invalidate(0);
            }
            if (i == 1) {
                panel_dirty = g_use_item_select_panels_69b994[1]->m_fEnabled != 0 &&
                              (g_use_item_select_panels_69b994[1]->m_fDirty != 0 ||
                               g_use_item_select_panels_69b994[1]->m_fLayoutDirty != 0);
            }
            g_use_item_select_panels_69b994[i]->Redraw();
            if (panel_dirty) {
                RedrawTextBoxScrollChrome();
            }
        }
    }
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

/* Keep the scroll buttons' enabled and secondary states in step with the
   list: a single-entry list locks both, combat shows the secondary-state
   paging instead of plain enabling. */
// FUNCTION: WIZ8 0x0059D070
void UpdateUseItemScrollButtons0059D070(void)
{
    if (g_use_item_select_mode_0069b98c == -1 && g_use_item_list_count_0069b990 == 1) {
        g_use_item_select_scroll_buttons[0]->SetEnabled(0);
        g_use_item_select_scroll_buttons[1]->SetEnabled(0);
        return;
    }
    if (gXStatus.fCombatMode) {
        if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                       g_W8TextControlMask005ED570) == 0) {
            g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
            g_use_item_select_scroll_buttons[0]->Invalidate(0);
            if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[0]->m_stateFlags &
                                           g_W8TextControlMask005ED570) != 0) {
                if (static_cast<unsigned char>(g_use_item_select_scroll_buttons[1]->m_stateFlags &
                                               g_W8TextControlMask005ED570) != 0) {
                    g_use_item_select_scroll_buttons[1]->DisableSecondaryState(0);
                    g_use_item_select_scroll_buttons[1]->Invalidate(0);
                }
                RebuildUseItemSelectList0059D230(0, 0);
            } else {
                g_use_item_select_scroll_buttons[0]->EnableSecondaryState(0);
            }
        }
        if (g_use_item_select_scroll_buttons[1]->m_enabled) {
            g_use_item_select_scroll_buttons[1]->SetEnabled(0);
            g_use_item_select_scroll_buttons[1]->Invalidate(0);
        }
        return;
    }
    if (!g_use_item_select_scroll_buttons[1]->m_enabled) {
        g_use_item_select_scroll_buttons[1]->SetEnabled(1);
        g_use_item_select_scroll_buttons[1]->Invalidate(0);
    }
    if (!g_use_item_select_scroll_buttons[0]->m_enabled) {
        g_use_item_select_scroll_buttons[0]->SetEnabled(1);
        g_use_item_select_scroll_buttons[0]->Invalidate(0);
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
    UpdateUseItemDetailPanel0059DFA0(g_use_item_list_0069b9b4[iTextLine]);
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
        TakeUseItemIntoHand0059E0F0();
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

/* Show the item in the detail control: its catalog icon plus a quantity line -
   stack counts only past one, uses/charges always once the item is identified,
   '?' while it still is not. */
// FUNCTION: WIZ8 0x0059DFA0
void UpdateUseItemDetailPanel0059DFA0(W8ItemInstance* item)
{
    wchar_t text[0x20];
    const wchar_t* value;
    short count;
    bool charges = false;

    g_use_item_select_controls[0]->m_imageObject =
        g_item_video_objects_68ec68.GetOrCreateVideoObject(item->item_id);
    g_use_item_select_controls[0]->m_measured_w = -1;
    g_use_item_select_controls[0]->m_measured_h = -1;
    g_use_item_select_controls[0]->m_imageFrame = 0;
    g_use_item_select_controls[0]->m_normalSprite = 0;
    g_use_item_select_controls[0]->m_pressedSprite = 0;
    count = 0;
    switch (g_item_records[item->item_id].quantity_kind) {
    case 1:
        count = item->stack_count;
        break;
    case 2:
    case 3:
        charges = true;
        if (item->identified == 0) {
            count = -1;
        } else {
            count = item->uses_or_charges;
        }
        break;
    case 4:
        charges = true;
        count = item->uses_or_charges;
        break;
    }
    if (count == -1) {
        value = L"?";
    } else if (count > 1 || charges) {
        swprintf(text, g_format_d_0060aa20, count);
        value = text;
    } else {
        value = g_wchar_0068ee58;
    }
    g_use_item_select_controls[0]->m_textBuffer.SetText(value, g_font_683660);
    g_use_item_select_controls[0]->Invalidate(1);
    g_use_item_detail_item_0069bf2c = item;
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

/* Move the selected item into the cursor hand. When the hand is already
   holding an item that one goes back to the owner or party first; if the
   store shifted the party pool, the selection is re-anchored onto the entry
   the move slid it to before the hand copy is refreshed. */
// FUNCTION: WIZ8 0x0059E0F0
void TakeUseItemIntoHand0059E0F0(void)
{
    unsigned int i;
    int old_count;

    if (g_status_685170.item_in_cursor == 0) {
        CopyItemInstance(&g_status_685170.item_in_hand_235b, g_value_69b9a0, 0, 1);
        return;
    }
    if (g_use_item_owner_index_0069b9b0 == -1) {
        srAssertFail("giUseItemChar != BAD_INDEX", MGSUSEITEMSELECT_CPP, 0x6c1, 0);
    }
    old_count = g_status_685170.party_item_count_1791;
    GiveItemToCharacterOrParty(g_use_item_owner_index_0069b9b0, &g_status_685170.item_in_hand_235b,
                               1);
    if (g_use_item_select_mode_0069b98c == 1 &&
        old_count != g_status_685170.party_item_count_1791 &&
        g_status_685170.party_item_count_1791 != 0) {
        for (i = 0; i < static_cast<unsigned int>(g_status_685170.party_item_count_1791); i++) {
            if (g_value_69b9a0 == &g_status_685170.party_item_pool_0021[i]) {
                g_value_69b9a0 = &g_status_685170.party_item_pool_0021[i + 1];
                CopyItemInstance(&g_status_685170.item_in_hand_235b, g_value_69b9a0, 0, 1);
                return;
            }
        }
    }
    CopyItemInstance(&g_status_685170.item_in_hand_235b, g_value_69b9a0, 0, 1);
}

// FUNCTION: WIZ8 0x0059E1E0
void SetValue69B9A4(W8ItemInstance* value)
{
    g_value_69b9a4 = value;
}
