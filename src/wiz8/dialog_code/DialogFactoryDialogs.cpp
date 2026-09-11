#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/ButtonUserData.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/cursor.h"
#include "wiz8/utility.h"
#include "wiz8/fonts.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/video_object_catalog.h"

#include "Button System.h"
#include "Font.h"
#include "english.h"
#include "himage.h"
#include "input.h"
#include "mousesystem_macros.h"
#include "vsurface.h"

#include <ctype.h>
#include <stdlib.h>

/* Dialog Code\DialogFactoryDialogs.cpp. The factory dialogs are the list-box
   dialog (kind 3), the split-item dialog (kind 5) and the trigger-owned item
   picker. Their button callbacks and the small positioning helpers are
   declared but not recovered in this change; the calls still match the retail
   sites. */

// FUNCTION: WIZ8 0x005cbb40
W8Dialog005CBB40::W8Dialog005CBB40()
{
    int index;

    m_field_078 = 0.05f;
    m_field_07c = 0.2f;
    m_field_080 = 0.9f;
    m_field_084 = 0.75f;
    m_field_074 = 0;
    m_field_088 = 0x6000;
    m_text_button_08c = -1;
    m_second_text_button_090 = -1;
    m_inlay_image_094 = -1;
    m_area_button_098 = -1;
    m_up_button_09c = -1;
    m_up_image_0a0 = -1;
    m_down_button_0a4 = -1;
    m_down_image_0a8 = -1;
    m_inlay_image_0b4 = -1;
    m_third_text_button_0b8 = -1;
    m_slider_button_0ac = -1;
    m_slider_image_0b0 = -1;
    m_ok_button_0bc = -1;
    m_ok_image_0c0 = -1;
    m_cancel_button_0d4 = -1;
    m_cancel_image_0d8 = -1;
    m_inlay_image_0f8 = -1;
    int line_count = m_lines_054.GetCount();
    for (index = 0; index < line_count; ++index) {
        W8WideChar* line = *m_lines_054.GetAt(index);
        if (line != 0) {
            free(line);
        }
    }
    m_lines_054.count = 0;
    m_field_064.count = 0;
    m_selected_line_0f4 = -1;
    m_field_074 = 0;
    m_field_0ec = 0;
    m_first_visible_line_0f0 = 0;
}

// SYNTHETIC: WIZ8 0x005cbcc0
// W8Dialog005CBB40::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005cbce0
W8Dialog005CBB40::~W8Dialog005CBB40()
{
    if (m_destroy_callback != 0) {
        m_destroy_callback(this);
        m_destroy_callback = 0;
    }
    DestroyControls();
}

// FUNCTION: WIZ8 0x005cbd70
void W8Dialog005CBB40::SetText(const wchar_t* text)
{
    if (m_text_button_08c != -1) {
        SpecifyButtonText(m_text_button_08c,
                          const_cast<unsigned short*>(text));
        W8DialogBase::SetText(0);
        return;
    }
    W8DialogBase::SetText(text);
}

/* The list-box dialog. The retail layout puts the line strings in the vector
   at 0x54 and builds two text buttons, the scrolling text area, the up and
   down arrows, a slider and the confirmation pair. */
