#include "wiz8/dialog_code/DialogBase.h"

#include "wiz8/dirty_tiles.h"
#include "wiz8/utility.h"

#include "input.h"
#include "mousesystem_macros.h"
#include "Button System.h"

#include <stdlib.h>
#include <string.h>

extern "C" void ClearSurfaceRect(
    int left, unsigned int top, int right, unsigned int bottom);

extern int g_dword_69ca28;
extern "C" int g_dialog_font_64fde8;
extern "C" unsigned char g_dialog_font_enabled_69ca32;
extern "C" unsigned char g_dialog_font_foreground_64fdec;
extern "C" unsigned char g_dialog_font_background_64fded;

void GetScreenPoint004284F0(W8ScreenPoint* point);

// FUNCTION: WIZ8 0x005dc7a0
W8DialogBase005DC7A0::W8DialogBase005DC7A0()
{
    m_resource = -1;
    m_dirty_flags = 0;
    m_error = 0;
    m_text = 0;
    m_font = g_dialog_font_64fde8;
    m_foreground = g_dialog_font_foreground_64fdec;
    m_background = g_dialog_font_background_64fded;
    m_border = -1;
    m_x = -1;
    m_y = -1;
    m_width = -1;
    m_height = -1;
    m_background_path = 0;
    m_background_flags = 0;
    m_field_4c = 0;
    m_initialized = 0;
    m_field_41 = 1;
    m_destroy_callback = 0;
    m_field_48 = 0;
    ++g_dword_69ca28;
    m_field_50 = 0;
}

// SYNTHETIC: WIZ8 0x005dc810
// W8DialogBase005DC7A0::`scalar deleting destructor'
// FUNCTION: WIZ8 0x005dc860
W8DialogBase005DC7A0::~W8DialogBase005DC7A0()
{
    if (m_destroy_callback) {
        m_destroy_callback(this);
    }
    ResetSubobjectAndRefresh();
    --g_dword_69ca28;
}

// FUNCTION: WIZ8 0x005dc890
void W8DialogBase005DC7A0::vslot3()
{
    if ((m_dirty_flags & 1) == 0) {
        return;
    }
    if (!m_initialized) {
        vslot1();
    }
    if (m_error == 0 && m_resource != -1) {
        if (m_width != GetButtonWidth(m_resource) ||
            m_height != GetButtonHeight(m_resource)) {
            ResizeButton(m_resource,
                         static_cast<short>(m_width),
                         static_cast<short>(m_height));
        }
        if (!DrawButton(m_resource)) {
            m_error = 9;
            m_dirty_flags &= ~1u;
            return;
        }
        MarkScreenRectDirty(
            m_x, m_y, m_x + m_width, m_y + m_height, 1);
    }
    m_dirty_flags &= ~1u;
}

// FUNCTION: WIZ8 0x005dc940
void W8DialogBase005DC7A0::vslot5(const wchar_t* text)
{
    if (m_text) {
        free(m_text);
        m_text = 0;
    }
    if (text) {
        if (wcslen(text) != 0) {
            m_text = static_cast<wchar_t*>(
                malloc((wcslen(text) + 1) * sizeof(wchar_t)));
            wcscpy(m_text, text);
        }
    }
    if (m_resource != -1) {
        SpecifyButtonText(
            m_resource,
            const_cast<unsigned short*>(
                reinterpret_cast<const unsigned short*>(text)));
    }
    m_dirty_flags |= 1;
}

// FUNCTION: WIZ8 0x005dc9c0
void W8DialogBase005DC7A0::SetOrigin(int x, int y)
{
    m_x = x;
    m_y = y;
    if (m_resource != -1) {
        SetButtonPosition(
            m_resource, static_cast<short>(x), static_cast<short>(y));
    }
    m_dirty_flags |= 1;
}

// FUNCTION: WIZ8 0x005dc9f0
void W8DialogBase005DC7A0::SetExtent(int width, int height)
{
    if (m_width != width || m_height != height) {
        if (m_initialized && m_width > 0 && m_height > 0) {
            ClearSurfaceRect(m_x, m_y,
                             m_x + m_width + 1, m_y + m_height + 1);
            MarkScreenRectDirty(m_x, m_y,
                                m_x + m_width + 1, m_y + m_height + 1, 1);
        }
        m_width = width;
        m_height = height;
        m_dirty_flags |= 1;
    }
}

