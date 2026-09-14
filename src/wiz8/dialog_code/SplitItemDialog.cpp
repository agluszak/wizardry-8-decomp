#include "wiz8/dialog_code/DialogFactoryDialogs.h"

#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/item_instance.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/screen_state.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

#include "input.h"
#include "mousesystem_macros.h"

#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>

/* The split-stack dialog ("popup_splititem.sti"). Inventory splits use eight
   buttons and ten labels; the two trade modes add a passive frame and a price
   label for each side of the transaction. The running-totals refresh is
   written out at each of the six sites that need it, matching the six inline
   copies the retail binary carries. */

struct W8SplitButtonOffset {
    int x;
    int y;
};

// GLOBAL: WIZ8 0x0064FE38
W8SplitButtonOffset g_split_button_offsets[10] = {
    {0xd9, 0x5a}, {0xbe, 0x5a}, {0xbc, 0x49},  {0xbc, 0x6f}, {0x81, 0x35},
    {0x81, 0x83}, {0xed, 0x95}, {0x111, 0x95}, {0xf4, 0x35}, {0xf4, 0x83},
};

// GLOBAL: WIZ8 0x0064FE88
W8ControlsRect g_split_text_bounds[14] = {
    {0x48, 0x49, 0xb6, 0x55},  {0x48, 0x6f, 0xb6, 0x7b},  {0xbc, 0x49, 0xef, 0x55},
    {0x48, 0x0f, 0x12a, 0x1f}, {0x48, 0x23, 0x7d, 0x2f},  {0x81, 0x23, 0x12a, 0x2f},
    {0x48, 0x35, 0x7d, 0x41},  {0x81, 0x35, 0xb7, 0x41},  {0x48, 0x83, 0x7d, 0x8f},
    {0x81, 0x83, 0xb7, 0x8f},  {0xbb, 0x35, 0xf0, 0x41},  {0xf4, 0x35, 0x12a, 0x41},
    {0xbb, 0x83, 0xf0, 0x8f},  {0x90, 0x83, 0x12a, 0x8f},
};

// GLOBAL: WIZ8 0x0064FF68
int g_split_text_string_ids[14] = {
    268, 269, 270, 270, 271, 270, 272, 270, 272, 270, 273, 270, 273, 270,
};

// GLOBAL: WIZ8 0x0064FFA0
W8ScreenRect g_split_count_field_bounds = {0xbc, 0x6f, 0xef, 0x7b};

// FUNCTION: WIZ8 0x005DCED0
W8SplitItemDialog::W8SplitItemDialog(int kind, W8ItemInstance* item, int count)
{
    int index;

    SetExtent(0x142, 0xbd);
    if (kind == 1 || kind == 2) {
        SetBackground("Data\\Dialogs\\popup_splititem.sti", 0);
    } else if (kind == 0) {
        SetBackground("Data\\Dialogs\\popup_splititem.sti", 1);
    }
    m_kind_0cc = kind;
    for (index = 0; index < 10; ++index) {
        m_buttons_054[index] = 0;
    }
    for (index = 0; index < 14; ++index) {
        m_texts_07c[index] = 0;
    }
    m_count_input_0b4 = 0;
    m_remaining_0bc = item->stack_count;
    m_stack_total_0c4 = item->stack_count;
    if (count == -1) {
        if (g_item_records[item->item_id].maximum_quantity <= 0xa) {
            split_count_0c0 = 1;
        } else {
            split_count_0c0 = item->stack_count >> 1;
        }
        m_remaining_0bc = item->stack_count - split_count_0c0;
    } else {
        split_count_0c0 = count;
        m_remaining_0bc = item->stack_count - count;
    }
    m_item_0d0 = item;
    split_result_0c8 = 0;
    m_active_input_0b8 = 0;
    m_first_draw_0d4 = 0;
}

// SYNTHETIC: WIZ8 0x005DD010
// W8SplitItemDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005DD030
W8SplitItemDialog::~W8SplitItemDialog()
{
    int index;
    int count;

    W8DialogBase::DestroyControls();
    count = 0;
    if (m_kind_0cc == 0) {
        count = 8;
    } else if (m_kind_0cc <= 2) {
        count = 10;
    }
    for (index = 0; index < count; ++index) {
        if (m_buttons_054[index] != 0) {
            delete m_buttons_054[index];
            m_buttons_054[index] = 0;
        }
    }
    count = 0;
    if (m_kind_0cc == 0) {
        count = 10;
    } else if (m_kind_0cc <= 2) {
        count = 14;
    }
    for (index = 0; index < count; ++index) {
        if (m_texts_07c[index] != 0) {
            delete m_texts_07c[index];
            m_texts_07c[index] = 0;
        }
    }
    if (m_count_input_0b4 != 0) {
        NoOp();
        ::operator delete(m_count_input_0b4);
        m_count_input_0b4 = 0;
    }
}

