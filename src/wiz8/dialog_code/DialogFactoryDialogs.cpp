#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/sgp_text.h"
#include "wiz8/dialog_code/ButtonUserData.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/cursor.h"
#include "wiz8/utility.h"
#include "wiz8/fonts.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/video_object_catalog.h"

#include "Button System.h"
#include "Font.h"
#include "english.h"
#include "himage.h"
#include "input.h"
#include "mousesystem_macros.h"
#include "soundman.h"
#include "vsurface.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Dialog Code\DialogFactoryDialogs.cpp. The factory dialogs are the list-box
   dialog (kind 3) and the trigger-owned item picker; the split-item dialog
   owns its own translation unit. The list-box helpers' bodies are recovered
   below; their addresses sit in an attribution gap, so their original TU is
   unproven rather than their bodies unrecovered. */

// FUNCTION: WIZ8 0x005cbb40
W8ListBoxDialog::W8ListBoxDialog()
{
    int index;

    m_field_078 = 0.05f;
    m_field_07c = 0.2f;
    m_field_080 = 0.9f;
    m_field_084 = 0.75f;
    m_field_074 = 0;
    m_fill_colour = 0x6000;
    m_text_button = -1;
    m_second_text_button = -1;
    area_inlay = -1;
    m_area_button = -1;
    m_up_button = -1;
    m_up_image = -1;
    m_down_button = -1;
    m_down_image = -1;
    third_btn_inlay = -1;
    m_third_text_button = -1;
    m_slider_button = -1;
    m_slider_image = -1;
    m_ok_button = -1;
    m_ok_image = -1;
    m_cancel_button = -1;
    m_cancel_image = -1;
    edge_inlay = -1;
    int line_count = m_lines.GetCount();
    for (index = 0; index < line_count; ++index) {
        wchar_t* line = *m_lines.GetAt(index);
        if (line != 0) {
            free(line);
        }
    }
    m_lines.Clear();
    m_field_064.Clear();
    m_selected_line = -1;
    m_field_074 = 0;
    m_scrollable = false;
    m_first_visible_line = 0;
}

// FUNCTION: WIZ8 0x005cbce0
W8ListBoxDialog::~W8ListBoxDialog()
{
    if (m_destroy_callback != 0) {
        m_destroy_callback(this);
        m_destroy_callback = 0;
    }
    DestroyControls();
}

// FUNCTION: WIZ8 0x005cbd70
void W8ListBoxDialog::SetText(const wchar_t* text)
{
    if (m_text_button != -1) {
        SpecifyButtonText(m_text_button, const_cast<wchar_t*>(text));
        W8DialogBase::SetText(0);
        return;
    }
    W8DialogBase::SetText(text);
}

// FUNCTION: WIZ8 0x005CC650
int W8ListBoxDialog::GetVisibleLineCount()
{
    if (m_area_button == -1) {
        return 0;
    }
    return GetButtonHeight(m_area_button) /
           static_cast<int>(static_cast<unsigned int>(GetFontHeight(g_dialog_interface_font)));
}

// FUNCTION: WIZ8 0x005CCB80
void W8ListBoxDialog::SetCurrentLine(int line)
{
    if (m_selected_line == line) {
        return;
    }
    if (line == -1) {
        m_selected_line = -1;
        return;
    }
    int target = line < 0 ? 0 : line;
    if (m_lines.count - 1 < target) {
        target = m_lines.count - 1;
    }
    int visible = GetVisibleLineCount();
    if (target < m_first_visible_line) {
        if (m_area_button != 0) {
            int first = target;
            if (m_lines.count - GetVisibleLineCount() < target) {
                first = m_lines.count - GetVisibleLineCount();
            }
            if (first < 0) {
                first = 0;
            } else if (m_lines.count - GetVisibleLineCount() < target) {
                first = m_lines.count - GetVisibleLineCount();
            }
            if (first != m_first_visible_line) {
                m_first_visible_line = first;
                m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
            }
        }
    } else if (target < m_lines.count && m_first_visible_line - 1 + visible < target) {
        int first = target - visible + 1;
        if (m_area_button != 0) {
            if (m_lines.count - GetVisibleLineCount() < first) {
                first = m_lines.count - GetVisibleLineCount();
            }
            if (first < 0) {
                first = 0;
            } else if (m_lines.count - GetVisibleLineCount() < first) {
                first = m_lines.count - GetVisibleLineCount();
            }
            if (first != m_first_visible_line) {
                m_first_visible_line = first;
                m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
            }
        }
    }
    m_selected_line = target;
    m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
}

// FUNCTION: WIZ8 0x005CD2B0
bool W8ListBoxDialog::HandleInputEvent(const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        return m_keep_open;
    }

    if (gfKeyState[VK_UP] != 0) {
        if (m_selected_line > 0) {
            SetCurrentLine(m_selected_line - 1);
        }
    } else if (gfKeyState[VK_DOWN] != 0 && m_selected_line < m_lines.GetCount()) {
        SetCurrentLine(m_selected_line + 1);
    }

    switch (toupper(input->usParam)) {
    case ESC:
        m_selected_line = -1;
        m_keep_open = false;
        return false;
    case VK_RETURN:
        if (m_selected_line != -1) {
            m_keep_open = false;
        }
        return m_keep_open;
    case VK_PRIOR:
        SetCurrentLine(m_selected_line - GetVisibleLineCount());
        return m_keep_open;
    case VK_NEXT:
        SetCurrentLine(m_selected_line + GetVisibleLineCount());
        return m_keep_open;
    case VK_HOME:
        SetCurrentLine(0);
        return m_keep_open;
    case VK_END:
        SetCurrentLine(m_lines.GetCount() - 1);
        return m_keep_open;
    default:
        return m_keep_open;
    }
}

/* The list-box dialog. The retail layout puts the line strings in the vector
   at 0x54 and builds two text buttons, the scrolling text area, the up and
   down arrows, a slider and the confirmation pair. */
