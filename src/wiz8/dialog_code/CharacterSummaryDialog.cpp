#include "wiz8/dialog_code/CharacterSummaryDialog.h"

#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/xstatus.h"

#include "input.h"
#include "english.h"
#include "mousesystem_macros.h"
#include "timer.h"

#include <ctype.h>
#include <string.h>

/* The bodies from 0x005E0320 through 0x005E0C30 form one dialog lifecycle
   cluster immediately after StatInfoDialogs.cpp.  Retail does not expose its
   original translation-unit or class spelling. */

// GLOBAL: WIZ8 0x00650250
static const W8ControlsRect g_character_summary_quote_bounds[1] = {
    {121, 24, 345, 72},
};

// FUNCTION: WIZ8 0x005e0320
W8CharacterSummaryDialog::W8CharacterSummaryDialog(W8Character* character)
    : m_voice_started_054(0), m_quote_text_058(0), m_numeric_input_05c(0), m_field_060(0),
      m_field_064(0), m_field_068(0), m_field_06c(0), m_field_070(0), m_character_074(character),
      m_field_1af8(0), m_portrait_clock_started_1af9(0)
{
    SetExtent(0x171, 0x60);
    SetBackground("Data\\Dialogs\\popup_quote.sti", 0);
}

// SYNTHETIC: WIZ8 0x005e03f0
// W8CharacterSummaryDialog::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005e0410
W8CharacterSummaryDialog::~W8CharacterSummaryDialog()
{
    DestroyControls();
}

// FUNCTION: WIZ8 0x005e04e0
int W8CharacterSummaryDialog::CreateControls()
{
    W8DialogBase::CreateControls();
    m_field_070 = 0;
    m_field_1af8 = 0;
    memcpy(&m_saved_character_078, g_status.buffers.Char, sizeof(m_saved_character_078));
    memcpy(static_cast<void*>(&m_saved_monster_entry_18da),
           static_cast<const void*>(&gXStatus.monster_manager_entries[0]),
           sizeof(m_saved_monster_entry_18da));
    memcpy(&m_saved_party_row_19f2, g_status.buffers.XChar, sizeof(m_saved_party_row_19f2));
    memcpy(g_status.buffers.Char, m_character_074, sizeof(*m_character_074));
    ResetPartySlotRow(0);
    ResetGameplaySlot(0);
    g_status.buffers.Char[0].fInParty = true;
    g_status.buffers.XChar[0].npc_index = -1;
    if (!CreateQuoteText()) {
        m_error = 7;
        return 7;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005e0590
void W8CharacterSummaryDialog::DestroyControls()
{
    W8DialogBase::DestroyControls();
    delete m_quote_text_058;
    m_quote_text_058 = 0;
    gXStatus.character_event_queue->CompleteAllActiveEvents();
    if (!m_field_1af8) {
        memcpy(g_status.buffers.Char, &m_saved_character_078, sizeof(m_saved_character_078));
        memcpy(static_cast<void*>(&gXStatus.monster_manager_entries[0]),
               static_cast<const void*>(&m_saved_monster_entry_18da),
               sizeof(m_saved_monster_entry_18da));
        memcpy(g_status.buffers.XChar, &m_saved_party_row_19f2, sizeof(m_saved_party_row_19f2));
    }
}

// FUNCTION: WIZ8 0x005e0600
bool W8CharacterSummaryDialog::CreateQuoteText()
{
    W8TextBuffer** buffers[] = {&m_quote_text_058};

    for (int index = 0; index < 1; ++index) {
        W8ControlsRect absolute = {g_character_summary_quote_bounds[index].left + m_x,
                                   g_character_summary_quote_bounds[index].top + m_y,
                                   g_character_summary_quote_bounds[index].right + m_x,
                                   g_character_summary_quote_bounds[index].bottom + m_y};
        W8Character* character = m_field_1af8 ? m_character_074 : g_status.buffers.Char;
        W8CharacterEvent* event =
            new W8CharacterEvent(character, g_effect_005ee588, 0, g_effect_argument_005ed8c8,
                                 g_effect_argument_005ed914);
        *buffers[index] = new W8TextBuffer(
            &absolute, event->GetQuoteText(), g_font_683660,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED54C, 4);
        delete event;
        if (*buffers[index] == 0) {
            delete m_quote_text_058;
            m_quote_text_058 = 0;
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x005e07b0
void W8CharacterSummaryDialog::Draw()
{
    if (m_dirty_flags & 1) {
        if (!m_initialized) {
            CreateControls();
        }
        m_quote_text_058->SetGeometryDirty();
        W8DialogBase::Draw();
    }
    if (m_quote_text_058 != 0) {
        m_quote_text_058->RenderToTarget(0, 0, -14);
    }
    RenderPartyPortrait(m_character_074->portrait_index, m_x + 11, m_y + 11, 2, 1, 0);
    if (!m_portrait_clock_started_1af9) {
        m_portrait_clock_started_1af9 = 1;
        m_portrait_clock_1afc = GetClock();
    }
}

// FUNCTION: WIZ8 0x005e0830
void W8CharacterSummaryDialog::DrawPortraitAnimationFrame()
{
    BlitPartyPortraitAnimation(m_character_074->portrait_index, m_x + 11, m_y + 11, 2, 0, 0);
}

// FUNCTION: WIZ8 0x005e0860
void W8CharacterSummaryDialog::OnNumericInputChanged(int value)
{
    if (value == 0) {
        m_field_068 = m_numeric_input_05c->m_value;
        m_field_064 = m_field_06c - m_field_068;
    }
}

// FUNCTION: WIZ8 0x005e0880
unsigned char W8CharacterSummaryDialog::HandleInputEvent(const InputAtom* input)
{
    W8DialogNumericInput* inputs[] = {m_numeric_input_05c};
    for (int index = 0; index < 1; ++index) {
        if (inputs[index] != 0 && inputs[index]->m_active) {
            if (inputs[index]->HandleInput(input)) {
                return 1;
            }
            break;
        }
    }
    if (input->usEvent == KEY_DOWN || input->usEvent == KEY_REPEAT) {
        switch (toupper(input->usParam)) {
        case BACKSPACE:
        case ENTER:
        case ESC:
        case SPACE:
            m_keep_open = false;
            break;
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005e0920
unsigned char W8CharacterSummaryDialog::ProcessInput()
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    if (!m_voice_started_054 && m_portrait_clock_1afc + 750 < GetClock()) {
        m_voice_started_054 = 1;
        W8Character* character = m_field_1af8 ? m_character_074 : g_status.buffers.Char;
        QueueCharacterEvent(character, g_effect_005ee588, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    while (DequeueEvent(&input) == 1) {
        switch (input.usEvent) {
        case LEFT_BUTTON_DOWN:
        case LEFT_BUTTON_REPEAT:
            m_keep_open = false;
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case LEFT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(LEFT_BUTTON_UP, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case RIGHT_BUTTON_DOWN:
            m_keep_open = false;
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_DOWN, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        case RIGHT_BUTTON_UP:
            MSYS_SGP_Mouse_Handler_Hook(RIGHT_BUTTON_UP, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
            break;
        default:
            return HandleInputEvent(&input);
        }
    }
    return m_keep_open;
}

// FUNCTION: WIZ8 0x005e0c30
int W8CharacterSummaryDialog::GetDialogType()
{
    return 6;
}