// FUNCTION: WIZ8 0x005DD130
int W8SplitItemDialog::CreateControls()
{
    int index;
    int count;
    wchar_t text[34];

    W8DialogBase::CreateControls();
    m_first_draw_0d4 = 1;
    split_result_0c8 = 0;
    if (!CreateButtons005DD480()) {
        m_error = 7;
        return 7;
    }
    if (!CreateTextBuffers005DD750()) {
        count = 0;
        if (m_kind_0cc == 0) {
            count = 8;
        } else if (m_kind_0cc <= 2) {
            count = 10;
        }
        for (index = 0; index < count; ++index) {
            if (m_buttons_054[index] != 0) {
                delete m_buttons_054[index];
                m_buttons_054[index] = 0;
            }
        }
        m_error = 7;
        return 7;
    }
    if (!CreateNumericInput005DDA60()) {
        count = 0;
        if (m_kind_0cc == 0) {
            count = 8;
        } else if (m_kind_0cc <= 2) {
            count = 10;
        }
        for (index = 0; index < count; ++index) {
            if (m_buttons_054[index] != 0) {
                delete m_buttons_054[index];
                m_buttons_054[index] = 0;
            }
        }
        count = 0;
        if (m_kind_0cc == 0) {
            count = 10;
        } else if (m_kind_0cc <= 2) {
            count = 14;
        }
        for (index = 0; index < count; ++index) {
            if (m_texts_07c[index] != 0) {
                delete m_texts_07c[index];
                m_texts_07c[index] = 0;
            }
        }
        m_error = 7;
        return 7;
    }
    UpdateArrowStates005DDE60();
    UpdateAcceptButton005DDEE0();
    swprintf(text, g_format_d_0060aa20, m_remaining_0bc);
    m_texts_07c[2]->SetText(text, g_font_683660);
    m_buttons_054[2]->m_dirty = 1;
    m_texts_07c[2]->m_geometryDirty = 1;
    m_count_input_0b4->SetValue(split_count_0c0);
    m_buttons_054[3]->m_dirty = 1;
    m_count_input_0b4->m_dirty = 1;
    m_count_input_0b4->m_button->m_dirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(m_item_0d0) * m_remaining_0bc) * g_float_005ed8b8);
    m_texts_07c[7]->SetText(text, g_font_683660);
    m_buttons_054[4]->m_dirty = 1;
    m_texts_07c[7]->m_geometryDirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(m_item_0d0) * split_count_0c0) * g_float_005ed8b8);
    m_texts_07c[9]->SetText(text, g_font_683660);
    m_buttons_054[5]->m_dirty = 1;
    m_texts_07c[9]->m_geometryDirty = 1;
    UpdateCostLabels005DCC00();
    return 0;
}

// FUNCTION: WIZ8 0x005DD3C0
void W8SplitItemDialog::DestroyControls()
{
    int index;
    int count;

    W8DialogBase::DestroyControls();
    count = 0;
    if (m_kind_0cc == 0) {
        count = 8;
    } else if (m_kind_0cc <= 2) {
        count = 10;
    }
    for (index = 0; index < count; ++index) {
        if (m_buttons_054[index] != 0) {
            delete m_buttons_054[index];
            m_buttons_054[index] = 0;
        }
    }
    count = 0;
    if (m_kind_0cc == 0) {
        count = 10;
    } else if (m_kind_0cc <= 2) {
        count = 14;
    }
    for (index = 0; index < count; ++index) {
        if (m_texts_07c[index] != 0) {
            delete m_texts_07c[index];
            m_texts_07c[index] = 0;
        }
    }
    if (m_count_input_0b4 != 0) {
        NoOp();
        ::operator delete(m_count_input_0b4);
        m_count_input_0b4 = 0;
    }
}