// FUNCTION: WIZ8 0x005cbdb0
int W8ListBoxDialog::CreateControls()
{
    if (W8DialogBase::CreateControls() != 0) {
        return m_error;
    }
    edge_inlay = LoadGenericButtonImages(0, Wiz8ToSgpText("Data\\Dialogs\\DialogEdge.STI"), 0,
                                         Wiz8ToSgpText("Data\\Dialogs\\DialogEdge.STI"), 0,
                                         Wiz8ToSgpText(m_background_path),
                                         static_cast<short>(m_background_flags), 0, 0);
    m_text_button =
        CreateTextButton(m_text, g_dialog_interface_font, g_dialog_font_foreground,
                         g_dialog_font_background, edge_inlay, static_cast<short>(m_x) + 9,
                         static_cast<short>(m_y) + 9, static_cast<short>(m_width) - 0x12,
                         static_cast<short>(GetFontHeight(g_dialog_interface_font) * 0x96 / 100),
                         BUTTON_NO_TOGGLE | BUTTON_IGNORE_CLICKS, MSYS_PRIORITY_HIGHEST - 1,
                         BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    if (m_text_button == -1) {
        m_error = 7;
        return 7;
    }
    if (m_text != 0) {
        SetText(m_text);
    }
    if (m_text_button == -1) {
        m_error = 7;
        return 7;
    }
    SpecifyButtonMultiColorFont(m_text_button, g_dialog_font_enabled);
    area_inlay =
        LoadGenericButtonImages(0, Wiz8ToSgpText("Data\\Dialogs\\DialogInlay.STI"), 0,
                                Wiz8ToSgpText("Data\\Dialogs\\DialogInlay.STI"), 0,
                                Wiz8ToSgpText("Data\\Dialogs\\DialogBackground_dark.STI"), 0, 3, 3);
    if (area_inlay == -1) {
        m_error = 4;
        return 4;
    }
    m_area_button = CreateTextButton(
        0, g_dialog_interface_font, g_dialog_font_foreground, g_dialog_font_background, area_inlay,
        static_cast<short>(m_x + (GetButtonX(m_text_button) - m_x)),
        static_cast<short>(m_y +
                           (GetButtonY(m_text_button) + GetButtonHeight(m_text_button) + 4 - m_y)),
        static_cast<short>(GetButtonWidth(m_text_button)), 0x14, BUTTON_NO_TOGGLE,
        MSYS_PRIORITY_HIGHEST - 1, TextAreaButtonCallback, TextAreaButtonCallback);
    if (m_area_button == -1) {
        m_error = 7;
        return 7;
    }
    SetButtonUserDataPointer(m_area_button, this);
    m_up_image = LoadButtonImage(Wiz8ToSgpText("Data\\Dialogs\\DialogUpArrow.STI"), 3, 0, 1, 2, 2);
    if (m_up_image != -1) {
        m_up_button =
            QuickCreateButton(m_up_image, 0, 0, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGHEST - 1,
                              UpButtonCallback, UpButtonCallback);
    }
    m_down_image =
        LoadButtonImage(Wiz8ToSgpText("Data\\Dialogs\\DialogDownArrow.STI"), 3, 0, 1, 2, 2);
    if (m_down_image != -1) {
        m_down_button =
            QuickCreateButton(m_down_image, 0, 0, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGHEST - 1,
                              DownButtonCallback, DownButtonCallback);
    }
    m_slider_image =
        LoadButtonImage(Wiz8ToSgpText("Data\\Dialogs\\DialogSlideBar.STI"), BUTTON_NO_IMAGE, 0,
                        BUTTON_NO_IMAGE, BUTTON_NO_IMAGE, BUTTON_NO_IMAGE);
    if (m_slider_image != -1) {
        m_slider_button =
            QuickCreateButton(m_slider_image, 0, 0, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGHEST - 2,
                              BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    }
    m_ok_image =
        LoadButtonImage(Wiz8ToSgpText("Data\\Dialogs\\DialogConfirmation.STI"), 3, 0, 1, 2, 2);
    if (m_ok_image != -1) {
        m_ok_button = QuickCreateButton(m_ok_image, 0, 0, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGHEST,
                                        OkButtonCallback, OkButtonCallback);
    }
    m_cancel_image =
        LoadButtonImage(Wiz8ToSgpText("Data\\Dialogs\\DialogConfirmation.STI"), 7, 4, 5, 6, 6);
    if (m_cancel_image != -1) {
        m_cancel_button =
            QuickCreateButton(m_cancel_image, 0, 0, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGHEST,
                              CancelButtonCallback, CancelButtonCallback);
    }
    if (m_up_button == -1 || m_down_button == -1 || m_slider_button == -1 || m_ok_button == -1 ||
        m_cancel_button == -1) {
        DestroyControls();
    } else {
        SetButtonUserDataPointer(m_up_button, this);
        SetButtonUserDataPointer(m_down_button, this);
        SetButtonUserDataPointer(m_slider_button, this);
        SetButtonUserDataPointer(m_ok_button, this);
        SetButtonUserDataPointer(m_cancel_button, this);
        third_btn_inlay = LoadGenericButtonImages(
            0, Wiz8ToSgpText("Data\\Dialogs\\DialogInlay.STI"), 0,
            Wiz8ToSgpText("Data\\Dialogs\\DialogInlay.STI"), 0,
            Wiz8ToSgpText("Data\\Dialogs\\DialogBackground_dark.STI"), 0, 3, 3);
        if (third_btn_inlay != -1) {
            m_third_text_button = CreateTextButton(
                0, g_dialog_interface_font, g_dialog_font_foreground, g_dialog_font_background,
                third_btn_inlay, 0, 0, 1, 1, BUTTON_NO_TOGGLE, MSYS_PRIORITY_HIGHEST - 2,
                SliderTrackButtonCallback, SliderTrackButtonCallback);
            if (m_third_text_button != -1) {
                SetButtonUserDataPointer(m_third_text_button, this);
                m_second_text_button = CreateTextButton(
                    0, g_dialog_interface_font, g_dialog_font_foreground, g_dialog_font_background,
                    edge_inlay, static_cast<short>(m_x + 9),
                    static_cast<short>((m_height - GetButtonHeight(m_ok_button) * 0x96 / 100) - 9 +
                                       m_y),
                    static_cast<short>(m_width - 0x12),
                    static_cast<short>(GetButtonHeight(m_ok_button) * 0x96 / 100),
                    BUTTON_NO_TOGGLE | BUTTON_IGNORE_CLICKS, MSYS_PRIORITY_HIGHEST - 1,
                    BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
                SetButtonPosition(
                    m_cancel_button,
                    GetButtonX(m_second_text_button) - GetButtonWidth(m_cancel_button) +
                        GetButtonWidth(m_second_text_button),
                    GetButtonHeight(m_second_text_button) / 2 -
                        GetButtonHeight(m_cancel_button) / 2 + GetButtonY(m_second_text_button));
                SetButtonPosition(m_ok_button,
                                  GetButtonX(m_cancel_button) -
                                      GetButtonWidth(m_cancel_button) / 2 -
                                      GetButtonWidth(m_ok_button),
                                  GetButtonY(m_cancel_button));
                ResizeButton(m_area_button, static_cast<short>(GetButtonWidth(m_text_button)),
                             static_cast<short>(-6 - GetButtonY(m_text_button) -
                                                GetButtonHeight(m_text_button) +
                                                GetButtonY(m_second_text_button)));
                int selected = m_selected_line;
                m_selected_line = 0;
                SetCurrentLine(selected);
                return 0;
            }
        }
    }
    m_error = 6;
    return 6;
}

// FUNCTION: WIZ8 0x005cc430
void W8ListBoxDialog::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    if (m_text_button != -1) {
        RemoveButton(m_text_button);
        m_text_button = -1;
    }
    if (m_second_text_button != -1) {
        RemoveButton(m_second_text_button);
        m_second_text_button = -1;
    }
    if (m_area_button != -1) {
        RemoveButton(m_area_button);
        m_area_button = -1;
    }
    if (m_up_button != -1) {
        RemoveButton(m_up_button);
        m_up_button = -1;
    }
    if (m_down_button != -1) {
        RemoveButton(m_down_button);
        m_down_button = -1;
    }
    if (m_slider_button != -1) {
        RemoveButton(m_slider_button);
        m_slider_button = -1;
    }
    if (m_third_text_button != -1) {
        RemoveButton(m_third_text_button);
        m_third_text_button = -1;
    }
    if (m_ok_button != -1) {
        RemoveButton(m_ok_button);
        m_ok_button = -1;
    }
    if (m_cancel_button != -1) {
        RemoveButton(m_cancel_button);
        m_cancel_button = -1;
    }
    if (area_inlay != -1) {
        UnloadGenericButtonImage(area_inlay);
        area_inlay = -1;
    }
    if (third_btn_inlay != -1) {
        UnloadGenericButtonImage(third_btn_inlay);
        third_btn_inlay = -1;
    }
    if (edge_inlay != -1) {
        UnloadGenericButtonImage(edge_inlay);
        edge_inlay = -1;
    }
    if (m_up_image != -1) {
        UnloadButtonImage(m_up_image);
        m_up_image = -1;
    }
    if (m_down_image != -1) {
        UnloadButtonImage(m_down_image);
        m_down_image = -1;
    }
    if (m_slider_image != -1) {
        UnloadButtonImage(m_slider_image);
        m_slider_image = -1;
    }
    if (m_ok_image != -1) {
        UnloadButtonImage(m_ok_image);
        m_ok_image = -1;
    }
    if (m_cancel_image != -1) {
        UnloadButtonImage(m_cancel_image);
        m_cancel_image = -1;
    }
    int line_count = m_lines.GetCount();
    for (index = 0; index < line_count; ++index) {
        wchar_t* line = *m_lines.GetAt(index);
        if (line != 0) {
            free(line);
        }
    }
    m_lines.Clear();
    m_field_064.Clear();
    m_selected_line = -1;
    m_field_074 = 0;
    m_scrollable = false;
    m_first_visible_line = 0;
}

/* The scrolling text area. Rows shrink by the scroll arrow when the list does
   not fit; the slider position tracks the selected line. */
// FUNCTION: WIZ8 0x005cc690
void W8ListBoxDialog::Draw()
{
    SGPRect rect;
    int index;
    int line;

    if ((m_dirty_flags & W8_DIALOG_DIRTY_REDRAW) == 0) {
        return;
    }
    W8DialogBase::Draw();
    DrawButton(m_text_button);
    DrawButton(m_second_text_button);
    int dx = GetButtonX(m_text_button) - m_x;
    int dy = GetButtonY(m_text_button) + GetButtonHeight(m_text_button) + 4 - m_y;
    int width = GetButtonWidth(m_text_button);
    int height = -6 - GetButtonY(m_text_button) - GetButtonHeight(m_text_button) +
                 GetButtonY(m_second_text_button);
    unsigned int visible_lines;
    if (m_lines.GetCount() < height / static_cast<int>(static_cast<unsigned int>(
                                          GetFontHeight(g_dialog_interface_font)))) {
        m_scrollable = false;
        visible_lines = m_lines.GetCount();
    } else {
        int rows = height / static_cast<int>(
                                static_cast<unsigned int>(GetFontHeight(g_dialog_interface_font)));
        if (m_lines.GetCount() > rows) {
            width = width + (-7 - GetButtonWidth(m_up_button));
            m_scrollable = true;
            visible_lines = rows;
        } else {
            m_scrollable = false;
            visible_lines = m_lines.GetCount();
        }
    }
    ResizeButton(m_area_button, static_cast<short>(width), static_cast<short>(height));
    DrawButton(m_area_button);
    if (m_scrollable) {
        SetButtonPosition(m_up_button, m_x + dx + 4 + width, m_y + 4 + dy);
        SetButtonPosition(m_down_button, m_x + dx + 4 + width,
                          m_y + dy + height - GetButtonHeight(m_down_button) - 4);
        SetButtonPosition(
            m_slider_button, m_x + dx + 4 + width,
            GetButtonHeight(m_area_button) +
                ((height - GetButtonHeight(m_up_button) - GetButtonHeight(m_down_button) - 7) *
                 m_selected_line) /
                    m_lines.GetCount() +
                m_y + 3 + dy);
        ColorFillVideoSurfaceArea(-0xe, m_x + width + dx, m_y + dy + GetButtonHeight(m_up_button),
                                  m_x + width + dx + GetButtonWidth(m_up_button),
                                  m_y + height + dy - GetButtonHeight(m_down_button),
                                  Get16BPPColor(m_fill_colour));
        SetButtonPosition(m_third_text_button, m_x + dx + width, m_y + dy);
        ResizeButton(m_third_text_button, static_cast<short>(GetButtonWidth(m_up_button) + 7),
                     static_cast<short>(height));
        DrawButton(m_third_text_button);
        DrawButton(m_up_button);
        DrawButton(m_down_button);
        DrawButton(m_slider_button);
    }
    if (m_ok_button != -1) {
        DrawButton(m_ok_button);
    }
    if (m_cancel_button != -1) {
        DrawButton(m_cancel_button);
    }
    SetFont(g_dialog_interface_font);
    GetButtonArea(m_area_button, &rect);
    SaveFontSettings();
    SetFontDestBuffer(-0xe, rect.iLeft + 3, rect.iTop + 3, rect.iRight - 3, rect.iBottom - 3, 0);
    if (m_lines.GetCount() <= static_cast<int>(visible_lines + m_first_visible_line)) {
        visible_lines = m_lines.GetCount() - m_first_visible_line;
    }
    for (index = 0; index < static_cast<int>(visible_lines); ++index) {
        line = m_first_visible_line + index;
        wchar_t* text = *m_lines.GetAt(line);
        if (line == m_selected_line) {
            ColorFillVideoSurfaceArea(
                -0xe, m_x + 3 + dx,
                m_y + static_cast<unsigned int>(GetFontHeight(g_dialog_interface_font)) * index +
                    2 + dy,
                width + m_x - 3 + dx,
                static_cast<unsigned int>(GetFontHeight(g_dialog_interface_font)) + m_y +
                    static_cast<unsigned int>(GetFontHeight(g_dialog_interface_font)) * index + dy,
                Get16BPPColor(m_fill_colour));
        }
        gprintf(m_x + 3 + dx,
                static_cast<unsigned int>(GetFontHeight(g_dialog_interface_font)) * index + m_y +
                    2 + dy,
                text);
    }
    RestoreFontSettings();
}

// FUNCTION: WIZ8 0x005cd470
bool W8ListBoxDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    if (gfLeftButtonState != 0) {
        if (m_selected_line != -1 &&
            IsCursorInRectangle(m_ok_rect.left, m_ok_rect.top, m_ok_rect.right, m_ok_rect.bottom)) {
            m_keep_open = false;
            return false;
        }
        if (IsCursorInRectangle(m_cancel_rect.left, m_cancel_rect.top, m_cancel_rect.right,
                                m_cancel_rect.bottom)) {
            m_selected_line = -1;
            m_keep_open = false;
            return false;
        }
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
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
        case MOUSE_WHEEL:
            SetCurrentLine(m_selected_line - GetMouseWheelDeltaValue(input.usParam));
            break;
        default:
            HandleInputEvent(&input);
            break;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005cd700
W8DialogKind W8ListBoxDialog::GetDialogType()
{
    return W8_DIALOG_LIST_BOX;
}

/* The split-size dialog ("popup_splititem.sti" with a numeric entry field):
   two stepper buttons, two passive frames around the field, accept/cancel. */
struct W8SplitAmountButtonOffset {
    int x;
    int y;
};

// GLOBAL: WIZ8 0x0064fc08
static W8SplitAmountButtonOffset g_split_amount_button_offsets[6] = {
    {0xdb, 0x20}, {0xc0, 0x20}, {0xbe, 0xf}, {0xbe, 0x35}, {0xed, 0x47}, {0x111, 0x47},
};

// GLOBAL: WIZ8 0x0064fc38
static W8ControlsRect g_split_amount_text_bounds[3] = {
    {0x4a, 0xf, 0xb8, 0x1b},
    {0x4a, 0x35, 0xb8, 0x41},
    {0xbe, 0xf, 0xf4, 0x1b},
};

// GLOBAL: WIZ8 0x0064fc68
static W8ScreenRect g_split_amount_field_bounds = {0xbe, 0x35, 0xf4, 0x41};

// GLOBAL: WIZ8 0x0064fc78
static int g_split_amount_string_ids[3] = {265, 266, 267};

// FUNCTION: WIZ8 0x005d97d0
W8SplitAmountDialog::W8SplitAmountDialog()
{
    int index;

    SetExtent(322, 111);
    SetBackground("Data\\Dialogs\\popup_splititem.sti", 2);
    for (index = 0; index < 6; ++index) {
        m_buttons[index] = 0;
    }
    m_text_buffers[0] = 0;
    m_text_buffers[1] = 0;
    m_text_buffers[2] = 0;
    m_split_input = 0;
    m_remaining = 0;
    m_taken = 0;
    m_total = 0;
    m_result = W8_SPLIT_RESULT_PENDING;
    m_active_field = 0;
}

// FUNCTION: WIZ8 0x005d9890
W8SplitAmountDialog::W8SplitAmountDialog(int total)
{
    int index;

    SetExtent(322, 111);
    SetBackground("Data\\Dialogs\\popup_splititem.sti", 2);
    for (index = 0; index < 6; ++index) {
        m_buttons[index] = 0;
    }
    m_text_buffers[0] = 0;
    m_text_buffers[1] = 0;
    m_text_buffers[2] = 0;
    m_split_input = 0;
    m_remaining = total;
    m_total = total;
    m_taken = 0;
    m_result = W8_SPLIT_RESULT_PENDING;
    m_active_field = 0;
}

// FUNCTION: WIZ8 0x005d9ac0
void W8SplitAmountDialog::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
    for (index = 0; index < 3; ++index) {
        if (m_text_buffers[index] != 0) {
            delete m_text_buffers[index];
            m_text_buffers[index] = 0;
        }
    }
    if (m_split_input != 0) {
        NoOp();
        delete m_split_input;
        m_split_input = 0;
    }
}

// FUNCTION: WIZ8 0x005d9930
W8SplitAmountDialog::~W8SplitAmountDialog()
{
    DestroyControls();
}

/* The split-item dialog. Three text buffers show the running totals, the
   numeric field edits the taken amount, and the buttons step it. */
// FUNCTION: WIZ8 0x005d99f0
int W8SplitAmountDialog::CreateControls()
{
    int index;

    W8DialogBase::CreateControls();
    m_result = W8_SPLIT_RESULT_PENDING;
    if (!CreateButtons()) {
        m_error = 7;
        return 7;
    }
    if (!CreateTextBuffers()) {
        for (index = 0; index < 6; ++index) {
            if (m_buttons[index] != 0) {
                delete m_buttons[index];
                m_buttons[index] = 0;
            }
        }
        m_error = 7;
        return 7;
    }
    if (!CreateNumericInput()) {
        for (index = 0; index < 6; ++index) {
            if (m_buttons[index] != 0) {
                delete m_buttons[index];
                m_buttons[index] = 0;
            }
        }
        for (index = 0; index < 3; ++index) {
            if (m_text_buffers[index] != 0) {
                delete m_text_buffers[index];
                m_text_buffers[index] = 0;
            }
        }
        m_error = 7;
        return 7;
    }
    UpdateButtonStates();
    UpdateTextBuffers();
    return 0;
}

// FUNCTION: WIZ8 0x005d9b30
bool W8SplitAmountDialog::CreateButtons()
{
    int index;

    for (index = 0; index < 6; ++index) {
        m_buttons[index] = new W8DialogButton;
        if (m_buttons[index] == 0) {
            for (index = 0; index < 6; ++index) {
                if (m_buttons[index] != 0) {
                    delete m_buttons[index];
                    m_buttons[index] = 0;
                }
            }
            return false;
        }
    }
    m_buttons[0]->Configure("Data\\Dialogs\\popup_splititem.sti", 0xc, 9, 10, 0xd, 0xb,
                            SplitDecrementOne, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, SplitDecrementFive, BUTTON_NO_CALLBACK);
    m_buttons[1]->Configure("Data\\Dialogs\\popup_splititem.sti", 7, 4, 5, 8, 6, SplitIncrementOne,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, SplitIncrementFive, BUTTON_NO_CALLBACK);
    m_buttons[2]->Configure("Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3,
                            BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[3]->Configure("Data\\Dialogs\\popup_splititem.sti", BUTTON_NO_IMAGE, 3,
                            BUTTON_NO_IMAGE, 3, BUTTON_NO_IMAGE, SplitActivateField,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[4]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                            SplitAccept, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[5]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 5, 6, 9, 7,
                            SplitCancel, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[0]->m_fires_on_press = true;
    m_buttons[1]->m_fires_on_press = true;
    for (index = 0; index < 6; ++index) {
        m_buttons[index]->SetPosition(g_split_amount_button_offsets[index].x + m_x,
                                      g_split_amount_button_offsets[index].y + m_y);
        m_buttons[index]->m_owner = this;
    }
    return true;
}

// FUNCTION: WIZ8 0x005d9d10
bool W8SplitAmountDialog::CreateTextBuffers()
{
    int index;
    W8ControlsRect bounds;

    for (index = 0; index < 3; ++index) {
        bounds.left = g_split_amount_text_bounds[index].left + m_x;
        bounds.top = g_split_amount_text_bounds[index].top + m_y;
        bounds.right = g_split_amount_text_bounds[index].right + m_x;
        bounds.bottom = g_split_amount_text_bounds[index].bottom + m_y;
        m_text_buffers[index] = new W8TextBuffer(
            &bounds, gppStringList[g_split_amount_string_ids[index]], g_wiz_text_font_secondary,
            g_W8TextBufferAlignMiddle | g_W8TextBufferAlignRight, 4);
        if (m_text_buffers[index] == 0) {
            for (index = 0; index < 3; ++index) {
                if (m_text_buffers[index] != 0) {
                    delete m_text_buffers[index];
                    m_text_buffers[index] = 0;
                }
            }
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005d9e30
bool W8SplitAmountDialog::CreateNumericInput()
{
    W8ControlsRect bounds;

    m_active_field = 0;
    bounds.left = g_split_amount_field_bounds.left + m_x;
    bounds.top = g_split_amount_field_bounds.top + m_y;
    bounds.right = g_split_amount_field_bounds.right + m_x;
    bounds.bottom = g_split_amount_field_bounds.bottom + m_y;
    m_split_input = new W8DialogNumericInput(0, &bounds, m_taken, g_wiz_text_font_secondary, this,
                                             m_buttons[3]);
    if (m_split_input == 0) {
        NoOp();
        delete m_split_input;
        m_split_input = 0;
        return false;
    }
    m_split_input->m_maximum = 1000000;
    return true;
}

// FUNCTION: WIZ8 0x005d9f20
void W8SplitAmountDialog::Draw()
{
    int index;

    if ((m_dirty_flags & W8_DIALOG_DIRTY_REDRAW) != 0) {
        if (!m_initialized) {
            CreateControls();
        }
        for (index = 0; index < 6; ++index) {
            m_buttons[index]->m_dirty = true;
        }
        for (index = 0; index < 3; ++index) {
            m_text_buffers[index]->SetGeometryDirty();
        }
        W8DialogNumericInput* numeric = m_split_input;
        numeric->m_dirty = true;
        numeric->m_button->m_dirty = true;
        W8DialogBase::Draw();
        DrawCatalogImage(-0xe, 0x1ac, 0, 0, m_x + 0x18, m_y + 0x1a, 2, 0);
    }
    if (m_buttons[3]->m_dirty) {
        W8DialogNumericInput* numeric = m_split_input;
        numeric->m_dirty = true;
        numeric->m_button->m_dirty = true;
    }
    for (index = 0; index < 6; ++index) {
        if (m_buttons[index] != 0) {
            m_buttons[index]->Draw();
        }
    }
    for (index = 0; index < 3; ++index) {
        if (m_text_buffers[index] != 0) {
            m_text_buffers[index]->RenderToTarget(0, false, -0xe);
        }
    }
    if (m_split_input != 0) {
        m_split_input->Draw(false);
    }
}

// FUNCTION: WIZ8 0x005da000
void W8SplitAmountDialog::UpdateTextBuffers()
{
    wchar_t text[12];

    swprintf(text, g_format_d, m_remaining);
    m_text_buffers[2]->SetText(text, g_wiz_text_font_secondary);
    m_buttons[2]->m_dirty = true;
    m_text_buffers[2]->SetGeometryDirty();
    if (m_remaining < 0) {
        m_text_buffers[2]->SetFontStateIndex(0);
    } else {
        m_text_buffers[2]->SetFontStateIndex(-1);
    }
    m_split_input->SetValue(m_taken);
    m_buttons[3]->m_dirty = true;
    m_split_input->m_dirty = true;
    m_split_input->m_button->m_dirty = true;
}

// FUNCTION: WIZ8 0x005da090
void W8SplitAmountDialog::UpdateButtonStates()
{
    if (m_taken == 0) {
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
    if (m_remaining < 0) {
        m_buttons[4]->SetEnabled(false);
        m_buttons[4]->m_dirty = true;
        return;
    }
    m_buttons[4]->SetEnabled(true);
    m_buttons[4]->m_dirty = true;
}

// FUNCTION: WIZ8 0x005da140
void W8SplitAmountDialog::OnNumericInputChanged(int value)
{
    if (value == 0) {
        int field_value = m_split_input->m_value;
        m_remaining = m_total - field_value;
        m_taken = field_value;
        UpdateButtonStates();
        UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da180
bool W8SplitAmountDialog::HandleInputEvent(const InputAtom* input)
{
    int index;
    W8DialogNumericInput** field;

    field = &m_split_input;
    for (index = 0; index < 1; ++index) {
        if (*field != 0 && (*field)->m_active != 0 && (*field)->HandleInput(input)) {
            return true;
        }
        ++field;
    }
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        int key = toupper(input->usParam);
        if (key == 0x1b) {
            m_keep_open = false;
        } else if (key == 0x2b) {
            m_remaining = __max(0, m_remaining - 1);
            m_taken = __min(m_taken + 1, m_total);
            UpdateButtonStates();
            UpdateTextBuffers();
            return m_keep_open;
        } else if (key == 0x2d) {
            m_taken = __max(0, m_taken - 1);
            m_remaining = __min(m_remaining + 1, m_total);
            UpdateButtonStates();
            UpdateTextBuffers();
            return m_keep_open;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005da2a0
bool W8SplitAmountDialog::ProcessInput()
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
            if (m_active_field != 0) {
                m_active_field->SetActive(false);
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

// FUNCTION: WIZ8 0x005da440
void W8SplitAmountDialog::SplitDecrementOne(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        dialog->m_taken -= 1;
        if (dialog->m_taken < 0) {
            dialog->m_taken = 0;
        }
        dialog->m_remaining += 1;
        if (dialog->m_remaining > dialog->m_total) {
            dialog->m_remaining = dialog->m_total;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da490
void W8SplitAmountDialog::SplitDecrementFive(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        dialog->m_taken -= 5;
        if (dialog->m_taken < 0) {
            dialog->m_taken = 0;
        }
        dialog->m_remaining += 5;
        if (dialog->m_remaining > dialog->m_total) {
            dialog->m_remaining = dialog->m_total;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da4e0
void W8SplitAmountDialog::SplitIncrementOne(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        dialog->m_remaining -= 1;
        if (dialog->m_remaining < 0) {
            dialog->m_remaining = 0;
        }
        dialog->m_taken += 1;
        if (dialog->m_taken > dialog->m_total) {
            dialog->m_taken = dialog->m_total;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da530
void W8SplitAmountDialog::SplitIncrementFive(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        dialog->m_remaining -= 5;
        if (dialog->m_remaining < 0) {
            dialog->m_remaining = 0;
        }
        dialog->m_taken += 5;
        if (dialog->m_taken > dialog->m_total) {
            dialog->m_taken = dialog->m_total;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da580
void W8SplitAmountDialog::SplitAccept(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        dialog->m_result = W8_SPLIT_RESULT_CONFIRMED;
        dialog->m_keep_open = false;
    }
}

// FUNCTION: WIZ8 0x005da5a0
void W8SplitAmountDialog::SplitCancel(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        dialog->m_result = W8_SPLIT_RESULT_CANCELLED;
        dialog->m_keep_open = false;
    }
}

// FUNCTION: WIZ8 0x005da5c0
void W8SplitAmountDialog::SplitActivateField(W8DialogButton* button)
{
    POINT mouse;
    POINT point;

    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner);
        SGPMouseGetPos(&mouse);
        if (dialog->m_split_input != 0) {
            point.x = mouse.x - dialog->m_x;
            point.y = mouse.y - dialog->m_y;
            if (!ScreenPointInRect(&g_split_amount_field_bounds, &point)) {
                return;
            }
            point.x -= g_split_amount_field_bounds.left;
            point.y -= g_split_amount_field_bounds.top;
            dialog->m_split_input->SetActive(true, &point);
            dialog->m_active_field = dialog->m_split_input;
        }
    }
}

/* The trigger-owned item picker. The thirteen same-sized button slots are
   allocated by CreateControls; the constructor only clears them. */
// FUNCTION: WIZ8 0x005cd710
W8TriggerItemPickerDialog::W8TriggerItemPickerDialog()
{
    int index;

    for (index = 0; index < 13; ++index) {
        m_buttons[index] = 0;
    }
    SetExtent(200, 100);
    SetOrigin(0x84, 0x50);
    SetBackground("Data\\Dialogs\\DialogBackground.STI", 0);
    m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
    m_first_item = 0;
}

// FUNCTION: WIZ8 0x005cd820
W8TriggerItemPickerDialog::~W8TriggerItemPickerDialog()
{
    int index;

    if (m_destroy_callback != 0) {
        m_destroy_callback(this);
        m_destroy_callback = 0;
    }
    W8DialogBase::DestroyControls();
    for (index = 0; index < 13; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
}

/* Allocate the thirteen button slots, then configure their frame images,
   callbacks and tooltips. Buttons 5..8 are the four item rows, 9..12 the
   scroll bar. A nil allocation clears every slot and reports failure. */
// FUNCTION: WIZ8 0x005cd8d0
bool W8TriggerItemPickerDialog::CreateButtons()
{
    int index;

    for (index = 0; index < 13; ++index) {
        m_buttons[index] = new W8DialogButton;
        if (m_buttons[index] == 0) {
            for (index = 0; index < 13; ++index) {
                if (m_buttons[index] != 0) {
                    delete m_buttons[index];
                    m_buttons[index] = 0;
                }
            }
            return false;
        }
        m_buttons[index]->m_owner = this;
    }
    m_buttons[0]->Configure("Data\\Dialogs\\popup_chest_selectionbuttons.sti", 3, 0, 1, 2, 2,
                            ToggleAllItems, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST, 0x15,
                            BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[1]->Configure("Data\\Dialogs\\chest_confirmationbuttons.sti", 0xd, 10, 0xb, 0xc, 0xc,
                            TakeSelectedToParty, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            0x13, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[2]->Configure("Data\\Dialogs\\chest_confirmationbuttons.sti", 3, 0, 1, 2, 2,
                            TakeSelectedToCharacter, BUTTON_NO_CALLBACK, false,
                            MSYS_PRIORITY_HIGHEST, 0x14, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[3]->Configure("Data\\Dialogs\\chest_confirmationbuttons.sti", 8, 5, 6, 7, 7,
                            CloseOwningDialog, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            0x16, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[4]->Configure("Data\\Dialogs\\popup_chest2.sti", BUTTON_NO_IMAGE, 0, BUTTON_NO_IMAGE,
                            BUTTON_NO_IMAGE, BUTTON_NO_IMAGE, BUTTON_NO_CALLBACK,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[5]->Configure("Data\\Dialogs\\popup_chest2.sti", BUTTON_NO_IMAGE, 1, BUTTON_NO_IMAGE,
                            2, BUTTON_NO_IMAGE, ToggleVisibleItem0, BUTTON_NO_CALLBACK, true,
                            MSYS_PRIORITY_HIGHEST - 1, W8_DIALOG_BUTTON_NO_TOOLTIP,
                            ShowVisibleItemInfo0, BUTTON_NO_CALLBACK);
    m_buttons[6]->Configure("Data\\Dialogs\\popup_chest2.sti", BUTTON_NO_IMAGE, 1, BUTTON_NO_IMAGE,
                            2, BUTTON_NO_IMAGE, ToggleVisibleItem1, BUTTON_NO_CALLBACK, true,
                            MSYS_PRIORITY_HIGHEST - 1, W8_DIALOG_BUTTON_NO_TOOLTIP,
                            ShowVisibleItemInfo1, BUTTON_NO_CALLBACK);
    m_buttons[7]->Configure("Data\\Dialogs\\popup_chest2.sti", BUTTON_NO_IMAGE, 1, BUTTON_NO_IMAGE,
                            2, BUTTON_NO_IMAGE, ToggleVisibleItem2, BUTTON_NO_CALLBACK, true,
                            MSYS_PRIORITY_HIGHEST - 1, W8_DIALOG_BUTTON_NO_TOOLTIP,
                            ShowVisibleItemInfo2, BUTTON_NO_CALLBACK);
    m_buttons[8]->Configure("Data\\Dialogs\\popup_chest2.sti", BUTTON_NO_IMAGE, 1, BUTTON_NO_IMAGE,
                            2, BUTTON_NO_IMAGE, ToggleVisibleItem3, BUTTON_NO_CALLBACK, true,
                            MSYS_PRIORITY_HIGHEST - 1, W8_DIALOG_BUTTON_NO_TOOLTIP,
                            ShowVisibleItemInfo3, BUTTON_NO_CALLBACK);
    m_buttons[9]->Configure("Data\\Dialogs\\maininterface_scroll.STI", 3, 0, 1, 2, 2, ScrollItemsUp,
                            BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                            W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[10]->Configure("Data\\Dialogs\\maininterface_scroll.STI", 0xb, 8, 9, 10, 10,
                             ScrollItemsDown, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_HIGHEST,
                             W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[11]->Configure("Data\\Dialogs\\maininterface_scroll.STI", 7, 4, 5, 6, 6,
                             BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK, false, MSYS_PRIORITY_LOWEST,
                             W8_DIALOG_BUTTON_NO_TOOLTIP, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[12]->Configure("Data\\Dialogs\\popup_chest2.sti", BUTTON_NO_IMAGE, 3, 3, 3, 3,
                             BUTTON_NO_CALLBACK, ScrollItemsToMouse, false,
                             MSYS_PRIORITY_HIGHEST - 1, W8_DIALOG_BUTTON_NO_TOOLTIP,
                             BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    m_buttons[5]->m_right_toggles = true;
    m_buttons[6]->m_right_toggles = true;
    m_buttons[7]->m_right_toggles = true;
    m_buttons[8]->m_right_toggles = true;
    for (index = 0; index < 13; ++index) {
        if (m_buttons[index] == 0) {
            for (index = 0; index < 13; ++index) {
                if (m_buttons[index] != 0) {
                    delete m_buttons[index];
                    m_buttons[index] = 0;
                }
            }
            return false;
        }
    }
    return true;
}

/* The picker reports the fourth factory kind. */
// FUNCTION: WIZ8 0x005cf240
W8DialogKind W8TriggerItemPickerDialog::GetDialogType()
{
    return W8_DIALOG_TRIGGER_ITEM_PICKER;
}

/* Sync the four visible item buttons with the scroll offset and the per-item
   enable flags. A short list pins the first row; a row scrolled past the end
   clears its pressed state and hides the button. */
// FUNCTION: WIZ8 0x005ce420
void W8TriggerItemPickerDialog::RefreshScrollButtons()
{
    int index;
    W8DialogButton** button;

    if (items.GetCount() <= 4) {
        m_first_item = 0;
    }
    button = &m_buttons[5];
    for (index = 0; index < 4; ++index) {
        int item = m_first_item + index;
        if (item < items.GetCount()) {
            (*button)->SetPressed(IsItemSelected(item));
        } else if ((*button)->IsPressed() != 0) {
            (*button)->SetPressed(false);
            (*button)->SetVisible(false);
        }
        ++button;
    }
}

void W8TriggerItemPickerDialog::SetFirstVisible(int index)
{
    if (items.GetCount() <= 4) {
        m_first_item = 0;
        return;
    }
    if (index < 0 || index > items.GetCount() - 4) {
        return;
    }
    m_first_item = index;
    m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
}

bool W8TriggerItemPickerDialog::IsItemSelected(int index)
{
    if (index < 0 || index >= items.GetCount()) {
        return false;
    }
    return *flags.GetAt(index) != 0;
}

void W8TriggerItemPickerDialog::SetItemSelected(int index, bool selected)
{
    if (index >= 0 && index < items.GetCount()) {
        flags.SetAt(index, selected);
        if (index >= m_first_item && index <= m_first_item + 3) {
            m_buttons[5 + index - m_first_item]->SetPressed(selected);
            m_buttons[5 + index - m_first_item]->m_dirty = true;
        }
    }
}

void W8TriggerItemPickerDialog::ToggleItem(int index, W8DialogButton* button)
{
    bool selected = !IsItemSelected(index);
    if (index < 0 || index >= items.GetCount()) {
        button->SetPressed(false);
    } else {
        SetItemSelected(index, selected);
    }
}

/* Move every selected item to the destination: -1 hands a copy to the shared
   party pool, any other value gives it to that party slot's character. A
   successful transfer unlinks the item entry and its flag and retries the same
   row; any failure raises the beep flag. Afterwards the scroll state refreshes
   and an emptied picker closes itself. */
// FUNCTION: WIZ8 0x005ce4c0
void W8TriggerItemPickerDialog::TransferSelectedItems(int destination)
{
    int index;
    bool failed = false;

    for (index = 0; index < items.GetCount(); ++index) {
        if (*flags.GetAt(index) != 0) {
            /* Retail 0x005CE4C0 passes the allocated copy directly to the add
               API; this caller emits neither a null check nor a free. */
            W8ItemInstance* instance = CopyWorldItemInstance(*items.GetAt(index));
            bool added;
            if (destination == -1) {
                added = AddItemToParty(instance, 1, false);
            } else {
                added =
                    AddItemToCharacter(&g_status.buffers.Char[destination], instance, 0, 1, false);
            }
            if (added) {
                items.RemoveAt(index);
                flags.RemoveAt(index);
                --index;
                m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
            } else {
                failed = true;
            }
        }
    }
    if (failed) {
        /* "\\b" in the original literal: retail stores a backspace, not a separator. */
        SoundPlay("Data\\Sound\\Misc\beep2.wav", 0);
    }
    RefreshScrollButtons();
    if (items.GetCount() == 0) {
        m_keep_open = false;
    }
}

/* The select-all button: clear every flag when all are set, otherwise set them
   all. The four visible row buttons are synced with the flags. */
// FUNCTION: WIZ8 0x005ce5f0
void W8TriggerItemPickerDialog::ToggleAllItems(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        int index;
        bool all_selected = true;

        for (index = 0; index < dialog->items.GetCount(); ++index) {
            if (!all_selected) {
                break;
            }
            if (*dialog->flags.GetAt(index) == 0) {
                all_selected = false;
            }
        }
        bool selected = !all_selected;
        for (index = 0; index < dialog->items.GetCount(); ++index) {
            dialog->SetItemSelected(index, selected);
        }
    }
}

/* Hand the selected items to the shared party pool. */
// FUNCTION: WIZ8 0x005ce6a0
void W8TriggerItemPickerDialog::TakeSelectedToParty(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8TriggerItemPickerDialog*>(button->m_owner)->TransferSelectedItems(-1);
    }
}

/* Hand the selected items to the currently selected party member. */
// FUNCTION: WIZ8 0x005ce6c0
void W8TriggerItemPickerDialog::TakeSelectedToCharacter(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8TriggerItemPickerDialog*>(button->m_owner)
            ->TransferSelectedItems(g_status.selected_character);
    }
}

/* Toggle the flag on the first visible row and press its button to match. An
   out-of-range row just unpresses the clicked button. */
// FUNCTION: WIZ8 0x005ce6f0
void W8TriggerItemPickerDialog::ToggleVisibleItem0(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        dialog->ToggleItem(dialog->m_first_item, button);
    }
}

// FUNCTION: WIZ8 0x005ce790
void W8TriggerItemPickerDialog::ToggleVisibleItem1(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount() + 1) {
            dialog->ToggleItem(dialog->m_first_item + 1, button);
        }
    }
}

// FUNCTION: WIZ8 0x005ce830
void W8TriggerItemPickerDialog::ToggleVisibleItem2(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount() + 2) {
            dialog->ToggleItem(dialog->m_first_item + 2, button);
        }
    }
}

// FUNCTION: WIZ8 0x005ce8d0
void W8TriggerItemPickerDialog::ToggleVisibleItem3(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount() + 3) {
            dialog->ToggleItem(dialog->m_first_item + 3, button);
        }
    }
}

/* The four right-click callbacks copy the visible row's world item and open
   the assay dialog on it. */
// FUNCTION: WIZ8 0x005ce970
void W8TriggerItemPickerDialog::ShowVisibleItemInfo0(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount()) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items.GetAt(dialog->m_first_item));
            OpenAssayDialog(instance, -1);
        }
    }
}

// FUNCTION: WIZ8 0x005ce9b0
void W8TriggerItemPickerDialog::ShowVisibleItemInfo1(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount() + 1) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items.GetAt(dialog->m_first_item + 1));
            OpenAssayDialog(instance, -1);
        }
    }
}

// FUNCTION: WIZ8 0x005ce9f0
void W8TriggerItemPickerDialog::ShowVisibleItemInfo2(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount() + 2) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items.GetAt(dialog->m_first_item + 2));
            OpenAssayDialog(instance, -1);
        }
    }
}

// FUNCTION: WIZ8 0x005cea30
void W8TriggerItemPickerDialog::ShowVisibleItemInfo3(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        if (dialog->m_first_item < dialog->items.GetCount() + 3) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items.GetAt(dialog->m_first_item + 3));
            OpenAssayDialog(instance, -1);
        }
    }
}

/* The two scroll arrows step the first visible row through SetFirstVisible. */
// FUNCTION: WIZ8 0x005cea70
void W8TriggerItemPickerDialog::ScrollItemsUp(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        dialog->SetFirstVisible(dialog->m_first_item - 1);
    }
}

// FUNCTION: WIZ8 0x005ceab0
void W8TriggerItemPickerDialog::ScrollItemsDown(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        dialog->SetFirstVisible(dialog->m_first_item + 1);
    }
}