// FUNCTION: WIZ8 0x005cbdb0
int W8Dialog005CBB40::CreateControls()
{
    if (W8DialogBase::CreateControls() != 0) {
        return m_error;
    }
    m_inlay_image_0f8 = LoadGenericButtonImages(
        0,
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogEdge.STI")),
        0,
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogEdge.STI")),
        0, reinterpret_cast<unsigned char*>(m_background_path), // reinterpret-ok: SGP image API takes UINT8*
        static_cast<short>(m_background_flags), 0, 0);
    m_text_button_08c = CreateTextButton(
        m_text, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec,
        g_dialog_font_background_64fded, m_inlay_image_0f8,
        static_cast<short>(m_x) + 9, static_cast<short>(m_y) + 9,
        static_cast<short>(m_width) - 0x12,
        static_cast<short>(GetFontHeight(g_dialog_font_64fde8) * 0x96 / 100),
        0x8004, 0x7e, 0, 0);
    if (m_text_button_08c == -1) {
        m_error = 7;
        return 7;
    }
    if (m_text != 0) {
        SetText(m_text);
    }
    if (m_text_button_08c == -1) {
        m_error = 7;
        return 7;
    }
    SpecifyButtonMultiColorFont(m_text_button_08c,
                                g_dialog_font_enabled_69ca32);
    m_inlay_image_094 = LoadGenericButtonImages(
        0,
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
        0,
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
        0,
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogBackground_dark.STI")),
        0, 3, 3);
    if (m_inlay_image_094 == -1) {
        m_error = 4;
        return 4;
    }
    m_area_button_098 = CreateTextButton(
        0, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec,
        g_dialog_font_background_64fded, m_inlay_image_094,
        static_cast<short>(m_x + (GetButtonX(m_text_button_08c) - m_x)),
        static_cast<short>(
            m_y + (GetButtonY(m_text_button_08c) +
                   GetButtonHeight(m_text_button_08c) + 4 - m_y)),
        static_cast<short>(GetButtonWidth(m_text_button_08c)),
        0x14, 4, 0x7e, Function5CCE70, Function5CCE70);
    if (m_area_button_098 == -1) {
        m_error = 7;
        return 7;
    }
    SetButtonUserDataPointer(m_area_button_098, this);
    m_up_image_0a0 = LoadButtonImage(
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogUpArrow.STI")),
        3, 0, 1, 2, 2);
    if (m_up_image_0a0 != -1) {
        m_up_button_09c = QuickCreateButton(m_up_image_0a0, 0, 0, 4, 0x7e,
                                            Function5CCF30, Function5CCF30);
    }
    m_down_image_0a8 = LoadButtonImage(
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogDownArrow.STI")),
        3, 0, 1, 2, 2);
    if (m_down_image_0a8 != -1) {
        m_down_button_0a4 = QuickCreateButton(m_down_image_0a8, 0, 0, 4, 0x7e,
                                              Function5CCFE0, Function5CCFE0);
    }
    m_slider_image_0b0 = LoadButtonImage(
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogSlideBar.STI")),
        -1, 0, -1, -1, -1);
    if (m_slider_image_0b0 != -1) {
        m_slider_button_0ac = QuickCreateButton(
            m_slider_image_0b0, 0, 0, 4, 0x7d, 0, 0);
    }
    m_ok_image_0c0 = LoadButtonImage(
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogConfirmation.STI")),
        3, 0, 1, 2, 2);
    if (m_ok_image_0c0 != -1) {
        m_ok_button_0bc = QuickCreateButton(m_ok_image_0c0, 0, 0, 4, 0x7f,
                                            Function5CD090, Function5CD090);
    }
    m_cancel_image_0d8 = LoadButtonImage(
        reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
            const_cast<char*>("Data\\Dialogs\\DialogConfirmation.STI")),
        7, 4, 5, 6, 6);
    if (m_cancel_image_0d8 != -1) {
        m_cancel_button_0d4 = QuickCreateButton(m_cancel_image_0d8, 0, 0, 4,
                                                0x7f, Function5CD130,
                                                Function5CD130);
    }
    if (m_up_button_09c == -1 || m_down_button_0a4 == -1 ||
        m_slider_button_0ac == -1 || m_ok_button_0bc == -1 ||
        m_cancel_button_0d4 == -1) {
        DestroyControls();
    }
    else {
        SetButtonUserDataPointer(m_up_button_09c, this);
        SetButtonUserDataPointer(m_down_button_0a4, this);
        SetButtonUserDataPointer(m_slider_button_0ac, this);
        SetButtonUserDataPointer(m_ok_button_0bc, this);
        SetButtonUserDataPointer(m_cancel_button_0d4, this);
        m_inlay_image_0b4 = LoadGenericButtonImages(
            0,
            reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
                const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
            0,
            reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
                const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
            0,
            reinterpret_cast<unsigned char*>( // reinterpret-ok: SGP image API takes UINT8*
                const_cast<char*>("Data\\Dialogs\\DialogBackground_dark.STI")),
            0, 3, 3);
        if (m_inlay_image_0b4 != -1) {
            m_third_text_button_0b8 = CreateTextButton(
                0, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec,
                g_dialog_font_background_64fded, m_inlay_image_0b4,
                0, 0, 1, 1, 4, 0x7d, Function5CD1E0, Function5CD1E0);
            if (m_third_text_button_0b8 != -1) {
                SetButtonUserDataPointer(m_third_text_button_0b8, this);
                m_second_text_button_090 = CreateTextButton(
                    0, g_dialog_font_64fde8,
                    g_dialog_font_foreground_64fdec,
                    g_dialog_font_background_64fded, m_inlay_image_0f8,
                    static_cast<short>(m_x + 9),
                    static_cast<short>(
                        (m_height -
                         GetButtonHeight(m_ok_button_0bc) * 0x96 / 100) -
                        9 + m_y),
                    static_cast<short>(m_width - 0x12),
                    static_cast<short>(
                        GetButtonHeight(m_ok_button_0bc) * 0x96 / 100),
                    0x8004, 0x7e, 0, 0);
                SetButtonPosition(
                    m_cancel_button_0d4,
                    GetButtonX(m_second_text_button_090) -
                        GetButtonWidth(m_cancel_button_0d4) +
                        GetButtonWidth(m_second_text_button_090),
                    GetButtonHeight(m_second_text_button_090) / 2 -
                        GetButtonHeight(m_cancel_button_0d4) / 2 +
                        GetButtonY(m_second_text_button_090));
                SetButtonPosition(
                    m_ok_button_0bc,
                    GetButtonX(m_cancel_button_0d4) -
                        GetButtonWidth(m_cancel_button_0d4) / 2 -
                        GetButtonWidth(m_ok_button_0bc),
                    GetButtonY(m_cancel_button_0d4));
                ResizeButton(
                    m_area_button_098,
                    static_cast<short>(GetButtonWidth(m_text_button_08c)),
                    static_cast<short>(
                        -6 - GetButtonY(m_text_button_08c) -
                        GetButtonHeight(m_text_button_08c) +
                        GetButtonY(m_second_text_button_090)));
                int selected = m_selected_line_0f4;
                m_selected_line_0f4 = 0;
                SetCurrentLine005CCB80(selected);
                return 0;
            }
        }
    }
    m_error = 6;
    return 6;
}

