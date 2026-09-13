#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/ButtonUserData.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "Font.h"
#include "mousesystem.h"

/* Retail Dialog Code\stListBox.cpp - the SGP button callbacks that give
   W8Dialog005CBB40 its scrolling text-list behaviour. The list state lives on
   the dialog (first visible line 0x0f0, selected line 0x0f4) and each button's
   userdata slot carries the dialog pointer. The gap helpers it calls -
   GetVisibleLineCount 0x005CC650, SetCurrentLine 0x005CCB80 and the keyboard
   path 0x005CD2B0 - are not proven to this unit and stay declared in
   DialogFactoryDialogs.h for these call sites. */

// FUNCTION: WIZ8 0x005cce70
void W8Dialog005CBB40::TextAreaButtonCallback(GUI_BUTTON* button, INT32 reason)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
    W8Dialog005CBB40* dialog = GetButtonUserDataPointer<W8Dialog005CBB40>(button);
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (dialog == 0) {
            srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x2f5,
                         0);
        }
        POINT cursor;
        SGPMouseGetPos(&cursor);
        /* Retail reads this top edge uninitialized when the area button is
           absent; the value is the leftover argument slot. */
        int top;
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
        dialog->SetCurrentLine005CCB80(line);
    }
}
#pragma clang diagnostic pop

// FUNCTION: WIZ8 0x005ccf30
void W8Dialog005CBB40::UpButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8Dialog005CBB40* dialog = GetButtonUserDataPointer<W8Dialog005CBB40>(button);
    if (dialog == 0) {
        srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x305, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        dialog->SetCurrentLine005CCB80(dialog->m_selected_line_0f4 - 1);
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
void W8Dialog005CBB40::DownButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8Dialog005CBB40* dialog = GetButtonUserDataPointer<W8Dialog005CBB40>(button);
    if (dialog == 0) {
        srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x32a, 0);
    }
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        dialog->SetCurrentLine005CCB80(dialog->m_selected_line_0f4 + 1);
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
void W8Dialog005CBB40::OkButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8Dialog005CBB40* dialog = GetButtonUserDataPointer<W8Dialog005CBB40>(button);
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
void W8Dialog005CBB40::CancelButtonCallback(GUI_BUTTON* button, INT32 reason)
{
    W8Dialog005CBB40* dialog = GetButtonUserDataPointer<W8Dialog005CBB40>(button);
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
            dialog->SetCurrentLine005CCB80(-1);
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
void W8Dialog005CBB40::SliderTrackButtonCallback(GUI_BUTTON* button, INT32 reason)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
    W8Dialog005CBB40* dialog = GetButtonUserDataPointer<W8Dialog005CBB40>(button);
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
        if (dialog == 0) {
            srAssertFail("pDialog", "C:\\Projects\\Wizardry 8\\Dialog Code\\stListBox.cpp", 0x398,
                         0);
        }
        POINT cursor;
        SGPMouseGetPos(&cursor);
        /* Retail reads both track edges uninitialized when the text-area
           button is absent; they are the leftover argument slots. */
        int top;
        int bottom;
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
        dialog->SetCurrentLine005CCB80((cursor.y - top) * dialog->m_lines_054.count /
                                       (bottom - top));
    }
}
#pragma clang diagnostic pop
