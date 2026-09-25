#include "wiz8/dialog_code/NpcDialog.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/text_input.h"

#include "input.h"
#include "mousesystem_macros.h"
#include "Font.h"

#include <wchar.h>

/* Dialog Code\NpcDialog.cpp. The NPC dialogue popup; see the header for the
   request-buffer layout. The option-list strings and the two price-check
   strings live in the gppStringList message table. */

// GLOBAL: WIZ8 0x0061c4b4
const wchar_t g_format_S[] = L"%S";

/* The live popup; the constructor publishes it so the option callback and the
   Enter handling in ProcessInput can reach the active instance. */
// GLOBAL: WIZ8 0x0069ca20
static W8NpcDialog* g_npc_dialog;

// GLOBAL: WIZ8 0x0064fc84
static const wchar_t g_format_s_dg[] = L"%s %dg";

// FUNCTION: WIZ8 0x005DA6B0
W8NpcDialog::W8NpcDialog(W8NpcQuoteEntry* message, int aux_data)
{
    short height = 0x46;
    short max_width = -1;
    short width = 0;
    int index;
    wchar_t line[1024];

    SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    for (index = 0; index < 2; ++index) {
        m_text_buffers[index] = 0;
    }
    for (index = 0; index < 3; ++index) {
        m_buttons[index] = 0;
    }
    g_npc_dialog = this;
    m_message = message;
    m_aux_data = aux_data;

    char opcode = message->kind_00;
    if (opcode == 5) {
        if (message->sub_entry_count == 2) {
            m_compact_options = 0;
        } else {
            m_compact_options = 1;
        }
        for (index = 0; index < message->sub_entry_count; ++index) {
            swprintf(line, g_format_S, message->sub_entries[index].text);
            short length = StringPixLength(line, g_wiz_text_mono_font);
            if (max_width < length) {
                max_width = length;
            }
        }
        m_text_width = max_width + 6;
        width = (m_text_width + 0xa) * message->sub_entry_count + 0x1e;
    } else if (opcode == 18 || opcode == 30) {
        m_compact_options = 0;
        short length = StringPixLength(gppStringList[0x7df], g_wiz_text_mono_font);
        height = 0x50;
        m_text_width = length + 6;
        width = m_text_width * 2 + 0x32;
    } else if (opcode == 19 || m_compact_options == 0) {
        width = 200;
    }
    /* A computed width under 200 only sticks when the compact flag is set,
       so a two-option list keeps the narrow width. */
    if (width < 200 && m_compact_options == 0) {
        width = 200;
    }
    SetOrigin((0x280 - width) / 2, 0xb8);
    SetExtent(width, height);
}

// SYNTHETIC: WIZ8 0x005da840
// W8NpcDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005DA860
W8NpcDialog::~W8NpcDialog()
{
    DestroyControls();
}