// FUNCTION: WIZ8 0x005cc430
void W8Dialog005CBB40::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    if (m_text_button_08c != -1) {
        RemoveButton(m_text_button_08c);
        m_text_button_08c = -1;
    }
    if (m_second_text_button_090 != -1) {
        RemoveButton(m_second_text_button_090);
        m_second_text_button_090 = -1;
    }
    if (m_area_button_098 != -1) {
        RemoveButton(m_area_button_098);
        m_area_button_098 = -1;
    }
    if (m_up_button_09c != -1) {
        RemoveButton(m_up_button_09c);
        m_up_button_09c = -1;
    }
    if (m_down_button_0a4 != -1) {
        RemoveButton(m_down_button_0a4);
        m_down_button_0a4 = -1;
    }
    if (m_slider_button_0ac != -1) {
        RemoveButton(m_slider_button_0ac);
        m_slider_button_0ac = -1;
    }
    if (m_third_text_button_0b8 != -1) {
        RemoveButton(m_third_text_button_0b8);
        m_third_text_button_0b8 = -1;
    }
    if (m_ok_button_0bc != -1) {
        RemoveButton(m_ok_button_0bc);
        m_ok_button_0bc = -1;
    }
    if (m_cancel_button_0d4 != -1) {
        RemoveButton(m_cancel_button_0d4);
        m_cancel_button_0d4 = -1;
    }
    if (m_inlay_image_094 != -1) {
        UnloadGenericButtonImage(m_inlay_image_094);
        m_inlay_image_094 = -1;
    }
    if (m_inlay_image_0b4 != -1) {
        UnloadGenericButtonImage(m_inlay_image_0b4);
        m_inlay_image_0b4 = -1;
    }
    if (m_inlay_image_0f8 != -1) {
        UnloadGenericButtonImage(m_inlay_image_0f8);
        m_inlay_image_0f8 = -1;
    }
    if (m_up_image_0a0 != -1) {
        UnloadButtonImage(m_up_image_0a0);
        m_up_image_0a0 = -1;
    }
    if (m_down_image_0a8 != -1) {
        UnloadButtonImage(m_down_image_0a8);
        m_down_image_0a8 = -1;
    }
    if (m_slider_image_0b0 != -1) {
        UnloadButtonImage(m_slider_image_0b0);
        m_slider_image_0b0 = -1;
    }
    if (m_ok_image_0c0 != -1) {
        UnloadButtonImage(m_ok_image_0c0);
        m_ok_image_0c0 = -1;
    }
    if (m_cancel_image_0d8 != -1) {
        UnloadButtonImage(m_cancel_image_0d8);
        m_cancel_image_0d8 = -1;
    }
    int line_count = m_lines_054.GetCount();
    for (index = 0; index < line_count; ++index) {
        W8WideChar* line = *m_lines_054.GetAt(index);
        if (line != 0) {
            free(line);
        }
    }
    m_lines_054.count = 0;
    m_field_064.count = 0;
    m_selected_line_0f4 = -1;
    m_field_074 = 0;
    m_field_0ec = 0;
    m_first_visible_line_0f0 = 0;
}

/* The scrolling text area. Rows shrink by the scroll arrow when the list does
   not fit; the slider position tracks the selected line. */
// FUNCTION: WIZ8 0x005cc690
void W8Dialog005CBB40::Draw()
{
    SGPRect rect;
    int index;
    int line;

    if ((m_dirty_flags & 1) == 0) {
        return;
    }
    W8DialogBase::Draw();
    DrawButton(m_text_button_08c);
    DrawButton(m_second_text_button_090);
    int dx = GetButtonX(m_text_button_08c) - m_x;
    int dy = GetButtonY(m_text_button_08c) +
             GetButtonHeight(m_text_button_08c) + 4 - m_y;
    int width = GetButtonWidth(m_text_button_08c);
    int height = -6 - GetButtonY(m_text_button_08c) -
                 GetButtonHeight(m_text_button_08c) +
                 GetButtonY(m_second_text_button_090);
    unsigned int visible_lines;
    if (m_lines_054.GetCount() <
        height / (int)(unsigned int)GetFontHeight(g_dialog_font_64fde8)) {
        m_field_0ec = 0;
        visible_lines = m_lines_054.GetCount();
    }
    else {
        int rows = height /
                   (int)(unsigned int)GetFontHeight(g_dialog_font_64fde8);
        if (m_lines_054.GetCount() > rows) {
            width = width + (-7 - GetButtonWidth(m_up_button_09c));
            m_field_0ec = 1;
            visible_lines = rows;
        }
        else {
            m_field_0ec = 0;
            visible_lines = m_lines_054.GetCount();
        }
    }
    ResizeButton(m_area_button_098, static_cast<short>(width),
                 static_cast<short>(height));
    DrawButton(m_area_button_098);
    if (m_field_0ec != 0) {
        SetButtonPosition(m_up_button_09c, m_x + dx + 4 + width,
                          m_y + 4 + dy);
        SetButtonPosition(
            m_down_button_0a4, m_x + dx + 4 + width,
            m_y + dy + height - GetButtonHeight(m_down_button_0a4) - 4);
        SetButtonPosition(
            m_slider_button_0ac, m_x + dx + 4 + width,
            GetButtonHeight(m_area_button_098) +
                ((height - GetButtonHeight(m_up_button_09c) -
                  GetButtonHeight(m_down_button_0a4) - 7) *
                 m_selected_line_0f4) /
                    m_lines_054.GetCount() +
                m_y + 3 + dy);
        ColorFillVideoSurfaceArea(
            -0xe, m_x + width + dx,
            m_y + dy + GetButtonHeight(m_up_button_09c),
            m_x + width + dx + GetButtonWidth(m_up_button_09c),
            m_y + height + dy - GetButtonHeight(m_down_button_0a4),
            Get16BPPColor(m_field_088));
        SetButtonPosition(m_third_text_button_0b8, m_x + dx + width,
                          m_y + dy);
        ResizeButton(m_third_text_button_0b8,
                     static_cast<short>(GetButtonWidth(m_up_button_09c) + 7),
                     static_cast<short>(height));
        DrawButton(m_third_text_button_0b8);
        DrawButton(m_up_button_09c);
        DrawButton(m_down_button_0a4);
        DrawButton(m_slider_button_0ac);
    }
    if (m_ok_button_0bc != -1) {
        DrawButton(m_ok_button_0bc);
    }
    if (m_cancel_button_0d4 != -1) {
        DrawButton(m_cancel_button_0d4);
    }
    SetFont(g_dialog_font_64fde8);
    GetButtonArea(m_area_button_098, &rect);
    SaveFontSettings();
    SetFontDestBuffer(-0xe, rect.iLeft + 3, rect.iTop + 3, rect.iRight - 3,
                      rect.iBottom - 3, 0);
    if (m_lines_054.GetCount() <=
        (int)(visible_lines + m_first_visible_line_0f0)) {
        visible_lines = m_lines_054.GetCount() - m_first_visible_line_0f0;
    }
    for (index = 0; index < (int)visible_lines; ++index) {
        line = m_first_visible_line_0f0 + index;
        W8WideChar* text = *m_lines_054.GetAt(line);
        if (line == m_selected_line_0f4) {
            ColorFillVideoSurfaceArea(
                -0xe, m_x + 3 + dx,
                m_y + (unsigned int)GetFontHeight(g_dialog_font_64fde8) *
                          index +
                    2 + dy,
                width + m_x - 3 + dx,
                (unsigned int)GetFontHeight(g_dialog_font_64fde8) + m_y +
                    (unsigned int)GetFontHeight(g_dialog_font_64fde8) *
                        index +
                    dy,
                Get16BPPColor(m_field_088));
        }
        gprintf(m_x + 3 + dx,
                (unsigned int)GetFontHeight(g_dialog_font_64fde8) * index +
                    m_y + 2 + dy,
                text);
    }
    RestoreFontSettings();
}

