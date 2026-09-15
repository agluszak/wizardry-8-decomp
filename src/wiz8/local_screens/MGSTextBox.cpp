#include "wiz8/3d_code/PList.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/cursor.h"
#include "wiz8/xstatus.h"
#include "timer.h"
#include "font.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/sr_api.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
// GLOBAL: WIZ8 0x0068f2d4
W8MainGameScreen* g_main_game_screen;

/*
 * Local Screens\MGSTextBox.cpp.
 *
 * The message box on the main game screen. Everything it reads and writes
 * lives on the level runtime block at 0x0068EDCC, which is the same object the
 * item manager and the movement rules reach through - the text state simply
 * occupies a different part of it.
 */

#define MGS_TEXT_BOX_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\MGSTextBox.cpp"

/* The redraw the text box asks for whenever anything it shows changes. */
enum { W8_REDRAW_TEXT_BOX = 0x800 };

// GLOBAL: WIZ8 0x0069b7b8
unsigned char g_text_box_mode_0069b7b8;
// GLOBAL: WIZ8 0x0069b7bc
int g_notice_line_count_0069b7bc;
// GLOBAL: WIZ8 0x0064bd54
int g_text_box_value_0064bd54 = 12;
/* 0x0068F2D4: the screen the text box belongs to; its two panels sit at 0x0c
   and 0x14. */

/* Release every heap object owned by the four-by-350 message store, then
   return each record to its all-zero initial state.  The retail body walks
   the same record boundary twice: 0x15e records per run and four runs up to
   the next global at 0x0069B7D0. */
W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];

// FUNCTION: WIZ8 0x0058fd30
void ReleaseMessageStorage(void)
{
    for (int row = 0; row < 4; ++row) {
        for (int index = 0; index < 0x15e; ++index) {
            W8MessageStorageRecord* record = &g_message_storage_68f2d8[row][index];
            if (record->wString) {
                free(record->wString);
            }
            W8PList* entries = record->entries_18;
            if (entries) {
                unsigned int count = PLLength(entries);
                for (unsigned int entry = 0; entry < count; ++entry) {
                    free(PLGet(entries, entry));
                }
                PListClear(entries);
                PLDestroy(entries);
                record->entries_18 = 0;
            }
            memset(record, 0, sizeof(*record));
        }
    }
}

/* How many lines the text box can still be scrolled through. */
// FUNCTION: WIZ8 0x0058fb30
int GetTextBoxScrollRange(void)
{
    return g_level_block->text_box_right - g_level_block->text_box_left;
}

/* One entry of the second slot table. */
// FUNCTION: WIZ8 0x0058fa60
int GetTextSlot1E8(int index)
{
    return g_level_block->text_slots_1e8[index];
}

/* Empty one entry of either slot table and ask for a redraw. The two bodies
   differ only in which table they clear, which is what pairs them. */
