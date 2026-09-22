#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/ButtonUserData.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "Font.h"
#include "mousesystem.h"

/* Retail Dialog Code\stListBox.cpp - the SGP button callbacks that give
   W8ListBoxDialog its scrolling text-list behaviour. The list state lives on
   the dialog (first visible line 0x0f0, selected line 0x0f4) and each button's
   userdata slot carries the dialog pointer. The gap helpers it calls -
   GetVisibleLineCount 0x005CC650, SetCurrentLine 0x005CCB80 and the keyboard
   path 0x005CD2B0 - are not proven to this unit and stay declared in
   DialogFactoryDialogs.h for these call sites. */

// FUNCTION: WIZ8 0x005cce70
void W8ListBoxDialog::TextAreaButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8ListBoxDialog* dialog = GetButtonUserDataPointer<W8ListBoxDialog>(button);
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (dialog == 0) {
            srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x2f5,
                         0);
        }
        POINT cursor;
        SGPMouseGetPos(&cursor);
        /* Retail read this top edge uninitialized when the area button is
           absent (the leftover argument slot); deterministic zero models
           that defect path. */
        int top = 0;
        if (dialog->m_area_button_098 != -1) {
            SGPRect area;
            GetButtonArea(dialog->m_area_button_098, &area);
            top = area.iTop;
        }
        int line = (cursor.y - top) / (int)(unsigned int)GetFontHeight(g_dialog_font_64fde8) +
                   dialog->m_first_visible_line_0f0;
        if (line == dialog->m_selected_line_0f4) {
            dialog->m_keep_open = 0;
            return;
        }
        dialog->SetCurrentLine(line);
    }
}

// FUNCTION: WIZ8 0x005ccf30
void W8ListBoxDialog::UpButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8ListBoxDialog* dialog = GetButtonUserDataPointer<W8ListBoxDialog>(button);
    if (dialog == 0) {
        srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x305, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        dialog->SetCurrentLine(dialog->m_selected_line_0f4 - 1);
        if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
            button->uiFlags |= BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & BUTTON_CLICKED_ON) {
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    }
}

// FUNCTION: WIZ8 0x005ccfe0
void W8ListBoxDialog::DownButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8ListBoxDialog* dialog = GetButtonUserDataPointer<W8ListBoxDialog>(button);
    if (dialog == 0) {
        srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x32a, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        dialog->SetCurrentLine(dialog->m_selected_line_0f4 + 1);
        if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
            button->uiFlags |= BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & BUTTON_CLICKED_ON) {
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    }
}

// FUNCTION: WIZ8 0x005cd090
void W8ListBoxDialog::OkButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8ListBoxDialog* dialog = GetButtonUserDataPointer<W8ListBoxDialog>(button);
    if (dialog == 0) {
        srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x34c, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
            button->uiFlags |= BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & BUTTON_CLICKED_ON) {
            dialog->m_keep_open = 0;
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    }
}

// FUNCTION: WIZ8 0x005cd130
void W8ListBoxDialog::CancelButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8ListBoxDialog* dialog = GetButtonUserDataPointer<W8ListBoxDialog>(button);
    if (dialog == 0) {
        srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x36f, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
            button->uiFlags |= BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
        if (button->uiFlags & BUTTON_CLICKED_ON) {
            dialog->m_keep_open = 0;
            dialog->SetCurrentLine(-1);
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            dialog->m_dirty_flags |= 1;
        }
    } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        button->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    } else if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        button->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
        dialog->m_dirty_flags |= 1;
    }
}

// FUNCTION: WIZ8 0x005cd1e0
void W8ListBoxDialog::SliderTrackButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8ListBoxDialog* dialog = GetButtonUserDataPointer<W8ListBoxDialog>(button);
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (dialog == 0) {
            srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x398,
                         0);
        }
        POINT cursor;
        SGPMouseGetPos(&cursor);
        /* Retail read both track edges uninitialized when the text-area
           button is absent (the leftover argument slots); deterministic
           zeroes model that defect path. */
        int top = 0;
        int bottom = 0;
        if (dialog->m_third_text_button_0b8 != -1) {
            SGPRect area;
            GetButtonArea(dialog->m_third_text_button_0b8, &area);
            top = area.iTop + GetButtonHeight(dialog->m_up_button_09c);
            bottom = area.iBottom - GetButtonHeight(dialog->m_down_button_0a4);
        }
        if (cursor.y < top) {
            cursor.y = top;
        }
        if (cursor.y > bottom) {
            cursor.y = bottom;
        }
        dialog->SetCurrentLine((cursor.y - top) * dialog->m_lines_054.count / (bottom - top));
    }
}