// FUNCTION: WIZ8 0x005cd470
unsigned char W8Dialog005CBB40::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    if (gfLeftButtonState != 0) {
        if (m_selected_line_0f4 != -1 &&
            IsCursorInRectangle(m_ok_rect_0c4.left, m_ok_rect_0c4.top,
                                m_ok_rect_0c4.right,
                                m_ok_rect_0c4.bottom)) {
            m_field_41 = 0;
            return 0;
        }
        if (IsCursorInRectangle(m_cancel_rect_0dc.left, m_cancel_rect_0dc.top,
                                m_cancel_rect_0dc.right,
                                m_cancel_rect_0dc.bottom)) {
            m_selected_line_0f4 = -1;
            m_field_41 = 0;
            return 0;
        }
    }
    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case MOUSE_WHEEL:
            SetCurrentLine005CCB80(
                m_selected_line_0f4 - GetMouseWheelDeltaValue(input.uiParam));
            break;
        default:
            HandleInputEvent005CD2B0(&input);
            break;
        }
    }
    return m_field_41;
}

// FUNCTION: WIZ8 0x005cd700
int W8Dialog005CBB40::GetDialogType()
{
    return 3;
}

// FUNCTION: WIZ8 0x005d97d0
W8Dialog005D97D0::W8Dialog005D97D0()
{
    int index;

    SetExtent(322, 111);
    SetBackground("Data\\Dialogs\\popup_splititem.sti", 2);
    for (index = 0; index < 6; ++index) {
        m_fields_54[index] = 0;
    }
    m_field_6c = 0;
    m_field_70 = 0;
    m_field_74 = 0;
    m_field_78 = 0;
    m_remaining_080 = 0;
    m_taken_084 = 0;
    m_total_088 = 0;
    m_result_08c = 0;
    m_active_field_7c = 0;
}

// FUNCTION: WIZ8 0x005d9ac0
void W8Dialog005D97D0::DestroyControls()
{
    int index;
    W8TextBuffer** field;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_fields_54[index] != 0) {
            delete m_fields_54[index];
            m_fields_54[index] = 0;
        }
    }
    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        if (*field != 0) {
            delete *field;
            *field = 0;
        }
        ++field;
    }
    if (m_field_78 != 0) {
        NoOp();
        ::operator delete(m_field_78);
        m_field_78 = 0;
    }
}

// FUNCTION: WIZ8 0x005d9930
W8Dialog005D97D0::~W8Dialog005D97D0()
{
    int index;
    W8TextBuffer** field;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_fields_54[index] != 0) {
            delete m_fields_54[index];
            m_fields_54[index] = 0;
        }
    }
    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        if (*field != 0) {
            delete *field;
            *field = 0;
        }
        ++field;
    }
    if (m_field_78 != 0) {
        NoOp();
        ::operator delete(m_field_78);
        m_field_78 = 0;
    }
}

/* The split-item dialog. Three text buffers show the running totals, the
   numeric field edits the taken amount, and the buttons step it. */
