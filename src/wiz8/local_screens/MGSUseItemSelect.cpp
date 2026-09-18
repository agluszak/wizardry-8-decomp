#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/MGSUseItemSelect.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_screens/MainGameScreen.h"
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
int g_value_69b988;

// GLOBAL: WIZ8 0x0069B950
W8TextControl* g_use_item_select_scroll_buttons[2];
// GLOBAL: WIZ8 0x0069B95C
int g_selected_use_item_line_0069b95c;
// GLOBAL: WIZ8 0x0069B960
W8TextControl* g_use_item_select_controls[9];
// GLOBAL: WIZ8 0x0069B9A0
int g_value_69b9a0;
// GLOBAL: WIZ8 0x0069B9A4
int g_value_69b9a4;
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

// FUNCTION: WIZ8 0x0059CF30
void SetValue69B988(int value)
{
    g_value_69b988 = value;
}
// FUNCTION: WIZ8 0x0059CF40
void RedrawPanel69B998(void)
{
    g_panel_69b998->Invalidate(0);
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
    // reinterpret-ok: the selected use-item value is stored as an int
    g_value_69b9a0 = reinterpret_cast<int>(g_use_item_list_0069b9b4[iTextLine]);
    // reinterpret-ok: int-typed use-item slot back to the stored item pointer
    if (Function4D9F60(reinterpret_cast<W8ItemInstance*>(g_value_69b9a0)) != 0) {
        CloseUseItemSelectView();
        return;
    }
    // reinterpret-ok: int-typed use-item slot back to the stored item pointer
    if (IsUsableItemClass00522A00(reinterpret_cast<W8ItemInstance*>(g_value_69b9a0)) != 0) {
        Function59E0F0();
        CloseUseItemSelectView();
        return;
    }
    if (CanCastFromItem(&g_status_685170.buffers.characters[g_status_685170.selected_character],
                        // reinterpret-ok: int-typed use-item slot back to the stored item pointer
                        reinterpret_cast<W8ItemInstance*>(g_value_69b9a0))) {
        LearnSpellFromItem(&g_status_685170.buffers.characters[g_status_685170.selected_character],
                           // reinterpret-ok: int-typed use-item slot back to the stored item pointer
                           reinterpret_cast<W8ItemInstance*>(g_value_69b9a0));
        CloseUseItemSelectView();
        return;
    }
    SelectTextSlot1E8(iTextLine, 2);
    ClearTextSlot1D8(2);
    RedrawTextBox();
    target_type = GetSpellTargetType(
        // reinterpret-ok: int-typed use-item slot back to the stored item pointer
        GetItemSpell(reinterpret_cast<W8ItemInstance*>(g_value_69b9a0)),
        ItemClassNormalizesTarget(
            // reinterpret-ok: int-typed use-item slot back to the stored item pointer
            &g_item_records[reinterpret_cast<W8ItemInstance*>(g_value_69b9a0)->item_id]));
    ConfigureSpellTargetFilter(
        target_type,
        // reinterpret-ok: int-typed use-item slot back to the stored item pointer
        GetTargetNeededForItem(reinterpret_cast<W8ItemInstance*>(g_value_69b9a0)));
}

// FUNCTION: WIZ8 0x0059E0D0
int GetSelectedOrFallbackValue0059E0D0(void)
{
    int value = g_value_69b9a4;
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
void SetValue69B9A4(int value)
{
    g_value_69b9a4 = value;
}
