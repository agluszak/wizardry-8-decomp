#include "wiz8/3d_code/PList.h"
#include "wiz8/3d_code/IList.h"
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
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/combat_state.h"
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
// GLOBAL: WIZ8 0x0068f2d8
W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];

void AppendNoticeLine(unsigned char font_palette, const wchar_t* text, short text_box,
                      int wrapped_line);

// FUNCTION: WIZ8 0x0058fdd0
int GetNextNoticeWord(int cursor, const wchar_t* text, W8NoticeWord* word)
{
    wchar_t buffer[150];
    int length = 0;
    buffer[0] = 0;
    while (text[cursor] == L' ') {
        ++cursor;
    }
    if (text[cursor] == 0) {
        return -1;
    }
    word->start = static_cast<short>(cursor);
    // SGP's read-only font API predates const-correct declarations.
    word->x_start =
        StringPixLengthArg(g_level_block->text_box_font, cursor, const_cast<wchar_t*>(text));
    while (text[cursor] != L' ' && text[cursor] != 0) {
        buffer[length++] = text[cursor++];
    }
    if (length != 0) {
        word->end = static_cast<short>(cursor - 1);
        buffer[length] = 0;
        word->x_end = StringPixLength(buffer, g_level_block->text_box_font) + word->x_start;
        return cursor;
    }
    return -1;
}

/* Rebuild every stored line's word list from its string - used when the text
   box geometry or font metrics change and the per-word hit ranges must be
   recomputed without dropping the lines themselves. */