// FUNCTION: WIZ8 0x005d99f0
int W8Dialog005D97D0::CreateControls()
{
    int index;
    W8TextBuffer** field;

    W8DialogBase::CreateControls();
    m_result_08c = 0;
    if (CreateButtons005D9B30() == 0) {
        m_error = 7;
        return 7;
    }
    if (CreateTextBuffers005D9D10() == 0) {
        for (index = 0; index < 6; ++index) {
            if (m_fields_54[index] != 0) {
                delete m_fields_54[index];
                m_fields_54[index] = 0;
            }
        }
        m_error = 7;
        return 7;
    }
    if (CreateNumericInput005D9E30() == 0) {
        for (index = 0; index < 6; ++index) {
            if (m_fields_54[index] != 0) {
                delete m_fields_54[index];
                m_fields_54[index] = 0;
            }
        }
        field = &m_field_6c;
        for (index = 0; index < 3; ++index) {
            if (*field != 0) {
                delete *field;
                *field = 0;
            }
            ++field;
        }
        m_error = 7;
        return 7;
    }
    UpdateButtonStates005DA090();
    UpdateTextBuffers005DA000();
    return 0;
}

// FUNCTION: WIZ8 0x005d9f20
void W8Dialog005D97D0::Draw()
{
    int index;
    W8TextBuffer** field;

    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        for (index = 0; index < 6; ++index) {
            m_fields_54[index]->m_dirty = 1;
        }
        field = &m_field_6c;
        for (index = 0; index < 3; ++index) {
            (*field)->m_geometryDirty = 1;
            ++field;
        }
        W8DialogNumericInput* numeric = m_field_78;
        numeric->m_dirty = 1;
        numeric->m_button->m_dirty = 1;
        W8DialogBase::Draw();
        DrawCatalogImage(-0xe, 0x1ac, 0, 0, m_x + 0x18, m_y + 0x1a, 2, 0);
    }
    if (m_fields_54[3]->m_dirty != 0) {
        W8DialogNumericInput* numeric = m_field_78;
        numeric->m_dirty = 1;
        numeric->m_button->m_dirty = 1;
    }
    for (index = 0; index < 6; ++index) {
        if (m_fields_54[index] != 0) {
            m_fields_54[index]->Draw();
        }
    }
    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        if (*field != 0) {
            (*field)->RenderToTarget(0, 0, -0xe);
        }
        ++field;
    }
    if (m_field_78 != 0) {
        m_field_78->Draw(0);
    }
}

// FUNCTION: WIZ8 0x005da140
void W8Dialog005D97D0::OnNumericInputChanged(int value)
{
    if (value == 0) {
        int field_value = m_field_78->m_value;
        m_remaining_080 = m_total_088 - field_value;
        m_taken_084 = field_value;
        UpdateButtonStates005DA090();
        UpdateTextBuffers005DA000();
    }
}

// FUNCTION: WIZ8 0x005da2a0
unsigned char W8Dialog005D97D0::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_REPEAT, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            if (m_active_field_7c != 0) {
                m_active_field_7c->SetActive(0);
            }
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_REPEAT, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        default:
            HandleInputEvent005DA180(&input);
            break;
        }
    }
    return m_field_41;
}

/* The trigger-owned item picker. The thirteen same-sized button slots are
   allocated by CreateControls; the constructor only clears them. */
// FUNCTION: WIZ8 0x005cd710
W8Dialog005CD710::W8Dialog005CD710()
{
    int index;

    for (index = 0; index < 13; ++index) {
        m_buttons_74[index] = 0;
    }
    SetExtent(200, 100);
    SetOrigin(0x84, 0x50);
    SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    m_dirty_flags |= 1;
    m_first_item_0a8 = 0;
}

// SYNTHETIC: WIZ8 0x005cd800
// W8Dialog005CD710::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005cd820
W8Dialog005CD710::~W8Dialog005CD710()
{
    int index;

    if (m_destroy_callback != 0) {
        m_destroy_callback(this);
        m_destroy_callback = 0;
    }
    W8DialogBase::DestroyControls();
    for (index = 0; index < 13; ++index) {
        if (m_buttons_74[index] != 0) {
            delete m_buttons_74[index];
            m_buttons_74[index] = 0;
        }
    }
}

/* Allocate the thirteen button slots, then configure their frame images,
   callbacks and tooltips. Buttons 5..8 are the four item rows, 9..12 the
   scroll bar. A nil allocation clears every slot and reports failure. */
