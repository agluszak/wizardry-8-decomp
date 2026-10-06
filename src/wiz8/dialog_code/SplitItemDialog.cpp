#include "wiz8/dialog_code/DialogFactoryDialogs.h"

#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

#include "input.h"
#include "mousesystem_macros.h"

#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/local_screens/CharacterScreen.h"

/* The split-stack dialog ("popup_splititem.sti"). Inventory splits use eight
   buttons and ten labels; the two trade modes add a passive frame and a price
   label for each side of the transaction. The six retail expansions of the running-totals refresh share UpdateTotals. */

struct W8SplitButtonOffset {
    int x;
    int y;
};

// GLOBAL: WIZ8 0x0064FE38
static W8SplitButtonOffset g_split_button_offsets[10] = {
    {0xd9, 0x5a}, {0xbe, 0x5a}, {0xbc, 0x49},  {0xbc, 0x6f}, {0x81, 0x35},
    {0x81, 0x83}, {0xed, 0x95}, {0x111, 0x95}, {0xf4, 0x35}, {0xf4, 0x83},
};

// GLOBAL: WIZ8 0x0064FE88
static W8ControlsRect g_split_text_bounds[14] = {
    {0x48, 0x49, 0xb6, 0x55},  {0x48, 0x6f, 0xb6, 0x7b},  {0xbc, 0x49, 0xef, 0x55},
    {0x48, 0x0f, 0x12a, 0x1f}, {0x48, 0x23, 0x7d, 0x2f},  {0x81, 0x23, 0x12a, 0x2f},
    {0x48, 0x35, 0x7d, 0x41},  {0x81, 0x35, 0xb7, 0x41},  {0x48, 0x83, 0x7d, 0x8f},
    {0x81, 0x83, 0xb7, 0x8f},  {0xbb, 0x35, 0xf0, 0x41},  {0xf4, 0x35, 0x12a, 0x41},
    {0xbb, 0x83, 0xf0, 0x8f},  {0x90, 0x83, 0x12a, 0x8f},
};

// GLOBAL: WIZ8 0x0064FF68
static int g_split_text_string_ids[14] = {
    268, 269, 270, 270, 271, 270, 272, 270, 272, 270, 273, 270, 273, 270,
};

// GLOBAL: WIZ8 0x0064FFA0
static W8ScreenRect g_split_count_field_bounds = {0xbc, 0x6f, 0xef, 0x7b};

// FUNCTION: WIZ8 0x005DCED0
W8SplitItemDialog::W8SplitItemDialog(W8ItemSplitMode mode, W8ItemInstance* item, int count)
{
    int index;

    SetExtent(0x142, 0xbd);
    if (mode == W8_ITEM_SPLIT_SELL || mode == W8_ITEM_SPLIT_BUY) {
        SetBackground("Data\\Dialogs\\popup_splititem.sti", 0);
    } else if (mode == W8_ITEM_SPLIT_INVENTORY) {
        SetBackground("Data\\Dialogs\\popup_splititem.sti", 1);
    }
    m_mode = mode;
    for (index = 0; index < 10; ++index) {
        m_buttons[index] = 0;
    }
    for (index = 0; index < 14; ++index) {
        m_texts[index] = 0;
    }
    m_count_input = 0;
    m_remaining = item->stack_count;
    m_stack_total = item->stack_count;
    if (count == -1) {
        if (g_item_records[item->iItemNo].maximum_quantity <= 0xa) {
            split_count = 1;
        } else {
            split_count = item->stack_count >> 1;
        }
        m_remaining = item->stack_count - split_count;
    } else {
        split_count = count;
        m_remaining = item->stack_count - count;
    }
    m_item = item;
    split_result = W8_SPLIT_RESULT_PENDING;
    m_active_input = 0;
    m_first_draw = false;
}

// FUNCTION: WIZ8 0x005DD030
W8SplitItemDialog::~W8SplitItemDialog()
{
    DestroyControls();
}

/* Shared cleanup in failure paths and DestroyControls. The helper name
   is descriptive; its mode-dependent extent and delete order come from retail. */
void W8SplitItemDialog::DestroyButtons()
{
    int count;
    int index;

    count = 0;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        count = 8;
    } else if (static_cast<unsigned int>(m_mode) <= W8_ITEM_SPLIT_BUY) {
        count = 10;
    }
    for (index = 0; index < count; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
}

/* Shared cleanup in failure paths and DestroyControls. The helper name
   is descriptive; its mode-dependent extent and delete order come from retail. */