// FUNCTION: WIZ8 0x005DD480
unsigned char W8SplitItemDialog::CreateButtons005DD480()
{
    int count;
    int index;

    count = 0;
    if (m_kind_0cc == 0) {
        count = 8;
    } else if (m_kind_0cc <= 2) {
        count = 10;
    }
    for (index = 0; index < count; ++index) {
        m_buttons_054[index] = new W8DialogButton;
        if (m_buttons_054[index] == 0) {
            count = 0;
            if (m_kind_0cc == 0) {
                count = 8;
            } else if (m_kind_0cc <= 2) {
                count = 10;
            }
            for (index = 0; index < count; ++index) {
                if (m_buttons_054[index] != 0) {
                    delete m_buttons_054[index];
                    m_buttons_054[index] = 0;
                }
            }
            return 0;
        }
    }
    m_buttons_054[0]->Configure("Data\\Dialogs\\popup_splititem.sti", 0xc, 9, 10, 0xd, 0xb,
                                OnSplitDecrement005DE350, 0, 0, 0x7f, -1,
                                OnSplitDecrementMany005DE670, 0);
    m_buttons_054[1]->Configure("Data\\Dialogs\\popup_splititem.sti", 7, 4, 5, 8, 6,
                                OnSplitIncrement005DE4E0, 0, 0, 0x7f, -1,
                                OnSplitIncrementMany005DE810, 0);
    m_buttons_054[2]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1, 0, 0, 0, 0,
                                -1, 0, 0);
    m_buttons_054[3]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1,
                                OnCountFieldClick005DE9F0, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_054[4]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1, 0, 0, 0, 0,
                                -1, 0, 0);
    m_buttons_054[5]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1, 0, 0, 0, 0,
                                -1, 0, 0);
    m_buttons_054[6]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                                OnAccept005DE9B0, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_054[7]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 5, 6, 9, 7,
                                OnCancel005DE9D0, 0, 0, 0x7f, -1, 0, 0);
    if (m_kind_0cc == 1 || m_kind_0cc == 2) {
        m_buttons_054[8]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1, 0, 0, 0,
                                    0, -1, 0, 0);
        m_buttons_054[9]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1, 0, 0, 0,
                                    0, -1, 0, 0);
    }
    m_buttons_054[0]->m_fires_on_press = 1;
    m_buttons_054[1]->m_fires_on_press = 1;
    for (index = 0; index < count; ++index) {
        m_buttons_054[index]->SetPosition(g_split_button_offsets[index].x + m_x,
                                          g_split_button_offsets[index].y + m_y);
        m_buttons_054[index]->m_owner_040 = this;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005DD750
unsigned char W8SplitItemDialog::CreateTextBuffers005DD750()
{
    int count;
    int index;
    const wchar_t* header;
    W8ControlsRect bounds;

    count = 0;
    if (m_kind_0cc == 0) {
        count = 10;
    } else if (m_kind_0cc <= 2) {
        count = 14;
    }
    for (index = 0; index < count; ++index) {
        bounds.left = g_split_text_bounds[index].left + m_x;
        bounds.top = g_split_text_bounds[index].top + m_y;
        bounds.right = g_split_text_bounds[index].right + m_x;
        bounds.bottom = g_split_text_bounds[index].bottom + m_y;
        m_texts_07c[index] = new W8TextBuffer(
            &bounds, gppStringList[g_split_text_string_ids[index]], g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
        if (m_texts_07c[index] == 0) {
            count = 0;
            if (m_kind_0cc == 0) {
                count = 10;
            } else if (m_kind_0cc <= 2) {
                count = 14;
            }
            for (index = 0; index < count; ++index) {
                if (m_texts_07c[index] != 0) {
                    delete m_texts_07c[index];
                    m_texts_07c[index] = 0;
                }
            }
            return 0;
        }
    }
    m_texts_07c[2]->SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                  g_W8TextBufferLayoutMask005ED554);
    m_texts_07c[7]->SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                  g_W8TextBufferLayoutMask005ED554);
    m_texts_07c[9]->SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                  g_W8TextBufferLayoutMask005ED554);
    if (m_kind_0cc == 1 || m_kind_0cc == 2) {
        m_texts_07c[11]->SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                       g_W8TextBufferLayoutMask005ED554);
        m_texts_07c[13]->SetLayoutMode(g_W8TextBufferLayoutMask005ED550 |
                                       g_W8TextBufferLayoutMask005ED554);
        if (m_kind_0cc == 1) {
            m_texts_07c[1]->SetText(gppStringList[274], g_font_683660);
            header = gppStringList[268];
        } else {
            m_texts_07c[1]->SetText(gppStringList[275], g_font_683660);
            header = gppStringList[276];
        }
        m_texts_07c[0]->SetText(header, g_font_683660);
    }
    m_texts_07c[3]->SetText(FormatItemDisplayName(m_item_0d0, 0), g_font_683660);
    m_texts_07c[4]->SetText(
        FormatWideString(
            L"%s (%s)", gppStringList[g_equip_class_name_ids_61e7dc[GetItemEquipClass(m_item_0d0)]],
            gppStringList[g_generic_item_name_notice[GetItemUnidentifiedNameIndex(m_item_0d0)]]),
        g_font_683660);
    return 1;
}