// FUNCTION: WIZ8 0x0058f960
void ClearTextSlot1D8(int index)
{
    g_level_block->text_slots_1d8[index] = -1;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058fa30
void ClearTextSlot1E8(int index)
{
    g_level_block->text_slots_1e8[index] = -1;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

/* Ask for the text box to be redrawn without changing anything. */
// FUNCTION: WIZ8 0x0058aa00
void RedrawTextBox(void)
{
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058ab60
void FormatNotice(int channel, short text_box, const wchar_t* format, ...)
{
    wchar_t text[4096];
    va_list arguments;
    va_start(arguments, format);
    vswprintf(text, format, arguments);
    va_end(arguments);

    if (text_box == -1) {
        if ((gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) || gXStatus.fCampMode != 0) {
            text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
        } else if (GetFlag68F105()) {
            text_box = 0;
        } else {
            text_box = gXStatus.fCombatMode != 0 ? 1 : 0;
        }
    }
    ShowNotice(channel, text, text_box, -1, 0);
}

/* The value the screen keeps beside the text. */
// FUNCTION: WIZ8 0x0058aa10
int GetTextBoxValue2E8(void)
{
    return g_level_block->value_2e8;
}

/* Whether the line the cursor is on has anything on it. */
// FUNCTION: WIZ8 0x0058b940
bool CurrentTextLineHasContent(void)
{
    return g_level_block->text_lines[g_status_685170.text_line_cursor_1795] != 0;
}

/* Scroll so the line the cursor is on is the last of eight showing, or to the
   top when it would fit anyway. */
// FUNCTION: WIZ8 0x0058b910
void ScrollTextBoxToCursor(void)
{
    if (g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] > 7) {
        ScrollTextBoxTo(
            g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] - 7);
        return;
    }
    ScrollTextBoxTo(0);
}

/* Recolour the character span [start, stop) of the most recent line of one
   text box. A -1 box means the one the current game mode posts to. When the
   line was wrapped, the box's split position says where the second row
   begins: a span ending before it recolours the previous row alone, one
   crossing it is cut in two, and one past it moves down by the split. */
// FUNCTION: WIZ8 0x0058b410
void HighlightTextBoxRange(unsigned char color, unsigned char start, unsigned char stop,
                           short text_box)
{
    W8MessageStorageRecord* line;
    W8MessageStorageRecord* previous;

    if (!(stop >= start)) {
        srAssertFail("ubStopChar >= ubStartChar", MGS_TEXT_BOX_CPP, 0x247, 0);
    }
    if (g_current_screen_state.id != 7) {
        return;
    }
    if (text_box == -1) {
        if ((gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) || gXStatus.fCampMode != 0) {
            text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
        } else if (GetFlag68F105()) {
            text_box = 0;
        } else {
            text_box = gXStatus.fCombatMode != 0;
        }
    }
    if (!(g_status_685170.text_box_lines_used_4997[text_box] > 0)) {
        srAssertFail("gStatus.uiTextBoxLinesUsed[iTextBuffer] > 0", MGS_TEXT_BOX_CPP, 0x261, 0);
    }
    line =
        &g_message_storage_68f2d8[text_box][g_status_685170.text_box_lines_used_4997[text_box] - 1];
    if (!(line->wString != 0)) {
        srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0x263, 0);
    }
    if (g_level_block->text_lines[8 + text_box] == (unsigned int)-1) {
        return;
    }
    if (g_level_block->text_lines[8 + text_box] == 0) {
        line->highlight_stop = stop;
        line->highlight_start = start;
        line->highlight_color = color;
        return;
    }
    if (!(g_status_685170.text_box_lines_used_4997[text_box] >= 2)) {
        srAssertFail("gStatus.uiTextBoxLinesUsed[iTextBuffer] >= 2", MGS_TEXT_BOX_CPP, 0x277, 0);
    }
    previous =
        &g_message_storage_68f2d8[text_box][g_status_685170.text_box_lines_used_4997[text_box] - 2];
    if (start >= g_level_block->text_lines[8 + text_box]) {
        line->highlight_start = start - g_level_block->text_lines[8 + text_box];
    } else {
        previous->highlight_start = start;
        previous->highlight_color = color;
        if (stop < g_level_block->text_lines[8 + text_box]) {
            previous->highlight_stop = stop;
            return;
        }
        previous->highlight_stop = (unsigned char)wcslen(previous->wString);
        line->highlight_start = 0;
    }
    line->highlight_stop = stop - g_level_block->text_lines[8 + text_box];
    line->highlight_color = color;
}

/* The dirty byte of the dormant typed-dialogue input state, or nothing when
   no input is open. */
// FUNCTION: WIZ8 0x0058d7c0
unsigned char GetOpenDialogueFlag(void)
{
    if (g_level_block->dialogue_text_input_open != 0 && g_level_block->dialogue_text_input != 0) {
        return g_level_block->dialogue_text_input->dirty;
    }
    return 0;
}

static unsigned int FindDialogueTextLine(const W8DialogueTextState* input)
{
    unsigned int line = 1;
    while (line < input->line_count && input->cursor >= input->line_offsets[line]) {
        ++line;
    }
    return line;
}

static int GetTextBoxVisibleLineCount(void)
{
    return gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0 ||
                   gXStatus.fCampMode != 0 ||
                   (gXStatus.fNpcDialogueMode != 0 && g_screen_state_00649f1c->flag_261 != 0)
               ? 7
               : 1;
}

static unsigned int GetTextBoxLineCount(short text_box)
{
    unsigned int count = g_status_685170.text_box_lines_shown_49a7[text_box];
    if (g_level_block->dialogue_text_input_open != 0 && g_level_block->dialogue_text_input != 0 &&
        g_level_block->dialogue_text_input->text_box == text_box) {
        count += g_level_block->dialogue_text_input->line_count;
    }
    return count;
}

