#include "wiz8/dialog_code/DialogFactoryDialogs.h"
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
        wchar_t* line = *m_lines_054.GetAt(index);
        if (line != 0) {
            free(line);
        }
    }
    m_lines_054.Clear();
    m_field_064.Clear();
    m_selected_line_0f4 = -1;
    m_field_074 = 0;
    m_scrollable = 0;
    m_first_visible_line_0f0 = 0;
}

// SYNTHETIC: WIZ8 0x005cbcc0
// W8ListBoxDialog::`scalar deleting destructor'

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
    if (m_text_button_08c != -1) {
        SpecifyButtonText(m_text_button_08c, const_cast<wchar_t*>(text));
        W8DialogBase::SetText(0);
        return;
    }
    W8DialogBase::SetText(text);
}

// FUNCTION: WIZ8 0x005CC650
int W8ListBoxDialog::GetVisibleLineCount()
{
    if (m_area_button_098 == -1) {
        return 0;
    }
    return GetButtonHeight(m_area_button_098) /
           (int)(unsigned int)GetFontHeight(g_dialog_font_64fde8);
}

// FUNCTION: WIZ8 0x005CCB80
void W8ListBoxDialog::SetCurrentLine(int line)
{
    if (m_selected_line_0f4 == line) {
        return;
    }
    if (line == -1) {
        m_selected_line_0f4 = -1;
        return;
    }
    int target = line < 0 ? 0 : line;
    if (m_lines_054.count - 1 < target) {
        target = m_lines_054.count - 1;
    }
    int visible = GetVisibleLineCount();
    if (target < m_first_visible_line_0f0) {
        if (m_area_button_098 != 0) {
            int first = target;
            if (m_lines_054.count - GetVisibleLineCount() < target) {
                first = m_lines_054.count - GetVisibleLineCount();
            }
            if (first < 0) {
                first = 0;
            } else if (m_lines_054.count - GetVisibleLineCount() < target) {
                first = m_lines_054.count - GetVisibleLineCount();
            }
            if (first != m_first_visible_line_0f0) {
                m_first_visible_line_0f0 = first;
                m_dirty_flags |= 1;
            }
        }
    } else if (target < m_lines_054.count && m_first_visible_line_0f0 - 1 + visible < target) {
        int first = target - visible + 1;
        if (m_area_button_098 != 0) {
            if (m_lines_054.count - GetVisibleLineCount() < first) {
                first = m_lines_054.count - GetVisibleLineCount();
            }
            if (first < 0) {
                first = 0;
            } else if (m_lines_054.count - GetVisibleLineCount() < first) {
                first = m_lines_054.count - GetVisibleLineCount();
            }
            if (first != m_first_visible_line_0f0) {
                m_first_visible_line_0f0 = first;
                m_dirty_flags |= 1;
            }
        }
    }
    m_selected_line_0f4 = target;
    m_dirty_flags |= 1;
}

