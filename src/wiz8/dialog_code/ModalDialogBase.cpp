#include "wiz8/dialog_base.h"
#include "wiz8/sr_api.h"

#include "english.h"
#include "mousesystem_macros.h"
#include "Button System.h"
#include "Font.h"

#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>

extern void Function40C710(int resource);
extern unsigned char SetValue5FF5F0(int font);
extern void Function407650(
    int x, int y, const wchar_t* format, const wchar_t* text);
extern "C" int g_dialog_font_64fde8;
extern "C" unsigned char g_dialog_font_foreground_64fdec;
extern "C" unsigned char g_dialog_font_background_64fded;
extern void Function5D32C0(GUI_BUTTON* button, int reason);
extern void Function5D3370(GUI_BUTTON* button, int reason);

// FUNCTION: WIZ8 0x005d32c0
void Function5D32C0(GUI_BUTTON* button, int reason)
{
    W8ModalDialogBase* dialog = reinterpret_cast<W8ModalDialogBase*>(
        MSYS_GetBtnUserData(button, 0));
    if (!dialog) {
        srAssertFail(
            "pDialog",
            "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogBase.cpp",
            0x267, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (!(button->uiFlags & 2)) {
            button->uiFlags |= 2;
            dialog->m_dirty_flags |= 1;
        }
    }
    else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & 2) {
            dialog->close_result = 1;
            dialog->is_open = 0;
            button->uiFlags &= ~2u;
            dialog->m_dirty_flags |= 1;
        }
    }
    else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= 1;
        dialog->m_dirty_flags |= 1;
    }
    else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~1u;
        dialog->m_dirty_flags |= 1;
    }
}

// FUNCTION: WIZ8 0x005d3370
void Function5D3370(GUI_BUTTON* button, int reason)
{
    W8ModalDialogBase* dialog = reinterpret_cast<W8ModalDialogBase*>(
        MSYS_GetBtnUserData(button, 0));
    if (!dialog) {
        srAssertFail(
            "pDialog",
            "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogBase.cpp",
            0x28c, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (!(button->uiFlags & 2)) {
            button->uiFlags |= 2;
            dialog->m_dirty_flags |= 1;
        }
    }
    else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & 2) {
            dialog->close_result = 0;
            dialog->is_open = 0;
            button->uiFlags &= ~2u;
            dialog->m_dirty_flags |= 1;
        }
    }
    else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= 1;
        dialog->m_dirty_flags |= 1;
    }
    else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~1u;
        dialog->m_dirty_flags |= 1;
    }
}

/* Dialog Code. The shared dialog base at vtable 0x005EF8B0. Its lifetime
   bodies are what every derived dialog runs before and after its own. */

// FUNCTION: WIZ8 0x005d25b0
W8ModalDialogBase::W8ModalDialogBase()
    : close_result(0),
      is_open(1),
      m_field_56(-1),
      m_field_58(-1),
      m_field_5c(-1),
      m_field_60(-1),
      m_field_74(-1),
      m_field_78(-1),
      m_lines(0),
      m_line_count(0)
{
}

/* A virtual called from a destructor has a fixed dynamic type, so the
   compiler dispatches it directly; that direct call is slot 2. */
// SYNTHETIC: WIZ8 0x005d25f0
// W8ModalDialogBase::`scalar deleting destructor'
// FUNCTION: WIZ8 0x005d2610
W8ModalDialogBase::~W8ModalDialogBase()
{
    ResetSubobjectAndRefresh();
}