// FUNCTION: WIZ8 0x005DDA60
unsigned char W8SplitItemDialog::CreateNumericInput005DDA60()
{
    W8ControlsRect bounds;

    m_active_input_0b8 = 0;
    bounds.left = g_split_count_field_bounds.left + m_x;
    bounds.top = g_split_count_field_bounds.top + m_y;
    bounds.right = g_split_count_field_bounds.right + m_x;
    bounds.bottom = g_split_count_field_bounds.bottom + m_y;
    m_count_input_0b4 = new W8DialogNumericInput(0, &bounds, split_count_0c0, g_font_683660, this,
                                                 m_buttons_054[3]);
    if (m_count_input_0b4 == 0) {
        NoOp();
        ::operator delete(m_count_input_0b4);
        m_count_input_0b4 = 0;
        return 0;
    }
    m_count_input_0b4->m_maximum = m_stack_total_0c4;
    return 1;
}

// FUNCTION: WIZ8 0x005DDB60
void W8SplitItemDialog::Draw()
{
    int index;
    int button_count;
    int text_count;

    button_count = 0;
    if (m_kind_0cc == 0) {
        button_count = 8;
    } else if (m_kind_0cc <= 2) {
        button_count = 10;
    }
    text_count = 0;
    if (m_kind_0cc == 0) {
        text_count = 10;
    } else if (m_kind_0cc <= 2) {
        text_count = 14;
    }
    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        m_first_draw_0d4 = 1;
        for (index = 0; index < button_count; ++index) {
            m_buttons_054[index]->m_dirty = 1;
        }
        for (index = 0; index < text_count; ++index) {
            m_texts_07c[index]->m_geometryDirty = 1;
        }
        m_count_input_0b4->m_dirty = 1;
        m_count_input_0b4->m_button->m_dirty = 1;
        W8DialogBase::Draw();
    }
    if (m_first_draw_0d4 != 0) {
        DrawCatalogImageAndInvalidate(
            -0xe, g_item_video_objects_68ec68.GetOrCreateVideoObject(m_item_0d0->item_id), 0, 0,
            m_x + 0x18, m_y + 0xe, 2, 0);
        m_first_draw_0d4 = 0;
    }
    if (m_buttons_054[3]->m_dirty) {
        m_count_input_0b4->m_dirty = 1;
        m_count_input_0b4->m_button->m_dirty = 1;
    }
    for (index = 0; index < button_count; ++index) {
        if (m_buttons_054[index] != 0) {
            m_buttons_054[index]->Draw();
        }
    }
    for (index = 0; index < text_count; ++index) {
        if (m_texts_07c[index] != 0) {
            m_texts_07c[index]->RenderToTarget(0, 0, -0xe);
        }
    }
    if (m_count_input_0b4 != 0) {
        m_count_input_0b4->Draw(0);
    }
}