// FUNCTION: WIZ8 0x0058FEE0
void ResetMessageStorage(void)
{
    int row;
    int index;

    for (row = 0; row < 4; ++row) {
        for (index = 0; index < 0x15e; ++index) {
            W8MessageStorageRecord* record = &g_message_storage_68f2d8[row][index];
            if (record->wString == 0) {
                continue;
            }
            if (record->entries_18 == 0) {
                record->entries_18 = PLCreate();
            } else {
                W8PList* entries = record->entries_18;
                // reinterpret-ok: retail counts the pointer-list through the IList API
                unsigned int count = ILLength(reinterpret_cast<W8IList*>(entries));
                unsigned int entry;
                for (entry = 0; entry < count; ++entry) {
                    free(PLGet(entries, entry));
                }
                PListClear(entries);
            }
            {
                int cursor = 0;
                W8NoticeWord word;
                while ((cursor = GetNextNoticeWord(cursor, record->wString, &word)) != -1) {
                    W8NoticeWord* stored = static_cast<W8NoticeWord*>(malloc(sizeof(W8NoticeWord)));
                    *stored = word;
                    stored->flag_08 = 0;
                    stored->flag_09 = 0;
                    PLAdoptAppend(record->entries_18, stored);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x0058af60
void AppendNoticeLine(unsigned char font_palette, const wchar_t* text, short text_box,
                      int wrapped_line)
{
    if (text_box == -1) {
        if ((gXStatus.fNpcDialogueMode && !CanOpenNpcDialogue()) || gXStatus.fCampMode) {
            text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
        } else if (GetFlag68F105()) {
            text_box = 0;
        } else {
            text_box = gXStatus.fCombatMode != 0 ? 1 : 0;
        }
    }
    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME &&
        g_current_screen_state.id != W8_SCREEN_CAMP &&
        g_current_screen_state.id != W8_SCREEN_PLEASE_WAIT &&
        g_current_screen_state.id != W8_SCREEN_CHARACTER) {
        return;
    }
    if (g_status_685170.text_box_lines_used_4997[text_box] == 350) {
        W8MessageStorageRecord* first = &g_message_storage_68f2d8[text_box][0];
        if (first->wString) {
            free(first->wString);
            first->wString = 0;
        }
        if (first->entries_18) {
            W8PList* entries = first->entries_18;
            int count = PLLength(entries);
            for (int index = 0; index < count; ++index) {
                free(PLGet(entries, index));
            }
            PListClear(entries);
            PLDestroy(first->entries_18);
            first->entries_18 = 0;
        }
        memmove(first, first + 1, 349 * sizeof(*first));
        memset(first + 349, 0, sizeof(*first));
        --g_status_685170.text_box_lines_used_4997[text_box];
        if (g_status_685170.text_box_lines_shown_49a7[text_box] != 0) {
            --g_status_685170.text_box_lines_shown_49a7[text_box];
            if (g_status_685170.text_box_lines_shown_49a7[text_box] <
                g_level_block->text_lines[text_box]) {
                g_level_block->text_lines[text_box] =
                    g_status_685170.text_box_lines_shown_49a7[text_box];
            }
        }
    }
    unsigned int index = g_status_685170.text_box_lines_used_4997[text_box]++;
    W8MessageStorageRecord* record = &g_message_storage_68f2d8[text_box][index];
    record->wString = static_cast<wchar_t*>(malloc((wcslen(text) + 1) * sizeof(wchar_t)));
    if (record->wString) {
        wcscpy(record->wString, text);
        record->entries_18 = 0;
        if (!record->entries_18) {
            record->entries_18 = PLCreate();
        } else {
            W8PList* entries = record->entries_18;
            int count = PLLength(entries);
            for (int entry = 0; entry < count; ++entry) {
                free(PLGet(entries, entry));
            }
            PListClear(entries);
        }
        int cursor = 0;
        W8NoticeWord word;
        while ((cursor = GetNextNoticeWord(cursor, record->wString, &word)) != -1) {
            W8NoticeWord* stored = static_cast<W8NoticeWord*>(malloc(sizeof(W8NoticeWord)));
            *stored = word;
            stored->flag_08 = 0;
            stored->flag_09 = 0;
            PLAdoptAppend(record->entries_18, stored);
        }
    }
    record->font_palette = font_palette;
    record->highlight_color = 0xff;
    record->link_10 = wrapped_line;
    record->value_14 = -1;
    ++g_notice_line_count_0069b7bc;
    if (g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
        g_status_685170.text_line_cursor_1795 == text_box) {
        if (ScreenLifecycleSuccess()) {
            AdvanceNoticeLine(text_box);
        }
    } else if (text_box == 3) {
        g_status_685170.text_box_lines_shown_49a7[3] = g_status_685170.text_box_lines_used_4997[3];
        g_level_block->text_lines[3] = g_status_685170.text_box_lines_used_4997[3];
        W8MessageStorageRecord* last =
            &g_message_storage_68f2d8[3][g_status_685170.text_box_lines_shown_49a7[3] - 1];
        unsigned int delay = g_settings_6850c8.text_display_delay_ms *
                             (wcslen(last->wString) * 100 / 20 + 100) / 100;
        if (delay > 60000) {
            delay = 60000;
        }
        last->clock_08 = SetCountdownClock(delay);
    }
}

// FUNCTION: WIZ8 0x0058fa80
void NoticeDialogDestroyed(W8DialogBase*)
{
    if (g_current_screen_state.id == W8_SCREEN_OPTIONS) {
        NoOp();
    }
}

/* Store the dialogue text-box rectangle and re-seat region 0x55 plus the three
   scrollbar hit regions that hang off its right edge. */
// FUNCTION: WIZ8 0x0058FA90
void SetTextBoxRegionBounds(int left, int top, int right, int bottom)
{
    unsigned short right_u;

    g_level_block->text_box_left = left;
    g_level_block->text_box_top = top;
    g_level_block->text_box_right = right;
    g_level_block->text_box_bottom = bottom;
    right_u = static_cast<unsigned short>(right);
    SetRegionBounds(0x55, static_cast<unsigned short>(left), static_cast<unsigned short>(top),
                    right_u, static_cast<unsigned short>(bottom));
    SetRegionBounds(0x52, static_cast<unsigned short>(right_u + 5), 0x16b,
                    static_cast<unsigned short>(right_u + 0x14), 0x17a);
    SetRegionBounds(0x53, static_cast<unsigned short>(right_u + 5), 0x1ad,
                    static_cast<unsigned short>(right_u + 0x14), 0x1bc);
    SetRegionBounds(0x54, static_cast<unsigned short>(right_u + 9), 0x17b,
                    static_cast<unsigned short>(right_u + 0x10), 0x1ac);
}

// FUNCTION: WIZ8 0x0058ac00
void ShowNotice(unsigned int font_palette, const wchar_t* text, short text_box,
                unsigned int wrap_width, bool force_dialog)
{
    GetFlag68F105();
    if (g_level_block != 0 && (g_current_screen_state.id == W8_SCREEN_MAIN_GAME ||
                               (g_current_screen_state.id == W8_SCREEN_CAMP && !force_dialog) ||
                               g_current_screen_state.id == W8_SCREEN_PLEASE_WAIT ||
                               g_current_screen_state.id == W8_SCREEN_CHARACTER)) {
        if (font_palette >= 16) {
            srAssertFail("uiFontPaletteIndex <= FONT_PALETTE_COUNT", MGS_TEXT_BOX_CPP, 0x15c, 0);
        }
        if (wrap_width == ~0U) {
            wrap_width = g_level_block->text_box_right - g_level_block->text_box_left - 10;
        }
        if (text_box == -1) {
            if ((gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) ||
                gXStatus.fCampMode != 0) {
                text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
            } else if (GetFlag68F105()) {
                text_box = 0;
            } else {
                text_box = gXStatus.fCombatMode != 0 ? 1 : 0;
            }
        }
        if (g_status_685170.quote_audit_2431) {
            g_notice_line_count_0069b7bc = 0;
            g_status_685170.long_quote_2432 = 0;
        }
        g_level_block->text_lines[8 + text_box] = 0;
        if (static_cast<unsigned int>(StringPixLength(const_cast<wchar_t*>(text),
                                                      g_level_block->text_box_font)) < wrap_width) {
            AppendNoticeLine(static_cast<unsigned char>(font_palette), text, text_box, 0);
        } else {
            wchar_t line[4096];
            bool more = true;
            int line_index = 0;
            /* This repeated default-box selection is present in the retail
               wrapping path as well as the unwrapped entry above. */
            if (text_box == -1) {
                if ((gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) ||
                    gXStatus.fCampMode != 0) {
                    text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
                } else if (GetFlag68F105()) {
                    text_box = 0;
                } else {
                    text_box = gXStatus.fCombatMode != 0 ? 1 : 0;
                }
            }
            while (more) {
                int words = 0;
                wchar_t* output = line;
                for (;;) {
                    const wchar_t* word_start = text;
                    wchar_t* output_start = output;
                    *output = *text;
                    output[1] = 0;
                    if (*text == L'\n') {
                        ++text;
                        *output = 0;
                        break;
                    }
                    bool wrapped = false;
                    for (;;) {
                        if (static_cast<unsigned int>(StringPixLength(
                                line, g_level_block->text_box_font)) >= wrap_width &&
                            words != 0) {
                            text = word_start;
                            *output_start = 0;
                            wrapped = true;
                            break;
                        }
                        ++text;
                        ++output;
                        if (*text == L' ' || *text == L'\t' || *text == 0 || *text == L'\n') {
                            break;
                        }
                        *output = *text;
                        output[1] = 0;
                    }
                    if (wrapped) {
                        break;
                    }
                    if (*text == 0) {
                        more = false;
                        break;
                    }
                    ++words;
                }
                AppendNoticeLine(static_cast<unsigned char>(font_palette), line, text_box,
                                 line_index);
                ++line_index;
                if (more) {
                    if (line_index == 1) {
                        g_level_block->text_lines[8 + text_box] += wcslen(line);
                    } else {
                        g_level_block->text_lines[8 + text_box] = ~0U;
                    }
                }
                line[0] = 0;
            }
        }
        if (g_status_685170.quote_audit_2431 && g_notice_line_count_0069b7bc > 7) {
            g_status_685170.long_quote_2432 = 1;
        }
        g_text_box_mode_0069b7b8 = 0;
    } else if (force_dialog) {
        W8MessageDialogBase* dialog = static_cast<W8MessageDialogBase*>(CreateDialogByKind(1));
        dialog->SetMessage(text, 1, 50, 1, 0, 1, 0, 0, 0);
        SetDialogDestroyCallback(dialog, NoticeDialogDestroyed);
        OpenModal(dialog);
        DrawDialog(g_modal_owner_0068edd0);
    }
}

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

/* One entry of text_slots_1e8. Index 2 is the secondary NPC-dialogue item
   editor slot; other indices remain positional. */
// FUNCTION: WIZ8 0x0058fa60
int GetTextSlot1E8(int index)
{
    return g_level_block->text_slots_1e8[index];
}

/* Empty one entry of either slot table and ask for a redraw. The two bodies
   differ only in which table they clear, which is what pairs them. The 0x1d8
   table still has no agreeing producer beyond init/clear. */
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
    return g_level_block->text_box_font;
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

/* Append text to the last used line of one text box. The line's record is
   removed and the combined string posted again so wrapping, the shown count
   and the continuation links rebuild, while the line keeps its channel,
   highlight span and clock. With the merge mode off the text posts as a new
   line instead. A -1 box means the one the current game mode posts to. */
// FUNCTION: WIZ8 0x005905F0
void AppendToLastTextLine(const wchar_t* text, int text_box)
{
    size_t length = wcslen(text);
    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME &&
        g_current_screen_state.id != W8_SCREEN_CAMP) {
        return;
    }
    if (g_text_box_mode_0069b7b8 == 0) {
        ShowNotice(g_text_box_value_0064bd54, text, text_box, -1, 0);
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
    unsigned int* lines_used = &g_status_685170.text_box_lines_used_4997[text_box];
    if (!(*lines_used > 0)) {
        srAssertFail("gStatus.uiTextBoxLinesUsed[iTextBuffer] > 0", MGS_TEXT_BOX_CPP, 0xf92, 0);
    }
    W8MessageStorageRecord* line = &g_message_storage_68f2d8[text_box][*lines_used - 1];
    if (!(line->wString != 0)) {
        srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0xf94, 0);
    }
    wchar_t* merged = static_cast<wchar_t*>(operator new((length + wcslen(line->wString)) * 2 + 2));
    wcscpy(merged, line->wString);
    wcscat(merged, text);
    unsigned char channel = line->font_palette;
    int link = line->link_10;
    unsigned char color = line->highlight_color;
    unsigned char stop = line->highlight_stop;
    unsigned char start = line->highlight_start;
    if (line->wString) {
        free(line->wString);
        line->wString = 0;
    }
    W8PList* entries = line->entries_18;
    if (entries) {
        // reinterpret-ok: retail counts the pointer-list through the IList API
        unsigned int count = ILLength(reinterpret_cast<W8IList*>(entries));
        for (unsigned int entry = 0; entry < count; ++entry) {
            free(PLGet(entries, entry));
        }
        PListClear(entries);
        PLDestroy(entries);
        line->entries_18 = 0;
    }
    if (g_status_685170.text_box_lines_shown_49a7[text_box] == *lines_used) {
        if (!(g_status_685170.text_box_lines_shown_49a7[text_box] > 0)) {
            srAssertFail("gStatus.uiTextBoxLinesShown[iTextBuffer] > 0", MGS_TEXT_BOX_CPP, 0xfa9,
                         0);
        }
        --g_status_685170.text_box_lines_shown_49a7[text_box];
    }
    --*lines_used;
    ShowNotice(channel, merged, text_box, -1, 0);
    if (color != 0xff) {
        HighlightTextBoxRange(color, start, stop, static_cast<short>(text_box));
    }
    if (link != 0) {
        W8MessageStorageRecord* last = &g_message_storage_68f2d8[text_box][*lines_used - 1];
        if (!(last->wString != 0)) {
            srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0xfbd, 0);
        }
        int propagated = 0;
        if (last->link_10 != -1 && last->link_10 + 1 > 0) {
            W8MessageStorageRecord* entry = last;
            do {
                entry->link_10 += link;
                ++propagated;
                --entry;
            } while (propagated < last->link_10 + 1);
        }
    }
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
    return (gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_261 == 0) &&
                   (gXStatus.fSpellCastMode != 0 || gXStatus.fItemSelectMode != 0 ||
                    gXStatus.fCampMode != 0 || gXStatus.fNpcDialogueMode != 0)
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

/* Whether more lines sit below the visible window on the cursor's text box,
   including the dormant typed-dialogue editor's extra rows when it is open on
   that box. */
// FUNCTION: WIZ8 0x0058b960
bool CurrentDialogueLineHasContent(void)
{
    short text_box = g_status_685170.text_line_cursor_1795;

    return g_level_block->text_lines[text_box] + GetTextBoxVisibleLineCount() <
           GetTextBoxLineCount(text_box);
}

// FUNCTION: WIZ8 0x0058b5f0
void AdvanceNoticeLine(short text_box)
{
    if (text_box == -1) {
        text_box = g_status_685170.text_line_cursor_1795;
    }
    if (g_status_685170.text_box_lines_shown_49a7[text_box] >= 350) {
        srAssertFail("gStatus.uiTextBoxLinesShown[iTextBuffer] < MAX_TEXT_BOX_LINES",
                     MGS_TEXT_BOX_CPP, 0x2a5, 0);
    }
    W8MessageStorageRecord* record =
        &g_message_storage_68f2d8[text_box][g_status_685170.text_box_lines_shown_49a7[text_box]];
    unsigned int delay =
        g_settings_6850c8.text_display_delay_ms * (wcslen(record->wString) * 100 / 20 + 100) / 100;
    if (delay > 60000) {
        delay = 60000;
    }
    record->clock_08 = SetCountdownClock(delay);
    unsigned int shown = ++g_status_685170.text_box_lines_shown_49a7[text_box];
    if (g_level_block->text_scroll_drag_idle) {
        if (gXStatus.fNpcDialogueMode && g_screen_state_00649f1c->flag_261) {
            ScrollTextBoxTo(shown);
            g_screen_state_00649f1c->flag_261 = 0;
        } else if (!gXStatus.fSpellCastMode && !gXStatus.fNpcDialogueMode &&
                   !gXStatus.fItemSelectMode && !gXStatus.fCampMode) {
            if (gXStatus.fCombatMode && g_combat_state->notice_scroll_pending_a57) {
                ScrollTextBoxTo(shown);
                g_combat_state->notice_scroll_pending_a57 = 0;
            } else if (shown == 350) {
                unsigned int scroll = g_level_block->text_lines[text_box];
                if (scroll >= 344) {
                    ScrollTextBoxTo(scroll - 1);
                } else if (scroll <= 342) {
                    ScrollTextBoxTo(343);
                }
            } else if (shown >= 8 && shown - 7 > g_level_block->text_lines[text_box]) {
                ScrollTextBoxTo(shown - 7);
            }
        } else if (!IsNpcDialogueTextBoxActive577830()) {
            ScrollTextBoxTo(GetTextBoxLineCount(g_status_685170.text_line_cursor_1795) -
                            GetTextBoxVisibleLineCount());
        }
    }
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058bbc0
void ScrollTextBoxTo(int line)
{
    short text_box = g_status_685170.text_line_cursor_1795;
    unsigned int count = GetTextBoxLineCount(text_box);
    unsigned int visible = GetTextBoxVisibleLineCount();
    if (count <= visible) {
        return;
    }
    unsigned int previous = g_level_block->text_lines[text_box];
    unsigned int target = static_cast<unsigned int>(line);
    if (target + visible <= count) {
        if (target > 349) {
            target = 350;
        }
        g_level_block->text_lines[text_box] = target;
    } else {
        g_level_block->text_lines[text_box] = count - visible;
    }
    if (g_level_block->text_lines[text_box] != previous) {
        g_level_block->text_content_region = (g_level_block->text_lines[text_box] != 0) + 0x56;
        g_level_block->dialogue_content_region =
            (g_level_block->text_lines[text_box] + GetTextBoxVisibleLineCount() <
             GetTextBoxLineCount(text_box)) +
            0x59;
        RequestRedraw(W8_REDRAW_TEXT_BOX);
    }
}

// FUNCTION: WIZ8 0x0058D7E0
static bool GrowDialogueTextBuffer(void)
{
    if (g_level_block->dialogue_text_input_open == 0) {
        return false;
    }
    if (g_level_block->dialogue_text_input == 0) {
        return false;
    }
    if (g_level_block->dialogue_text_input->text_capacity + 0x400 > 0x3ff8) {
        return false;
    }

    wchar_t* previous = g_level_block->dialogue_text_input->text;
    g_level_block->dialogue_text_input->text_capacity += 0x400;
    g_level_block->dialogue_text_input->text =
        new wchar_t[g_level_block->dialogue_text_input->text_capacity];
    if (g_level_block->dialogue_text_input->text == 0) {
        g_level_block->dialogue_text_input->text = previous;
        g_level_block->dialogue_text_input->text_capacity -= 0x400;
        return false;
    }
    wcscpy(g_level_block->dialogue_text_input->text, previous);
    delete[] previous;
    return true;
}

// FUNCTION: WIZ8 0x0058D890
static bool GrowDialogueLineOffsets(void)
{
    if (g_level_block->dialogue_text_input_open == 0) {
        return false;
    }
    if (g_level_block->dialogue_text_input == 0) {
        return false;
    }

    unsigned int* previous = g_level_block->dialogue_text_input->line_offsets;
    g_level_block->dialogue_text_input->line_capacity += 0x20;
    g_level_block->dialogue_text_input->line_offsets =
        new unsigned int[g_level_block->dialogue_text_input->line_capacity];
    if (g_level_block->dialogue_text_input->line_offsets == 0) {
        g_level_block->dialogue_text_input->line_offsets = previous;
        g_level_block->dialogue_text_input->line_capacity -= 0x20;
        return false;
    }
    memcpy(g_level_block->dialogue_text_input->line_offsets, previous,
           g_level_block->dialogue_text_input->line_count * sizeof(unsigned int));
    delete[] previous;
    return true;
}

// FUNCTION: WIZ8 0x0058D940
static void ReleaseDialogueTextInput(void)
{
    if (g_level_block->dialogue_text_input != 0) {
        delete[] g_level_block->dialogue_text_input->text;
        delete[] g_level_block->dialogue_text_input->line_offsets;
        delete[] g_level_block->dialogue_text_input->first_line_prefix;
        delete g_level_block->dialogue_text_input;
        g_level_block->dialogue_text_input = 0;
    }
}

// FUNCTION: WIZ8 0x0058DF60
static void InvalidateDialogueTextCursor(void)
{
    g_level_block->dialogue_text_input->dirty = 1;
    RequestRedraw(0x80000000);

    W8ControlsRect bounds;
    bounds.left = g_level_block->text_box_left;
    bounds.top = g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] -
                 g_level_block->text_lines[g_status_685170.text_line_cursor_1795] +
                 g_level_block->text_box_top;
    bounds.right =
        StringPixLength(g_level_block->dialogue_text_input->text, g_level_block->text_box_font) +
        bounds.left;
    bounds.bottom = GetFontHeight(g_level_block->text_box_font) + bounds.top;
    InvalidateMainGameActionPanelRect(&bounds);
}

// FUNCTION: WIZ8 0x0058DCA0
static void RewrapDialogueTextFromLine(unsigned int line)
{
    do {
        unsigned int start = g_level_block->dialogue_text_input->line_offsets[line - 1];
        int width = 0;
        if (line == 1 && g_level_block->dialogue_text_input->first_line_prefix != 0) {
            width = StringPixLength(g_level_block->dialogue_text_input->first_line_prefix,
                                    g_level_block->text_box_font);
        }
        g_level_block->dialogue_text_input->line_count = line;

        size_t word_length = wcscspn(g_level_block->dialogue_text_input->text + start, L" ");
        width += StringPixLengthArg(g_level_block->text_box_font, word_length + 1,
                                    g_level_block->dialogue_text_input->text + start);
        if (static_cast<unsigned int>(width) > g_level_block->dialogue_text_input->wrap_width) {
            return;
        }

        for (;;) {
            start += word_length;
            if (g_level_block->dialogue_text_input->text[start] == 0) {
                return;
            }
            ++start;
            word_length = wcscspn(g_level_block->dialogue_text_input->text + start, L" ");
            width += StringPixLengthArg(g_level_block->text_box_font, word_length + 1,
                                        g_level_block->dialogue_text_input->text + start);
            if (static_cast<unsigned int>(width) > g_level_block->dialogue_text_input->wrap_width) {
                break;
            }
        }

        if (g_level_block->dialogue_text_input->line_count ==
                g_level_block->dialogue_text_input->line_capacity &&
            !GrowDialogueLineOffsets()) {
            return;
        }
        g_level_block->dialogue_text_input->line_offsets[line] = start;
        ++g_level_block->dialogue_text_input->line_count;

        if (g_level_block->dialogue_text_input->cursor >
                g_level_block->dialogue_text_input->line_offsets[line] &&
            !IsNpcDialogueTextBoxActive577830()) {
            ScrollTextBoxTo(GetTextBoxLineCount(g_status_685170.text_line_cursor_1795) -
                            GetTextBoxVisibleLineCount());
        }
        ++line;
    } while (true);
}

// FUNCTION: WIZ8 0x0058D9C0
static void InsertDialogueTextCharacter(wchar_t character)
{
    if (g_level_block->dialogue_text_input_open == 0) {
        return;
    }
    if (g_level_block->dialogue_text_input == 0) {
        return;
    }

    size_t length = wcslen(g_level_block->dialogue_text_input->text);
    if (g_level_block->dialogue_text_input->text_capacity < length + 2 &&
        !GrowDialogueTextBuffer()) {
        return;
    }

    unsigned int line = FindDialogueTextLine(g_level_block->dialogue_text_input);
    unsigned int shown =
        g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795];
    unsigned int scroll = g_level_block->text_lines[g_status_685170.text_line_cursor_1795];
    if (shown < scroll && shown + line < scroll) {
        ScrollTextBoxTo(shown + line - 1);
    } else if (shown - scroll + line > 7) {
        ScrollTextBoxTo(shown - 8 + line);
    }

    unsigned char joins_previous_line = 0;
    if (character == L' ') {
        unsigned int word_line = FindDialogueTextLine(g_level_block->dialogue_text_input);
        if (word_line > 1) {
            unsigned int previous_start =
                g_level_block->dialogue_text_input->line_offsets[word_line - 1];
            joins_previous_line =
                wcscspn(g_level_block->dialogue_text_input->text + previous_start, L" ") >=
                g_level_block->dialogue_text_input->cursor - previous_start;
        }
    }

    for (int index = static_cast<int>(length);
         index >= static_cast<int>(g_level_block->dialogue_text_input->cursor); --index) {
        g_level_block->dialogue_text_input->text[index + 1] =
            g_level_block->dialogue_text_input->text[index];
    }

    if (joins_previous_line != 0) {
        g_level_block->dialogue_text_input->text[g_level_block->dialogue_text_input->cursor++] =
            character;
        RewrapDialogueTextFromLine(--line);
    } else {
        g_level_block->dialogue_text_input->text[g_level_block->dialogue_text_input->cursor++] =
            character;
        if (line < g_level_block->dialogue_text_input->line_count) {
            RewrapDialogueTextFromLine(line);
        } else {
            int width = 0;
            if (line == 1 && g_level_block->dialogue_text_input->first_line_prefix != 0) {
                width = StringPixLength(g_level_block->dialogue_text_input->first_line_prefix,
                                        g_level_block->text_box_font);
            }
            width += StringPixLength(g_level_block->dialogue_text_input->text +
                                         g_level_block->dialogue_text_input->line_offsets[line - 1],
                                     g_level_block->text_box_font);
            if (static_cast<unsigned int>(width) > g_level_block->dialogue_text_input->wrap_width) {
                RewrapDialogueTextFromLine(line);
            }
        }
    }
    InvalidateDialogueTextCursor();
}

// FUNCTION: WIZ8 0x0058E010
static void DeleteDialogueTextCharacter(unsigned int key)
{
    size_t length = wcslen(g_level_block->dialogue_text_input->text);

    if (key == 8) {
        if (g_level_block->dialogue_text_input->cursor != 0) {
            unsigned int line = FindDialogueTextLine(g_level_block->dialogue_text_input);
            unsigned int word_line = FindDialogueTextLine(g_level_block->dialogue_text_input);
            unsigned char joins_previous_line = 0;
            if (word_line > 1) {
                unsigned int previous_start =
                    g_level_block->dialogue_text_input->line_offsets[word_line - 1];
                joins_previous_line =
                    wcscspn(g_level_block->dialogue_text_input->text + previous_start, L" ") >=
                    g_level_block->dialogue_text_input->cursor - previous_start;
            }
            for (unsigned int index = g_level_block->dialogue_text_input->cursor; index <= length;
                 ++index) {
                g_level_block->dialogue_text_input->text[index - 1] =
                    g_level_block->dialogue_text_input->text[index];
            }
            --g_level_block->dialogue_text_input->cursor;
            if (joins_previous_line != 0) {
                RewrapDialogueTextFromLine(--line);
            } else if (line != g_level_block->dialogue_text_input->line_count) {
                RewrapDialogueTextFromLine(line);
            }
        }
    } else if (key == 0x2e) {
        if (g_level_block->dialogue_text_input->cursor < length) {
            unsigned int line = FindDialogueTextLine(g_level_block->dialogue_text_input);
            unsigned int word_line = FindDialogueTextLine(g_level_block->dialogue_text_input);
            unsigned char joins_previous_line = 0;
            if (word_line > 1) {
                unsigned int previous_start =
                    g_level_block->dialogue_text_input->line_offsets[word_line - 1];
                joins_previous_line =
                    wcscspn(g_level_block->dialogue_text_input->text + previous_start, L" ") >=
                    g_level_block->dialogue_text_input->cursor - previous_start;
            }
            for (unsigned int index = g_level_block->dialogue_text_input->cursor; index < length;
                 ++index) {
                g_level_block->dialogue_text_input->text[index] =
                    g_level_block->dialogue_text_input->text[index + 1];
            }
            if (joins_previous_line != 0) {
                RewrapDialogueTextFromLine(--line);
            } else if (line != g_level_block->dialogue_text_input->line_count) {
                RewrapDialogueTextFromLine(line);
            }
        }
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
    } else {
        position += 2;
        if (position > 0x2e) {
            position = 0x2e;
        }
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
            ShowNotice(input->notice_channel, input->text, input->text_box, input->wrap_width, 0);
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