// FUNCTION: WIZ8 0x005DA8B0
int W8NpcDialog::CreateControls()
{
    int index;
    wchar_t line[1024];
    W8ControlsRect bounds;

    W8DialogBase::CreateControls();
    if (m_message->kind_00 == 5) {
        int x =
            (m_width - static_cast<short>((m_text_width + 10) * m_message->sub_entry_count - 10)) /
            2;
        for (index = 0; index < m_message->sub_entry_count; ++index) {
            swprintf(line, g_format_S, m_message->sub_entries[index].text);
            m_buttons[index] = new W8DialogButton;
            m_buttons[index]->ConfigureTextButton(
                line, g_wiz_text_mono_font, 4, 5, static_cast<short>(m_x + x),
                static_cast<short>(m_y + 0x23), m_text_width, 0x14, OptionSelected, index);
            x += m_text_width + 10;
        }
        bounds.left = m_x + 10;
        bounds.top = m_y + 10;
        bounds.right = m_x + m_width - 10;
        bounds.bottom = m_y + 0x19;
        m_text_buffers[0] = new W8TextBuffer(
            &bounds, gppStringList[0x7e4], g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
    } else if (m_message->kind_00 == 18 || m_message->kind_00 == 30) {
        short x = static_cast<short>((m_width - static_cast<short>((m_text_width + 5) * 2)) / 2);
        m_buttons[0] = new W8DialogButton;
        m_buttons[0]->ConfigureTextButton(
            gppStringList[0x7df], g_wiz_text_mono_font, 4, 5, static_cast<short>(m_x + x),
            static_cast<short>(m_y + 0x1e), m_text_width, 0x14, OptionSelected, 0);
        m_buttons[1] = new W8DialogButton;
        m_buttons[1]->ConfigureTextButton(gppStringList[0x7e0], g_wiz_text_mono_font, 4, 5,
                                          static_cast<short>(m_x + x + m_text_width + 10),
                                          static_cast<short>(m_y + 0x1e), m_text_width, 0x14,
                                          OptionSelected, 1);
        swprintf(line, gppStringList[0x7e5], g_screen_state_00649f1c->pending_price_204);
        bounds.left = m_x + 10;
        bounds.top = m_y + 10;
        bounds.right = m_x + m_width - 10;
        bounds.bottom = m_y + 0x19;
        m_text_buffers[0] = new W8TextBuffer(
            &bounds, line, g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
        swprintf(line, g_format_s_dg, gppStringList[0x7e6], g_status.party_gold);
        bounds.left = m_x + 10;
        bounds.top = m_y + 0x37;
        bounds.right = m_x + m_width - 10;
        bounds.bottom = m_y + 0x46;
        m_text_buffers[1] = new W8TextBuffer(
            &bounds, line, g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
    } else if (m_message->kind_00 == 19) {
        SetTextInputScheme(1);
        m_input_field = AddTextInputField(m_x + (m_width - 0x8c) / 2, m_y + 0x23, 0x8c, 0x10, 0x7f,
                                          &g_wchar_00689b34, 0x28, 0xf, 1);
        SetActiveField(m_input_field);
        bounds.left = m_x + 10;
        bounds.top = m_y + 10;
        bounds.right = m_x + m_width - 10;
        bounds.bottom = m_y + 0x19;
        m_text_buffers[0] = new W8TextBuffer(
            &bounds, gppStringList[0x7e4], g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
    }
    return 0;
}

// FUNCTION: WIZ8 0x005DAD90
void W8NpcDialog::DestroyControls()
{
    W8DialogBase::DestroyControls();
    int index;
    for (index = 0; index < 3; ++index) {
        if (m_buttons[index] != 0) {
            delete m_buttons[index];
            m_buttons[index] = 0;
        }
    }
    for (index = 0; index < 2; ++index) {
        if (m_text_buffers[index] != 0) {
            delete m_text_buffers[index];
            m_text_buffers[index] = 0;
        }
    }
    if (m_message->kind_00 == 19) {
        RemoveTextInputField(m_input_field);
    }
}

// FUNCTION: WIZ8 0x005DAE00
void W8NpcDialog::Draw()
{
    int index;
    if ((m_dirty_flags & 1) != 0) {
        if (m_initialized == 0) {
            CreateControls();
        }
        for (index = 0; index < 2; ++index) {
            if (m_text_buffers[index] != 0) {
                m_text_buffers[index]->SetGeometryDirty();
            }
        }
        for (index = 0; index < 3; ++index) {
            if (m_buttons[index] != 0) {
                m_buttons[index]->m_dirty = true;
            }
        }
        W8DialogBase::Draw();
    }
    for (index = 0; index < 3; ++index) {
        if (m_buttons[index] != 0) {
            m_buttons[index]->Draw();
        }
    }
    for (index = 0; index < 2; ++index) {
        if (m_text_buffers[index] != 0) {
            m_text_buffers[index]->RenderToTarget(0, 0, -0xe);
        }
    }
}

// FUNCTION: WIZ8 0x005DAE90
unsigned char W8NpcDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == TRUE) {
        if ((input.usEvent != KEY_DOWN && input.usEvent != KEY_REPEAT) ||
            static_cast<char>(HandleTextInput(&input)) == 0) {
            if (input.usEvent == KEY_DOWN && input.usParam == 0xd && m_message->kind_00 == 19) {
                Get16BitStringFromField(m_input_field, m_input_text);
                g_npc_dialog->m_keep_open = false;
                return 1;
            }
            unsigned short type;
            switch (input.usEvent) {
            case LEFT_BUTTON_DOWN:
            case LEFT_BUTTON_REPEAT:
                type = LEFT_BUTTON_DOWN;
                break;
            case LEFT_BUTTON_UP:
                type = LEFT_BUTTON_UP;
                break;
            case RIGHT_BUTTON_DOWN:
                type = RIGHT_BUTTON_DOWN;
                break;
            case RIGHT_BUTTON_UP:
                type = RIGHT_BUTTON_UP;
                break;
            default:
                continue;
            }
            MSYS_SGP_Mouse_Handler_Hook(type, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005DB150
void W8NpcDialog::OptionSelected(W8DialogButton* button)
{
    g_npc_dialog->m_selected_option = static_cast<unsigned char>(button->GetUserData());
    g_npc_dialog->m_keep_open = false;
}

// FUNCTION: WIZ8 0x005DB1A0
int W8NpcDialog::GetDialogType()
{
    return 7;
}