// FUNCTION: WIZ8 0x005d2a50
unsigned int W8ModalDialogBase::WrapMessage(wchar_t* message)
{
    wchar_t lines[32][256];
    wchar_t* remaining;
    wchar_t* line;
    unsigned int line_index = 0;
    unsigned int words_on_line = 0;
    int line_width = 0;
    int space_width = StringPixLength(
        const_cast<unsigned short*>(L" "), g_dialog_font_64fde8);
    int maximum_width = m_width + 0xf;
    unsigned int index;

    remaining = new wchar_t[wcslen(message) + 1];
    if (!remaining) {
        srAssertFail(
            "pRemainingText",
            "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogBase.cpp",
            0xd5, 0);
    }
    wcscpy(remaining, message);
    for (index = 0; index < 32; ++index) {
        wcscpy(lines[index], L"");
    }

    line = lines[0];
    size_t word_length = wcscspn(remaining, L" ");
    while (remaining[word_length] != L'\0') {
        remaining[word_length] = L'\0';
        int word_width = StringPixLength(
            reinterpret_cast<unsigned short*>(remaining),
            g_dialog_font_64fde8);
        if (line_width + word_width > maximum_width) {
            if (words_on_line != 0) {
                ++line_index;
                line = lines[line_index];
                wcscpy(line, remaining);
                line_width = word_width;
                words_on_line = 1;
            }
            else {
                wcscat(line, remaining);
                line_width += space_width + word_width;
                ++words_on_line;
            }
        }
        else {
            if (words_on_line != 0) {
                wcscat(line, L" ");
            }
            wcscat(line, remaining);
            line_width += space_width + word_width;
            ++words_on_line;
        }
        remaining += word_length + 1;
        word_length = wcscspn(remaining, L" ");
    }

    int word_width = StringPixLength(
        reinterpret_cast<unsigned short*>(remaining), g_dialog_font_64fde8);
    if (line_width + word_width > m_width) {
        ++line_index;
        wcscpy(lines[line_index], remaining);
    }
    else {
        wcscat(lines[line_index], L" ");
        wcscat(lines[line_index], remaining);
    }

    unsigned int count = line_index + 1;
    m_lines = static_cast<wchar_t**>(malloc(count * sizeof(wchar_t*)));
    for (index = 0; index < count; ++index) {
        m_lines[index] = static_cast<wchar_t*>(
            malloc((wcslen(lines[index]) + 1) * sizeof(wchar_t)));
        wcscpy(m_lines[index], lines[index]);
    }
    return count;
}

// FUNCTION: WIZ8 0x005d2d00
int W8ModalDialogBase::vslot1()
{
    W8DialogBase005DC7A0::vslot1();
    if (m_field_56 == -1) {
        m_field_56 = LoadGenericButtonImages(
            0,
            reinterpret_cast<unsigned char*>(
                const_cast<char*>("Data\\Dialogs\\DialogEdge.STI")),
            0,
            reinterpret_cast<unsigned char*>(
                const_cast<char*>("Data\\Dialogs\\DialogEdge.STI")),
            0, reinterpret_cast<unsigned char*>(m_background_path),
            static_cast<short>(m_background_flags), 0, 0);
        if (m_field_56 == -1) {
            return m_error = 3;
        }
    }
    m_field_58 = CreateTextButton(
        0, g_dialog_font_64fde8,
        g_dialog_font_foreground_64fdec,
        g_dialog_font_background_64fded,
        m_field_56, static_cast<short>(m_x + 9),
        static_cast<short>(m_y + 9),
        static_cast<short>(m_width - 0x12),
        static_cast<short>(m_height - 0x12),
        0x8004, 0x7e, 0, 0);

    m_field_60 = LoadButtonImage(
        reinterpret_cast<unsigned char*>(
            const_cast<char*>("Data\\Dialogs\\DialogConfirmation.sti")),
        3, 0, 1, 2, 2);
    if (m_field_60 != -1) {
        m_field_5c = QuickCreateButton(
            m_field_60, 0, 0, 4, 0x7f,
            Function5D32C0, Function5D32C0);
    }
    m_field_78 = LoadButtonImage(
        reinterpret_cast<unsigned char*>(
            const_cast<char*>("Data\\Dialogs\\DialogConfirmation.sti")),
        7, 4, 5, 6, 6);
    if (m_field_78 != -1) {
        m_field_74 = QuickCreateButton(
            m_field_78, 0, 0, 4, 0x7f,
            Function5D3370, Function5D3370);
    }
    if (m_field_5c != -1 && m_field_74 != -1) {
        int button_width;
        int button_y;
        int button_x;

        MSYS_SetBtnUserData(m_field_5c, 0, reinterpret_cast<int>(this));
        MSYS_SetBtnUserData(m_field_74, 0, reinterpret_cast<int>(this));
        button_width = GetButtonWidth(m_field_5c);
        button_y = m_y + m_height - GetButtonHeight(m_field_5c) - 0xf;
        if (allow_cancel) {
            button_x = m_x + (m_width - button_width * 3) / 2;
        }
        else {
            button_x = m_x + (m_width - button_width) / 2;
        }
        SetButtonPosition(m_field_5c, button_x, button_y);
        SetButtonPosition(
            m_field_74,
            m_x + m_width / 2 + GetButtonWidth(m_field_5c) / 2,
            GetButtonY(m_field_5c));
        return 0;
    }
    ResetSubobjectAndRefresh();
    return m_error = 6;
}