// FUNCTION: WIZ8 0x0058D7E0
bool GrowDialogueTextBuffer(void)
{
    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0) {
        return false;
    }

    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    if (input->text_capacity + 0x400 > 0x3ff8) {
        return false;
    }

    wchar_t* previous = input->text;
    input->text_capacity += 0x400;
    input->text = new wchar_t[input->text_capacity];
    if (input->text == 0) {
        input->text = previous;
        input->text_capacity -= 0x400;
        return false;
    }
    wcscpy(input->text, previous);
    delete[] previous;
    return true;
}

// FUNCTION: WIZ8 0x0058D890
bool GrowDialogueLineOffsets(void)
{
    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0) {
        return false;
    }

    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    unsigned int* previous = input->line_offsets;
    input->line_capacity += 0x20;
    input->line_offsets = new unsigned int[input->line_capacity];
    if (input->line_offsets == 0) {
        input->line_offsets = previous;
        input->line_capacity -= 0x20;
        return false;
    }
    for (unsigned int line = 0; line < input->line_count; ++line) {
        input->line_offsets[line] = previous[line];
    }
    delete[] previous;
    return true;
}

// FUNCTION: WIZ8 0x0058D940
void ReleaseDialogueTextInput(void)
{
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    if (input != 0) {
        delete[] input->text;
        delete[] input->line_offsets;
        delete[] input->first_line_prefix;
        delete input;
        g_level_block->dialogue_text_input = 0;
    }
}

// FUNCTION: WIZ8 0x0058DF60
void InvalidateDialogueTextCursor(void)
{
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    input->dirty = 1;
    RequestRedraw(0x80000000);

    W8ControlsRect bounds;
    bounds.left = g_level_block->text_box_left;
    bounds.top = g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] -
                 g_level_block->text_lines[g_status_685170.text_line_cursor_1795] +
                 g_level_block->text_box_top;
    bounds.right = StringPixLength(input->text, g_level_block->value_2e8) + bounds.left;
    bounds.bottom = GetFontHeight(g_level_block->value_2e8) + bounds.top;
    InvalidateMainGameActionPanelRect(&bounds);
}

// FUNCTION: WIZ8 0x0058DCA0
void RewrapDialogueTextFromLine(unsigned int line)
{
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    do {
        unsigned int start = input->line_offsets[line - 1];
        int width = 0;
        if (line == 1 && input->first_line_prefix != 0) {
            width = StringPixLength(input->first_line_prefix, g_level_block->value_2e8);
        }
        input->line_count = line;

        size_t word_length = wcscspn(input->text + start, L" ");
        width += StringPixLengthArg(g_level_block->value_2e8, word_length + 1, input->text + start);
        if (static_cast<unsigned int>(width) > input->wrap_width) {
            return;
        }

        for (;;) {
            if (input->text[start + word_length] == 0) {
                return;
            }
            start += word_length + 1;
            word_length = wcscspn(input->text + start, L" ");
            width +=
                StringPixLengthArg(g_level_block->value_2e8, word_length + 1, input->text + start);
            if (static_cast<unsigned int>(width) > input->wrap_width) {
                break;
            }
        }

        if (input->line_count == input->line_capacity && !GrowDialogueLineOffsets()) {
            return;
        }
        input->line_offsets[line] = start;
        ++input->line_count;

        if (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->value_fc != 4) {
            unsigned int count = GetTextBoxLineCount(g_status_685170.text_line_cursor_1795);
            ScrollTextBoxTo(count - GetTextBoxVisibleLineCount());
        }
        ++line;
    } while (true);
}

// FUNCTION: WIZ8 0x0058D9C0
void InsertDialogueTextCharacter(wchar_t character)
{
    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0) {
        return;
    }

    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    size_t length = wcslen(input->text);
    if (input->text_capacity < length + 2 && !GrowDialogueTextBuffer()) {
        return;
    }

    unsigned int line = FindDialogueTextLine(input);
    unsigned int shown =
        g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795];
    unsigned int scroll = g_level_block->text_lines[g_status_685170.text_line_cursor_1795];
    if (shown < scroll && shown + line < scroll) {
        ScrollTextBoxTo(shown + line - 1);
    } else if (shown - scroll + line > 7) {
        ScrollTextBoxTo(shown - 8 + line);
    }

    bool joins_previous_line = false;
    if (character == L' ' && line > 1) {
        unsigned int previous_start = input->line_offsets[line - 1];
        joins_previous_line =
            wcscspn(input->text + previous_start, L" ") >= input->cursor - previous_start;
    }

    for (size_t index = length + 1; index > input->cursor; --index) {
        input->text[index] = input->text[index - 1];
    }
    input->text[input->cursor++] = character;

    if (joins_previous_line) {
        --line;
    } else if (line >= input->line_count) {
        int width = 0;
        if (line == 1 && input->first_line_prefix != 0) {
            width = StringPixLength(input->first_line_prefix, g_level_block->value_2e8);
        }
        unsigned int start = input->line_offsets[line - 1];
        width += StringPixLength(input->text + start, g_level_block->value_2e8);
        if (static_cast<unsigned int>(width) > input->wrap_width) {
            RewrapDialogueTextFromLine(line);
        }
    }
    InvalidateDialogueTextCursor();
}