/* Thumb drag callback: while the left button is held, map the live cursor
   position between the scroll buttons' top and bottom bounds onto the item
   list, then clamp through SetFirstVisible. */
// FUNCTION: WIZ8 0x005ceaf0
void W8TriggerItemPickerDialog::ScrollItemsToMouse(W8DialogButton* button)
{
    POINT point;
    int top;
    int bottom;
    int index;

    if (button != 0 && gfLeftButtonState != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner);
        SGPMouseGetPos(&point);
        if (dialog->m_buttons[12] != 0) {
            dialog->m_buttons[12]->GetX();
            dialog->m_buttons[12]->GetWidth();
            bottom = dialog->m_buttons[9]->GetHeight() + dialog->m_buttons[12]->GetY();
            top = bottom - dialog->m_buttons[10]->GetHeight() + dialog->m_buttons[12]->GetHeight();
        } else {
            bottom = top =
                reinterpret_cast< // reinterpret-ok: retail uses the button address as the fallback coordinate
                    int>(button);
        }
        bottom += dialog->m_buttons[9] != 0 ? dialog->m_buttons[9]->GetHeight() : -1;
        top -= (dialog->m_buttons[10] != 0 ? dialog->m_buttons[10]->GetHeight() : -1) +
               (dialog->m_buttons[11] != 0 ? dialog->m_buttons[11]->GetHeight() : -1);
        if (point.y < bottom) {
            point.y = bottom;
        }
        if (point.y > top) {
            point.y = top;
        }
        index = (point.y - bottom) * dialog->items.GetCount() / (top - bottom);
        dialog->SetFirstVisible(index);
    }
}