// FUNCTION: WIZ8 0x005d2800
void W8ModalDialogBase::SetMessage(
    void* payload, int line_count, int characters_per_line,
    int confirmation, int cancel, int size_to_message, int wrap_message,
    int maximum_width, int maximum_height)
{
    wchar_t* message = static_cast<wchar_t*>(payload);
    unsigned int index;

    if (!message) {
        srAssertFail(
            "pMessage",
            "C:\\Projects\\Wizardry 8\\Dialog Code\\DialogBase.cpp",
            0x83, 0);
    }
    if (m_lines) {
        for (index = 0; index < m_line_count; ++index) {
            free(m_lines[index]);
        }
        free(m_lines);
        m_lines = 0;
    }
    if (line_count == 1 && wrap_message) {
        line_count = WrapMessage(message);
    }
    else {
        m_lines = static_cast<wchar_t**>(
            malloc(line_count * sizeof(wchar_t*)));
        for (index = 0; index < static_cast<unsigned int>(line_count); ++index) {
            m_lines[index] = static_cast<wchar_t*>(
                malloc(characters_per_line * sizeof(wchar_t) + sizeof(wchar_t)));
            wcscpy(m_lines[index], message);
            message += characters_per_line;
        }
    }
    m_line_count = line_count;
    m_field_94 = static_cast<unsigned char>(confirmation);
    allow_cancel = static_cast<unsigned char>(cancel);
    if (size_to_message) {
        unsigned int width = 0;
        int height;

        for (index = 0; index < m_line_count; ++index) {
            short line_width = StringPixLength(
                reinterpret_cast<unsigned short*>(m_lines[index]),
                g_dialog_font_64fde8);
            if (width < static_cast<unsigned int>(line_width)) {
                width = line_width;
            }
        }
        width = width * 5 / 4;
        if (width < 0xaa) {
            width = 0xaa;
        }
        GetFontHeight(g_dialog_font_64fde8);
        height = GetFontHeight(g_dialog_font_64fde8) * m_line_count + 0x1e;
        if (m_field_94 || allow_cancel) {
            height = GetFontHeight(g_dialog_font_64fde8) * m_line_count + 0x3f;
        }
        if (height < GetFontHeight(g_dialog_font_64fde8) * 7) {
            height = GetFontHeight(g_dialog_font_64fde8) * 7;
        }
        if (maximum_width && maximum_width < static_cast<int>(width)) {
            width = maximum_width;
        }
        if (maximum_height && maximum_height < height) {
            height = maximum_height;
        }
        int old_width = m_width;
        int old_height = m_height;
        SetExtent(width, height);
        SetOrigin(m_x + (old_width - static_cast<int>(width)) / 2,
                  m_y + (old_height - height) / 2);
    }
}

