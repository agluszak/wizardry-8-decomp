#include "wiz8/cursor.h"
#include "wiz8/sgp_text.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/DialogInterface.h"

#include "wiz8/utility.h"

#include "input.h"
#include "mousesystem_macros.h"
#include "Button System.h"

#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x0069CA28
int g_live_dialog_count;

// FUNCTION: WIZ8 0x005dc7a0
W8DialogBase::W8DialogBase()
{
    m_resource = -1;
    m_dirty_flags = 0;
    m_error = 0;
    m_text = 0;
    m_font = g_dialog_interface_font;
    m_foreground = g_dialog_font_foreground;
    m_background = g_dialog_font_background;
    m_border = -1;
    m_x = -1;
    m_y = -1;
    m_width = -1;
    m_height = -1;
    m_background_path = 0;
    m_background_flags = 0;
    m_field_4c = 0;
    m_initialized = false;
    m_keep_open = true;
    m_destroy_callback = 0;
    m_destroy_callback_context = 0;
    ++g_live_dialog_count;
    m_right_button_down = false;
}

// FUNCTION: WIZ8 0x005dc860
W8DialogBase::~W8DialogBase()
{
    if (m_destroy_callback) {
        m_destroy_callback(this);
    }
    DestroyControls();
    --g_live_dialog_count;
}

// FUNCTION: WIZ8 0x005dc890
void W8DialogBase::Draw()
{
    if ((m_dirty_flags & W8_DIALOG_DIRTY_REDRAW) == 0) {
        return;
    }
    if (!m_initialized) {
        CreateControls();
    }
    if (m_error == 0 && m_resource != -1) {
        if (m_width != GetButtonWidth(m_resource) || m_height != GetButtonHeight(m_resource)) {
            ResizeButton(m_resource, static_cast<short>(m_width), static_cast<short>(m_height));
        }
        if (!DrawButton(m_resource)) {
            m_error = 9;
            m_dirty_flags &= ~W8_DIALOG_DIRTY_REDRAW;
            return;
        }
        InvalidateRegion(m_x, m_y, m_x + m_width, m_y + m_height, 1);
    }
    m_dirty_flags &= ~W8_DIALOG_DIRTY_REDRAW;
}

// FUNCTION: WIZ8 0x005dc940
void W8DialogBase::SetText(const wchar_t* text)
{
    if (m_text) {
        free(m_text);
        m_text = 0;
    }
    if (text) {
        if (wcslen(text) != 0) {
            m_text = static_cast<wchar_t*>(malloc((wcslen(text) + 1) * sizeof(wchar_t)));
            wcscpy(m_text, text);
        }
    }
    if (m_resource != -1) {
        SpecifyButtonText(m_resource, const_cast<wchar_t*>(text));
    }
    m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
}

// FUNCTION: WIZ8 0x005dc9c0
void W8DialogBase::SetOrigin(int x, int y)
{
    m_x = x;
    m_y = y;
    if (m_resource != -1) {
        SetButtonPosition(m_resource, static_cast<short>(x), static_cast<short>(y));
    }
    m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
}

// FUNCTION: WIZ8 0x005dc9f0
void W8DialogBase::SetExtent(int width, int height)
{
    if (m_width != width || m_height != height) {
        if (m_initialized && m_width > 0 && m_height > 0) {
            ClearSurfaceRect(m_x, m_y, m_x + m_width + 1, m_y + m_height + 1);
            InvalidateRegion(m_x, m_y, m_x + m_width + 1, m_y + m_height + 1, 1);
        }
        m_width = width;
        m_height = height;
        m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
    }
}

// FUNCTION: WIZ8 0x005dca70
void W8DialogBase::SetBackground(const char* path, int flags)
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
    m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
}

// FUNCTION: WIZ8 0x005dcaf0
int W8DialogBase::CreateControls()
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
        m_border = LoadGenericButtonImages(0, Wiz8ToSgpText("Data\\Dialogs\\DialogBorder.STI"), 0,
                                           Wiz8ToSgpText("Data\\Dialogs\\DialogBorder.STI"), 0,
                                           Wiz8ToSgpText(m_background_path),
                                           static_cast<short>(m_background_flags), 0, 0);
        if (m_border == -1) {
            return m_error = 4;
        }
    }
    m_resource =
        CreateTextButton(0, m_font, m_foreground, m_background, m_border, static_cast<short>(m_x),
                         static_cast<short>(m_y), static_cast<short>(m_width),
                         static_cast<short>(m_height), BUTTON_NO_TOGGLE | BUTTON_IGNORE_CLICKS,
                         MSYS_PRIORITY_HIGHEST - 2, BUTTON_NO_CALLBACK, BUTTON_NO_CALLBACK);
    if (m_resource != -1) {
        SpecifyButtonTextOffsets(m_resource, 3, 3, 1);
        SpecifyButtonMultiColorFont(m_resource, g_dialog_font_enabled);
        m_dirty_flags |= W8_DIALOG_DIRTY_REDRAW;
        m_initialized = true;
        return 0;
    }
    return m_error = 7;
}

// FUNCTION: WIZ8 0x005dcc30
void W8DialogBase::DestroyControls()
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
    InvalidateRegion(m_x, m_y, m_x + m_width + 1, m_y + m_height + 1, 1);
    m_initialized = false;
}

// FUNCTION: WIZ8 0x005dcce0
bool W8DialogBase::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input)) {
        switch (input.usEvent) {
        case RIGHT_BUTTON_DOWN:
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            OnRightButtonDown();
            break;
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_UP, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_UP, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            OnRightButtonUp();
            break;
        case MOUSE_WHEEL:
            OnMouseWheel(GetMouseWheelDeltaValue(input.usParam));
            break;
        case KEY_DOWN:
            if (input.usParam == 0x1b) {
                m_keep_open = false;
            }
            break;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005d6eb0
void DialogCloseButtonCallback(W8DialogButton* button)
{
    W8DialogBase* dialog = button->m_owner;
    if (dialog != 0) {
        dialog->m_keep_open = false;
    }
}

// FUNCTION: WIZ8 0x005d6fa0
W8DialogKind W8DialogBase::GetDialogType()
{
    return W8_DIALOG_BASIC;
}

/* Retail ICF shares this one-argument no-op with the retained widget Redraw
   body at 0x005B1BE0; this override has no separate retail address marker. */
void W8DialogBase::OnNumericInputChanged(int) {}

// FUNCTION: WIZ8 0x005ad270
void W8DialogBase::OnRightButtonDown()
{
    m_right_button_down = true;
}

// FUNCTION: WIZ8 0x005b1bf0
void W8DialogBase::OnRightButtonUp() {}

/* Retail ICF shares this no-op with the retained body at 0x005B1BE0. */
void W8DialogBase::OnMouseWheel(int) {}