// FUNCTION: WIZ8 0x005DDCC0
void W8SplitItemDialog::UpdateCostLabels005DCC00()
{
    W8ItemInstance stack;
    wchar_t text[34];
    int remaining_price;
    int split_price;

    stack = *m_item_0d0;
    if (m_kind_0cc == 0) {
        return;
    }
    if (m_kind_0cc == 1) {
        stack.stack_count = (unsigned char)split_count_0c0;
        if (split_count_0c0 == 0) {
            split_price = 0;
        } else {
            split_price = Function55B5E0(g_screen_state_00649f1c->value_1d4, &stack, 0);
        }
        stack.stack_count = (unsigned char)m_remaining_0bc;
        if (m_remaining_0bc == 0) {
            remaining_price = 0;
        } else {
            remaining_price = Function55B5E0(g_screen_state_00649f1c->value_1d4, &stack, 0);
        }
    } else if (m_kind_0cc == 2) {
        stack.stack_count = (unsigned char)split_count_0c0;
        if (split_count_0c0 == 0) {
            split_price = 0;
        } else {
            split_price = Function55B5E0(g_screen_state_00649f1c->value_1d4, &stack, 1);
        }
        stack.stack_count = (unsigned char)m_remaining_0bc;
        if (m_remaining_0bc == 0) {
            remaining_price = 0;
        } else {
            remaining_price = Function55B5E0(g_screen_state_00649f1c->value_1d4, &stack, 1);
        }
    } else {
        remaining_price = GetItemStackValue(m_item_0d0);
        split_price = remaining_price;
    }
    swprintf(text, g_format_d_0060aa20, remaining_price);
    m_texts_07c[11]->SetText(text, g_font_683660);
    m_buttons_054[8]->m_dirty = 1;
    m_texts_07c[11]->m_geometryDirty = 1;
    swprintf(text, g_format_d_0060aa20, split_price);
    m_texts_07c[13]->SetText(text, g_font_683660);
    m_buttons_054[9]->m_dirty = 1;
    m_texts_07c[13]->m_geometryDirty = 1;
}

// FUNCTION: WIZ8 0x005DDE60
void W8SplitItemDialog::UpdateArrowStates005DDE60()
{
    if (split_count_0c0 == 0) {
        m_buttons_054[0]->SetEnabled(0);
        m_buttons_054[0]->m_dirty = 1;
    } else if (!m_buttons_054[0]->IsEnabled()) {
        m_buttons_054[0]->SetEnabled(1);
        m_buttons_054[0]->m_dirty = 1;
    }
    if (m_remaining_0bc == 0) {
        m_buttons_054[1]->SetEnabled(0);
        m_buttons_054[1]->m_dirty = 1;
    } else if (!m_buttons_054[1]->IsEnabled()) {
        m_buttons_054[1]->SetEnabled(1);
        m_buttons_054[1]->m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005DDEE0
void W8SplitItemDialog::UpdateAcceptButton005DDEE0()
{
    W8ItemInstance stack;
    unsigned char can_accept;

    stack = *m_item_0d0;
    stack.stack_count = (unsigned char)split_count_0c0;
    can_accept = split_count_0c0 != 0;
    switch (m_kind_0cc) {
    case 1:
        if (g_status_685170.party_gold <
            (unsigned int)Function55B5E0(g_screen_state_00649f1c->value_1d4, &stack, 0)) {
            can_accept = 0;
        }
        break;
    case 2:
        if (g_status_685170.party_gold <
            (unsigned int)Function55B5E0(g_screen_state_00649f1c->value_1d4, &stack, 1)) {
            can_accept = 0;
        }
        break;
    }
    if (can_accept != 0) {
        m_buttons_054[6]->SetEnabled(1);
    } else {
        m_buttons_054[6]->SetEnabled(0);
    }
}

// FUNCTION: WIZ8 0x005DDFA0
void W8SplitItemDialog::OnNumericInputChanged(int value)
{
    wchar_t text[34];

    if (value != 0) {
        return;
    }
    split_count_0c0 = m_count_input_0b4->m_value;
    m_remaining_0bc = m_stack_total_0c4 - m_count_input_0b4->m_value;
    UpdateArrowStates005DDE60();
    UpdateAcceptButton005DDEE0();
    swprintf(text, g_format_d_0060aa20, m_remaining_0bc);
    m_texts_07c[2]->SetText(text, g_font_683660);
    m_buttons_054[2]->m_dirty = 1;
    m_texts_07c[2]->m_geometryDirty = 1;
    m_count_input_0b4->SetValue(split_count_0c0);
    m_buttons_054[3]->m_dirty = 1;
    m_count_input_0b4->m_dirty = 1;
    m_count_input_0b4->m_button->m_dirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(m_item_0d0) * m_remaining_0bc) * g_float_005ed8b8);
    m_texts_07c[7]->SetText(text, g_font_683660);
    m_buttons_054[4]->m_dirty = 1;
    m_texts_07c[7]->m_geometryDirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(m_item_0d0) * split_count_0c0) * g_float_005ed8b8);
    m_texts_07c[9]->SetText(text, g_font_683660);
    m_buttons_054[5]->m_dirty = 1;
    m_texts_07c[9]->m_geometryDirty = 1;
    UpdateCostLabels005DCC00();
}

