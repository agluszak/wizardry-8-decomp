#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"

#include "Font.h"
#include "input.h"
#include "line.h"
#include "vobject.h"

#include <wchar.h>

/* Dialog Code numeric entry field, reconstructed logical owner: the original
   translation-unit identity is unproven. The functions at 0x005E1460-0x005E1B22
   sit in the gap after DialogScrollBar.cpp (TrackButtonCallback ends at
   0x005E13F1) and before the VC6 runtime block at 0x005E1C10, and this scratch
   buffer is referenced only by them. */

// GLOBAL: WIZ8 0x0069ca4c
static wchar_t g_numeric_input_text[12];

// FUNCTION: WIZ8 0x005e1460
W8DialogNumericInput::W8DialogNumericInput(int control_id, const W8ControlsRect* bounds, int value,
                                           int font, W8DialogBase* dialog, W8DialogButton* button)
{
    m_font = font;
    m_value = value;
    m_dialog = dialog;
    m_caret = -1;
    m_maximum = -1;
    m_control_id = control_id;
    m_button = button;
    m_dirty = false;
    m_bounds = *bounds;
}

// FUNCTION: WIZ8 0x005e14c0
void W8DialogNumericInput::SetValue(int value)
{
    m_value = value;
    m_dirty = true;
}

// FUNCTION: WIZ8 0x005e14d0
void W8DialogNumericInput::SetActive(unsigned char active)
{
    m_active = active;
    if (active == 0) {
        m_caret = -1;
        m_dialog->m_field_4c = 0;
        m_dirty = true;
        m_button->m_dirty = true;
    }
}

// FUNCTION: WIZ8 0x005e1500
void W8DialogNumericInput::SetActive(unsigned char active, const POINT* point)
{
    m_active = active;
    if (active == 0) {
        m_caret = -1;
        m_dialog->m_field_4c = 0;
        m_dirty = true;
        m_button->m_dirty = true;
        return;
    }
    swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
    size_t length = wcslen(g_numeric_input_text);
    unsigned int count = 0;
    m_caret = -1;
    wchar_t* suffix = g_numeric_input_text + length - 1;
    for (; count < length; ++count, --suffix) {
        short suffix_width = StringPixLength(suffix, m_font);
        if ((m_bounds.right - point->x) - m_bounds.left < suffix_width) {
            m_caret = count;
            break;
        }
    }
    if (m_caret == -1) {
        m_caret = length;
    }
    m_dirty = true;
    m_button->m_dirty = true;
}

// FUNCTION: WIZ8 0x005e15c0
void W8DialogNumericInput::Draw(unsigned char force)
{
    if (force == 0 && m_dirty == 0) {
        return;
    }
    swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
    size_t length = wcslen(g_numeric_input_text);
    SetFont(m_font);
    HVOBJECT font_object = GetFontObject(m_font);
    if (font_object == 0) {
        return;
    }
    swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
    SetObjectShade(font_object, 4);
    SetFontDestBuffer(-14, m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.bottom, 0);
    unsigned short font_height = GetFontHeight(m_font);
    int text_y = m_bounds.top + __max(0, (m_bounds.bottom - m_bounds.top - font_height) / 2);
    int text_x = m_bounds.right - StringPixLength(g_numeric_input_text, m_font);
    if (text_x <= m_bounds.left) {
        text_x = m_bounds.left;
    }
    gprintfDirty(text_x, text_y, const_cast<wchar_t*>(g_format_s_006068e4), g_numeric_input_text);
    if (m_active != 0 && m_caret != -1) {
        int caret_x =
            m_bounds.right - StringPixLength(g_numeric_input_text + length - m_caret, m_font) - 1;
        font_height = GetFontHeight(m_font);
        int caret_top = m_bounds.top + __max(0, (m_bounds.bottom - m_bounds.top - font_height) / 2);
        UINT32 pitch;
        char* screen = static_cast<char*>(LockPrimarySurface(&pitch));
        int caret_bottom = m_bounds.bottom;
        if (caret_top + GetFontHeight(m_font) < caret_bottom) {
            caret_bottom = caret_top + GetFontHeight(m_font);
        }
        LineDraw(0, caret_x, caret_top, caret_x, caret_bottom, -1, screen);
        UnlockPrimarySurface();
    }
    SetFontDestBuffer(-14, 0, 0, 640, 480, 0);
    m_dirty = false;
}