// FUNCTION: WIZ8 0x0058E010
void DeleteDialogueTextCharacter(unsigned int key)
{
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    size_t length = wcslen(input->text);
    unsigned int line = FindDialogueTextLine(input);
    bool joins_previous_line = false;
    if (line > 1) {
        unsigned int previous_start = input->line_offsets[line - 1];
        joins_previous_line =
            wcscspn(input->text + previous_start, L" ") >= input->cursor - previous_start;
    }

    if (key == 8 && input->cursor != 0) {
        for (unsigned int index = input->cursor; index <= length; ++index) {
            input->text[index - 1] = input->text[index];
        }
        --input->cursor;
    } else if (key == 0x2e && input->cursor < length) {
        for (unsigned int index = input->cursor; index < length; ++index) {
            input->text[index] = input->text[index + 1];
        }
    }

    if (joins_previous_line) {
        --line;
    } else if (line != input->line_count) {
        RewrapDialogueTextFromLine(line);
    }
    InvalidateDialogueTextCursor();
}

// FUNCTION: WIZ8 0x0058E9F0
unsigned char TextBoxScrollThumbRegionEvent(const InputAtom* input_event, W8Region* region)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (input_event->usEvent != MOUSE_POS) {
        return 0;
    }

    short text_box = g_status_685170.text_line_cursor_1795;
    unsigned int visible_lines = GetTextBoxVisibleLineCount();
    unsigned int line_count = GetTextBoxLineCount(text_box);
    if (line_count <= visible_lines) {
        return 0;
    }

    if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
        g_level_block->text_scroll_drag_idle = 1;
        return 0;
    }
    if (gfLeftButtonState == 0) {
        g_level_block->text_scroll_drag_idle = 1;
        ClearActiveRegionIfMatches(0x54);
        return 0;
    }

    ActivateDialogRegion(0x54);
    int position = GetAtomCursorY004285A0(input_event) - region->y1 - 1;
    if (position < 0) {
        position = 0;
    } else if (position > 0x2e) {
        position = 0x2e;
    }

    unsigned int previous = g_level_block->text_lines[text_box];
    g_level_block->text_lines[text_box] = (line_count - visible_lines) * position / 0x2f;
    unsigned int current = g_level_block->text_lines[text_box];
    if (current != previous) {
        g_level_block->text_content_region = current != 0 ? 0x57 : 0x56;
        g_level_block->dialogue_content_region = current + visible_lines < line_count ? 0x5a : 0x59;
        RequestRedraw(W8_REDRAW_TEXT_BOX);
    }
    g_level_block->text_scroll_drag_idle = 0;
    return 1;
}