// FUNCTION: WIZ8 0x005DE120
unsigned char W8SplitItemDialog::HandleInputEvent005DE120(const InputAtom* input)
{
    int index;
    W8DialogNumericInput** field;

    field = &m_count_input_0b4;
    for (index = 0; index < 1; ++index) {
        if (*field != 0 && (*field)->m_active != 0 && (*field)->HandleInput(input) != 0) {
            return 1;
        }
        ++field;
    }
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        int key = toupper(input->usParam);
        if (key == '\r') {
            split_result_0c8 = 1;
            m_keep_open = 0;
        } else if (key == 0x1b) {
            m_keep_open = 0;
            return m_keep_open;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005DE1B0
unsigned char W8SplitItemDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_REPEAT, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            if (m_active_input_0b8 != 0) {
                m_active_input_0b8->SetActive(0);
            }
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_UP, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_UP, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case RIGHT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_REPEAT, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        default:
            HandleInputEvent005DE120(&input);
            break;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005DE350
void W8SplitItemDialog::OnSplitDecrement005DE350(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;
    wchar_t text[34];

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    dialog->split_count_0c0 = __max(0, dialog->split_count_0c0 - 1);
    dialog->m_remaining_0bc = __min(dialog->m_stack_total_0c4, dialog->m_remaining_0bc + 1);
    dialog->UpdateArrowStates005DDE60();
    dialog->UpdateAcceptButton005DDEE0();
    swprintf(text, g_format_d_0060aa20, dialog->m_remaining_0bc);
    dialog->m_texts_07c[2]->SetText(text, g_font_683660);
    dialog->m_buttons_054[2]->m_dirty = 1;
    dialog->m_texts_07c[2]->m_geometryDirty = 1;
    dialog->m_count_input_0b4->SetValue(dialog->split_count_0c0);
    dialog->m_buttons_054[3]->m_dirty = 1;
    dialog->m_count_input_0b4->m_dirty = 1;
    dialog->m_count_input_0b4->m_button->m_dirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->m_remaining_0bc) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[7]->SetText(text, g_font_683660);
    dialog->m_buttons_054[4]->m_dirty = 1;
    dialog->m_texts_07c[7]->m_geometryDirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->split_count_0c0) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[9]->SetText(text, g_font_683660);
    dialog->m_buttons_054[5]->m_dirty = 1;
    dialog->m_texts_07c[9]->m_geometryDirty = 1;
    dialog->UpdateCostLabels005DCC00();
}

// FUNCTION: WIZ8 0x005DE4E0
void W8SplitItemDialog::OnSplitIncrement005DE4E0(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;
    wchar_t text[34];

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    dialog->m_remaining_0bc = __max(0, dialog->m_remaining_0bc - 1);
    dialog->split_count_0c0 = __min(dialog->m_stack_total_0c4, dialog->split_count_0c0 + 1);
    dialog->UpdateArrowStates005DDE60();
    dialog->UpdateAcceptButton005DDEE0();
    swprintf(text, g_format_d_0060aa20, dialog->m_remaining_0bc);
    dialog->m_texts_07c[2]->SetText(text, g_font_683660);
    dialog->m_buttons_054[2]->m_dirty = 1;
    dialog->m_texts_07c[2]->m_geometryDirty = 1;
    dialog->m_count_input_0b4->SetValue(dialog->split_count_0c0);
    dialog->m_buttons_054[3]->m_dirty = 1;
    dialog->m_count_input_0b4->m_dirty = 1;
    dialog->m_count_input_0b4->m_button->m_dirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->m_remaining_0bc) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[7]->SetText(text, g_font_683660);
    dialog->m_buttons_054[4]->m_dirty = 1;
    dialog->m_texts_07c[7]->m_geometryDirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->split_count_0c0) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[9]->SetText(text, g_font_683660);
    dialog->m_buttons_054[5]->m_dirty = 1;
    dialog->m_texts_07c[9]->m_geometryDirty = 1;
    dialog->UpdateCostLabels005DCC00();
}