/* Keyboard list navigation. The up/down keys move the first visible row, the
   paging keys jump, the digit keys toggle one of the four visible flags and
   ESC closes the picker. */
// FUNCTION: WIZ8 0x005cec20
bool W8TriggerItemPickerDialog::HandleInputEvent(const InputAtom* input)
{
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        if (gfKeyState[VK_UP] == 0) {
            if (gfKeyState[VK_DOWN] != 0) {
                SetFirstVisible(m_first_item + 1);
            }
        } else {
            SetFirstVisible(m_first_item - 1);
        }
        switch (toupper(input->usParam)) {
        case ESC:
            m_keep_open = false;
            return false;
        case VK_PRIOR: {
            int target = m_first_item - 4;
            if (target < 0) {
                target = 0;
            }
            SetFirstVisible(target);
            return m_keep_open;
        }
        case VK_NEXT: {
            int target = m_first_item + 4;
            if (target > items.GetCount() - 4) {
                target = items.GetCount() - 4;
            }
            SetFirstVisible(target);
            return m_keep_open;
        }
        case VK_END:
            SetFirstVisible(items.GetCount() - 4);
            return m_keep_open;
        case VK_HOME:
            SetFirstVisible(0);
            return m_keep_open;
        case '1':
        case '2':
        case '3':
        case '4': {
            int index = m_first_item + input->usParam - '1';
            if (index < items.GetCount()) {
                SetItemSelected(index, !IsItemSelected(index));
            }
            break;
        }
        default:
            break;
        }
    }
    return m_keep_open;
}