// FUNCTION: WIZ8 0x0058F250
unsigned char HandleDialogueTextInput(const InputAtom* input_event)
{
    if (g_level_block->dialogue_text_input_open == 0 ||
        (input_event->usKeyState & (CTRL_DOWN | ALT_DOWN)) != 0) {
        return 0;
    }
    if (input_event->usEvent != KEY_DOWN && input_event->usEvent != KEY_REPEAT) {
        return 1;
    }

    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    unsigned int key = input_event->usParam;
    switch (key) {
    case 8:
    case 0x2e:
        DeleteDialogueTextCharacter(key);
        return 1;
    case 0x0d:
    case 0x1b:
        if (input->completion_callback != 0) {
            input->completion_callback();
        }
        g_level_block->dialogue_text_input_open = 0;
        g_level_block->text_lines[input->text_box] = input->saved_scroll_line;
        if (input->text[0] != 0) {
            Function58AC00(input->notice_channel, input->text, input->text_box, input->wrap_width,
                           0);
        }
        ReleaseDialogueTextInput();
        RequestRedraw(W8_REDRAW_TEXT_BOX);
        return 1;
    case 0x20:
        InsertDialogueTextCharacter(L' ');
        return 1;
    case 0x23: {
        size_t length = wcslen(input->text);
        if (input->cursor < length) {
            input->cursor = length;
            InvalidateDialogueTextCursor();
        }
        return 1;
    }
    case 0x24:
        if (input->cursor != 0) {
            input->cursor = 0;
            InvalidateDialogueTextCursor();
        }
        return 1;
    case 0x25:
        if (input->cursor != 0) {
            --input->cursor;
            InvalidateDialogueTextCursor();
        }
        return 1;
    case 0x27:
        if (input->cursor < wcslen(input->text)) {
            ++input->cursor;
            InvalidateDialogueTextCursor();
        }
        return 1;
    default:
        break;
    }

    wchar_t character = TranslateKeyToCharacter(
        static_cast<unsigned short>(key), static_cast<unsigned char>(input_event->usKeyState));
    if ((character >= L'0' && character <= L'9') || (character >= L'A' && character <= L'Z') ||
        (character >= L'a' && character <= L'z') || (character >= L'!' && character <= L'/') ||
        (character >= L':' && character <= L'@') || (character >= L'[' && character <= L'_') ||
        (character >= L'{' && character <= L'}')) {
        InsertDialogueTextCharacter(character);
        return 1;
    }
    return 0;
}

/* The text box's mode, and the setter that also records a value when one is
   given - passing -1 leaves the value alone. */
// FUNCTION: WIZ8 0x005905e0
unsigned char GetTextBoxMode(void)
{
    return g_text_box_mode_0069b7b8;
}

// FUNCTION: WIZ8 0x005905c0
void SetTextBoxMode(unsigned char mode, int value)
{
    g_text_box_mode_0069b7b8 = mode;
    if (value != -1) {
        g_text_box_value_0064bd54 = value;
    }
}

/* Redraw the whole text box: its frame, its body, and its frame again on top -
   the second panel is drawn after the body rather than with the first. The two
   panels are Local Code\\Controls.cpp's Controls, and the null rectangle is how
   that class spells "all of it"; the screen holds them at 0x0c and 0x14. */
// FUNCTION: WIZ8 0x0058a8c0
void RedrawTextBoxComplete(void)
{
    W8MainGameScreen* screen = g_main_game_screen;

    screen->m_text_panel_00c->Invalidate(0);
    RedrawTextBoxBody();
    screen->m_action_panel_014->Invalidate(0);
}

/* The last message on the current line whose clock has stopped, searched from
   the newest backwards - so the first one found is the most recent finished
   message rather than the oldest. */
// FUNCTION: WIZ8 0x0058d760
int FindStoppedTextLine(void)
{
    int index = g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795];

    if (index == 0) {
        return -1;
    }
    while (--index >= 0) {
        if (ClockIsTicking(
                g_message_storage_68f2d8[g_status_685170.text_line_cursor_1795][index].clock_08) ==
            0) {
            return index;
        }
    }
    return -1;
}

/* Hand one key to the text box's own handler. A key that moved the selection
   is followed by the two calls that settle it; a key that did not is not. */
// FUNCTION: WIZ8 0x0058a8f0
char TextBoxHandleKey(const void* event)
{
    W8MainGameTextPanel* panel = g_main_game_screen->m_text_panel_00c;
    int before = panel->m_selection_078;
    char handled;

    handled = panel->m_key_handler_074->HandleKey(*(const unsigned short*)((const char*)event + 8));

    if (handled != 0 && panel->m_selection_078 != before) {
        ResetButtonSoundScheme();
        PlayButtonSound(3);
    }
    return handled;
}

/* Record what the Knock Knock spell is aimed at. Casting it anywhere the
   overlay is not up says so and records nothing - the message is the
   function's own name in the player's words. */
// FUNCTION: WIZ8 0x0058a9c0
void SetKnockKnockTarget(int target)
{
    W8MainGameScreen* screen = g_main_game_screen;

    if (gXStatus.fTrapInteractMode == 0) {
        ShowNotice(0xc, L"You can't cast Knock Knock here!", -1, -1, 0);
        return;
    }
    screen->m_target_14c = target;
    screen->m_status_panel_010->m_target_068 = target;
    screen->m_text_panel_00c->m_target_changed_140 = 1;
}
