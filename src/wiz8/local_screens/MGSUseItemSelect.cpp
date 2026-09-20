#include <wchar.h>
#include "wiz8/fonts.h"
#include "wiz8/item_video_object_vector.h"
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
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
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
// GLOBAL: WIZ8 0x0069B98C
int g_use_item_flag_0069b98c;
/* 0x00614B54: seven-glyph placeholder the quantity text shows for
   unidentified charge-count items. */
// GLOBAL: WIZ8 0x00614B54
const wchar_t g_unidentified_quantity_00614b54[] = {0x06a0, 0x06a1, 0x06a2, 0x06a3,
                                                    0x06a4, 0x06a5, 0x06a6, 0};

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
// GLOBAL: WIZ8 0x0069BF2C
W8ItemInstance* g_use_item_display_item_0069bf2c;
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
    UpdateUseItemIcon0059DFA0(g_use_item_list_0069b9b4[iTextLine]);
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
        MoveUsedItemToCursor0059E0F0();
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

/* Refresh the use-item icon control with the item's video object and
   quantity text, and record it as the displayed item. Stackable items show
   their stack count when it is not 0 or 1; charge-based items show their
   remaining charges, or the placeholder glyphs while unidentified. */
// FUNCTION: WIZ8 0x0059DFA0
void UpdateUseItemIcon0059DFA0(W8ItemInstance* item)
{
    wchar_t quantity_text[32];
    const wchar_t* text;
    unsigned short value;
    unsigned char show_charges;

    show_charges = 0;
    text = 0;
    g_use_item_select_controls[0]->m_imageObject =
        g_item_video_objects_68ec68.GetOrCreateVideoObject(item->item_id);
    g_use_item_select_controls[0]->m_measured_w = -1;
    g_use_item_select_controls[0]->m_measured_h = -1;
    g_use_item_select_controls[0]->m_imageFrame = 0;
    g_use_item_select_controls[0]->m_normalSprite = 0;
    g_use_item_select_controls[0]->m_pressedSprite = 0;
    switch (g_item_records[item->item_id].quantity_kind) {
    case 1:
        value = item->stack_count;
        break;
    case 2:
    case 3:
        if (item->identified == 0) {
            text = g_unidentified_quantity_00614b54;
        } else {
            show_charges = 1;
            value = item->uses_or_charges;
        }
        break;
    case 4:
        show_charges = 1;
        value = item->uses_or_charges;
        break;
    default:
        text = g_wchar_0068ee58;
        break;
    }
    if (text == 0) {
        if (value == 0xffff) {
            text = g_unidentified_quantity_00614b54;
        } else if (value > 1 || show_charges != 0) {
            swprintf(quantity_text, g_format_d_0060aa20, value);
            text = quantity_text;
        } else {
            text = g_wchar_0068ee58;
        }
    }
    g_use_item_select_controls[0]->m_textBuffer.SetText(text, g_font_683660);
    g_use_item_select_controls[0]->Invalidate(1);
    g_use_item_display_item_0069bf2c = item;
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

/* Commit the selected use-item onto the cursor. When the cursor already
   holds an item it is first stowed on the owner (or the party pool); if the
   pool shifted underneath it, the selected pointer is re-resolved to the
   following pool slot. */
// FUNCTION: WIZ8 0x0059E0F0
void MoveUsedItemToCursor0059E0F0(void)
{
    int old_count;
    int i;

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
    if (g_use_item_flag_0069b98c == 1 && old_count != g_status_685170.party_item_count_1791 &&
        g_status_685170.party_item_count_1791 != 0) {
        for (i = 0; i < g_status_685170.party_item_count_1791; i++) {
            if (g_value_69b9a0 == &g_status_685170.party_item_pool_0021[i]) {
                g_value_69b9a0 = &g_status_685170.party_item_pool_0021[i + 1];
                break;
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