/* Move the trigger's whole item group into this picker, merging each item
   through the add path. */
// FUNCTION: WIZ8 0x005cf0c0
void W8TriggerItemPickerDialog::SetItemGroup(W8WorldItem* group)
{
    W8WorldItem* item;

    m_item_group = group;
    item = ItemInfoGroupGetNext(group);
    while (item != 0) {
        ItemInfoRemoveFromGroup(group, item);
        AddItem(item);
        item = ItemInfoGroupGetNext(group);
    }
}

/* Hand every picker item back to the trigger's group, keeping the order the
   picker displayed them in. */
// FUNCTION: WIZ8 0x005cf110
W8WorldItem* W8TriggerItemPickerDialog::ReturnItemsToGroup()
{
    if (items.GetCount() == 0) {
        return m_item_group;
    }
    do {
        W8WorldItem* item = *items.GetAt(0);
        items.RemoveAt(0);
        flags.RemoveAt(0);
        ItemInfoAddToGroup(m_item_group, item);
    } while (items.GetCount() != 0);
    return m_item_group;
}

/* Merge one item into the picker. Identification comes first, then the item is
   placed next to the existing entries with the same equipment class and, among
   those, the same unidentified name, keeping each name run ordered by item id.
   Both vectors grow five at a time on this insertion path. A nil result means
   the caller's group handed in nothing. */