// FUNCTION: WIZ8 0x005cd8d0
unsigned char W8Dialog005CD710::CreateButtons005CD8D0()
{
    int index;

    for (index = 0; index < 13; ++index) {
        m_buttons_74[index] = new W8DialogButton;
        if (m_buttons_74[index] == 0) {
            for (index = 0; index < 13; ++index) {
                if (m_buttons_74[index] != 0) {
                    delete m_buttons_74[index];
                    m_buttons_74[index] = 0;
                }
            }
            return 0;
        }
        m_buttons_74[index]->m_owner_040 = this;
    }
    m_buttons_74[0]->Configure(
        "Data\\Dialogs\\popup_chest_selectionbuttons.sti", 3, 0, 1, 2, 2,
        Function5CE5F0, 0, 0, 0x7f, 0x15, 0, 0);
    m_buttons_74[1]->Configure(
        "Data\\Dialogs\\chest_confirmationbuttons.sti", 0xd, 10, 0xb, 0xc, 0xc,
        Function5CE6A0, 0, 0, 0x7f, 0x13, 0, 0);
    m_buttons_74[2]->Configure(
        "Data\\Dialogs\\chest_confirmationbuttons.sti", 3, 0, 1, 2, 2,
        Function5CE6C0, 0, 0, 0x7f, 0x14, 0, 0);
    m_buttons_74[3]->Configure(
        "Data\\Dialogs\\chest_confirmationbuttons.sti", 8, 5, 6, 7, 7,
        Function5CE6E0, 0, 0, 0x7f, 0x16, 0, 0);
    m_buttons_74[4]->Configure(
        "Data\\Dialogs\\popup_chest2.sti", -1, 0, -1, -1, -1, 0, 0, 0, 0, -1,
        0, 0);
    m_buttons_74[5]->Configure(
        "Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1, Function5CE6F0,
        0, 1, 0x7e, -1, Function5CE970, 0);
    m_buttons_74[6]->Configure(
        "Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1, Function5CE790,
        0, 1, 0x7e, -1, Function5CE9B0, 0);
    m_buttons_74[7]->Configure(
        "Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1, Function5CE830,
        0, 1, 0x7e, -1, Function5CE9F0, 0);
    m_buttons_74[8]->Configure(
        "Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1, Function5CE8D0,
        0, 1, 0x7e, -1, Function5CEA30, 0);
    m_buttons_74[9]->Configure(
        "Data\\Dialogs\\maininterface_scroll.STI", 3, 0, 1, 2, 2,
        Function5CEA70, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_74[10]->Configure(
        "Data\\Dialogs\\maininterface_scroll.STI", 0xb, 8, 9, 10, 10,
        Function5CEAB0, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_74[11]->Configure(
        "Data\\Dialogs\\maininterface_scroll.STI", 7, 4, 5, 6, 6, 0, 0, 0, 0,
        -1, 0, 0);
    m_buttons_74[12]->Configure(
        "Data\\Dialogs\\popup_chest2.sti", -1, 3, 3, 3, 3, 0, Function5CEAF0,
        0, 0x7e, -1, 0, 0);
    m_buttons_74[5]->unknown_037 = 1;
    m_buttons_74[6]->unknown_037 = 1;
    m_buttons_74[7]->unknown_037 = 1;
    m_buttons_74[8]->unknown_037 = 1;
    for (index = 0; index < 13; ++index) {
        if (m_buttons_74[index] == 0) {
            for (index = 0; index < 13; ++index) {
                if (m_buttons_74[index] != 0) {
                    delete m_buttons_74[index];
                    m_buttons_74[index] = 0;
                }
            }
            return 0;
        }
    }
    return 1;
}

/* The picker reports the fourth factory kind. */
// FUNCTION: WIZ8 0x005cf240
int W8Dialog005CD710::GetDialogType()
{
    return 4;
}

/* Sync the four visible item buttons with the scroll offset and the per-item
   enable flags. A short list pins the first row; a row scrolled past the end
   clears its pressed state and hides the button. */
// FUNCTION: WIZ8 0x005ce420
void W8Dialog005CD710::RefreshScrollButtons005CE420()
{
    int index;
    W8DialogButton** button;

    if (items_54.GetCount() <= 4) {
        m_first_item_0a8 = 0;
    }
    button = &m_buttons_74[5];
    for (index = 0; index < 4; ++index) {
        int item = m_first_item_0a8 + index;
        if (item < items_54.GetCount()) {
            if (item < 0 || item >= items_54.GetCount()) {
                unsigned char flag = 0;
                (*button)->SetPressed(flag);
            }
            else {
                unsigned char flag = *flags_64.GetAt(item);
                (*button)->SetPressed(flag);
            }
        }
        else if ((*button)->IsPressed() != 0) {
            (*button)->SetPressed(0);
            (*button)->SetVisible(0);
        }
        ++button;
    }
}

/* Inlined into HandleInputEvent005CEC20 in this unit; no out-of-line emission
   survives, so this definition stays in the owning unit rather than the header. */
void W8Dialog005CD710::SetFirstVisible(int index)
{
    if (items_54.GetCount() <= 4) {
        m_first_item_0a8 = 0;
        return;
    }
    if (index < 0 || index > items_54.GetCount() - 4) {
        return;
    }
    m_first_item_0a8 = index;
    m_dirty_flags |= 1;
}

/* Keyboard list navigation. The up/down keys move the first visible row, the
   paging keys jump, the digit keys toggle one of the four visible flags and
   ESC closes the picker. */
// FUNCTION: WIZ8 0x005cec20
unsigned char W8Dialog005CD710::HandleInputEvent005CEC20(const InputAtom* input)
{
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        if (gfKeyState[VK_UP] == 0) {
            if (gfKeyState[VK_DOWN] != 0) {
                SetFirstVisible(m_first_item_0a8 + 1);
            }
        }
        else {
            SetFirstVisible(m_first_item_0a8 - 1);
        }
        switch (toupper(input->usParam)) {
        case ESC:
            m_field_41 = 0;
            return 0;
        case VK_PRIOR: {
            int target = m_first_item_0a8 - 4;
            if (target < 0) {
                target = 0;
            }
            SetFirstVisible(target);
            return m_field_41;
        }        case VK_NEXT: {
            int target = m_first_item_0a8 + 4;
            if (target > items_54.GetCount() - 4) {
                target = items_54.GetCount() - 4;
            }
            SetFirstVisible(target);
            return m_field_41;
        }
        case VK_END:
            SetFirstVisible(items_54.GetCount() - 4);
            return m_field_41;
        case VK_HOME:
            SetFirstVisible(0);
            return m_field_41;
        case '1':
        case '2':
        case '3':
        case '4': {
            int index = m_first_item_0a8 + input->usParam - '1';
            if (index < items_54.GetCount()) {
                unsigned char flag;
                if (index < 0) {
                    flag = 0;
                }
                else {
                    flag = *flags_64.GetAt(index);
                }
                if (index >= 0 && index < items_54.GetCount()) {
                    flags_64.SetAt(index, flag == 0);
                    if (index >= m_first_item_0a8 &&
                        index <= m_first_item_0a8 + 3) {
                        m_buttons_74[5 + index - m_first_item_0a8]
                            ->SetPressed(flag == 0);
                        m_buttons_74[5 + index - m_first_item_0a8]
                            ->m_dirty = 1;
                        return m_field_41;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return m_field_41;
}

/* Move the trigger's whole item group into this picker, merging each item
   through the add path. */
// FUNCTION: WIZ8 0x005cf0c0
void W8Dialog005CD710::SetItemGroup005CF0C0(W8WorldItem* group)
{
    W8WorldItem* item;

    m_item_group_0ac = group;
    item = ItemInfoGroupGetNext(group);
    while (item != 0) {
        ItemInfoRemoveFromGroup(group, item);
        AddItem005CE210(item);
        item = ItemInfoGroupGetNext(group);
    }
}

/* Hand every picker item back to the trigger's group, keeping the order the
   picker displayed them in. */
// FUNCTION: WIZ8 0x005cf110
W8WorldItem* W8Dialog005CD710::ReturnItemsToGroup005CF110()
{
    if (items_54.GetCount() == 0) {
        return m_item_group_0ac;
    }
    do {
        W8WorldItem* item = *items_54.GetAt(0);
        items_54.RemoveAt(0);
        flags_64.RemoveAt(0);
        ItemInfoAddToGroup(m_item_group_0ac, item);
    } while (items_54.GetCount() != 0);
    return m_item_group_0ac;
}

/* Merge one item into the picker. Identification comes first, then the item is
   placed next to the existing entries with the same equipment class and, among
   those, the same unidentified name, keeping each name run ordered by item id.
   Both vectors grow five at a time on this insertion path. A nil result means
   the caller's group handed in nothing. */
// FUNCTION: WIZ8 0x005ce210
int W8Dialog005CD710::AddItem005CE210(W8WorldItem* item)
{
    W8ItemInstance* instance = &item->item;

    if (instance != 0) {
        int index;

        PartyAttemptsToIdentifyItem(instance, 0);
        for (index = 0; index < items_54.GetCount(); ++index) {
            W8WorldItem* other = *items_54.GetAt(index);
            if (ItemsShareEquipClass(instance, &other->item)) {
                break;
            }
        }
        if (index < items_54.GetCount()) {
            W8WorldItem* other = *items_54.GetAt(index);
            while (index < items_54.GetCount() &&
                   !ItemsShareUnidentifiedName(instance, &other->item)) {
                other = *items_54.GetAt(index);
                ++index;
            }
            if (index < items_54.GetCount()) {
                while (index < items_54.GetCount() &&
                       other->item.item_id != instance->item_id) {
                    other = *items_54.GetAt(index);
                    ++index;
                }
                if (index < items_54.GetCount()) {
                    items_54.InsertAt(index, item);
                    flags_64.InsertAt(index, 0);
                    return -1;
                }
            }
        }
        items_54.Add(item);
        flags_64.Add(0);
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
unsigned char W8Dialog005CD710::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input)) {
        if ((input.usEvent == LEFT_BUTTON_DOWN ||
             input.usEvent == RIGHT_BUTTON_DOWN) &&
            Function577A40() != 0) {
            continue;
        }
        if (HitTestPartyPortrait(&input) != 0) {
            continue;
        }
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                LEFT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            break;
        case MOUSE_WHEEL: {
            short delta = GetMouseWheelDeltaValue(input.uiParam);
            int first_item = m_first_item_0a8 - delta;
            if (items_54.GetCount() < 5) {
                m_first_item_0a8 = 0;
            }
            else if (first_item >= 0 &&
                     first_item <= items_54.GetCount() - 4) {
                m_first_item_0a8 = first_item;
                m_dirty_flags |= 1;
            }
            break;
        }
        default:
            HandleInputEvent005CEC20(&input);
            break;
        }
    }
    return m_field_41;
}

/* Draw the picker. Up to four item rows share the five buttons at the top of
   the slot array; the first dirty row draws the item's catalogue image, its
   display name and its weight. A short list hides the four scroll buttons,
   otherwise they are placed around the row area and the scroll bar tracks the
   first visible item. */
// FUNCTION: WIZ8 0x005cdc70
void W8Dialog005CD710::Draw()
{
    int count = items_54.GetCount();
    int visible_rows;
    if (count < 3) {
        visible_rows = 2;
    }
    else if (count > 3) {
        visible_rows = 4;
    }
    else {
        visible_rows = count;
    }

    if (m_initialized == 0) {
        CreateControls();
    }
    if (g_in_combat_00683f94 != 0) {
        m_buttons_74[2]->SetEnabled(0);
    }
    if ((m_dirty_flags & 1) != 0) {
        for (int index = 0; index < 13; ++index) {
            m_buttons_74[index]->m_dirty = 1;
        }
        SetExtent(m_buttons_74[4]->GetWidth() + 0xe,
                  m_buttons_74[5]->GetHeight() * visible_rows +
                      m_buttons_74[4]->GetHeight() + 0xe);
        W8DialogBase::Draw();
    }

    if (count > 4) {
        m_buttons_74[12]->SetVisible(1);
        m_buttons_74[9]->SetVisible(1);
        m_buttons_74[10]->SetVisible(1);
        m_buttons_74[11]->SetVisible(1);
        if (m_buttons_74[12]->m_dirty != 0) {
            m_buttons_74[9]->m_dirty = 1;
            m_buttons_74[10]->m_dirty = 1;
            m_buttons_74[11]->m_dirty = 1;
        }

        m_buttons_74[12]->SetPosition(
            m_x + m_width - m_buttons_74[12]->GetWidth() - 9, m_y + 7);
        m_buttons_74[12]->Draw();
        m_buttons_74[9]->SetPosition(m_buttons_74[12]->GetX() + 4,
                                     m_buttons_74[12]->GetY() + 3);
        m_buttons_74[9]->Draw();
        m_buttons_74[10]->SetPosition(
            m_buttons_74[9]->GetX(),
            m_buttons_74[12]->GetY() + m_buttons_74[12]->GetHeight() - 4 -
                m_buttons_74[10]->GetHeight());
        m_buttons_74[10]->Draw();

        int progress = 0;
        if (m_first_item_0a8 != 0) {
            progress = items_54.GetCount();
            if (m_first_item_0a8 + 4 < items_54.GetCount()) {
                progress = m_first_item_0a8 + 2;
            }
        }
        int travel = m_buttons_74[12]->GetHeight() -
                     m_buttons_74[9]->GetHeight() -
                     m_buttons_74[10]->GetHeight() -
                     m_buttons_74[11]->GetHeight();
        m_buttons_74[11]->SetPosition(
            m_buttons_74[9]->GetX(),
            m_buttons_74[9]->GetHeight() + m_buttons_74[12]->GetY() +
                travel * progress / items_54.GetCount() + 1);
        m_buttons_74[11]->Draw();
    }
    else {
        m_buttons_74[12]->SetVisible(0);
        m_buttons_74[9]->SetVisible(0);
        m_buttons_74[10]->SetVisible(0);
        m_buttons_74[11]->SetVisible(0);
    }

    m_buttons_74[4]->SetPosition(
        m_x + 7, m_y + m_height - m_buttons_74[4]->GetHeight() - 5);
    m_buttons_74[4]->Draw();
    m_buttons_74[3]->SetPosition(
        m_buttons_74[4]->GetX() - m_buttons_74[3]->GetWidth() +
            m_buttons_74[4]->GetWidth() - 1,
        m_buttons_74[4]->GetY());
    m_buttons_74[3]->Draw();
    m_buttons_74[2]->SetPosition(
        m_buttons_74[3]->GetX() - m_buttons_74[2]->GetWidth(),
        m_buttons_74[4]->GetY());
    m_buttons_74[2]->Draw();
    m_buttons_74[1]->SetPosition(
        m_buttons_74[2]->GetX() - m_buttons_74[1]->GetWidth(),
        m_buttons_74[4]->GetY());
    m_buttons_74[1]->Draw();
    m_buttons_74[0]->SetPosition(
        m_buttons_74[1]->GetX() - m_buttons_74[0]->GetWidth(),
        m_buttons_74[4]->GetY());
    m_buttons_74[0]->Draw();
    RefreshScrollButtons005CE420();

    for (int row = 0; row < visible_rows; ++row) {
        W8DialogButton* button = m_buttons_74[5 + row];

        button->SetVisible(1);
        button->SetPosition(m_x + 7,
                            m_y + button->GetHeight() * row + 7);
        if (button->m_dirty == 0) {
            continue;
        }
        button->Draw();
        if (items_54.GetCount() == 0) {
            continue;
        }
        int item_index = m_first_item_0a8 + row;
        if (item_index >= items_54.GetCount()) {
            continue;
        }
        W8WorldItem* world_item = *items_54.GetAt(item_index);
        W8ItemInstance* item = &world_item->item;
        int video_object =
            g_item_video_objects_68ec68.GetOrCreateVideoObject(item->item_id);
        DrawCatalogImage(-0xe, video_object, 0, 0, button->GetX() + 2,
                         button->GetY() + 2, 2, 0);
        SetFont(g_wiz_text_font_683640);
        if (item->stack_count > 1) {
            W8WideChar* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6,
                    (unsigned short*)L"%s (%d)", name, item->stack_count);
        }
        else {
            W8WideChar* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6,
                    (unsigned short*)name);
        }
        unsigned short weight = g_item_records[item->item_id].weight;
        gprintf(button->GetX() + 0x3c,
                button->GetY() + GetFontHeight(g_wiz_text_font_683640) + 6,
                (unsigned short*)L"%4.1f lbs",
                (double)((float)weight * 0.1f));
    }
}

/* Create the base controls first, then the thirteen button slots. A failed
   button allocation is reported as the dialog's own error 7. */
// FUNCTION: WIZ8 0x005cdc10
int W8Dialog005CD710::CreateControls()
{
    if (W8DialogBase::CreateControls() != 0) {
        return m_error;
    }
    if (CreateButtons005CD8D0() == 0) {
        m_error = 7;
        return 7;
    }
    return 0;
}

/* Release the base controls and every allocated button slot. */
// FUNCTION: WIZ8 0x005cdc40
void W8Dialog005CD710::DestroyControls()
{
    int index;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 13; ++index) {
        if (m_buttons_74[index] != 0) {
            delete m_buttons_74[index];
            m_buttons_74[index] = 0;
        }
    }
}