// FUNCTION: WIZ8 0x005dca70
void W8DialogBase005DC7A0::SetBackground(const char* path, int flags)
{
    if (m_background_path) {
        free(m_background_path);
        m_background_path = 0;
    }
    if (path) {
        if (strlen(path) != 0) {
            m_background_path = static_cast<char*>(malloc(strlen(path) + 1));
            strcpy(m_background_path, path);
        }
    }
    m_background_flags = flags;
    m_dirty_flags |= 1;
}

// FUNCTION: WIZ8 0x005dcaf0
int W8DialogBase005DC7A0::vslot1()
{
    if (m_x < 0 || m_y < 0) {
        return m_error = 1;
    }
    if (m_width < 0 || m_height < 0) {
        return m_error = 3;
    }
    if (m_font == -1) {
        return m_error = 2;
    }
    if (!m_background_path) {
        return m_error = 4;
    }
    if (m_border == -1) {
        m_border = LoadGenericButtonImages(
            0,
            reinterpret_cast<unsigned char*>(
                const_cast<char*>("Data\\Dialogs\\DialogBorder.STI")),
            0,
            reinterpret_cast<unsigned char*>(
                const_cast<char*>("Data\\Dialogs\\DialogBorder.STI")),
            0, reinterpret_cast<unsigned char*>(m_background_path),
            static_cast<short>(m_background_flags), 0, 0);
        if (m_border == -1) {
            return m_error = 4;
        }
    }
    m_resource = CreateTextButton(
        0, m_font, m_foreground, m_background, m_border,
        static_cast<short>(m_x), static_cast<short>(m_y),
        static_cast<short>(m_width), static_cast<short>(m_height),
        0x8004, 0x7d, 0, 0);
    if (m_resource != -1) {
        SpecifyButtonTextOffsets(m_resource, 3, 3, 1);
        SpecifyButtonMultiColorFont(m_resource, g_dialog_font_enabled_69ca32);
        m_dirty_flags |= 1;
        m_initialized = 1;
        return 0;
    }
    return m_error = 7;
}

// FUNCTION: WIZ8 0x005dcc30
void W8DialogBase005DC7A0::ResetSubobjectAndRefresh()
{
    if (m_resource != -1) {
        RemoveButton(m_resource);
        m_resource = -1;
    }
    if (m_border != -1) {
        UnloadGenericButtonImage(m_border);
        m_border = -1;
    }
    if (m_text) {
        free(m_text);
        m_text = 0;
    }
    if (m_background_path) {
        free(m_background_path);
        m_background_path = 0;
        m_background_flags = 0;
    }
    ClearSurfaceRect(m_x, m_y, m_x + m_width + 1, m_y + m_height + 1);
    MarkScreenRectDirty(
        m_x, m_y, m_x + m_width + 1, m_y + m_height + 1, 1);
    m_initialized = 0;
}

// FUNCTION: WIZ8 0x005dcce0
unsigned char W8DialogBase005DC7A0::ProcessInput()
{
    W8ScreenPoint mouse;
    InputAtom input;

    GetScreenPoint004284F0(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(
        MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input)) {
        switch (input.usEvent) {
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_DOWN, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            vslot11();
            break;
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
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(
                RIGHT_BUTTON_UP, mouse.x, mouse.y,
                gfLeftButtonState, gfRightButtonState);
            ClearField41IfEnabled();
            break;
        case MOUSE_WHEEL:
            vslot13(GetMouseWheelDeltaValue(input.uiParam));
            break;
        case KEY_DOWN:
            if (input.usParam == 0x1b) {
                m_field_41 = 0;
            }
            break;
        }
    }
    return m_field_41;
}

// FUNCTION: WIZ8 0x005d6fa0
int W8DialogBase005DC7A0::vslot4()
{
    return 0;
}

void W8DialogBase005DC7A0::vslot10(int)
{
}

// FUNCTION: WIZ8 0x005ad270
void W8DialogBase005DC7A0::vslot11()
{
    m_field_50 = 1;
}

// FUNCTION: WIZ8 0x005b1bf0
void W8DialogBase005DC7A0::ClearField41IfEnabled()
{
}

void W8DialogBase005DC7A0::vslot13(int)
{
}