// FUNCTION: WIZ8 0x005d2660
void W8ModalDialogBase::vslot3()
{
    int y;
    unsigned int index;

    W8DialogBase005DC7A0::vslot3();
    DrawButton(m_field_58);
    if (!m_field_94 && !allow_cancel) {
        y = m_y +
            (m_height - GetFontHeight(g_dialog_font_64fde8) * m_line_count) / 2;
    }
    else {
        y = m_y + ((m_height - 0x21) -
                   GetFontHeight(g_dialog_font_64fde8) * m_line_count) / 2;
    }
    if (m_lines) {
        SaveFontSettings();
        SetValue5FF5F0(g_dialog_font_64fde8);
        SetFontForeground(g_dialog_font_foreground_64fdec);
        SetFontBackground(g_dialog_font_background_64fded);
        for (index = 0; index < m_line_count; ++index) {
            wchar_t* line = m_lines[index];
            short width = StringPixLengthArg(
                g_dialog_font_64fde8, wcslen(line),
                reinterpret_cast<unsigned short*>(line), y, line);
            Function407650(m_x + (m_width - width) / 2, y, line, line);
            y += GetFontHeight(g_dialog_font_64fde8);
        }
        RestoreFontSettings();
    }
    if (m_field_94) {
        if (m_field_5c != -1) {
            DrawButton(m_field_5c);
        }
        if (allow_cancel && m_field_74 != -1) {
            DrawButton(m_field_74);
        }
    }
}

// FUNCTION: WIZ8 0x005ad280
int W8ModalDialogBase::vslot4()
{
    return 1;
}

// FUNCTION: WIZ8 0x005d2f40
void W8ModalDialogBase::ResetSubobjectAndRefresh()
{
    unsigned int index;

    W8DialogBase005DC7A0::ResetSubobjectAndRefresh();
    if (m_field_56 != -1) {
        UnloadGenericButtonImage(m_field_56);
        m_field_56 = -1;
    }
    if (m_field_58 != -1) {
        RemoveButton(m_field_58);
        m_field_58 = -1;
    }
    if (m_field_5c != -1) {
        RemoveButton(m_field_5c);
        m_field_5c = -1;
    }
    if (m_field_74 != -1) {
        RemoveButton(m_field_74);
        m_field_74 = -1;
    }
    if (m_field_60 != -1) {
        Function40C710(m_field_60);
        m_field_60 = -1;
    }
    if (m_field_78 != -1) {
        Function40C710(m_field_78);
        m_field_78 = -1;
    }
    if (m_lines) {
        for (index = 0; index < m_line_count; ++index) {
            free(m_lines[index]);
        }
        free(m_lines);
        m_lines = 0;
    }
}

// FUNCTION: WIZ8 0x005d2cb0
void W8ModalDialogBase::SetClientExtent(int width, int height)
{
    int old_width = m_width;
    int old_height = m_height;

    SetExtent(width, height);
    SetOrigin(m_x + (old_width - width) / 2,
              m_y + (old_height - height) / 2);
}

// FUNCTION: WIZ8 0x005d3020
unsigned char W8ModalDialogBase::HandleInput(
    const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN) {
        return is_open;
    }

    if (allow_cancel != 0) {
        int key = toupper(input->usParam);
        if (key == ESC) {
            close_result = 0;
            is_open = 0;
            return 0;
        }
        if (key != '\r') {
            return is_open;
        }
    }

    close_result = 1;
    is_open = 0;
    return 0;
}

// FUNCTION: WIZ8 0x005d3080
unsigned char W8ModalDialogBase::ProcessInput()
{
    SGPPoint mouse;
    InputAtom input;

    GetMousePos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS,
        mouse.iX,
        mouse.iY,
        gfLeftButtonState,
        gfRightButtonState);

    while (DequeueEvent(&input)) {
        unsigned short mouse_event;

        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            mouse_event = LEFT_BUTTON_DOWN;
            break;
        case LEFT_BUTTON_UP:
            mouse_event = LEFT_BUTTON_UP;
            break;
        case RIGHT_BUTTON_DOWN:
            mouse_event = RIGHT_BUTTON_DOWN;
            break;
        case RIGHT_BUTTON_UP:
            mouse_event = RIGHT_BUTTON_UP;
            break;
        default:
            return HandleInput(&input);
        }

        MSYS_SGP_Mouse_Handler_Hook(
            mouse_event,
            mouse.iX,
            mouse.iY,
            gfLeftButtonState,
            gfRightButtonState);
    }

    return is_open;
}
