#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "wiz8/dialog_code/ButtonUserData.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"
#include "Button System.h"
#include "input.h"
#include "mousesystem.h"
#include "mousesystem_macros.h"

/* Dialog Code\stButton.cpp. The live hull is the singleton assertion at
   0x005DB620 (DialogButtonCallback). The rest of W8DialogButton sits in the
   gap between AssayDialog.cpp (upper 0x005D9460) and that hull; those methods
   stay in this file so the class and the sources.cmake slot stay together. */

// SYNTHETIC: WIZ8 0x005db210
// W8DialogButton::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005db4e0
void W8DialogButton::Draw()
{
    if (m_dirty && m_button_01c != -1) {
        int left = GetButtonX(m_button_01c);
        int top = GetButtonY(m_button_01c);
        DrawButton(m_button_01c);
        InvalidateRegion(left, top, left + GetButtonWidth(m_button_01c),
                         top + GetButtonHeight(m_button_01c), 0);
        m_dirty = 0;
    }
}

// FUNCTION: WIZ8 0x005db550
void W8DialogButton::SetPosition(int x, int y)
{
    if (m_button_01c != -1 && (x != GetButtonX(m_button_01c) || y != GetButtonY(m_button_01c))) {
        SetButtonPosition(m_button_01c, static_cast<short>(x), static_cast<short>(y));
        m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005db5a0
int W8DialogButton::GetWidth()
{
    return m_button_01c != -1 ? GetButtonWidth(m_button_01c) : 0;
}

// FUNCTION: WIZ8 0x005db5c0
int W8DialogButton::GetHeight()
{
    return m_button_01c != -1 ? GetButtonHeight(m_button_01c) : 0;
}

// FUNCTION: WIZ8 0x005db5e0
int W8DialogButton::GetX()
{
    return m_button_01c != -1 ? GetButtonX(m_button_01c) : 0;
}

// FUNCTION: WIZ8 0x005db600
int W8DialogButton::GetY()
{
    return m_button_01c != -1 ? GetButtonY(m_button_01c) : 0;
}

// FUNCTION: WIZ8 0x005db8d0
void W8DialogButton::SetEnabled(bool enabled)
{
    GUI_BUTTON* button = GetButtonPtr(m_button_01c);
    m_enabled_035 = enabled;
    if (button) {
        if (enabled) {
            if (!(button->uiFlags & BUTTON_ENABLED)) {
                button->uiFlags |= BUTTON_ENABLED;
                m_dirty = 1;
            }
        } else if (button->uiFlags & BUTTON_ENABLED) {
            button->uiFlags &= ~BUTTON_ENABLED;
            m_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005db920
bool W8DialogButton::IsEnabled()
{
    GUI_BUTTON* button = GetButtonPtr(m_button_01c);
    return button ? (button->uiFlags & BUTTON_ENABLED) != 0 : false;
}

// FUNCTION: WIZ8 0x005db950
void W8DialogButton::SetPressed(bool pressed)
{
    GUI_BUTTON* button = GetButtonPtr(m_button_01c);
    if (button) {
        if (pressed) {
            if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
                button->uiFlags |= BUTTON_CLICKED_ON;
                m_dirty = 1;
            }
        } else if (button->uiFlags & BUTTON_CLICKED_ON) {
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            m_dirty = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005db9a0
unsigned char W8DialogButton::IsPressed()
{
    return static_cast<unsigned char>(GetButtonPtr(m_button_01c)->uiFlags & BUTTON_CLICKED_ON);
}

// FUNCTION: WIZ8 0x005db9d0
void W8DialogButton::SetVisible(bool visible)
{
    if (visible) {
        if (!(GetButtonPtr(m_button_01c)->Area.uiFlags & MSYS_REGION_ENABLED)) {
            ShowButton(m_button_01c);
            MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, 0, 0, gfLeftButtonState, gfRightButtonState);
        }
    } else if (GetButtonPtr(m_button_01c)->Area.uiFlags & MSYS_REGION_ENABLED) {
        HideButton(m_button_01c);
        MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, 0, 0, gfLeftButtonState, gfRightButtonState);
    }
}

// FUNCTION: WIZ8 0x005dbaf0
int W8DialogButton::GetUserData()
{
    return ButtonList[m_button_01c]->UserData[1];
}

// FUNCTION: WIZ8 0x005db1b0
W8DialogButton::W8DialogButton()
{
    m_image_018 = -1;
    m_button_01c = -1;
    m_left_callback = 0;
    m_right_callback = 0;
    m_move_callback = 0;
    m_double_click_callback = 0;
    m_clicked = 0;
    m_enabled_035 = 1;
    m_left_toggles = 0;
    m_right_toggles = 0;
    m_tooltip_index = -1;
    m_dirty = 1;
    unknown_039 = 0;
    unknown_03a = 0;
    unknown_03b = 0;
    unknown_03c = 0;
    m_owner_040 = 0;
    m_gray_frame = -1;
    m_off_normal_frame = -1;
    m_off_hover_frame = -1;
    m_on_normal_frame = -1;
    m_on_hover_frame = -1;
    m_live_dialog_count = g_dword_69ca28;
}

// FUNCTION: WIZ8 0x005db260
W8DialogButton::~W8DialogButton()
{
    if (m_image_018 != -1) {
        UnloadButtonImage(m_image_018);
        m_image_018 = -1;
    }
    if (m_button_01c != -1) {
        RemoveButton(m_button_01c);
        m_button_01c = -1;
    }
}

#define STBUTTON_CPP "C:\\Projects\\Wizardry 8\\Dialog Code\\stButton.cpp"

// FUNCTION: WIZ8 0x005db3e0
unsigned char W8DialogButton::Configure(const char* image_path, int gray_frame,
                                        int off_normal_frame, int off_hover_frame,
                                        int on_normal_frame, int on_hover_frame,
                                        W8DialogButtonCallback left_callback,
                                        W8DialogButtonCallback move_callback,
                                        unsigned char left_toggles, short priority,
                                        int tooltip_index, W8DialogButtonCallback right_callback,
                                        W8DialogButtonCallback double_click_callback)
{
    m_image_018 =
        LoadButtonImage(reinterpret_cast<UINT8*>(const_cast<char*>(image_path)), gray_frame,
                        off_normal_frame, off_hover_frame, on_normal_frame,
                        on_hover_frame); // reinterpret-ok: SGP LoadButtonImage takes UINT8*
    if (m_image_018 != -1) {
        m_button_01c = QuickCreateButton(m_image_018, 0, 0, BUTTON_NO_TOGGLE, priority,
                                         DialogButtonCallback, DialogButtonCallback);
    }
    m_off_normal_frame = off_normal_frame;
    m_gray_frame = gray_frame;
    m_off_hover_frame = off_hover_frame;
    m_on_normal_frame = on_normal_frame;
    m_on_hover_frame = on_hover_frame;
    if (m_button_01c != -1) {
        SetButtonUserDataPointer(m_button_01c, this);
        m_move_callback = move_callback;
        m_left_callback = left_callback;
        m_right_callback = right_callback;
        m_double_click_callback = double_click_callback;
        m_left_toggles = left_toggles;
        m_tooltip_index = tooltip_index;
        if (tooltip_index != -1 && g_settings_6850c8.tooltips_enabled != 0) {
            SetButtonFastHelpText(
                m_button_01c,
                reinterpret_cast<UINT16*>(
                    gppStringList
                        [tooltip_index])); // reinterpret-ok: SGP help text is UINT16*, string table is wchar_t*
        }
        m_dirty = 1;
        return 1;
    }
    if (m_image_018 != -1) {
        UnloadButtonImage(m_image_018);
        m_image_018 = -1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005db620
void DialogButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8DialogButton* self = GetButtonUserDataPointer<W8DialogButton>(button);
    W8DialogButtonCallback left_callback;
    W8DialogButtonCallback right_callback;
    W8DialogButtonCallback move_callback;
    GUI_BUTTON* owned;
    INT32 handle;

    if (button == 0) {
        srAssertFail("pButton", STBUTTON_CPP, 0x197, 0);
    }
    left_callback = self->m_left_callback;
    right_callback = self->m_right_callback;
    move_callback = self->m_move_callback;
    if (left_callback == 0 && right_callback == 0 && move_callback == 0) {
        return;
    }
    if (self->m_live_dialog_count != g_dword_69ca28) {
        return;
    }
    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DOUBLECLICK) != 0 &&
        self->m_double_click_callback != 0) {
        if (self->m_left_toggles != 0 && (button->uiFlags & BUTTON_CLICKED_ON) == 0) {
            return;
        }
        self->m_double_click_callback(self);
        return;
    }
    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) == 0) {
        if ((reason & MSYS_CALLBACK_REASON_LBUTTON_UP) == 0) {
            if ((reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT) == 0) {
                if ((reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) == 0) {
                    if ((reason & MSYS_CALLBACK_REASON_RBUTTON_UP) == 0) {
                        if ((reason & MSYS_CALLBACK_REASON_RBUTTON_REPEAT) == 0) {
                            if ((reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) != 0) {
                                button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
                                self->m_dirty = 1;
                                if (self->unknown_03a != 0) {
                                    return;
                                }
                                if (self->unknown_039 != 0) {
                                    return;
                                }
                                PlayButtonSound(0);
                                return;
                            }
                            if ((reason & MSYS_CALLBACK_REASON_LOST_MOUSE) != 0) {
                                button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
                                self->unknown_03c = 0;
                                self->m_dirty = 1;
                                handle = self->m_button_01c;
                                if (self->m_left_toggles == 0 && handle >= 0 &&
                                    handle < MAX_BUTTONS) {
                                    owned = ButtonList[handle];
                                    if (owned != 0 && (owned->uiFlags & BUTTON_CLICKED_ON) != 0) {
                                        owned->uiFlags &= ~BUTTON_CLICKED_ON;
                                        self->m_dirty = 1;
                                    }
                                }
                                if (self->unknown_03a != 0) {
                                    return;
                                }
                                if (self->unknown_039 != 0) {
                                    return;
                                }
                                PlayButtonSound(1);
                                return;
                            }
                            if ((reason & MSYS_CALLBACK_REASON_MOVE) == 0) {
                                return;
                            }
                            if (move_callback == 0) {
                                return;
                            }
                            move_callback(self);
                            return;
                        }
                        if (self->unknown_03b == 0) {
                            return;
                        }
                        if (self->m_right_toggles != 0) {
                            return;
                        }
                        if ((button->uiFlags & BUTTON_CLICKED_ON) == 0) {
                            return;
                        }
                        self->unknown_03c = 1;
                    } else {
                        if (self->m_right_toggles != 0) {
                            return;
                        }
                        if ((button->uiFlags & BUTTON_CLICKED_ON) == 0) {
                            return;
                        }
                        button->uiFlags &= ~BUTTON_CLICKED_ON;
                        self->m_dirty = 1;
                        if (self->unknown_03b != 0 && self->unknown_03c != 0) {
                            self->unknown_03c = 0;
                            return;
                        }
                    }
                } else {
                    if (self->m_right_toggles == 0) {
                        goto press_down;
                    }
                    button->uiFlags ^= BUTTON_CLICKED_ON;
                    self->m_dirty = 1;
                    self->m_clicked =
                        static_cast<unsigned char>(button->uiFlags & BUTTON_CLICKED_ON);
                }
                if (right_callback != 0) {
                    right_callback(self);
                }
            } else {
                if (self->unknown_03b == 0) {
                    return;
                }
                if (self->m_left_toggles != 0) {
                    return;
                }
                if ((button->uiFlags & BUTTON_CLICKED_ON) == 0) {
                    return;
                }
                self->unknown_03c = 1;
                if (left_callback != 0) {
                    left_callback(self);
                }
            }
            goto play_click;
        }
        if (self->m_left_toggles != 0) {
            return;
        }
        if ((button->uiFlags & BUTTON_CLICKED_ON) == 0) {
            return;
        }
        button->uiFlags &= ~BUTTON_CLICKED_ON;
        self->m_dirty = 1;
        if (self->unknown_03b != 0 && self->unknown_03c != 0) {
            self->unknown_03c = 0;
            return;
        }
    } else {
        if (self->m_left_toggles == 0) {
        press_down:
            if ((button->uiFlags & BUTTON_CLICKED_ON) != 0) {
                return;
            }
            button->uiFlags |= BUTTON_CLICKED_ON;
            self->m_dirty = 1;
            if (self->unknown_039 != 0) {
                return;
            }
            PlayButtonSound(2);
            return;
        }
        button->uiFlags ^= BUTTON_CLICKED_ON;
        self->m_dirty = 1;
        self->m_clicked = static_cast<unsigned char>(button->uiFlags & BUTTON_CLICKED_ON);
    }
    if (left_callback != 0) {
        left_callback(self);
    }
play_click:
    if (self->unknown_039 != 0) {
        return;
    }
    PlayButtonSound(3);
}