// FUNCTION: WIZ8 0x005DE670
void W8SplitItemDialog::OnSplitDecrementMany005DE670(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;
    wchar_t text[34];

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    dialog->split_count_0c0 = __max(0, dialog->split_count_0c0 - 5);
    dialog->m_remaining_0bc = __min(dialog->m_stack_total_0c4, dialog->m_remaining_0bc + 5);
    dialog->UpdateArrowStates005DDE60();
    dialog->UpdateAcceptButton005DDEE0();
    swprintf(text, g_format_d_0060aa20, dialog->m_remaining_0bc);
    dialog->m_texts_07c[2]->SetText(text, g_font_683660);
    dialog->m_buttons_054[2]->m_dirty = 1;
    dialog->m_texts_07c[2]->m_geometryDirty = 1;
    dialog->m_count_input_0b4->SetValue(dialog->split_count_0c0);
    dialog->m_buttons_054[3]->m_dirty = 1;
    dialog->m_count_input_0b4->m_dirty = 1;
    dialog->m_count_input_0b4->m_button->m_dirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->m_remaining_0bc) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[7]->SetText(text, g_font_683660);
    dialog->m_buttons_054[4]->m_dirty = 1;
    dialog->m_texts_07c[7]->m_geometryDirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->split_count_0c0) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[9]->SetText(text, g_font_683660);
    dialog->m_buttons_054[5]->m_dirty = 1;
    dialog->m_texts_07c[9]->m_geometryDirty = 1;
    dialog->UpdateCostLabels005DCC00();
}

// FUNCTION: WIZ8 0x005DE810
void W8SplitItemDialog::OnSplitIncrementMany005DE810(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;
    wchar_t text[34];

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    dialog->m_remaining_0bc = __max(0, dialog->m_remaining_0bc - 5);
    dialog->split_count_0c0 = __min(dialog->m_stack_total_0c4, dialog->split_count_0c0 + 5);
    dialog->UpdateArrowStates005DDE60();
    dialog->UpdateAcceptButton005DDEE0();
    swprintf(text, g_format_d_0060aa20, dialog->m_remaining_0bc);
    dialog->m_texts_07c[2]->SetText(text, g_font_683660);
    dialog->m_buttons_054[2]->m_dirty = 1;
    dialog->m_texts_07c[2]->m_geometryDirty = 1;
    dialog->m_count_input_0b4->SetValue(dialog->split_count_0c0);
    dialog->m_buttons_054[3]->m_dirty = 1;
    dialog->m_count_input_0b4->m_dirty = 1;
    dialog->m_count_input_0b4->m_button->m_dirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->m_remaining_0bc) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[7]->SetText(text, g_font_683660);
    dialog->m_buttons_054[4]->m_dirty = 1;
    dialog->m_texts_07c[7]->m_geometryDirty = 1;
    swprintf(text, g_assay_format_1f_0064fbb4,
             (double)(GetItemUnitWeight(dialog->m_item_0d0) * dialog->split_count_0c0) *
                 g_float_005ed8b8);
    dialog->m_texts_07c[9]->SetText(text, g_font_683660);
    dialog->m_buttons_054[5]->m_dirty = 1;
    dialog->m_texts_07c[9]->m_geometryDirty = 1;
    dialog->UpdateCostLabels005DCC00();
}

// FUNCTION: WIZ8 0x005DE9B0
void W8SplitItemDialog::OnAccept005DE9B0(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    dialog->split_result_0c8 = 1;
    dialog->m_keep_open = 0;
}

// FUNCTION: WIZ8 0x005DE9D0
void W8SplitItemDialog::OnCancel005DE9D0(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    dialog->split_result_0c8 = 2;
    dialog->m_keep_open = 0;
}

// FUNCTION: WIZ8 0x005DE9F0
void W8SplitItemDialog::OnCountFieldClick005DE9F0(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;
    POINT mouse;
    POINT point;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner_040);
    SGPMouseGetPos(&mouse);
    if (dialog->m_count_input_0b4 == 0) {
        return;
    }
    point.x = mouse.x - dialog->m_x;
    point.y = mouse.y - dialog->m_y;
    if (!ScreenPointInRect(&g_split_count_field_bounds, &point)) {
        return;
    }
    point.x = point.x - g_split_count_field_bounds.left;
    point.y = point.y - g_split_count_field_bounds.top;
    dialog->m_count_input_0b4->SetActive(1, &point);
    dialog->m_active_input_0b8 = dialog->m_count_input_0b4;
}