// FUNCTION: WIZ8 0x005CD2B0
unsigned char W8ListBoxDialog::HandleInputEvent(const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        return m_keep_open;
    }

    if (gfKeyState[VK_UP] != 0) {
        if (m_selected_line_0f4 > 0) {
            SetCurrentLine(m_selected_line_0f4 - 1);
        }
    } else if (gfKeyState[VK_DOWN] != 0 && m_selected_line_0f4 < m_lines_054.GetCount()) {
        SetCurrentLine(m_selected_line_0f4 + 1);
    }

    switch (toupper(input->usParam)) {
    case ESC:
        m_selected_line_0f4 = -1;
        m_keep_open = 0;
        return 0;
    case VK_RETURN:
        if (m_selected_line_0f4 != -1) {
            m_keep_open = 0;
        }
        return m_keep_open;
    case VK_PRIOR:
        SetCurrentLine(m_selected_line_0f4 - GetVisibleLineCount());
        return m_keep_open;
    case VK_NEXT:
        SetCurrentLine(m_selected_line_0f4 + GetVisibleLineCount());
        return m_keep_open;
    case VK_HOME:
        SetCurrentLine(0);
        return m_keep_open;
    case VK_END:
        SetCurrentLine(m_lines_054.GetCount() - 1);
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
    m_inlay_image_0f8 = LoadGenericButtonImages(
        0,
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogEdge.STI")),
        0,
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogEdge.STI")),
        0,
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            m_background_path),
        static_cast<short>(m_background_flags), 0, 0);
    m_text_button_08c = CreateTextButton(
        m_text, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec,
        g_dialog_font_background_64fded, m_inlay_image_0f8, static_cast<short>(m_x) + 9,
        static_cast<short>(m_y) + 9, static_cast<short>(m_width) - 0x12,
        static_cast<short>(GetFontHeight(g_dialog_font_64fde8) * 0x96 / 100), 0x8004, 0x7e, 0, 0);
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
    SpecifyButtonMultiColorFont(m_text_button_08c, g_dialog_font_enabled_69ca32);
    m_inlay_image_094 = LoadGenericButtonImages(
        0,
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
        0,
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
        0,
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogBackground_dark.STI")),
        0, 3, 3);
    if (m_inlay_image_094 == -1) {
        m_error = 4;
        return 4;
    }
    m_area_button_098 = CreateTextButton(
        0, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec, g_dialog_font_background_64fded,
        m_inlay_image_094, static_cast<short>(m_x + (GetButtonX(m_text_button_08c) - m_x)),
        static_cast<short>(
            m_y + (GetButtonY(m_text_button_08c) + GetButtonHeight(m_text_button_08c) + 4 - m_y)),
        static_cast<short>(GetButtonWidth(m_text_button_08c)), 0x14, 4, 0x7e,
        TextAreaButtonCallback, TextAreaButtonCallback);
    if (m_area_button_098 == -1) {
        m_error = 7;
        return 7;
    }
    SetButtonUserDataPointer(m_area_button_098, this);
    m_up_image_0a0 = LoadButtonImage(
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogUpArrow.STI")),
        3, 0, 1, 2, 2);
    if (m_up_image_0a0 != -1) {
        m_up_button_09c =
            QuickCreateButton(m_up_image_0a0, 0, 0, 4, 0x7e, UpButtonCallback, UpButtonCallback);
    }
    m_down_image_0a8 = LoadButtonImage(
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogDownArrow.STI")),
        3, 0, 1, 2, 2);
    if (m_down_image_0a8 != -1) {
        m_down_button_0a4 = QuickCreateButton(m_down_image_0a8, 0, 0, 4, 0x7e, DownButtonCallback,
                                              DownButtonCallback);
    }
    m_slider_image_0b0 = LoadButtonImage(
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogSlideBar.STI")),
        -1, 0, -1, -1, -1);
    if (m_slider_image_0b0 != -1) {
        m_slider_button_0ac = QuickCreateButton(m_slider_image_0b0, 0, 0, 4, 0x7d, 0, 0);
    }
    m_ok_image_0c0 = LoadButtonImage(
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogConfirmation.STI")),
        3, 0, 1, 2, 2);
    if (m_ok_image_0c0 != -1) {
        m_ok_button_0bc =
            QuickCreateButton(m_ok_image_0c0, 0, 0, 4, 0x7f, OkButtonCallback, OkButtonCallback);
    }
    m_cancel_image_0d8 = LoadButtonImage(
        reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
            const_cast<char*>("Data\\Dialogs\\DialogConfirmation.STI")),
        7, 4, 5, 6, 6);
    if (m_cancel_image_0d8 != -1) {
        m_cancel_button_0d4 = QuickCreateButton(m_cancel_image_0d8, 0, 0, 4, 0x7f,
                                                CancelButtonCallback, CancelButtonCallback);
    }
    if (m_up_button_09c == -1 || m_down_button_0a4 == -1 || m_slider_button_0ac == -1 ||
        m_ok_button_0bc == -1 || m_cancel_button_0d4 == -1) {
        DestroyControls();
    } else {
        SetButtonUserDataPointer(m_up_button_09c, this);
        SetButtonUserDataPointer(m_down_button_0a4, this);
        SetButtonUserDataPointer(m_slider_button_0ac, this);
        SetButtonUserDataPointer(m_ok_button_0bc, this);
        SetButtonUserDataPointer(m_cancel_button_0d4, this);
        m_inlay_image_0b4 = LoadGenericButtonImages(
            0,
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
            0,
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>("Data\\Dialogs\\DialogInlay.STI")),
            0,
            reinterpret_cast<UINT8*>( // reinterpret-ok: SGP API declared UINT8* for text
                const_cast<char*>("Data\\Dialogs\\DialogBackground_dark.STI")),
            0, 3, 3);
        if (m_inlay_image_0b4 != -1) {
            m_third_text_button_0b8 =
                CreateTextButton(0, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec,
                                 g_dialog_font_background_64fded, m_inlay_image_0b4, 0, 0, 1, 1, 4,
                                 0x7d, SliderTrackButtonCallback, SliderTrackButtonCallback);
            if (m_third_text_button_0b8 != -1) {
                SetButtonUserDataPointer(m_third_text_button_0b8, this);
                m_second_text_button_090 = CreateTextButton(
                    0, g_dialog_font_64fde8, g_dialog_font_foreground_64fdec,
                    g_dialog_font_background_64fded, m_inlay_image_0f8, static_cast<short>(m_x + 9),
                    static_cast<short>((m_height - GetButtonHeight(m_ok_button_0bc) * 0x96 / 100) -
                                       9 + m_y),
                    static_cast<short>(m_width - 0x12),
                    static_cast<short>(GetButtonHeight(m_ok_button_0bc) * 0x96 / 100), 0x8004, 0x7e,
                    0, 0);
                SetButtonPosition(m_cancel_button_0d4,
                                  GetButtonX(m_second_text_button_090) -
                                      GetButtonWidth(m_cancel_button_0d4) +
                                      GetButtonWidth(m_second_text_button_090),
                                  GetButtonHeight(m_second_text_button_090) / 2 -
                                      GetButtonHeight(m_cancel_button_0d4) / 2 +
                                      GetButtonY(m_second_text_button_090));
                SetButtonPosition(m_ok_button_0bc,
                                  GetButtonX(m_cancel_button_0d4) -
                                      GetButtonWidth(m_cancel_button_0d4) / 2 -
                                      GetButtonWidth(m_ok_button_0bc),
                                  GetButtonY(m_cancel_button_0d4));
                ResizeButton(m_area_button_098,
                             static_cast<short>(GetButtonWidth(m_text_button_08c)),
                             static_cast<short>(-6 - GetButtonY(m_text_button_08c) -
                                                GetButtonHeight(m_text_button_08c) +
                                                GetButtonY(m_second_text_button_090)));
                int selected = m_selected_line_0f4;
                m_selected_line_0f4 = 0;
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
        wchar_t* line = *m_lines_054.GetAt(index);
        if (line != 0) {
            free(line);
        }
    }
    m_lines_054.Clear();
    m_field_064.Clear();
    m_selected_line_0f4 = -1;
    m_field_074 = 0;
    m_scrollable = 0;
    m_first_visible_line_0f0 = 0;
}

/* The scrolling text area. Rows shrink by the scroll arrow when the list does
   not fit; the slider position tracks the selected line. */
// FUNCTION: WIZ8 0x005cc690
void W8ListBoxDialog::Draw()
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
    int dy = GetButtonY(m_text_button_08c) + GetButtonHeight(m_text_button_08c) + 4 - m_y;
    int width = GetButtonWidth(m_text_button_08c);
    int height = -6 - GetButtonY(m_text_button_08c) - GetButtonHeight(m_text_button_08c) +
                 GetButtonY(m_second_text_button_090);
    unsigned int visible_lines;
    if (m_lines_054.GetCount() < height / (int)(unsigned int)GetFontHeight(g_dialog_font_64fde8)) {
        m_scrollable = 0;
        visible_lines = m_lines_054.GetCount();
    } else {
        int rows = height / (int)(unsigned int)GetFontHeight(g_dialog_font_64fde8);
        if (m_lines_054.GetCount() > rows) {
            width = width + (-7 - GetButtonWidth(m_up_button_09c));
            m_scrollable = 1;
            visible_lines = rows;
        } else {
            m_scrollable = 0;
            visible_lines = m_lines_054.GetCount();
        }
    }
    ResizeButton(m_area_button_098, static_cast<short>(width), static_cast<short>(height));
    DrawButton(m_area_button_098);
    if (m_scrollable != 0) {
        SetButtonPosition(m_up_button_09c, m_x + dx + 4 + width, m_y + 4 + dy);
        SetButtonPosition(m_down_button_0a4, m_x + dx + 4 + width,
                          m_y + dy + height - GetButtonHeight(m_down_button_0a4) - 4);
        SetButtonPosition(m_slider_button_0ac, m_x + dx + 4 + width,
                          GetButtonHeight(m_area_button_098) +
                              ((height - GetButtonHeight(m_up_button_09c) -
                                GetButtonHeight(m_down_button_0a4) - 7) *
                               m_selected_line_0f4) /
                                  m_lines_054.GetCount() +
                              m_y + 3 + dy);
        ColorFillVideoSurfaceArea(
            -0xe, m_x + width + dx, m_y + dy + GetButtonHeight(m_up_button_09c),
            m_x + width + dx + GetButtonWidth(m_up_button_09c),
            m_y + height + dy - GetButtonHeight(m_down_button_0a4), Get16BPPColor(m_field_088));
        SetButtonPosition(m_third_text_button_0b8, m_x + dx + width, m_y + dy);
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
    SetFontDestBuffer(-0xe, rect.iLeft + 3, rect.iTop + 3, rect.iRight - 3, rect.iBottom - 3, 0);
    if (m_lines_054.GetCount() <= (int)(visible_lines + m_first_visible_line_0f0)) {
        visible_lines = m_lines_054.GetCount() - m_first_visible_line_0f0;
    }
    for (index = 0; index < (int)visible_lines; ++index) {
        line = m_first_visible_line_0f0 + index;
        wchar_t* text = *m_lines_054.GetAt(line);
        if (line == m_selected_line_0f4) {
            ColorFillVideoSurfaceArea(
                -0xe, m_x + 3 + dx,
                m_y + (unsigned int)GetFontHeight(g_dialog_font_64fde8) * index + 2 + dy,
                width + m_x - 3 + dx,
                (unsigned int)GetFontHeight(g_dialog_font_64fde8) + m_y +
                    (unsigned int)GetFontHeight(g_dialog_font_64fde8) * index + dy,
                Get16BPPColor(m_field_088));
        }
        gprintf(m_x + 3 + dx,
                (unsigned int)GetFontHeight(g_dialog_font_64fde8) * index + m_y + 2 + dy, text);
    }
    RestoreFontSettings();
}