// FUNCTION: WIZ8 0x005ce210
int W8TriggerItemPickerDialog::AddItem(W8WorldItem* item)
{
    W8ItemInstance* instance = &item->item;

    if (instance != 0) {
        int index;

        PartyAttemptsToIdentifyItem(instance, 0);
        for (index = 0; index < items.GetCount(); ++index) {
            W8WorldItem* other = *items.GetAt(index);
            if (ItemsShareEquipClass(instance, &other->item)) {
                break;
            }
        }
        if (index < items.GetCount()) {
            W8WorldItem* other = *items.GetAt(index);
            while (index < items.GetCount() &&
                   !ItemsShareUnidentifiedName(instance, &other->item)) {
                other = *items.GetAt(index);
                ++index;
            }
            if (index < items.GetCount()) {
                while (index < items.GetCount() && other->item.iItemNo != instance->iItemNo) {
                    other = *items.GetAt(index);
                    ++index;
                }
                if (index < items.GetCount()) {
                    items.InsertAt(index, item);
                    flags.InsertAt(index, 0);
                    return -1;
                }
            }
        }
        items.Add(item);
        flags.Add(0);
    }
    return -1;
}

/* Poll the input queue for this picker. Button presses over a party portrait
   belong to the party, not the picker, and the two button-down kinds first ask
   the screen helper whether it wants them. Every handled button is forwarded
   to the SGP mouse system; the wheel scrolls the picker's first visible row in
   four-row steps and sets the redraw bit. Anything else goes to the picker's
   own event handler. */