void W8SplitItemDialog::DestroyTextBuffers()
{
    int count;
    int index;

    count = 0;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        count = 10;
    } else if (static_cast<unsigned int>(m_mode) <= W8_ITEM_SPLIT_BUY) {
        count = 14;
    }
    for (index = 0; index < count; ++index) {
        if (m_texts[index] != 0) {
            delete m_texts[index];
            m_texts[index] = 0;
        }
    }
}

// FUNCTION: WIZ8 0x005DD130
int W8SplitItemDialog::CreateControls()
{
    W8DialogBase::CreateControls();
    m_first_draw = true;
    split_result = W8_SPLIT_RESULT_PENDING;
    if (!CreateButtons()) {
        m_error = 7;
        return 7;
    }
    if (!CreateTextBuffers()) {
        DestroyButtons();
        m_error = 7;
        return 7;
    }
    if (!CreateNumericInput()) {
        DestroyButtons();
        DestroyTextBuffers();
        m_error = 7;
        return 7;
    }
    UpdateTotals();
    return 0;
}

// FUNCTION: WIZ8 0x005DD3C0
void W8SplitItemDialog::DestroyControls()
{
    W8DialogBase::DestroyControls();
    DestroyButtons();
    DestroyTextBuffers();
    if (m_count_input != 0) {
        NoOp();
        delete m_count_input;
        m_count_input = 0;
    }
}

