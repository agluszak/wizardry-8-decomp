#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "Button System.h"
#include "input.h"
#include "mousesystem_macros.h"
#include "wiz8/engine_code/Video2.h"

/* Reconstructed logical owner; original translation-unit identity is unproven. */

// SYNTHETIC: WIZ8 0x005db210
// W8DialogButton::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005db4e0
void W8DialogButton::Draw()
{
    if (m_dirty && m_resource_01c != -1) {
        int left = GetButtonX(m_resource_01c);
        int top = GetButtonY(m_resource_01c);
        DrawButton(m_resource_01c);
        InvalidateRegion(left, top,
                            left + GetButtonWidth(m_resource_01c),
                            top + GetButtonHeight(m_resource_01c), 0);
        m_dirty = 0;
    }
}

// FUNCTION: WIZ8 0x005db550
void W8DialogButton::SetPosition(int x, int y)
{
    if (m_resource_01c != -1 &&
        (x != GetButtonX(m_resource_01c) || y != GetButtonY(m_resource_01c))) {
        SetButtonPosition(m_resource_01c, static_cast<short>(x), static_cast<short>(y));
        m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005db5a0
int W8DialogButton::GetWidth()
{
    return m_resource_01c != -1 ? GetButtonWidth(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db5c0
int W8DialogButton::GetHeight()
{
    return m_resource_01c != -1 ? GetButtonHeight(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db5e0
int W8DialogButton::GetX()
{
    return m_resource_01c != -1 ? GetButtonX(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db600
int W8DialogButton::GetY()
{
    return m_resource_01c != -1 ? GetButtonY(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db8d0
void W8DialogButton::SetEnabled(bool enabled)
{
    GUI_BUTTON* button = GetButtonPtr(m_resource_01c);
    m_enabled_035 = enabled;
    if (button) {
        if (enabled) {
            if (!(button->uiFlags & BUTTON_ENABLED)) {
                button->uiFlags |= BUTTON_ENABLED;
                m_dirty = 1;
            }
        }
        else if (button->uiFlags & BUTTON_ENABLED) {
            button->uiFlags &= ~BUTTON_ENABLED;
            m_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005db920
bool W8DialogButton::IsEnabled()
{
    GUI_BUTTON* button = GetButtonPtr(m_resource_01c);
    return button ? (button->uiFlags & BUTTON_ENABLED) != 0 : false;
}

// FUNCTION: WIZ8 0x005db950
void W8DialogButton::SetPressed(bool pressed)
{
    GUI_BUTTON* button = GetButtonPtr(m_resource_01c);
    if (button) {
        if (pressed) {
            if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
                button->uiFlags |= BUTTON_CLICKED_ON;
                m_dirty = 1;
            }
        }
        else if (button->uiFlags & BUTTON_CLICKED_ON) {
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            m_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005db9a0
unsigned char W8DialogButton::IsPressed()
{
    return static_cast<unsigned char>(GetButtonPtr(m_resource_01c)->uiFlags & BUTTON_CLICKED_ON);
}

// FUNCTION: WIZ8 0x005db9d0
void W8DialogButton::SetVisible(bool visible)
{
    if (visible) {
        if (!(GetButtonPtr(m_resource_01c)->Area.uiFlags & MSYS_REGION_ENABLED)) {
            ShowButton(m_resource_01c);
            MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, 0, 0, gfLeftButtonState, gfRightButtonState);
        }
    }
    else if (GetButtonPtr(m_resource_01c)->Area.uiFlags & MSYS_REGION_ENABLED) {
        HideButton(m_resource_01c);
        MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, 0, 0, gfLeftButtonState, gfRightButtonState);
    }
}

// FUNCTION: WIZ8 0x005dbaf0
int W8DialogButton::GetUserData()
{
    return ButtonList[m_resource_01c]->UserData[1];
}

// FUNCTION: WIZ8 0x005db1b0
W8DialogButton::W8DialogButton()
{
    m_resource_018 = -1;
    m_resource_01c = -1;
    unknown_024 = 0;
    unknown_028 = 0;
    unknown_02c = 0;
    unknown_030 = 0;
    unknown_034 = 0;
    m_enabled_035 = 1;
    unknown_036 = 0;
    unknown_037 = 0;
    unknown_020 = -1;
    m_dirty = 1;
    unknown_039 = 0;
    unknown_03a = 0;
    unknown_03b = 0;
    unknown_03c = 0;
    m_owner_040 = 0;
    unknown_004 = -1;
    unknown_008 = -1;
    unknown_00c = -1;
    unknown_010 = -1;
    unknown_014 = -1;
    unknown_044 = g_dword_69ca28;
}

// FUNCTION: WIZ8 0x005db260
W8DialogButton::~W8DialogButton()
{
    if (m_resource_018 != -1) {
        UnloadButtonImage(m_resource_018);
        m_resource_018 = -1;
    }
    if (m_resource_01c != -1) {
        RemoveButton(m_resource_01c);
        m_resource_01c = -1;
    }
}