// FUNCTION: WIZ8 0x005cef00
bool W8TriggerItemPickerDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        if ((input.usEvent == LEFT_BUTTON_DOWN || input.usEvent == RIGHT_BUTTON_DOWN) &&
            ProcessPendingEvent()) {
            continue;
        }
        if (HitTestPartyPortrait(&input) != 0) {
            continue;
        }
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
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
        case MOUSE_WHEEL: {
            short delta = GetMouseWheelDeltaValue(input.usParam);
            int first_item = m_first_item - delta;
            if (items.GetCount() <= 4) {
                m_first_item = 0;
            } else if (first_item >= 0 && first_item <= items.GetCount() - 4) {
                m_first_item = first_item;
                m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
            }
            break;
        }
        default:
            HandleInputEvent(&input);
            break;
        }
    }
    return m_keep_open;
}

/* Draw the picker. Up to four item rows share the five buttons at the top of
   the slot array; the first dirty row draws the item's catalogue image, its
   display name and its weight. A short list hides the four scroll buttons,
   otherwise they are placed around the row area and the scroll bar tracks the
   first visible item. */
// FUNCTION: WIZ8 0x005cdc70
void W8TriggerItemPickerDialog::Draw()
{
    int count = items.GetCount();
    int visible_rows;
    if (count < 3) {
        visible_rows = 2;
    } else if (count > 3) {
        visible_rows = 4;
    } else {
        visible_rows = count;
    }

    if (!m_initialized) {
        CreateControls();
    }
    if (gXStatus.fCombatMode) {
        m_buttons[2]->SetEnabled(false);
    }
    if ((m_dirty_flags & W8_DIALOG_DIRTY_REDRAW) != 0) {
        for (int index = 0; index < 13; ++index) {
            m_buttons[index]->m_dirty = true;
        }
        SetExtent(m_buttons[4]->GetWidth() + 0xe,
                  m_buttons[5]->GetHeight() * visible_rows + m_buttons[4]->GetHeight() + 0xe);
        W8DialogBase::Draw();
    }

    if (count > 4) {
        m_buttons[12]->SetVisible(true);
        m_buttons[9]->SetVisible(true);
        m_buttons[10]->SetVisible(true);
        m_buttons[11]->SetVisible(true);
        if (m_buttons[12]->m_dirty) {
            m_buttons[9]->m_dirty = true;
            m_buttons[10]->m_dirty = true;
            m_buttons[11]->m_dirty = true;
        }

        m_buttons[12]->SetPosition(m_x + m_width - m_buttons[12]->GetWidth() - 9, m_y + 7);
        m_buttons[12]->Draw();
        m_buttons[9]->SetPosition(m_buttons[12]->GetX() + 4, m_buttons[12]->GetY() + 3);
        m_buttons[9]->Draw();
        m_buttons[10]->SetPosition(m_buttons[9]->GetX(), m_buttons[12]->GetY() +
                                                             m_buttons[12]->GetHeight() - 4 -
                                                             m_buttons[10]->GetHeight());
        m_buttons[10]->Draw();

        int progress = 0;
        if (m_first_item != 0) {
            progress = items.GetCount();
            if (m_first_item + 4 < items.GetCount()) {
                progress = m_first_item + 2;
            }
        }
        int travel = m_buttons[12]->GetHeight() - m_buttons[9]->GetHeight() -
                     m_buttons[10]->GetHeight() - m_buttons[11]->GetHeight();
        m_buttons[11]->SetPosition(m_buttons[9]->GetX(),
                                   m_buttons[9]->GetHeight() + m_buttons[12]->GetY() +
                                       travel * progress / items.GetCount() + 1);
        m_buttons[11]->Draw();
    } else {
        m_buttons[12]->SetVisible(false);
        m_buttons[9]->SetVisible(false);
        m_buttons[10]->SetVisible(false);
        m_buttons[11]->SetVisible(false);
    }

    m_buttons[4]->SetPosition(m_x + 7, m_y + m_height - m_buttons[4]->GetHeight() - 5);
    m_buttons[4]->Draw();
    m_buttons[3]->SetPosition(m_buttons[4]->GetX() - m_buttons[3]->GetWidth() +
                                  m_buttons[4]->GetWidth() - 1,
                              m_buttons[4]->GetY());
    m_buttons[3]->Draw();
    m_buttons[2]->SetPosition(m_buttons[3]->GetX() - m_buttons[2]->GetWidth(),
                              m_buttons[4]->GetY());
    m_buttons[2]->Draw();
    m_buttons[1]->SetPosition(m_buttons[2]->GetX() - m_buttons[1]->GetWidth(),
                              m_buttons[4]->GetY());
    m_buttons[1]->Draw();
    m_buttons[0]->SetPosition(m_buttons[1]->GetX() - m_buttons[0]->GetWidth(),
                              m_buttons[4]->GetY());
    m_buttons[0]->Draw();
    RefreshScrollButtons();

    for (int row = 0; row < visible_rows; ++row) {
        W8DialogButton* button = m_buttons[5 + row];

        button->SetVisible(true);
        button->SetPosition(m_x + 7, m_y + button->GetHeight() * row + 7);
        if (!button->m_dirty) {
            continue;
        }
        button->Draw();
        if (items.GetCount() == 0) {
            continue;
        }
        int item_index = m_first_item + row;
        if (item_index >= items.GetCount()) {
            continue;
        }
        W8WorldItem* world_item = *items.GetAt(item_index);
        W8ItemInstance* item = &world_item->item;
        int video_object = g_item_video_objects.GetOrCreateVideoObject(item->iItemNo);
        DrawCatalogImage(-0xe, video_object, 0, 0, button->GetX() + 2, button->GetY() + 2, 2, 0);
        SetFont(g_wiz_text_font);
        if (item->stack_count > 1) {
            wchar_t* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6, L"%s(%d)", name, item->stack_count);
        } else {
            wchar_t* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6, name);
        }
        unsigned short weight = g_item_records[item->iItemNo].weight;
        gprintf(button->GetX() + 0x3c, button->GetY() + GetFontHeight(g_wiz_text_font) + 6,
                L"%4.1f lbs", weight * 0.1f);
    }
}

/* Create the base controls first, then the thirteen button slots. A failed
   button allocation is reported as the dialog's own error 7. */
// FUNCTION: WIZ8 0x005cdc10
int W8TriggerItemPickerDialog::CreateControls()
{
    if (W8DialogBase::CreateControls() != 0) {
        return m_error;
    }
    if (!CreateButtons()) {
        m_error = 7;
        return 7;
    }
    return 0;
}

/* Release the base controls and every allocated button slot. */
// FUNCTION: WIZ8 0x005cdc40
void W8TriggerItemPickerDialog::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 13; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
}

// FUNCTION: WIZ8 0x005ce6e0
void W8TriggerItemPickerDialog::CloseOwningDialog(W8DialogButton* button)
{
    if (button != 0) {
        button->m_owner->m_keep_open = false;
    }
}