// FUNCTION: WIZ8 0x005DD480
bool W8SplitItemDialog::CreateButtons()
{
    int count;
    int index;

    count = 0;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        count = 8;
    } else if (static_cast<unsigned int>(m_mode) <= W8_ITEM_SPLIT_BUY) {
        count = 10;
    }
    for (index = 0; index < count; ++index) {
        m_buttons[index] = new W8DialogButton;
        if (m_buttons[index] == 0) {
            DestroyButtons();
            return false;
        }
    }
    m_buttons[0]->Configure("Data\\Dialogs\\popup_splititem.sti", 0xc, 9, 10, 0xd, 0xb,
                            OnSplitDecrement, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, OnSplitDecrementMany, BUTTON_NO_CALLBACK);
    m_buttons[1]->Configure("Data\\Dialogs\\popup_splititem.sti", 7, 4, 5, 8, 6, OnSplitIncrement,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, OnSplitIncrementMany, BUTTON_NO_CALLBACK);
    m_buttons[2]->Configure("Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3,
                            BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[3]->Configure("Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3,
                            BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, OnCountFieldClick,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[4]->Configure("Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3,
                            BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[5]->Configure("Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3,
                            BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[6]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2, OnAccept,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[7]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 5, 6, 9, 7, OnCancel,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    if (m_mode == W8_ITEM_SPLIT_SELL || m_mode == W8_ITEM_SPLIT_BUY) {
        m_buttons[8]->Configure(
            "Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, 3,
            BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
        m_buttons[9]->Configure(
            "Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, 3,
            BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    }
    m_buttons[0]->m_fires_on_press = true;
    m_buttons[1]->m_fires_on_press = true;
    for (index = 0; index < count; ++index) {
        m_buttons[index]->SetPosition(g_split_button_offsets[index].x + m_x,
                                      g_split_button_offsets[index].y + m_y);
        m_buttons[index]->m_owner = this;
    }
    return true;
}

// FUNCTION: WIZ8 0x005DD750
bool W8SplitItemDialog::CreateTextBuffers()
{
    int count;
    int index;
    const wchar_t* header;
    W8ControlsRect bounds;

    count = 0;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        count = 10;
    } else if (static_cast<unsigned int>(m_mode) <= W8_ITEM_SPLIT_BUY) {
        count = 14;
    }
    for (index = 0; index < count; ++index) {
        bounds.left = g_split_text_bounds[index].left + m_x;
        bounds.top = g_split_text_bounds[index].top + m_y;
        bounds.right = g_split_text_bounds[index].right + m_x;
        bounds.bottom = g_split_text_bounds[index].bottom + m_y;
        m_texts[index] = new W8TextBuffer(&bounds, gppStringList[g_split_text_string_ids[index]],
                                          g_wiz_text_font_secondary,
                                          g_W8TextBufferAlignMiddle | g_W8TextBufferAlignCenter, 4);
        if (m_texts[index] == 0) {
            DestroyTextBuffers();
            return false;
        }
    }
    m_texts[2]->SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
    m_texts[7]->SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
    m_texts[9]->SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
    if (m_mode == W8_ITEM_SPLIT_SELL || m_mode == W8_ITEM_SPLIT_BUY) {
        m_texts[11]->SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
        m_texts[13]->SetLayoutMode(g_W8TextBufferAlignRight | g_W8TextBufferAlignMiddle);
        if (m_mode == W8_ITEM_SPLIT_SELL) {
            m_texts[1]->SetText(gppStringList[274], g_wiz_text_font_secondary);
            header = gppStringList[268];
        } else {
            m_texts[1]->SetText(gppStringList[275], g_wiz_text_font_secondary);
            header = gppStringList[276];
        }
        m_texts[0]->SetText(header, g_wiz_text_font_secondary);
    }
    m_texts[3]->SetText(FormatItemDisplayName(m_item, false), g_wiz_text_font_secondary);
    m_texts[5]->SetText(
        FormatWideString(
            L"%s (%s)", gppStringList[g_equip_class_name_ids[GetItemEquipClass(m_item)]],
            gppStringList[g_generic_item_name_notice[GetItemUnidentifiedNameIndex(m_item)]]),
        g_wiz_text_font_secondary);
    return true;
}

// FUNCTION: WIZ8 0x005DDA60
bool W8SplitItemDialog::CreateNumericInput()
{
    W8ControlsRect bounds;

    m_active_input = 0;
    bounds.left = g_split_count_field_bounds.left + m_x;
    bounds.top = g_split_count_field_bounds.top + m_y;
    bounds.right = g_split_count_field_bounds.right + m_x;
    bounds.bottom = g_split_count_field_bounds.bottom + m_y;
    m_count_input = new W8DialogNumericInput(0, &bounds, split_count, g_wiz_text_font_secondary,
                                             this, m_buttons[3]);
    if (m_count_input == 0) {
        NoOp();
        delete m_count_input;
        m_count_input = 0;
        return false;
    }
    m_count_input->m_maximum = m_stack_total;
    return true;
}

// FUNCTION: WIZ8 0x005DDB60
void W8SplitItemDialog::Draw()
{
    int index;
    int button_count;
    int text_count;

    button_count = 0;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        button_count = 8;
    } else if (static_cast<unsigned int>(m_mode) <= W8_ITEM_SPLIT_BUY) {
        button_count = 10;
    }
    text_count = 0;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        text_count = 10;
    } else if (static_cast<unsigned int>(m_mode) <= W8_ITEM_SPLIT_BUY) {
        text_count = 14;
    }
    if ((m_dirty_flags & W8_DIALOG_DIRTY_REDRAW) != 0) {
        if (!m_initialized) {
            CreateControls();
        }
        m_first_draw = true;
        for (index = 0; index < button_count; ++index) {
            m_buttons[index]->m_dirty = true;
        }
        for (index = 0; index < text_count; ++index) {
            m_texts[index]->SetGeometryDirty();
        }
        m_count_input->m_dirty = true;
        m_count_input->m_button->m_dirty = true;
        W8DialogBase::Draw();
    }
    if (m_first_draw) {
        DrawCatalogImageAndInvalidate(FRAME_BUFFER,
                                      g_item_video_objects.GetOrCreateVideoObject(m_item->iItemNo),
                                      0, 0, m_x + 0x18, m_y + 0xe, VO_BLT_SRCTRANSPARENCY, 0);
        m_first_draw = false;
    }
    if (m_buttons[3]->m_dirty) {
        m_count_input->m_dirty = true;
        m_count_input->m_button->m_dirty = true;
    }
    for (index = 0; index < button_count; ++index) {
        if (m_buttons[index] != 0) {
            m_buttons[index]->Draw();
        }
    }
    for (index = 0; index < text_count; ++index) {
        if (m_texts[index] != 0) {
            m_texts[index]->RenderToTarget(0, false, FRAME_BUFFER);
        }
    }
    if (m_count_input != 0) {
        m_count_input->Draw(false);
    }
}

// FUNCTION: WIZ8 0x005DDCC0
void W8SplitItemDialog::UpdateCostLabels()
{
    W8ItemInstance stack;
    wchar_t text[34];
    int remaining_price;
    int split_price;

    stack = *m_item;
    if (m_mode == W8_ITEM_SPLIT_INVENTORY) {
        return;
    }
    if (m_mode == W8_ITEM_SPLIT_SELL) {
        stack.stack_count = static_cast<unsigned char>(split_count);
        if (split_count == 0) {
            split_price = 0;
        } else {
            split_price = CalculateTradeStackPrice(g_npc_interaction_state->dialogue_npc, &stack,
                                                   W8_TRADE_PRICE_PARTY_SELLS);
        }
        stack.stack_count = static_cast<unsigned char>(m_remaining);
        if (m_remaining == 0) {
            remaining_price = 0;
        } else {
            remaining_price = CalculateTradeStackPrice(g_npc_interaction_state->dialogue_npc,
                                                       &stack, W8_TRADE_PRICE_PARTY_SELLS);
        }
    } else if (m_mode == W8_ITEM_SPLIT_BUY) {
        stack.stack_count = static_cast<unsigned char>(split_count);
        if (split_count == 0) {
            split_price = 0;
        } else {
            split_price = CalculateTradeStackPrice(g_npc_interaction_state->dialogue_npc, &stack,
                                                   W8_TRADE_PRICE_PARTY_BUYS);
        }
        stack.stack_count = static_cast<unsigned char>(m_remaining);
        if (m_remaining == 0) {
            remaining_price = 0;
        } else {
            remaining_price = CalculateTradeStackPrice(g_npc_interaction_state->dialogue_npc,
                                                       &stack, W8_TRADE_PRICE_PARTY_BUYS);
        }
    } else {
        remaining_price = GetItemStackValue(m_item);
        split_price = remaining_price;
    }
    swprintf(text, g_format_d, remaining_price);
    m_texts[11]->SetText(text, g_wiz_text_font_secondary);
    m_buttons[8]->m_dirty = true;
    m_texts[11]->SetGeometryDirty();
    swprintf(text, g_format_d, split_price);
    m_texts[13]->SetText(text, g_wiz_text_font_secondary);
    m_buttons[9]->m_dirty = true;
    m_texts[13]->SetGeometryDirty();
}

// FUNCTION: WIZ8 0x005DDE60
void W8SplitItemDialog::UpdateArrowStates()
{
    if (split_count == 0) {
        m_buttons[0]->SetEnabled(false);
        m_buttons[0]->m_dirty = true;
    } else if (!m_buttons[0]->IsEnabled()) {
        m_buttons[0]->SetEnabled(true);
        m_buttons[0]->m_dirty = true;
    }
    if (m_remaining == 0) {
        m_buttons[1]->SetEnabled(false);
        m_buttons[1]->m_dirty = true;
    } else if (!m_buttons[1]->IsEnabled()) {
        m_buttons[1]->SetEnabled(true);
        m_buttons[1]->m_dirty = true;
    }
}

// FUNCTION: WIZ8 0x005DDEE0
void W8SplitItemDialog::UpdateAcceptButton()
{
    W8ItemInstance stack;
    bool can_accept;

    stack = *m_item;
    stack.stack_count = static_cast<unsigned char>(split_count);
    can_accept = split_count != 0;
    switch (m_mode) {
    case W8_ITEM_SPLIT_SELL:
        if (g_status.party_gold <
            static_cast<unsigned int>(CalculateTradeStackPrice(
                g_npc_interaction_state->dialogue_npc, &stack, W8_TRADE_PRICE_PARTY_SELLS))) {
            can_accept = false;
        }
        break;
    case W8_ITEM_SPLIT_BUY:
        if (g_status.party_gold <
            static_cast<unsigned int>(CalculateTradeStackPrice(
                g_npc_interaction_state->dialogue_npc, &stack, W8_TRADE_PRICE_PARTY_BUYS))) {
            can_accept = false;
        }
        break;
    default:
        break;
    }
    if (can_accept) {
        m_buttons[6]->SetEnabled(true);
    } else {
        m_buttons[6]->SetEnabled(false);
    }
}

/* The same totals refresh appears in the numeric-field callback and all
   split-count button callbacks. */
void W8SplitItemDialog::UpdateTotals()
{
    wchar_t text[34];

    UpdateArrowStates();
    UpdateAcceptButton();
    swprintf(text, g_format_d, m_remaining);
    m_texts[2]->SetText(text, g_wiz_text_font_secondary);
    m_buttons[2]->m_dirty = true;
    m_texts[2]->SetGeometryDirty();
    m_count_input->SetValue(split_count);
    m_buttons[3]->m_dirty = true;
    m_count_input->m_dirty = true;
    m_count_input->m_button->m_dirty = true;
    swprintf(text, g_assay_format,
             static_cast<double>(GetItemUnitWeight(m_item) * m_remaining) *
                 g_item_weight_display_scale);
    m_texts[7]->SetText(text, g_wiz_text_font_secondary);
    m_buttons[4]->m_dirty = true;
    m_texts[7]->SetGeometryDirty();
    swprintf(text, g_assay_format,
             static_cast<double>(GetItemUnitWeight(m_item) * split_count) *
                 g_item_weight_display_scale);
    m_texts[9]->SetText(text, g_wiz_text_font_secondary);
    m_buttons[5]->m_dirty = true;
    m_texts[9]->SetGeometryDirty();
    UpdateCostLabels();
}

// FUNCTION: WIZ8 0x005DDFA0
void W8SplitItemDialog::OnNumericInputChanged(int value)
{
    if (value != 0) {
        return;
    }
    split_count = m_count_input->m_value;
    m_remaining = m_stack_total - m_count_input->m_value;
    UpdateTotals();
}

// FUNCTION: WIZ8 0x005DE120
bool W8SplitItemDialog::HandleInputEvent(const InputAtom* input)
{
    int index;
    W8DialogNumericInput** field;

    field = &m_count_input;
    for (index = 0; index < 1; ++index) {
        if (*field != 0 && (*field)->m_active != 0 && (*field)->HandleInput(input)) {
            return true;
        }
        ++field;
    }
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        int key = toupper(input->usParam);
        if (key == '\r') {
            split_result = W8_SPLIT_RESULT_CONFIRMED;
            m_keep_open = false;
        } else if (key == 0x1b) {
            m_keep_open = false;
            return m_keep_open;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005DE1B0
bool W8SplitItemDialog::ProcessInput()
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
            if (m_active_input != 0) {
                m_active_input->SetActive(false);
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
            HandleInputEvent(&input);
            break;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005DE350
void W8SplitItemDialog::OnSplitDecrement(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    dialog->split_count = __max(0, dialog->split_count - 1);
    dialog->m_remaining = __min(dialog->m_stack_total, dialog->m_remaining + 1);
    dialog->UpdateTotals();
}

// FUNCTION: WIZ8 0x005DE4E0
void W8SplitItemDialog::OnSplitIncrement(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    dialog->m_remaining = __max(0, dialog->m_remaining - 1);
    dialog->split_count = __min(dialog->m_stack_total, dialog->split_count + 1);
    dialog->UpdateTotals();
}

// FUNCTION: WIZ8 0x005DE670
void W8SplitItemDialog::OnSplitDecrementMany(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    dialog->split_count = __max(0, dialog->split_count - 5);
    dialog->m_remaining = __min(dialog->m_stack_total, dialog->m_remaining + 5);
    dialog->UpdateTotals();
}

// FUNCTION: WIZ8 0x005DE810
void W8SplitItemDialog::OnSplitIncrementMany(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    dialog->m_remaining = __max(0, dialog->m_remaining - 5);
    dialog->split_count = __min(dialog->m_stack_total, dialog->split_count + 5);
    dialog->UpdateTotals();
}

// FUNCTION: WIZ8 0x005DE9B0
void W8SplitItemDialog::OnAccept(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    dialog->split_result = W8_SPLIT_RESULT_CONFIRMED;
    dialog->m_keep_open = false;
}

// FUNCTION: WIZ8 0x005DE9D0
void W8SplitItemDialog::OnCancel(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    dialog->split_result = W8_SPLIT_RESULT_CANCELLED;
    dialog->m_keep_open = false;
}

// FUNCTION: WIZ8 0x005DE9F0
void W8SplitItemDialog::OnCountFieldClick(W8DialogButton* button)
{
    W8SplitItemDialog* dialog;
    POINT mouse;
    POINT point;

    if (button == 0) {
        return;
    }
    dialog = static_cast<W8SplitItemDialog*>(button->m_owner);
    SGPMouseGetPos(&mouse);
    if (dialog->m_count_input == 0) {
        return;
    }
    point.x = mouse.x - dialog->m_x;
    point.y = mouse.y - dialog->m_y;
    if (!ScreenPointInRect(&g_split_count_field_bounds, &point)) {
        return;
    }
    point.x -= g_split_count_field_bounds.left;
    point.y -= g_split_count_field_bounds.top;
    dialog->m_count_input->SetActive(true, &point);
    dialog->m_active_input = dialog->m_count_input;
}