// FUNCTION: WIZ8 0x005cd470
unsigned char W8ListBoxDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    if (gfLeftButtonState != 0) {
        if (m_selected_line_0f4 != -1 &&
            IsCursorInRectangle(m_ok_rect_0c4.left, m_ok_rect_0c4.top, m_ok_rect_0c4.right,
                                m_ok_rect_0c4.bottom)) {
            m_keep_open = 0;
            return 0;
        }
        if (IsCursorInRectangle(m_cancel_rect_0dc.left, m_cancel_rect_0dc.top,
                                m_cancel_rect_0dc.right, m_cancel_rect_0dc.bottom)) {
            m_selected_line_0f4 = -1;
            m_keep_open = 0;
            return 0;
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
            SetCurrentLine(m_selected_line_0f4 - GetMouseWheelDeltaValue(input.usParam));
            break;
        default:
            HandleInputEvent(&input);
            break;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005cd700
int W8ListBoxDialog::GetDialogType()
{
    return 3;
}

/* The split-size dialog ("popup_splititem.sti" with a numeric entry field):
   two stepper buttons, two passive frames around the field, accept/cancel. */
struct W8SplitAmountButtonOffset {
    int x;
    int y;
};

// GLOBAL: WIZ8 0x0064fc08
W8SplitAmountButtonOffset g_split_amount_button_offsets[6] = {
    {0xdb, 0x20}, {0xc0, 0x20}, {0xbe, 0xf}, {0xbe, 0x35}, {0xed, 0x47}, {0x111, 0x47},
};

// GLOBAL: WIZ8 0x0064fc38
W8ControlsRect g_split_amount_text_bounds[3] = {
    {0x4a, 0xf, 0xb8, 0x1b},
    {0x4a, 0x35, 0xb8, 0x41},
    {0xbe, 0xf, 0xf4, 0x1b},
};

// GLOBAL: WIZ8 0x0064fc68
W8ControlsRect g_split_amount_field_bounds = {0xbe, 0x35, 0xf4, 0x41};

// GLOBAL: WIZ8 0x0064fc78
int g_split_amount_string_ids[3] = {265, 266, 267};

// FUNCTION: WIZ8 0x005d97d0
W8SplitAmountDialog::W8SplitAmountDialog()
{
    int index;

    SetExtent(322, 111);
    SetBackground("Data\\Dialogs\\popup_splititem.sti", 2);
    for (index = 0; index < 6; ++index) {
        m_buttons_054[index] = 0;
    }
    m_field_6c = 0;
    m_field_70 = 0;
    m_field_74 = 0;
    m_split_input_078 = 0;
    m_remaining_080 = 0;
    m_taken_084 = 0;
    m_total_088 = 0;
    m_result_08c = 0;
    m_active_field_7c = 0;
}

// FUNCTION: WIZ8 0x005d9890
W8SplitAmountDialog::W8SplitAmountDialog(int total)
{
    int index;

    SetExtent(322, 111);
    SetBackground("Data\\Dialogs\\popup_splititem.sti", 2);
    for (index = 0; index < 6; ++index) {
        m_buttons_054[index] = 0;
    }
    m_field_6c = 0;
    m_field_70 = 0;
    m_field_74 = 0;
    m_split_input_078 = 0;
    m_remaining_080 = total;
    m_total_088 = total;
    m_taken_084 = 0;
    m_result_08c = 0;
    m_active_field_7c = 0;
}

// FUNCTION: WIZ8 0x005d9ac0
void W8SplitAmountDialog::DestroyControls()
{
    int index;
    W8TextBuffer** field;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_buttons_054[index] != 0) {
            delete m_buttons_054[index];
            m_buttons_054[index] = 0;
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
    if (m_split_input_078 != 0) {
        NoOp();
        delete m_split_input_078;
        m_split_input_078 = 0;
    }
}

// SYNTHETIC: WIZ8 0x005d9870
// W8SplitAmountDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d9930
W8SplitAmountDialog::~W8SplitAmountDialog()
{
    int index;
    W8TextBuffer** field;

    W8DialogBase::DestroyControls();
    for (index = 0; index < 6; ++index) {
        if (m_buttons_054[index] != 0) {
            delete m_buttons_054[index];
            m_buttons_054[index] = 0;
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
    if (m_split_input_078 != 0) {
        NoOp();
        delete m_split_input_078;
        m_split_input_078 = 0;
    }
}

/* The split-item dialog. Three text buffers show the running totals, the
   numeric field edits the taken amount, and the buttons step it. */
// FUNCTION: WIZ8 0x005d99f0
int W8SplitAmountDialog::CreateControls()
{
    int index;
    W8TextBuffer** field;

    W8DialogBase::CreateControls();
    m_result_08c = 0;
    if (CreateButtons() == 0) {
        m_error = 7;
        return 7;
    }
    if (CreateTextBuffers() == 0) {
        for (index = 0; index < 6; ++index) {
            if (m_buttons_054[index] != 0) {
                delete m_buttons_054[index];
                m_buttons_054[index] = 0;
            }
        }
        m_error = 7;
        return 7;
    }
    if (CreateNumericInput() == 0) {
        for (index = 0; index < 6; ++index) {
            if (m_buttons_054[index] != 0) {
                delete m_buttons_054[index];
                m_buttons_054[index] = 0;
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
    UpdateButtonStates();
    UpdateTextBuffers();
    return 0;
}

// FUNCTION: WIZ8 0x005d9b30
unsigned char W8SplitAmountDialog::CreateButtons()
{
    int index;

    for (index = 0; index < 6; ++index) {
        m_buttons_054[index] = new W8DialogButton;
        if (m_buttons_054[index] == 0) {
            for (index = 0; index < 6; ++index) {
                if (m_buttons_054[index] != 0) {
                    delete m_buttons_054[index];
                    m_buttons_054[index] = 0;
                }
            }
            return 0;
        }
    }
    m_buttons_054[0]->Configure("Data\\Dialogs\\popup_splititem.sti", 0xc, 9, 10, 0xd, 0xb,
                                SplitDecrementOne, 0, 0, 0x7f, -1, SplitDecrementFive, 0);
    m_buttons_054[1]->Configure("Data\\Dialogs\\popup_splititem.sti", 7, 4, 5, 8, 6,
                                SplitIncrementOne, 0, 0, 0x7f, -1, SplitIncrementFive, 0);
    m_buttons_054[2]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1, 0, 0, 0, 0,
                                -1, 0, 0);
    m_buttons_054[3]->Configure("Data\\Dialogs\\popup_splititem.sti", -1, 3, -1, 3, -1,
                                SplitActivateField, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_054[4]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 0, 1, 4, 2,
                                SplitAccept, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_054[5]->Configure("Data\\Dialogs\\popup_confirmationbuttons.sti", 3, 5, 6, 9, 7,
                                SplitCancel, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_054[0]->m_fires_on_press = 1;
    m_buttons_054[1]->m_fires_on_press = 1;
    for (index = 0; index < 6; ++index) {
        m_buttons_054[index]->SetPosition(g_split_amount_button_offsets[index].x + m_x,
                                          g_split_amount_button_offsets[index].y + m_y);
        m_buttons_054[index]->m_owner_040 = this;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d9d10
unsigned char W8SplitAmountDialog::CreateTextBuffers()
{
    int index;
    W8ControlsRect bounds;
    W8TextBuffer** field;

    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        bounds.left = g_split_amount_text_bounds[index].left + m_x;
        bounds.top = g_split_amount_text_bounds[index].top + m_y;
        bounds.right = g_split_amount_text_bounds[index].right + m_x;
        bounds.bottom = g_split_amount_text_bounds[index].bottom + m_y;
        *field = new W8TextBuffer(
            &bounds, gppStringList[g_split_amount_string_ids[index]], g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED550, 4);
        if (*field == 0) {
            field = &m_field_6c;
            for (index = 0; index < 3; ++index) {
                if (*field != 0) {
                    delete *field;
                    *field = 0;
                }
                ++field;
            }
            return 0;
        }
        ++field;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d9e30
unsigned char W8SplitAmountDialog::CreateNumericInput()
{
    W8ControlsRect bounds;

    m_active_field_7c = 0;
    bounds.left = g_split_amount_field_bounds.left + m_x;
    bounds.top = g_split_amount_field_bounds.top + m_y;
    bounds.right = g_split_amount_field_bounds.right + m_x;
    bounds.bottom = g_split_amount_field_bounds.bottom + m_y;
    m_split_input_078 =
        new W8DialogNumericInput(0, &bounds, m_taken_084, g_font_683660, this, m_buttons_054[3]);
    if (m_split_input_078 == 0) {
        NoOp();
        delete m_split_input_078;
        m_split_input_078 = 0;
        return 0;
    }
    m_split_input_078->m_maximum = 1000000;
    return 1;
}

// FUNCTION: WIZ8 0x005d9f20
void W8SplitAmountDialog::Draw()
{
    int index;
    W8TextBuffer** field;

    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        for (index = 0; index < 6; ++index) {
            m_buttons_054[index]->m_dirty = 1;
        }
        field = &m_field_6c;
        for (index = 0; index < 3; ++index) {
            (*field)->m_geometryDirty = 1;
            ++field;
        }
        W8DialogNumericInput* numeric = m_split_input_078;
        numeric->m_dirty = 1;
        numeric->m_button->m_dirty = 1;
        W8DialogBase::Draw();
        DrawCatalogImage(-0xe, 0x1ac, 0, 0, m_x + 0x18, m_y + 0x1a, 2, 0);
    }
    if (m_buttons_054[3]->m_dirty) {
        W8DialogNumericInput* numeric = m_split_input_078;
        numeric->m_dirty = 1;
        numeric->m_button->m_dirty = 1;
    }
    for (index = 0; index < 6; ++index) {
        if (m_buttons_054[index] != 0) {
            m_buttons_054[index]->Draw();
        }
    }
    field = &m_field_6c;
    for (index = 0; index < 3; ++index) {
        if (*field != 0) {
            (*field)->RenderToTarget(0, 0, -0xe);
        }
        ++field;
    }
    if (m_split_input_078 != 0) {
        m_split_input_078->Draw(0);
    }
}

// FUNCTION: WIZ8 0x005da000
void W8SplitAmountDialog::UpdateTextBuffers()
{
    wchar_t text[12];

    swprintf(text, g_format_d_0060aa20, m_remaining_080);
    m_field_74->SetText(text, g_font_683660);
    m_buttons_054[2]->m_dirty = 1;
    m_field_74->m_geometryDirty = 1;
    if (m_remaining_080 < 0) {
        m_field_74->m_fontStateIndex = 0;
    } else {
        m_field_74->m_fontStateIndex = -1;
    }
    m_split_input_078->SetValue(m_taken_084);
    m_buttons_054[3]->m_dirty = 1;
    m_split_input_078->m_dirty = 1;
    m_split_input_078->m_button->m_dirty = 1;
}

// FUNCTION: WIZ8 0x005da090
void W8SplitAmountDialog::UpdateButtonStates()
{
    if (m_taken_084 == 0) {
        m_buttons_054[0]->SetEnabled(0);
        m_buttons_054[0]->m_dirty = 1;
    } else if (m_buttons_054[0]->IsEnabled() == 0) {
        m_buttons_054[0]->SetEnabled(1);
        m_buttons_054[0]->m_dirty = 1;
    }
    if (m_remaining_080 == 0) {
        m_buttons_054[1]->SetEnabled(0);
        m_buttons_054[1]->m_dirty = 1;
    } else if (m_buttons_054[1]->IsEnabled() == 0) {
        m_buttons_054[1]->SetEnabled(1);
        m_buttons_054[1]->m_dirty = 1;
    }
    if (m_remaining_080 < 0) {
        m_buttons_054[4]->SetEnabled(0);
        m_buttons_054[4]->m_dirty = 1;
        return;
    }
    m_buttons_054[4]->SetEnabled(1);
    m_buttons_054[4]->m_dirty = 1;
}

// FUNCTION: WIZ8 0x005da140
void W8SplitAmountDialog::OnNumericInputChanged(int value)
{
    if (value == 0) {
        int field_value = m_split_input_078->m_value;
        m_remaining_080 = m_total_088 - field_value;
        m_taken_084 = field_value;
        UpdateButtonStates();
        UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da180
unsigned char W8SplitAmountDialog::HandleInputEvent(const InputAtom* input)
{
    int index;
    W8DialogNumericInput** field;

    field = &m_split_input_078;
    for (index = 0; index < 1; ++index) {
        if (*field != 0 && (*field)->m_active != 0 && (*field)->HandleInput(input) != 0) {
            return 1;
        }
        ++field;
    }
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        int key = toupper(input->usParam);
        if (key == 0x1b) {
            m_keep_open = 0;
        } else if (key == 0x2b) {
            m_remaining_080 = __max(0, m_remaining_080 - 1);
            m_taken_084 = __min(m_taken_084 + 1, m_total_088);
            UpdateButtonStates();
            UpdateTextBuffers();
            return m_keep_open;
        } else if (key == 0x2d) {
            m_taken_084 = __max(0, m_taken_084 - 1);
            m_remaining_080 = __min(m_remaining_080 + 1, m_total_088);
            UpdateButtonStates();
            UpdateTextBuffers();
            return m_keep_open;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005da2a0
unsigned char W8SplitAmountDialog::ProcessInput()
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
            if (m_active_field_7c != 0) {
                m_active_field_7c->SetActive(0);
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
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        dialog->m_taken_084 -= 1;
        if (dialog->m_taken_084 < 0) {
            dialog->m_taken_084 = 0;
        }
        dialog->m_remaining_080 += 1;
        if (dialog->m_remaining_080 > dialog->m_total_088) {
            dialog->m_remaining_080 = dialog->m_total_088;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da490
void W8SplitAmountDialog::SplitDecrementFive(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        dialog->m_taken_084 -= 5;
        if (dialog->m_taken_084 < 0) {
            dialog->m_taken_084 = 0;
        }
        dialog->m_remaining_080 += 5;
        if (dialog->m_remaining_080 > dialog->m_total_088) {
            dialog->m_remaining_080 = dialog->m_total_088;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da4e0
void W8SplitAmountDialog::SplitIncrementOne(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        dialog->m_remaining_080 -= 1;
        if (dialog->m_remaining_080 < 0) {
            dialog->m_remaining_080 = 0;
        }
        dialog->m_taken_084 += 1;
        if (dialog->m_taken_084 > dialog->m_total_088) {
            dialog->m_taken_084 = dialog->m_total_088;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da530
void W8SplitAmountDialog::SplitIncrementFive(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        dialog->m_remaining_080 -= 5;
        if (dialog->m_remaining_080 < 0) {
            dialog->m_remaining_080 = 0;
        }
        dialog->m_taken_084 += 5;
        if (dialog->m_taken_084 > dialog->m_total_088) {
            dialog->m_taken_084 = dialog->m_total_088;
        }
        dialog->UpdateButtonStates();
        dialog->UpdateTextBuffers();
    }
}

// FUNCTION: WIZ8 0x005da580
void W8SplitAmountDialog::SplitAccept(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        dialog->m_result_08c = 1;
        dialog->m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005da5a0
void W8SplitAmountDialog::SplitCancel(W8DialogButton* button)
{
    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        dialog->m_result_08c = 2;
        dialog->m_keep_open = 0;
    }
}

// FUNCTION: WIZ8 0x005da5c0
void W8SplitAmountDialog::SplitActivateField(W8DialogButton* button)
{
    POINT point;

    if (button != 0) {
        W8SplitAmountDialog* dialog = static_cast<W8SplitAmountDialog*>(button->m_owner_040);
        SGPMouseGetPos(&point);
        if (dialog->m_split_input_078 != 0) {
            dialog->m_split_input_078->SetActive(1, &point);
            dialog->m_active_field_7c = dialog->m_split_input_078;
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
        m_buttons_74[index] = 0;
    }
    SetExtent(200, 100);
    SetOrigin(0x84, 0x50);
    SetBackground("Data\\Dialogs\\DialogBackground.STI", 0);
    m_dirty_flags |= 1;
    m_first_item_0a8 = 0;
}

// SYNTHETIC: WIZ8 0x005cd800
// W8TriggerItemPickerDialog::`scalar deleting destructor'

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
unsigned char W8TriggerItemPickerDialog::CreateButtons()
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
    m_buttons_74[0]->Configure("Data\\Dialogs\\popup_chest_selectionbuttons.sti", 3, 0, 1, 2, 2,
                               ToggleAllItems, 0, 0, 0x7f, 0x15, 0, 0);
    m_buttons_74[1]->Configure("Data\\Dialogs\\chest_confirmationbuttons.sti", 0xd, 10, 0xb, 0xc,
                               0xc, TakeSelectedToParty, 0, 0, 0x7f, 0x13, 0, 0);
    m_buttons_74[2]->Configure("Data\\Dialogs\\chest_confirmationbuttons.sti", 3, 0, 1, 2, 2,
                               TakeSelectedToCharacter, 0, 0, 0x7f, 0x14, 0, 0);
    m_buttons_74[3]->Configure("Data\\Dialogs\\chest_confirmationbuttons.sti", 8, 5, 6, 7, 7,
                               CloseOwningDialog, 0, 0, 0x7f, 0x16, 0, 0);
    m_buttons_74[4]->Configure("Data\\Dialogs\\popup_chest2.sti", -1, 0, -1, -1, -1, 0, 0, 0, 0, -1,
                               0, 0);
    m_buttons_74[5]->Configure("Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1,
                               ToggleVisibleItem0, 0, 1, 0x7e, -1, ShowVisibleItemInfo0, 0);
    m_buttons_74[6]->Configure("Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1,
                               ToggleVisibleItem1, 0, 1, 0x7e, -1, ShowVisibleItemInfo1, 0);
    m_buttons_74[7]->Configure("Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1,
                               ToggleVisibleItem2, 0, 1, 0x7e, -1, ShowVisibleItemInfo2, 0);
    m_buttons_74[8]->Configure("Data\\Dialogs\\popup_chest2.sti", -1, 1, -1, 2, -1,
                               ToggleVisibleItem3, 0, 1, 0x7e, -1, ShowVisibleItemInfo3, 0);
    m_buttons_74[9]->Configure("Data\\Dialogs\\maininterface_scroll.STI", 3, 0, 1, 2, 2,
                               ScrollItemsUp, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_74[10]->Configure("Data\\Dialogs\\maininterface_scroll.STI", 0xb, 8, 9, 10, 10,
                                ScrollItemsDown, 0, 0, 0x7f, -1, 0, 0);
    m_buttons_74[11]->Configure("Data\\Dialogs\\maininterface_scroll.STI", 7, 4, 5, 6, 6, 0, 0, 0,
                                0, -1, 0, 0);
    m_buttons_74[12]->Configure("Data\\Dialogs\\popup_chest2.sti", -1, 3, 3, 3, 3, 0,
                                ScrollItemsToMouse, 0, 0x7e, -1, 0, 0);
    m_buttons_74[5]->m_right_toggles = 1;
    m_buttons_74[6]->m_right_toggles = 1;
    m_buttons_74[7]->m_right_toggles = 1;
    m_buttons_74[8]->m_right_toggles = 1;
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
int W8TriggerItemPickerDialog::GetDialogType()
{
    return 4;
}

/* Sync the four visible item buttons with the scroll offset and the per-item
   enable flags. A short list pins the first row; a row scrolled past the end
   clears its pressed state and hides the button. */
// FUNCTION: WIZ8 0x005ce420
void W8TriggerItemPickerDialog::RefreshScrollButtons()
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
            } else {
                unsigned char flag = *flags_64.GetAt(item);
                (*button)->SetPressed(flag);
            }
        } else if ((*button)->IsPressed() != 0) {
            (*button)->SetPressed(0);
            (*button)->SetVisible(0);
        }
        ++button;
    }
}

/* Inlined into every caller in this unit; no out-of-line emission survives, so
   this definition stays in the owning unit rather than the header. */
inline void W8TriggerItemPickerDialog::SetFirstVisible(int index)
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

/* Move every selected item to the destination: -1 hands a copy to the shared
   party pool, any other value gives it to that party slot's character. A
   successful transfer unlinks the item entry and its flag and retries the same
   row; any failure raises the beep flag. Afterwards the scroll state refreshes
   and an emptied picker closes itself. */
// FUNCTION: WIZ8 0x005ce4c0
void W8TriggerItemPickerDialog::TransferSelectedItems(int destination)
{
    int index;
    unsigned char failed = 0;

    for (index = 0; index < items_54.GetCount(); ++index) {
        if (*flags_64.GetAt(index) != 0) {
            /* Retail 0x005CE4C0 passes the allocated copy directly to the add
               API; this caller emits neither a null check nor a free. */
            W8ItemInstance* instance = CopyWorldItemInstance(*items_54.GetAt(index));
            bool added;
            if (destination == -1) {
                added = AddItemToParty(instance, 1, 0);
            } else {
                added = AddItemToCharacter(&g_status_685170.buffers.Char[destination], instance, 0,
                                           1, 0);
            }
            if (added != 0) {
                items_54.RemoveAt(index);
                flags_64.RemoveAt(index);
                --index;
                m_dirty_flags |= 1;
            } else {
                failed = 1;
            }
        }
    }
    if (failed != 0) {
        /* "\\b" in the original literal: retail stores a backspace, not a separator. */
        SoundPlay("Data\\Sound\\Misc\beep2.wav", 0);
    }
    RefreshScrollButtons();
    if (items_54.GetCount() == 0) {
        m_keep_open = 0;
    }
}

/* The select-all button: clear every flag when all are set, otherwise set them
   all. The four visible row buttons are synced with the flags. */
// FUNCTION: WIZ8 0x005ce5f0
void W8TriggerItemPickerDialog::ToggleAllItems(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        int index;
        unsigned char all_selected = 1;

        for (index = 0; index < dialog->items_54.GetCount(); ++index) {
            if (all_selected == 0) {
                break;
            }
            if (*dialog->flags_64.GetAt(index) == 0) {
                all_selected = 0;
            }
        }
        bool selected = all_selected == 0;
        for (index = 0; index < dialog->items_54.GetCount(); ++index) {
            if (index >= 0 && index < dialog->items_54.GetCount()) {
                dialog->flags_64.SetAt(index, selected);
                if (index >= dialog->m_first_item_0a8 && index <= dialog->m_first_item_0a8 + 3) {
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->SetPressed(
                        selected);
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->m_dirty = 1;
                }
            }
        }
    }
}

/* Hand the selected items to the shared party pool. */
// FUNCTION: WIZ8 0x005ce6a0
void W8TriggerItemPickerDialog::TakeSelectedToParty(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040)->TransferSelectedItems(-1);
    }
}

/* Hand the selected items to the currently selected party member. */
// FUNCTION: WIZ8 0x005ce6c0
void W8TriggerItemPickerDialog::TakeSelectedToCharacter(W8DialogButton* button)
{
    if (button != 0) {
        static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040)
            ->TransferSelectedItems(g_status_685170.selected_character);
    }
}

/* Toggle the flag on the first visible row and press its button to match. An
   out-of-range row just unpresses the clicked button. */
// FUNCTION: WIZ8 0x005ce6f0
void W8TriggerItemPickerDialog::ToggleVisibleItem0(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        int index = dialog->m_first_item_0a8;
        unsigned char flag;
        bool selected;

        if (index < 0 || index >= dialog->items_54.GetCount()) {
            flag = 0;
        } else {
            flag = *dialog->flags_64.GetAt(index);
        }
        selected = flag == 0;
        if (index < 0 || index >= dialog->items_54.GetCount()) {
            button->SetPressed(0);
        } else {
            dialog->flags_64.SetAt(index, selected);
            if (index >= dialog->m_first_item_0a8 && index <= dialog->m_first_item_0a8 + 3) {
                dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->SetPressed(selected);
                dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->m_dirty = 1;
            }
        }
    }
}

// FUNCTION: WIZ8 0x005ce790
void W8TriggerItemPickerDialog::ToggleVisibleItem1(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount() + 1) {
            int index = dialog->m_first_item_0a8 + 1;
            unsigned char flag;
            bool selected;

            if (index < 0 || index >= dialog->items_54.GetCount()) {
                flag = 0;
            } else {
                flag = *dialog->flags_64.GetAt(index);
            }
            selected = flag == 0;
            if (index < 0 || index >= dialog->items_54.GetCount()) {
                button->SetPressed(0);
            } else {
                dialog->flags_64.SetAt(index, selected);
                if (index >= dialog->m_first_item_0a8 && index <= dialog->m_first_item_0a8 + 3) {
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->SetPressed(
                        selected);
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->m_dirty = 1;
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x005ce830
void W8TriggerItemPickerDialog::ToggleVisibleItem2(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount() + 2) {
            int index = dialog->m_first_item_0a8 + 2;
            unsigned char flag;
            bool selected;

            if (index < 0 || index >= dialog->items_54.GetCount()) {
                flag = 0;
            } else {
                flag = *dialog->flags_64.GetAt(index);
            }
            selected = flag == 0;
            if (index < 0 || index >= dialog->items_54.GetCount()) {
                button->SetPressed(0);
            } else {
                dialog->flags_64.SetAt(index, selected);
                if (index >= dialog->m_first_item_0a8 && index <= dialog->m_first_item_0a8 + 3) {
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->SetPressed(
                        selected);
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->m_dirty = 1;
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x005ce8d0
void W8TriggerItemPickerDialog::ToggleVisibleItem3(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount() + 3) {
            int index = dialog->m_first_item_0a8 + 3;
            unsigned char flag;
            bool selected;

            if (index < 0 || index >= dialog->items_54.GetCount()) {
                flag = 0;
            } else {
                flag = *dialog->flags_64.GetAt(index);
            }
            selected = flag == 0;
            if (index < 0 || index >= dialog->items_54.GetCount()) {
                button->SetPressed(0);
            } else {
                dialog->flags_64.SetAt(index, selected);
                if (index >= dialog->m_first_item_0a8 && index <= dialog->m_first_item_0a8 + 3) {
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->SetPressed(
                        selected);
                    dialog->m_buttons_74[5 + index - dialog->m_first_item_0a8]->m_dirty = 1;
                }
            }
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
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount()) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items_54.GetAt(dialog->m_first_item_0a8));
            OpenAssayDialog0056AE20(instance, -1);
        }
    }
}

// FUNCTION: WIZ8 0x005ce9b0
void W8TriggerItemPickerDialog::ShowVisibleItemInfo1(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount() + 1) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items_54.GetAt(dialog->m_first_item_0a8 + 1));
            OpenAssayDialog0056AE20(instance, -1);
        }
    }
}

// FUNCTION: WIZ8 0x005ce9f0
void W8TriggerItemPickerDialog::ShowVisibleItemInfo2(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount() + 2) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items_54.GetAt(dialog->m_first_item_0a8 + 2));
            OpenAssayDialog0056AE20(instance, -1);
        }
    }
}

// FUNCTION: WIZ8 0x005cea30
void W8TriggerItemPickerDialog::ShowVisibleItemInfo3(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        if (dialog->m_first_item_0a8 < dialog->items_54.GetCount() + 3) {
            W8ItemInstance* instance =
                CopyWorldItemInstance(*dialog->items_54.GetAt(dialog->m_first_item_0a8 + 3));
            OpenAssayDialog0056AE20(instance, -1);
        }
    }
}

/* The two scroll arrows step the first visible row through SetFirstVisible. */
// FUNCTION: WIZ8 0x005cea70
void W8TriggerItemPickerDialog::ScrollItemsUp(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        dialog->SetFirstVisible(dialog->m_first_item_0a8 - 1);
    }
}

// FUNCTION: WIZ8 0x005ceab0
void W8TriggerItemPickerDialog::ScrollItemsDown(W8DialogButton* button)
{
    if (button != 0) {
        W8TriggerItemPickerDialog* dialog =
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        dialog->SetFirstVisible(dialog->m_first_item_0a8 + 1);
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
            static_cast<W8TriggerItemPickerDialog*>(button->m_owner_040);
        SGPMouseGetPos(&point);
        if (dialog->m_buttons_74[12] != 0) {
            dialog->m_buttons_74[12]->GetX();
            dialog->m_buttons_74[12]->GetWidth();
            bottom = dialog->m_buttons_74[9]->GetHeight() + dialog->m_buttons_74[12]->GetY();
            top = bottom - dialog->m_buttons_74[10]->GetHeight() +
                  dialog->m_buttons_74[12]->GetHeight();
        } else {
            bottom = top =
                reinterpret_cast< // reinterpret-ok: retail uses the button address as the fallback coordinate
                    int>(button);
        }
        bottom += dialog->m_buttons_74[9] != 0 ? dialog->m_buttons_74[9]->GetHeight() : -1;
        top -= (dialog->m_buttons_74[10] != 0 ? dialog->m_buttons_74[10]->GetHeight() : -1) +
               (dialog->m_buttons_74[11] != 0 ? dialog->m_buttons_74[11]->GetHeight() : -1);
        if (point.y > bottom) {
            point.y = bottom;
        }
        if (point.y < top) {
            point.y = top;
        }
        index = (point.y - bottom) * dialog->items_54.GetCount() / (top - bottom);
        dialog->SetFirstVisible(index);
    }
}

/* Keyboard list navigation. The up/down keys move the first visible row, the
   paging keys jump, the digit keys toggle one of the four visible flags and
   ESC closes the picker. */
// FUNCTION: WIZ8 0x005cec20
unsigned char W8TriggerItemPickerDialog::HandleInputEvent(const InputAtom* input)
{
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        if (gfKeyState[VK_UP] == 0) {
            if (gfKeyState[VK_DOWN] != 0) {
                SetFirstVisible(m_first_item_0a8 + 1);
            }
        } else {
            SetFirstVisible(m_first_item_0a8 - 1);
        }
        switch (toupper(input->usParam)) {
        case ESC:
            m_keep_open = 0;
            return 0;
        case VK_PRIOR: {
            int target = m_first_item_0a8 - 4;
            if (target < 0) {
                target = 0;
            }
            SetFirstVisible(target);
            return m_keep_open;
        }
        case VK_NEXT: {
            int target = m_first_item_0a8 + 4;
            if (target > items_54.GetCount() - 4) {
                target = items_54.GetCount() - 4;
            }
            SetFirstVisible(target);
            return m_keep_open;
        }
        case VK_END:
            SetFirstVisible(items_54.GetCount() - 4);
            return m_keep_open;
        case VK_HOME:
            SetFirstVisible(0);
            return m_keep_open;
        case '1':
        case '2':
        case '3':
        case '4': {
            int index = m_first_item_0a8 + input->usParam - '1';
            if (index < items_54.GetCount()) {
                unsigned char flag;
                if (index < 0) {
                    flag = 0;
                } else {
                    flag = *flags_64.GetAt(index);
                }
                if (index >= 0 && index < items_54.GetCount()) {
                    flags_64.SetAt(index, flag == 0);
                    if (index >= m_first_item_0a8 && index <= m_first_item_0a8 + 3) {
                        m_buttons_74[5 + index - m_first_item_0a8]->SetPressed(flag == 0);
                        m_buttons_74[5 + index - m_first_item_0a8]->m_dirty = 1;
                        return m_keep_open;
                    }
                }
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

    m_item_group_0ac = group;
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
int W8TriggerItemPickerDialog::AddItem(W8WorldItem* item)
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
                while (index < items_54.GetCount() && other->item.iItemNo != instance->iItemNo) {
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
unsigned char W8TriggerItemPickerDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        if ((input.usEvent == LEFT_BUTTON_DOWN || input.usEvent == RIGHT_BUTTON_DOWN) &&
            ProcessPendingEvent00577A40() != 0) {
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
            int first_item = m_first_item_0a8 - delta;
            if (items_54.GetCount() <= 4) {
                m_first_item_0a8 = 0;
            } else if (first_item >= 0 && first_item <= items_54.GetCount() - 4) {
                m_first_item_0a8 = first_item;
                m_dirty_flags |= 1;
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
    int count = items_54.GetCount();
    int visible_rows;
    if (count < 3) {
        visible_rows = 2;
    } else if (count > 3) {
        visible_rows = 4;
    } else {
        visible_rows = count;
    }

    if (m_initialized == 0) {
        CreateControls();
    }
    if (gXStatus.fCombatMode != 0) {
        m_buttons_74[2]->SetEnabled(0);
    }
    if ((m_dirty_flags & 1) != 0) {
        for (int index = 0; index < 13; ++index) {
            m_buttons_74[index]->m_dirty = 1;
        }
        SetExtent(m_buttons_74[4]->GetWidth() + 0xe,
                  m_buttons_74[5]->GetHeight() * visible_rows + m_buttons_74[4]->GetHeight() + 0xe);
        W8DialogBase::Draw();
    }

    if (count > 4) {
        m_buttons_74[12]->SetVisible(1);
        m_buttons_74[9]->SetVisible(1);
        m_buttons_74[10]->SetVisible(1);
        m_buttons_74[11]->SetVisible(1);
        if (m_buttons_74[12]->m_dirty) {
            m_buttons_74[9]->m_dirty = 1;
            m_buttons_74[10]->m_dirty = 1;
            m_buttons_74[11]->m_dirty = 1;
        }

        m_buttons_74[12]->SetPosition(m_x + m_width - m_buttons_74[12]->GetWidth() - 9, m_y + 7);
        m_buttons_74[12]->Draw();
        m_buttons_74[9]->SetPosition(m_buttons_74[12]->GetX() + 4, m_buttons_74[12]->GetY() + 3);
        m_buttons_74[9]->Draw();
        m_buttons_74[10]->SetPosition(m_buttons_74[9]->GetX(),
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
        int travel = m_buttons_74[12]->GetHeight() - m_buttons_74[9]->GetHeight() -
                     m_buttons_74[10]->GetHeight() - m_buttons_74[11]->GetHeight();
        m_buttons_74[11]->SetPosition(m_buttons_74[9]->GetX(),
                                      m_buttons_74[9]->GetHeight() + m_buttons_74[12]->GetY() +
                                          travel * progress / items_54.GetCount() + 1);
        m_buttons_74[11]->Draw();
    } else {
        m_buttons_74[12]->SetVisible(0);
        m_buttons_74[9]->SetVisible(0);
        m_buttons_74[10]->SetVisible(0);
        m_buttons_74[11]->SetVisible(0);
    }

    m_buttons_74[4]->SetPosition(m_x + 7, m_y + m_height - m_buttons_74[4]->GetHeight() - 5);
    m_buttons_74[4]->Draw();
    m_buttons_74[3]->SetPosition(m_buttons_74[4]->GetX() - m_buttons_74[3]->GetWidth() +
                                     m_buttons_74[4]->GetWidth() - 1,
                                 m_buttons_74[4]->GetY());
    m_buttons_74[3]->Draw();
    m_buttons_74[2]->SetPosition(m_buttons_74[3]->GetX() - m_buttons_74[2]->GetWidth(),
                                 m_buttons_74[4]->GetY());
    m_buttons_74[2]->Draw();
    m_buttons_74[1]->SetPosition(m_buttons_74[2]->GetX() - m_buttons_74[1]->GetWidth(),
                                 m_buttons_74[4]->GetY());
    m_buttons_74[1]->Draw();
    m_buttons_74[0]->SetPosition(m_buttons_74[1]->GetX() - m_buttons_74[0]->GetWidth(),
                                 m_buttons_74[4]->GetY());
    m_buttons_74[0]->Draw();
    RefreshScrollButtons();

    for (int row = 0; row < visible_rows; ++row) {
        W8DialogButton* button = m_buttons_74[5 + row];

        button->SetVisible(1);
        button->SetPosition(m_x + 7, m_y + button->GetHeight() * row + 7);
        if (!button->m_dirty) {
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
        int video_object = g_item_video_objects_68ec68.GetOrCreateVideoObject(item->iItemNo);
        DrawCatalogImage(-0xe, video_object, 0, 0, button->GetX() + 2, button->GetY() + 2, 2, 0);
        SetFont(g_wiz_text_font_683640);
        if (item->stack_count > 1) {
            wchar_t* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6, L"%s (%d)", name, item->stack_count);
        } else {
            wchar_t* name = GetItemDisplayName(item);
            gprintf(button->GetX() + 0x3c, button->GetY() + 6, name);
        }
        unsigned short weight = g_item_records[item->iItemNo].weight;
        gprintf(button->GetX() + 0x3c, button->GetY() + GetFontHeight(g_wiz_text_font_683640) + 6,
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
    if (CreateButtons() == 0) {
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
        if (m_buttons_74[index] != 0) {
            delete m_buttons_74[index];
            m_buttons_74[index] = 0;
        }
    }
}

// FUNCTION: WIZ8 0x005ce6e0
void W8TriggerItemPickerDialog::CloseOwningDialog(W8DialogButton* button)
{
    if (button != 0) {
        button->m_owner_040->m_keep_open = 0;
    }
}

// TEMPLATE: WIZ8 0x005CF200
// W8GrowableVector<unsigned char>::SetAt

// TEMPLATE: WIZ8 0x005CF220
// W8GrowableVector<unsigned char>::GetAt