// FUNCTION: WIZ8 0x005e17a0
void W8DialogNumericInput::TypeDigit(wchar_t digit)
{
    if (m_active != 0) {
        swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
        size_t length = wcslen(g_numeric_input_text);
        if (length < 10) {
            unsigned int value;
            g_numeric_input_text[length - m_caret] = digit;
            swscanf(g_numeric_input_text, g_format_d_0060aa20, &value);
            if (value <= m_maximum) {
                m_value = value;
                m_dirty = true;
                m_button->m_dirty = true;
                m_dialog->OnNumericInputChanged(m_control_id);
                --m_caret;
                if (m_caret < 0) {
                    m_caret = 0;
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x005e1840
void W8DialogNumericInput::DeleteForward()
{
    if (m_active != 0 && m_caret != 0) {
        swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
        size_t length = wcslen(g_numeric_input_text);
        unsigned int position = length - m_caret;
        if (position < length) {
            wchar_t* cursor = g_numeric_input_text + position;
            unsigned int remaining = length - position;
            do {
                *cursor = cursor[1];
                ++cursor;
                --remaining;
            } while (remaining != 0);
        }
        --m_caret;
        if (length == 1) {
            m_value = 0;
        } else {
            swscanf(g_numeric_input_text, g_format_d_0060aa20, &m_value);
        }
        m_dirty = true;
        m_button->m_dirty = true;
        m_dialog->OnNumericInputChanged(m_control_id);
    }
}

// FUNCTION: WIZ8 0x005e18f0
void W8DialogNumericInput::Backspace()
{
    if (m_active != 0) {
        swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
        size_t length = wcslen(g_numeric_input_text);
        if (length != 0 && m_caret != static_cast<int>(length)) {
            unsigned int position = (length - m_caret) - 1;
            if (position < length) {
                wchar_t* cursor = g_numeric_input_text + position;
                unsigned int remaining = length - position;
                do {
                    *cursor = cursor[1];
                    ++cursor;
                    --remaining;
                } while (remaining != 0);
            }
            if (m_caret == static_cast<int>(length)) {
                --m_caret;
            }
            if (length == 1) {
                m_value = 0;
            } else {
                swscanf(g_numeric_input_text, g_format_d_0060aa20, &m_value);
            }
            m_dirty = true;
            m_button->m_dirty = true;
            m_dialog->OnNumericInputChanged(m_control_id);
        }
    }
}

// FUNCTION: WIZ8 0x005e19a0
unsigned char W8DialogNumericInput::HandleInput(const InputAtom* input)
{
    if (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) {
        return 0;
    }
    switch (input->usParam) {
    case 8:
        if (m_active != 0 && m_caret != -1) {
            Backspace();
            return 1;
        }
        break;
    case 0x1b:
        m_active = 0;
        m_caret = -1;
        m_dialog->m_field_4c = 0;
        m_dirty = true;
        m_button->m_dirty = true;
        break;
    case 0x25:
        if (m_active != 0 && m_caret != -1) {
            swprintf(g_numeric_input_text, g_format_d_0060aa20, m_value);
            size_t length = wcslen(g_numeric_input_text);
            unsigned int next = m_caret + 1;
            if (next <= length) {
                length = next;
            }
            m_caret = length;
            m_dirty = true;
            m_button->m_dirty = true;
            return 1;
        }
        break;
    case 0x27:
        if (m_active != 0 && m_caret != -1) {
            int next = m_caret - 1;
            m_dirty = true;
            m_caret = next & ((next < 0) - 1);
            m_button->m_dirty = true;
            return 1;
        }
        break;
    case 0x2e:
        if (m_active != 0 && m_caret != -1) {
            DeleteForward();
            return 1;
        }
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        if (m_active != 0 && m_caret != -1) {
            TypeDigit(static_cast<wchar_t>(input->usParam));
            return 1;
        }
        break;
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
        if (m_active != 0 && m_caret != -1) {
            TypeDigit(static_cast<wchar_t>(input->usParam - 0x30));
            return 1;
        }
        break;
    default:
        return 0;
    }
    return 1;
}
