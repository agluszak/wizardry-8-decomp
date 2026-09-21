#include "wiz8/3d_code/PList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/sgp_wide_text.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/notices.h"
#include "wiz8/regions.h"
#include "wiz8/cursor.h"
#include "wiz8/xstatus.h"
#include "timer.h"
#include "font.h"
#include "FileMan.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/sr_api.h"
#include "wiz8/dialog_code/MessageDialogBase.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/fonts.h"
#include "wiz8/utility.h"
#include "vobject_blitters.h"
#include "Font.h"
#include "line.h"
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
                unsigned int count = PLLength(entries);
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

// FUNCTION: WIZ8 0x0058AA20
void ResetEditorStatusLine0058AA20(short line)
{
    if (line == -1) {
        line = g_status_685170.text_line_cursor_1795;
    }
    g_status_685170.text_box_lines_used_4997[line] = 0;
    g_status_685170.text_box_lines_shown_49a7[line] = 0;
    if (!IsNpcDialogueTextBoxActive577830()) {
        g_level_block->text_lines[line] = 0;
    }
    g_level_block->text_lines[4 + line] = -1;
    g_level_block->text_slots_1d8[line] = -1;
    g_level_block->text_slots_1e8[line] = -1;
    g_level_block->text_content_region = 0x56;
    g_level_block->dialogue_content_region = 0x59;
    g_level_block->dialogue_text_input_open = 0;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
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

/* Write the four message runs into the open TEXT chunk: a format dword, then
   per region the used-line count followed by each 0x24-byte record and its
   wide string. The saved record's wString carries the serialized character
   count including the terminator, not the live pointer; the copy is patched
   so the on-disk record stays self-contained. */
// FUNCTION: WIZ8 0x0058FB50
unsigned char SaveMessageStorage0058FB50(int file)
{
    W8MessageStorageRecord record;
    int format = 1;
    unsigned int region;
    unsigned int index;

    FileWrite(file, &format, 4, 0);
    for (region = 0; region < 4; ++region) {
        FileWrite(file, &g_status_685170.text_box_lines_used_4997[region], 4, 0);
        for (index = 0; index < g_status_685170.text_box_lines_used_4997[region]; ++index) {
            record = g_message_storage_68f2d8[region][index];
            if (record.wString != 0) {
                /* The serialized record stores the wide-char count where the
                   live record keeps its string pointer. */
                // c-style-cast-ok: patched pointer field carries a count
                record.wString = (wchar_t*)(wcslen(record.wString) + 1);
            }
            FileWrite(file, &record, sizeof(record), 0);
            FileWrite(file, g_message_storage_68f2d8[region][index].wString,
                      (unsigned int)record.wString * 2, // c-style-cast-ok: reads
                      // back the patched count for the string payload size
                      0);
        }
    }
    return 1;
}

/* Read the four message runs back from the open TEXT chunk: a format dword,
   then per region the used-line count followed by each 0x24-byte record and
   its wide string. The serialized record's wString field is the character
   count from the save, not a pointer; each live record gets a fresh buffer
   sized from it, and the stale entries_18 list pointer is dropped. Saves
   older than the prepath link-height constant never wrote a fourth region, so
   its count is forced to zero without consuming a count slot. */
// FUNCTION: WIZ8 0x0058FC30
unsigned char LoadMessageStorage0058FC30(int file)
{
    W8MessageStorageRecord record;
    int format;
    unsigned int region;
    unsigned int index;
    unsigned int size;
    wchar_t* text;

    ReleaseMessageStorage();
    FileRead(file, &format, 4, 0);
    for (region = 0; region < 4; ++region) {
        if (g_status_685170.buffers.save_version < g_prepath_link_height_5ed300 && region == 3) {
            g_status_685170.text_box_lines_used_4997[3] = 0;
        } else {
            FileRead(file, &g_status_685170.text_box_lines_used_4997[region], 4, 0);
        }
        for (index = 0; index < g_status_685170.text_box_lines_used_4997[region]; ++index) {
            FileRead(file, &record, sizeof(record), 0);
            g_message_storage_68f2d8[region][index] = record;
            g_message_storage_68f2d8[region][index].entries_18 = 0;
            /* The serialized wString field is the wide-char count including
               the terminator, patched over the live pointer on save. */
            // c-style-cast-ok: reads back the patched count for the buffer size
            size = (unsigned int)record.wString * 2;
            text = static_cast<wchar_t*>(malloc(size));
            g_message_storage_68f2d8[region][index].wString = text;
            if (text != 0) {
                FileRead(file, text, size, 0);
            }
        }
    }
    return 1;
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

// FUNCTION: WIZ8 0x0058aad0
void ShowNoticef(unsigned int font_palette, const wchar_t* format, ...)
{
    wchar_t text[4096];
    va_list arguments;
    short text_box;

    va_start(arguments, format);
    vswprintf(text, format, arguments);
    va_end(arguments);

    if ((gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) || gXStatus.fCampMode != 0) {
        text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
    } else if (GetFlag68F105()) {
        text_box = 0;
    } else {
        text_box = gXStatus.fCombatMode != 0;
    }
    ShowNotice(font_palette, text, text_box, -1, false);
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

/* Append text onto a box's current line, growing its string in place; the
   optional second argument is the box, -1 for the one the current game mode
   posts to. */
// FUNCTION: WIZ8 0x0058B300
void AppendTextBoxLine0058B300(const wchar_t* text, ...)
{
    va_list arguments;
    va_start(arguments, text);
    short text_box = static_cast<short>(va_arg(arguments, int));
    va_end(arguments);

    if (text_box == -1) {
        if ((gXStatus.fNpcDialogueMode != 0 && !CanOpenNpcDialogue()) || gXStatus.fCampMode != 0) {
            text_box = IsNpcDialogueTextBoxActive() ? 0 : 2;
        } else if (GetFlag68F105()) {
            text_box = 0;
        } else {
            text_box = gXStatus.fCombatMode != 0;
        }
    }
    W8MessageStorageRecord* line =
        &g_message_storage_68f2d8[text_box][g_status_685170.text_box_lines_used_4997[text_box] - 1];
    if (line->wString == 0) {
        srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0x223, 0);
    }
    line->value_14 = wcslen(line->wString);
    wchar_t* previous = line->wString;
    line->wString = static_cast<wchar_t*>(
        malloc((wcslen(previous) + wcslen(text)) * sizeof(wchar_t) + sizeof(wchar_t)));
    if (line->wString != 0) {
        wcscpy(line->wString, previous);
        wcscat(line->wString, text);
        free(previous);
    } else {
        line->wString = previous;
    }
}

/* Re-scroll the dialogue text box to the cursor's shown line. While the
   dormant dialogue input owns that box its line count shifts the target; the
   margin is one line in quiet modes and seven under an overlay. */
// FUNCTION: WIZ8 0x0058BA60
void ScrollDialogueTextBoxToLine0058BA60(void)
{
    W8DialogueTextState* input;
    int offset;

    if (IsNpcDialogueTextBoxActive577830()) {
        return;
    }
    if (g_level_block->dialogue_text_input_open &&
        (input = g_level_block->dialogue_text_input) != 0 &&
        g_status_685170.text_line_cursor_1795 == input->text_box) {
        if (gXStatus.fNpcDialogueMode != 0) {
            offset = g_screen_state_00649f1c->text_box_collapsed != 0 ? 1 : 7;
        } else {
            offset = gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0 &&
                             gXStatus.fCampMode == 0
                         ? 1
                         : 7;
        }
        ScrollTextBoxTo(
            g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] +
            input->line_count - offset);
        return;
    }
    if (gXStatus.fNpcDialogueMode != 0) {
        offset = g_screen_state_00649f1c->text_box_collapsed != 0 ? 1 : 7;
    } else {
        offset =
            gXStatus.fSpellCastMode == 0 && gXStatus.fItemSelectMode == 0 && gXStatus.fCampMode == 0
                ? 1
                : 7;
    }
    ScrollTextBoxTo(
        g_status_685170.text_box_lines_shown_49a7[g_status_685170.text_line_cursor_1795] - offset);
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
        unsigned int count = PLLength(entries);
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

/* Expands at call sites that retail inlines; 0x00590900 is the out-of-line
   emission for the few retail CALL sites. */
/* Retail returns 7 when the text box is in a multi-line mode (spell / item /
   camp / NPC dialogue with the transcript collapsed); otherwise 1. */
#define W8_TEXT_BOX_VISIBLE_LINE_COUNT()                                                           \
    (((gXStatus.fNpcDialogueMode == 0 || g_screen_state_00649f1c->flag_261 == 0) &&                \
      (gXStatus.fSpellCastMode != 0 || gXStatus.fNpcDialogueMode != 0 ||                           \
       gXStatus.fItemSelectMode != 0 || gXStatus.fCampMode != 0))                                  \
         ? 7                                                                                       \
         : 1)

// FUNCTION: WIZ8 0x00590900
int GetTextBoxVisibleLineCount(void)
{
    unsigned char dialogue = gXStatus.fNpcDialogueMode;
    if (dialogue != 0) {
        if (g_screen_state_00649f1c->text_box_collapsed != 0) {
            return 1;
        }
    }
    if (gXStatus.fSpellCastMode == 0 && dialogue == 0 && gXStatus.fItemSelectMode == 0 &&
        gXStatus.fCampMode == 0) {
        return 1;
    }
    return 7;
}

// FUNCTION: WIZ8 0x00590950
void PostCharacterNotice(int party_slot, const wchar_t* format, ...)
{
    wchar_t separator[2];
    wchar_t text[4096];
    va_list arguments;
    int stop;

    va_start(arguments, format);
    vswprintf(text, format, arguments);
    va_end(arguments);

    wcscpy(separator, (text[0] == L'\'' || text[0] == L':') ? &g_wchar_00689b34 : L" ");
    ShowNoticef(8, L"%s%s%s", g_status_685170.buffers.characters[party_slot].name, separator, text);
    stop = wcslen(g_status_685170.buffers.characters[party_slot].name);
    if (text[0] == L'\'') {
        ++stop;
        if (text[1] == L's') {
            ++stop;
        }
    }
    HighlightTextBoxRange(g_status_685170.buffers.party_rows[party_slot].party_order_index, 0, stop,
                          -1);
}

/* PostCharacterNotice with an explicit context instead of the automatic -1
   box; the weapon-set swap paths post it under the dialogue context. */
// FUNCTION: WIZ8 0x00590A40
void PostCharacterNoticeInContext00590A40(int party_slot, int context, const wchar_t* format, ...)
{
    wchar_t separator[2];
    wchar_t text[4096];
    va_list arguments;
    int stop;

    va_start(arguments, format);
    vswprintf(text, format, arguments);
    va_end(arguments);

    wcscpy(separator, (text[0] == L'\'' || text[0] == L':') ? &g_wchar_00689b34 : L" ");
    FormatNotice(8, context, L"%s%s%s", g_status_685170.buffers.characters[party_slot].name,
                 separator, text);
    stop = wcslen(g_status_685170.buffers.characters[party_slot].name);
    if (text[0] == L'\'') {
        ++stop;
        if (text[1] == L's') {
            ++stop;
        }
    }
    HighlightTextBoxRange(g_status_685170.buffers.party_rows[party_slot].party_order_index, 0, stop,
                          context);
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
        if (gXStatus.fNpcDialogueMode && g_screen_state_00649f1c->text_box_collapsed) {
            ScrollTextBoxTo(shown);
            g_screen_state_00649f1c->text_box_collapsed = 0;
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
                            W8_TEXT_BOX_VISIBLE_LINE_COUNT());
        }
    }
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058bbc0
void ScrollTextBoxTo(int line)
{
    short text_box = g_status_685170.text_line_cursor_1795;
    unsigned int input_lines;
    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0 ||
        g_level_block->dialogue_text_input->text_box != text_box) {
        input_lines = 0;
    } else {
        input_lines = g_level_block->dialogue_text_input->line_count;
    }
    unsigned int count = g_status_685170.text_box_lines_shown_49a7[text_box];
    unsigned int visible = W8_TEXT_BOX_VISIBLE_LINE_COUNT();
    if (input_lines + count <= visible) {
        return;
    }
    unsigned int previous = g_level_block->text_lines[text_box];
    unsigned int target = static_cast<unsigned int>(line);
    if (target + visible <= count + input_lines) {
        if (target > 349) {
            target = 350;
        }
        g_level_block->text_lines[text_box] = target;
    } else {
        g_level_block->text_lines[text_box] = (count - visible) + input_lines;
    }
    if (g_level_block->text_lines[text_box] != previous) {
        g_level_block->text_content_region = (g_level_block->text_lines[text_box] != 0) + 0x56;
        g_level_block->dialogue_content_region =
            (g_level_block->text_lines[text_box] + W8_TEXT_BOX_VISIBLE_LINE_COUNT() <
             count + input_lines) +
            0x59;
        RequestRedraw(W8_REDRAW_TEXT_BOX);
    }
}

// FUNCTION: WIZ8 0x0058BF00
void ScrollTextBoxUp(int lines)
{
    short text_box = g_status_685170.text_line_cursor_1795;
    unsigned int previous = g_level_block->text_lines[text_box];
    if (previous == 0) {
        return;
    }

    unsigned int amount = static_cast<unsigned int>(lines);
    if (previous < amount) {
        g_level_block->text_lines[text_box] = 0;
    } else {
        g_level_block->text_lines[text_box] = previous - amount;
    }

    unsigned int current = g_level_block->text_lines[text_box];
    if (current == previous) {
        return;
    }
    if (current + W8_TEXT_BOX_VISIBLE_LINE_COUNT() < GetTextBoxLineCount(text_box)) {
        g_level_block->dialogue_content_region = 0x5a;
    }
    if (current == 0) {
        g_level_block->text_content_region = 0x56;
    }
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058C060
void ScrollTextBoxDown(int lines)
{
    short text_box = g_status_685170.text_line_cursor_1795;
    unsigned int count = GetTextBoxLineCount(text_box);
    unsigned int visible = W8_TEXT_BOX_VISIBLE_LINE_COUNT();
    unsigned int previous = g_level_block->text_lines[text_box];
    if (previous + visible >= count) {
        return;
    }

    if (previous + visible + static_cast<unsigned int>(lines) <= count) {
        g_level_block->text_lines[text_box] = previous + lines;
    } else {
        g_level_block->text_lines[text_box] = count - visible;
    }

    unsigned int current = g_level_block->text_lines[text_box];
    if (current == previous) {
        return;
    }
    if (current != 0) {
        g_level_block->text_content_region = 0x57;
    }
    if (current + visible >= count) {
        g_level_block->dialogue_content_region = 0x59;
    }
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

/* Draw the dormant typed-dialogue editor's caret: a vertical bar on the row
   holding the insertion point, offset by the first-line prefix and the
   pixel length of the text before the cursor. */
// FUNCTION: WIZ8 0x0058C8E0
static void DrawDialogueTextCursor0058C8E0(int x, int y)
{
    unsigned int pitch = 0;
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    unsigned int line = 1;
    unsigned int hidden = 0;

    if (input->line_count > 1) {
        const unsigned int* offset = input->line_offsets + 1;
        while (input->cursor >= *offset) {
            ++line;
            ++offset;
            if (line >= input->line_count) {
                break;
            }
        }
    }
    short text_box = g_status_685170.text_line_cursor_1795;
    if (g_status_685170.text_box_lines_shown_49a7[text_box] < g_level_block->text_lines[text_box]) {
        hidden = g_level_block->text_lines[text_box] -
                 g_status_685170.text_box_lines_shown_49a7[text_box];
    }
    if (line <= hidden) {
        return;
    }

    int x_offset = 0;
    if (line == 1 && input->first_line_prefix != 0) {
        x_offset = StringPixLength(input->first_line_prefix, g_level_block->text_box_font);
    }
    unsigned int start = input->line_offsets[line - 1];
    x_offset +=
        StringNPixLength(input->text + start, input->cursor - start, g_level_block->text_box_font);

    int y_line = y + (line - hidden - 1) * 0xb;
    char* screen = static_cast<char*>(LockPrimarySurface(&pitch));
    if (y_line < g_level_block->text_box_bottom) {
        unsigned int bottom = g_level_block->text_box_bottom;
        unsigned int y_end = y_line + GetFontHeight(g_level_block->text_box_font);
        if (bottom <= y_end) {
            y_end = bottom;
        }
        int x_cursor = x + x_offset + 1;
        LineDraw(0, x_cursor, y_line, x_cursor, y_end, -0x100, screen);
    }
    UnlockPrimarySurface();
}

/* Print the dormant typed-dialogue editor's wrapped lines starting at
   first_line, temporarily terminating each row at the next line's start
   offset; tracks the widest row and invalidates the printed rectangle. */
// FUNCTION: WIZ8 0x0058CA30
static void DrawDialogueTextInputLines0058CA30(int x, int y, unsigned int first_line)
{
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    wchar_t* string = input->text + input->line_offsets[first_line];
    int max_width = 0;
    int x_start = x;
    int y_start = y;
    unsigned int line = first_line;
    bool clipped = false;

    if (input->first_line_prefix != 0 && first_line == 0) {
        gprintf(x, y, const_cast<wchar_t*>(g_format_s_006068e4), input->first_line_prefix);
        x += StringPixLength(input->first_line_prefix, g_level_block->text_box_font);
    }
    if (line < input->line_count - 1) {
        do {
            wchar_t* next = input->text + input->line_offsets[line + 1];
            wchar_t saved = *next;
            *next = 0;
            gprintf(x, y, const_cast<wchar_t*>(g_format_s_006068e4), string);
            int width = StringPixLength(string, g_level_block->text_box_font);
            if (max_width < width) {
                max_width = StringPixLength(string, g_level_block->text_box_font);
            }
            input->text[input->line_offsets[line + 1]] = saved;
            y += 0xb;
            if (y >= g_level_block->text_box_bottom) {
                clipped = true;
                break;
            }
            if (x != x_start) {
                x = x_start;
            }
            string = input->text + input->line_offsets[line + 1];
            ++line;
        } while (line < input->line_count - 1);
    }
    if (!clipped) {
        gprintf(x, y, const_cast<wchar_t*>(g_format_s_006068e4), string);
        ++line;
        int width = StringPixLength(string, g_level_block->text_box_font);
        if (max_width < width) {
            max_width = StringPixLength(string, g_level_block->text_box_font);
        }
    }
    InvalidateRegion(x, y_start, x + max_width, y_start + line * 0xb, 0);
}

/* Repaint the dormant typed-dialogue editor over the text box: the wrapped
   lines when the input is dirty, then the cursor. */
// FUNCTION: WIZ8 0x0058C790
void RedrawDialogueTextInput0058C790(void)
{
    W8DialogueTextState* input = g_level_block->dialogue_text_input;
    if ((input->dirty != 0 || input->unknown_2c != 0)) {
        short text_box = g_status_685170.text_line_cursor_1795;
        int offset = g_status_685170.text_box_lines_shown_49a7[text_box] -
                     g_level_block->text_lines[text_box];
        if (offset < 7) {
            SaveFontSettings();
            SetFontDestBuffer(0xfffffff2, g_level_block->text_box_left, g_level_block->text_box_top,
                              g_level_block->text_box_right, g_level_block->text_box_bottom, '\0');
            SetFontObjectPalette16BPP(g_level_block->text_box_font, g_level_block->palette_2ec);
            SetFont(g_level_block->text_box_font);

            int y = g_level_block->text_box_top + offset * 0xb;
            if (y < g_level_block->text_box_top) {
                y = g_level_block->text_box_top;
            }
            if (input->dirty == 0) {
                if (input->unknown_2c != 0) {
                    DrawDialogueTextCursor0058C8E0(g_level_block->text_box_left, y);
                }
            } else {
                unsigned int first_line = 0;
                if (offset < 0) {
                    first_line = g_level_block->text_lines[text_box] -
                                 g_status_685170.text_box_lines_shown_49a7[text_box];
                }
                DrawDialogueTextInputLines0058CA30(g_level_block->text_box_left, y, first_line);
                input->dirty = 0;
                DrawDialogueTextCursor0058C8E0(g_level_block->text_box_left, y);
            }

            SetFontObjectPalette16BPP(g_level_block->text_box_font, g_level_block->palette_2ec);
            RestoreFontSettings();
        }
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
                            W8_TEXT_BOX_VISIBLE_LINE_COUNT());
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
    unsigned int visible_lines = W8_TEXT_BOX_VISIBLE_LINE_COUNT();
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

/* Unrecovered mode-specific text-box body handlers / wheel helpers. */
unsigned char UseItemSelectTextBoxRegionEvent(const InputAtom* event,
                                              W8Region* region);                 /* 0x0059DB40 */
void UseItemSelectTextBoxWheelAt(short x, unsigned short y, unsigned char flag); /* 0x0059DD30 */
unsigned char SpellCastTextBoxRegionEvent(const InputAtom* event,
                                          W8Region* region); /* 0x005A0F70 */

// FUNCTION: WIZ8 0x0058E2A0
unsigned char TextBoxScrollUpRegionEvent(const InputAtom* event, W8Region* region)
{
    short text_box = g_status_685170.text_line_cursor_1795;
    if (g_level_block->text_lines[text_box] == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    unsigned short us_event = event->usEvent;
    if (us_event < RIGHT_BUTTON_DOWN + 1) {
        if (us_event == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_UP && us_event != LEFT_BUTTON_REPEAT) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }

        unsigned int previous = g_level_block->text_lines[text_box];
        if (previous == 0) {
            return 1;
        }
        g_level_block->text_lines[text_box] = previous - 1;
        unsigned int current = g_level_block->text_lines[text_box];
        if (current == previous) {
            return 1;
        }
        if (current + GetTextBoxVisibleLineCount() < GetTextBoxLineCount(text_box)) {
            g_level_block->dialogue_content_region = 0x5a;
        }
        if (g_level_block->text_lines[text_box] == 0) {
            g_level_block->text_content_region = 0x56;
            RequestRedraw(W8_REDRAW_TEXT_BOX);
            return 1;
        }
    } else {
        if (us_event != RIGHT_BUTTON_UP && us_event != RIGHT_BUTTON_REPEAT) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                if ((region->flags & W8_REGION_MOUSE_ENTER) != 0 &&
                    g_level_block->text_lines[text_box] != 0) {
                    g_level_block->text_content_region = 0x58;
                    RequestRedraw(W8_REDRAW_TEXT_BOX);
                }
            } else if (g_level_block->text_lines[text_box] != 0) {
                g_level_block->text_content_region = 0x57;
                RequestRedraw(W8_REDRAW_TEXT_BOX);
                return 0;
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) == 0) {
            return 1;
        }

        unsigned int previous = g_level_block->text_lines[text_box];
        if (previous == 0) {
            return 1;
        }
        if (previous < 7) {
            g_level_block->text_lines[text_box] = 0;
        } else {
            g_level_block->text_lines[text_box] = previous - 7;
        }
        unsigned int current = g_level_block->text_lines[text_box];
        if (current == previous) {
            return 1;
        }
        if (current + W8_TEXT_BOX_VISIBLE_LINE_COUNT() < GetTextBoxLineCount(text_box)) {
            g_level_block->dialogue_content_region = 0x5a;
        }
        if (g_level_block->text_lines[text_box] == 0) {
            g_level_block->text_content_region = 0x56;
        }
    }
    RequestRedraw(W8_REDRAW_TEXT_BOX);
    return 1;
}

// FUNCTION: WIZ8 0x0058E650
unsigned char TextBoxScrollDownRegionEvent(const InputAtom* event, W8Region* region)
{
    short text_box = g_status_685170.text_line_cursor_1795;
    unsigned int visible = W8_TEXT_BOX_VISIBLE_LINE_COUNT();
    unsigned int count = GetTextBoxLineCount(text_box);
    if (g_level_block->text_lines[text_box] + visible >= count) {
        PushButtonSoundScheme005587C0(0, 1);
    }

    unsigned short us_event = event->usEvent;
    if (us_event < RIGHT_BUTTON_DOWN + 1) {
        if (us_event == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (us_event == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (us_event != LEFT_BUTTON_UP && us_event != LEFT_BUTTON_REPEAT) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            ScrollTextBoxDown(1);
            return 1;
        }
    } else {
        if (us_event != RIGHT_BUTTON_UP && us_event != RIGHT_BUTTON_REPEAT) {
            if (us_event != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                    if (g_level_block->text_lines[text_box] + visible < count) {
                        g_level_block->dialogue_content_region = 0x5b;
                        RequestRedraw(W8_REDRAW_TEXT_BOX);
                    }
                }
            } else {
                if (g_level_block->text_lines[text_box] + visible < count) {
                    g_level_block->dialogue_content_region = 0x5a;
                    RequestRedraw(W8_REDRAW_TEXT_BOX);
                    return 0;
                }
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0) {
            ScrollTextBoxDown(7);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0058ED90
unsigned char TextBoxBodyRegionEvent(const InputAtom* event, W8Region* region)
{
    PushButtonSoundScheme005587C0(0, 1);
    if (event->usEvent == MOUSE_WHEEL) {
        short delta = GetMouseWheelDeltaValue(event->usParam);
        if (delta < 0) {
            ScrollTextBoxDown(-delta);
        } else {
            short text_box = g_status_685170.text_line_cursor_1795;
            unsigned int previous = g_level_block->text_lines[text_box];
            if (previous != 0) {
                unsigned int amount = static_cast<unsigned int>(delta);
                if (previous < amount) {
                    g_level_block->text_lines[text_box] = 0;
                } else {
                    g_level_block->text_lines[text_box] = previous - amount;
                }
                unsigned int current = g_level_block->text_lines[text_box];
                if (current != previous) {
                    if (current + W8_TEXT_BOX_VISIBLE_LINE_COUNT() <
                        GetTextBoxLineCount(text_box)) {
                        g_level_block->dialogue_content_region = 0x5a;
                    }
                    if (g_level_block->text_lines[text_box] == 0) {
                        g_level_block->text_content_region = 0x56;
                    }
                    RequestRedraw(W8_REDRAW_TEXT_BOX);
                }
            }
        }
        if (gXStatus.fNpcDialogueMode == 0) {
            if (gXStatus.fItemSelectMode != 0) {
                UseItemSelectTextBoxWheelAt(static_cast<short>(event->uiParam),
                                            static_cast<unsigned short>(event->uiParam >> 16), 1);
            }
            return 1;
        }
        NpcDialogueTextBoxWheelAt(static_cast<short>(event->uiParam),
                                  static_cast<unsigned short>(event->uiParam >> 16), 1);
        return 1;
    }
    if (gXStatus.fNpcDialogueMode != 0) {
        return NpcDialogueTextBoxRegionEvent(event, region);
    }
    if (gXStatus.fSpellCastMode == 0) {
        if (gXStatus.fItemSelectMode == 0) {
            return 0;
        }
        return UseItemSelectTextBoxRegionEvent(event, region);
    }
    return SpellCastTextBoxRegionEvent(event, region);
}

// FUNCTION: WIZ8 0x0058EFD0
unsigned char TextBoxChannelTabRegionEvent(const InputAtom* event, W8Region* region)
{
    POINT mouse_pos;

    PushButtonSoundScheme005587C0(0, 1);
    SGPMouseGetPos(&mouse_pos);
    if (event->usEvent != LEFT_BUTTON_UP) {
        return 0;
    }

    g_status_685170.text_line_cursor_1795 = static_cast<short>(region->callback_id);
    region->flags |= W8_REGION_LEFT_BUTTON_HELD;
    RequestRedraw(W8_REDRAW_TEXT_BOX);

    short text_box = g_status_685170.text_line_cursor_1795;
    if (g_level_block->text_lines[text_box] == 0) {
        g_level_block->text_content_region = 0x56;
    } else {
        g_level_block->text_content_region = 0x57;
    }

    unsigned int visible = GetTextBoxVisibleLineCount();
    unsigned int count = GetTextBoxLineCount(text_box);
    unsigned int scroll = g_level_block->text_lines[text_box];
    if (scroll + visible < count) {
        g_level_block->dialogue_content_region = 0x5a;
    } else {
        if (scroll + visible >= count) {
            g_level_block->dialogue_content_region = 0x59;
        }
    }

    if (region->callback_id == 3) {
        if (g_status_685170.text_box_lines_shown_49a7[text_box] > 7) {
            ScrollTextBoxTo(g_status_685170.text_box_lines_shown_49a7[text_box] - 7);
            return 1;
        }
        ScrollTextBoxTo(0);
    }
    return 1;
}

/* Background mute for the message/text-box plates that share this catalog
   callback - suppress the default region click sound and swallow the event. */
// FUNCTION: WIZ8 0x0058F240
unsigned char TextBoxMuteRegionEvent(const InputAtom*, W8Region*)
{
    PushButtonSoundScheme005587C0(0, 1);
    return 0;
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

/* 0x0058FFC0: draw clickable notice-word overlays for one painted line. */
void DrawNoticeWordOverlays(W8MessageStorageRecord* line, int x, int y);

/* Paint one message-storage line at (x, y). slot_1d8_match / slot_1e8_match
   select alternate palettes for the editor slot highlights; skip_invalidate
   is forwarded from RedrawTextBoxBody and skips the word-overlay pass when
   set. Retail reuses the leading bytes of the level block as a wchar scratch. */
// FUNCTION: WIZ8 0x0058D2C0
void DrawTextBoxLine(W8MessageStorageRecord* line, int x, int y, unsigned char slot_1d8_match,
                     unsigned char slot_1e8_match, unsigned char skip_invalidate)
{
    unsigned short* palette;
    int draw_x;
    int length;
    int width;
    wchar_t* scratch = g_level_block->text_paint_scratch_000;

    if (line->wString == 0) {
        srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0x584, 0);
    }
    if (skip_invalidate != 0) {
        DrawNoticeWordOverlays(line, x, y);
        return;
    }

    palette = g_font_state_palettes_68ee1c[3];
    if (slot_1e8_match == 0) {
        if (slot_1d8_match != 0) {
            palette = g_font_state_palettes_68ee1c[0];
            if (static_cast<char>(line->font_palette) != 5) {
                palette = g_font_state_palettes_68ee1c[5];
            }
            SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
            if (line->value_14 == -1) {
                mprintf(x, y, Wiz8ToSgpWideText(g_format_s_006068e4), line->wString);
            } else {
                wcsncpy(scratch, line->wString, line->value_14);
                scratch[line->value_14] = 0;
                mprintf(x, y, Wiz8ToSgpWideText(g_format_s_006068e4), scratch);
            }
        } else if (line->highlight_color == 0xff) {
            if (line->font_palette < 0xf) {
                palette = g_font_state_palettes_68ee1c[line->font_palette];
            } else {
                palette = g_level_block->palette_2ec;
            }
            SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
            if (line->value_14 == -1) {
                mprintf(x, y, Wiz8ToSgpWideText(g_format_s_006068e4), line->wString);
            } else {
                wcsncpy(scratch, line->wString, line->value_14);
                scratch[line->value_14] = 0;
                mprintf(x, y, Wiz8ToSgpWideText(g_format_s_006068e4), scratch);
            }
        } else {
            draw_x = x;
            if (line->highlight_start != 0) {
                if (line->font_palette < 0xf) {
                    palette = g_font_state_palettes_68ee1c[line->font_palette];
                } else {
                    palette = g_level_block->palette_2ec;
                }
                SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
                wcsncpy(scratch, line->wString, line->highlight_start);
                scratch[line->highlight_start] = 0;
                mprintf(draw_x, y, Wiz8ToSgpWideText(g_format_s_006068e4), scratch);
                draw_x += StringPixLength(Wiz8ToSgpWideText(scratch), g_level_block->text_box_font);
            }
            if (line->highlight_stop < line->highlight_start) {
                srAssertFail("pTextLine->ubStopChar >= pTextLine->ubStartChar", MGS_TEXT_BOX_CPP,
                             0x5da, 0);
            }
            if (line->value_14 == -1 ||
                line->highlight_stop <= static_cast<unsigned int>(line->value_14)) {
                length = line->highlight_stop - line->highlight_start;
            } else {
                length = line->value_14 - line->highlight_start;
            }
            if (length > 0) {
                if (line->highlight_color < 0xf) {
                    palette = g_font_state_palettes_68ee1c[line->highlight_color];
                } else {
                    palette = g_level_block->palette_2ec;
                }
                SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
                wcsncpy(scratch, line->wString + line->highlight_start, length);
                scratch[length] = 0;
                mprintf(draw_x, y, Wiz8ToSgpWideText(g_format_s_006068e4), scratch);
                draw_x += StringPixLength(Wiz8ToSgpWideText(scratch), g_level_block->text_box_font);
            }
            if (wcslen(line->wString) < line->highlight_stop) {
                srAssertFail("wcslen(pTextLine->wString) >= pTextLine->ubStopChar",
                             MGS_TEXT_BOX_CPP, 0x5ef,
                             FormatString("DrawTextMessage: ERROR - String length %d, stop %d",
                                          wcslen(line->wString), line->highlight_stop));
            }
            if (line->value_14 == -1) {
                length = static_cast<int>(wcslen(line->wString)) - line->highlight_stop;
            } else {
                length = line->value_14 - line->highlight_stop;
            }
            if (length > 0) {
                unsigned char palette_index;
                if (line->value_14 == -1 ||
                    line->highlight_stop <= static_cast<unsigned int>(line->value_14)) {
                    palette_index = line->font_palette;
                } else {
                    palette_index = line->highlight_color;
                }
                if (palette_index < 0xf) {
                    palette = g_font_state_palettes_68ee1c[palette_index];
                } else {
                    palette = g_level_block->palette_2ec;
                }
                SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
                wcsncpy(scratch, line->wString + line->highlight_stop, length);
                scratch[length] = 0;
                mprintf(draw_x, y, Wiz8ToSgpWideText(g_format_s_006068e4), scratch);
            }
        }
    } else {
        SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
        if (line->value_14 == -1) {
            mprintf(x, y, Wiz8ToSgpWideText(g_format_s_006068e4), line->wString);
        } else {
            wcsncpy(scratch, line->wString, line->value_14);
            scratch[line->value_14] = 0;
            mprintf(x, y, Wiz8ToSgpWideText(g_format_s_006068e4), scratch);
        }
    }

    if (line->value_14 != -1) {
        wcscpy(scratch, line->wString + line->value_14);
        width = StringPixLength(Wiz8ToSgpWideText(scratch), g_level_block->text_box_font);
        mprintf(g_level_block->text_box_right - width, y, Wiz8ToSgpWideText(g_format_s_006068e4),
                scratch);
        DrawNoticeWordOverlays(line, x, y);
        return;
    }
    DrawNoticeWordOverlays(line, x, y);
}

/* Repaint the visible text-box window. When skip_invalidate is clear, also
   invalidate the text rectangle first. */
// FUNCTION: WIZ8 0x0058C3A0
void RedrawTextBoxBody(unsigned char skip_invalidate)
{
    short text_box;
    unsigned int shown;
    unsigned int scroll;
    unsigned int editor_lines;
    unsigned int rows;
    unsigned int row;
    int y_offset;
    int x;
    int line_index;
    W8MessageStorageRecord* line;
    bool can_scroll_down;

    if (skip_invalidate == 0) {
        InvalidateRegion(g_level_block->text_box_left, g_level_block->text_box_top,
                         g_level_block->text_box_right, g_level_block->text_box_bottom, 0);
    }

    text_box = g_status_685170.text_line_cursor_1795;
    shown = g_status_685170.text_box_lines_shown_49a7[text_box];
    if (shown == 0) {
        return;
    }

    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0 ||
        g_level_block->dialogue_text_input->text_box != text_box) {
        editor_lines = 0;
    } else {
        editor_lines = g_level_block->dialogue_text_input->line_count;
    }

    g_level_block->text_lines[4 + text_box] = FindStoppedTextLine();

    scroll = g_level_block->text_lines[text_box];
    rows = (shown - scroll) + editor_lines;
    if (rows < 7) {
        if (rows == 0) {
            return;
        }
    } else {
        rows = 7;
    }

    if (g_level_block->action_panel_visible == 0 && g_level_block->flag_272 == 0) {
        can_scroll_down = scroll + static_cast<unsigned int>(GetTextBoxVisibleLineCount()) <
                          GetTextBoxLineCount(text_box);
        if (!can_scroll_down &&
            ClockIsTicking(g_message_storage_68f2d8[text_box][scroll + rows - 1].clock_08) == 0) {
            return;
        }
    }

    SaveFontSettings();
    SetFontDestBuffer(0xfffffff2, g_level_block->text_box_left, g_level_block->text_box_top,
                      g_level_block->text_box_right, g_level_block->text_box_bottom, 0);
    SetFontObjectPalette16BPP(g_level_block->text_box_font, g_level_block->palette_2ec);
    SetFont(g_level_block->text_box_font);

    y_offset = 0;
    for (row = 0; row < rows; ++row) {
        line_index = static_cast<int>(scroll + row);
        if (static_cast<unsigned int>(line_index) < 0x15e) {
            line = &g_message_storage_68f2d8[text_box][line_index];
            if (line->wString == 0) {
                srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0x43d, 0);
            }
            if (line->wString != 0) {
                if (text_box == 3 || g_level_block->flag_271 == 0) {
                    x = g_level_block->text_box_left;
                } else {
                    x = g_level_block->text_box_left +
                        ((g_level_block->text_box_right - g_level_block->text_box_left) / 2 -
                         StringPixLength(Wiz8ToSgpWideText(line->wString),
                                         g_level_block->text_box_font) /
                             2);
                }
                DrawTextBoxLine(
                    line, x, g_level_block->text_box_top + y_offset,
                    line_index - line->link_10 == g_level_block->text_slots_1d8[text_box],
                    line_index - line->link_10 == g_level_block->text_slots_1e8[text_box],
                    skip_invalidate);
            }
        }
        y_offset += 0xb;
    }

    SetFontObjectPalette16BPP(g_level_block->text_box_font, g_level_block->palette_2ec);
    RestoreFontSettings();
}

/* Redraw the text-box scroll chrome (up/down buttons and thumb) when the
   action panel is raised, invalidate the chrome strip, then repaint the body. */
// FUNCTION: WIZ8 0x0058CC10
void RedrawTextBoxScrollChrome(void)
{
    SGPRect saved_clip;
    SGPRect clip;
    short text_box;
    unsigned int scroll;
    unsigned int line_count;
    int visible;
    int thumb_y;

    GetClippingRect(&saved_clip);
    clip = saved_clip;
    clip.iBottom = 0x1db;
    SetClippingRect(&clip);

    if (g_level_block->action_panel_visible != 0) {
        text_box = g_status_685170.text_line_cursor_1795;
        scroll = g_level_block->text_lines[text_box];
        if (scroll == 0) {
            g_level_block->text_content_region = 0x56;
        } else if (g_level_block->text_content_region == 0x56) {
            g_level_block->text_content_region = 0x57;
        }

        switch (g_level_block->text_content_region) {
        case 0x57:
            DrawCatalogImage(-0xe, 0x86, 0, 0, g_level_block->text_box_right + 5, 0x16b, 2, 0);
            break;
        case 0x58:
            DrawCatalogImage(-0xe, 0x86, 0, 1, g_level_block->text_box_right + 5, 0x16b, 2, 0);
            break;
        case -1:
        case 0x56:
            DrawCatalogImage(-0xe, 0x86, 0, 3, g_level_block->text_box_right + 5, 0x16b, 2, 0);
            g_level_block->text_content_region = -1;
            break;
        }

        line_count = GetTextBoxLineCount(text_box);
        visible = GetTextBoxVisibleLineCount();
        if (scroll + static_cast<unsigned int>(visible) < line_count) {
            if (g_level_block->dialogue_content_region == 0x59) {
                g_level_block->dialogue_content_region = 0x5a;
            }
        } else {
            g_level_block->dialogue_content_region = 0x59;
        }

        switch (g_level_block->dialogue_content_region) {
        case 0x5a:
            DrawCatalogImage(-0xe, 0x86, 0, 8, g_level_block->text_box_right + 5, 0x1ad, 2, 0);
            break;
        case 0x5b:
            DrawCatalogImage(-0xe, 0x86, 0, 9, g_level_block->text_box_right + 5, 0x1ad, 2, 0);
            break;
        case -1:
        case 0x59:
            DrawCatalogImage(-0xe, 0x86, 0, 0xb, g_level_block->text_box_right + 5, 0x1ad, 2, 0);
            g_level_block->dialogue_content_region = -1;
            break;
        }

        if (line_count > static_cast<unsigned int>(visible)) {
            thumb_y = static_cast<int>(scroll * 0x28 / (line_count - visible)) + 0x17b;
            DrawCatalogImage(-0xe, 0x86, 0, 4, g_level_block->text_box_right + 5, thumb_y, 2, 0);
        }
    }

    InvalidateRegion(g_level_block->text_box_right + 5, 0x16b, g_level_block->text_box_right + 0x1e,
                     0x1c1, 0);
    RedrawTextBoxBody(0);
    if (g_level_block->dialogue_text_input_open != 0 && g_level_block->dialogue_text_input != 0) {
        g_level_block->dialogue_text_input->dirty = 1;
    }
    SetClippingRect(&saved_clip);
}

/* Invalidate the text and action panels around a status-panel text refresh. */
// FUNCTION: WIZ8 0x0058a8c0
void RedrawTextBoxComplete(void)
{
    W8MainGameScreen* screen = g_main_game_screen;

    screen->m_text_panel_00c->Invalidate(0);
    screen->m_status_panel_010->RefreshStatusTexts();
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
char TextBoxHandleKey(const InputAtom* event)
{
    W8MainGameTextPanel* panel = g_main_game_screen->m_text_panel_00c;
    int before = panel->m_selection_078;
    char handled;

    handled = panel->m_key_handler_074->HandleKey(event->usParam);

    if (handled != 0 && panel->m_selection_078 != before) {
        ResetButtonSoundScheme();
        PlayButtonSound(3);
    }
    return handled;
}

/* Record what the Knock Knock spell is aimed at. Casting it anywhere the
   overlay is not up says so and records nothing - the message is the
   function's own name in the player's words. Its caller passes the same
   (level, flag, backfire) triple CastSpellAtLockInteraction00587C80 takes;
   only the target is read here. */
/* The trap-mode half of CastSpellAtLockInteraction00587C80: without a
   backfire the disarm chance is `level * 5 + 0x32 - m_field_038 * 6`, a
   success parks the screen in state 7 (8 on a miss), and either way the text
   and action panels go quiet for a 1.5 second timer. */
// FUNCTION: WIZ8 0x0058A930
void AttemptTrapDisarm0058A930(int level, int /*flag*/, char backfire)
{
    W8MainGameScreen* screen = g_main_game_screen;
    int chance;

    if (backfire != '\0') {
        chance = 0;
    } else {
        chance = level * 5 + 0x32 + screen->m_field_038 * -6;
    }
    if (static_cast<int>(Random(100)) < chance) {
        screen->m_state_018 = 7;
    } else {
        screen->m_state_018 = 8;
    }
    screen->m_text_panel_00c->EnableRegionSet(0);
    screen->m_text_panel_00c->m_key_handler_074->m_range_038.EnableRegionSet(0);
    screen->m_action_panel_014->EnableRegionSet(0);
    screen->m_timer_154.SetDuration(1.5f);
    screen->m_timer_154.Restart();
}

// FUNCTION: WIZ8 0x0058a9c0
void SetKnockKnockTarget(int target, int /*flag*/, int /*backfire*/)
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

// FUNCTION: WIZ8 0x0058F6B0
void SelectTextBox(short text_box)
{
    g_status_685170.text_line_cursor_1795 = text_box;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
    if (g_level_block->text_lines[text_box] != 0) {
        g_level_block->text_content_region = 0x57;
    } else {
        g_level_block->text_content_region = 0x56;
    }

    bool can_scroll;
    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0 ||
        g_level_block->dialogue_text_input->text_box != text_box) {
        can_scroll = g_level_block->text_lines[text_box] + GetTextBoxVisibleLineCount() <
                     g_status_685170.text_box_lines_shown_49a7[text_box];
    } else {
        can_scroll = g_level_block->text_lines[text_box] + GetTextBoxVisibleLineCount() <
                     g_level_block->dialogue_text_input->line_count +
                         g_status_685170.text_box_lines_shown_49a7[text_box];
    }
    if (can_scroll) {
        g_level_block->dialogue_content_region = 0x5a;
        return;
    }
    if (g_level_block->dialogue_text_input_open == 0 || g_level_block->dialogue_text_input == 0 ||
        g_level_block->dialogue_text_input->text_box != text_box) {
        can_scroll = g_level_block->text_lines[text_box] + GetTextBoxVisibleLineCount() <
                     g_status_685170.text_box_lines_shown_49a7[text_box];
    } else {
        can_scroll = g_level_block->text_lines[text_box] + GetTextBoxVisibleLineCount() <
                     g_level_block->dialogue_text_input->line_count +
                         g_status_685170.text_box_lines_shown_49a7[text_box];
    }
    if (!can_scroll) {
        g_level_block->dialogue_content_region = 0x59;
    }
}

// FUNCTION: WIZ8 0x0058F8E0
void SelectTextSlot1D8(int line, int index)
{
    W8MessageStorageRecord* record;
    unsigned int first;

    if (line < 0x15e) {
        first = g_level_block->text_lines[g_status_685170.text_line_cursor_1795];
        if (first <= static_cast<unsigned int>(line) &&
            static_cast<unsigned int>(line) < first + 7) {
            record = &g_message_storage_68f2d8[index][line];
            while (record->link_10 != 0 && line != 0) {
                --line;
                --record;
            }
            g_level_block->text_slots_1d8[index] = line;
            RequestRedraw(W8_REDRAW_TEXT_BOX);
        }
    }
}

// FUNCTION: WIZ8 0x00590D90
void ClearTextLineEntry00590D90(int index)
{
    g_level_block->text_lines[index] = 0;
}

// FUNCTION: WIZ8 0x0058F990
int GetTextSlot1D8(int index)
{
    return g_level_block->text_slots_1d8[index];
}

// FUNCTION: WIZ8 0x0058F9B0
void SelectTextSlot1E8(int line, int index)
{
    W8MessageStorageRecord* record;
    unsigned int first;

    if (line < 0x15e) {
        first = g_level_block->text_lines[g_status_685170.text_line_cursor_1795];
        if (first <= static_cast<unsigned int>(line) &&
            static_cast<unsigned int>(line) < first + 7) {
            record = &g_message_storage_68f2d8[index][line];
            while (record->link_10 != 0 && line != 0) {
                --line;
                --record;
            }
            g_level_block->text_slots_1e8[index] = line;
            RequestRedraw(W8_REDRAW_TEXT_BOX);
        }
    }
}

// FUNCTION: WIZ8 0x0058FFC0
void DrawNoticeWordOverlays(W8MessageStorageRecord* line, int x, int y)
{
    wchar_t word_text[100];

    if (line->entries_18 == 0) {
        return;
    }
    // reinterpret-ok: retail counts the pointer-list through the IList API
    unsigned int count = PLLength(line->entries_18);
    for (int i = 0; i < static_cast<int>(count); ++i) {
        W8NoticeWord* word = static_cast<W8NoticeWord*>(PLGet(line->entries_18, i));
        if (word->flag_08 != 0) {
            unsigned short* palette = word->flag_08 == 2 ? g_font_state_palettes_68ee1c[3]
                                                         : g_font_state_palettes_68ee1c[5];
            SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
            memset(word_text, 0, sizeof(word_text));
            wcsncpy(word_text, line->wString + word->start, word->end - word->start + 1);
            gprintfDirty(word->x_start + x, y, Wiz8ToSgpWideText(g_format_s_006068e4),
                         Wiz8ToSgpWideText(word_text));
            word->flag_09 = 0;
        }
        if (word->flag_09 != 0) {
            unsigned short* palette;
            if (line->font_palette < 0xf) {
                palette = g_font_state_palettes_68ee1c[line->font_palette];
            } else {
                palette = g_level_block->palette_2ec;
            }
            SetFontObjectPalette16BPP(g_level_block->text_box_font, palette);
            memset(word_text, 0, sizeof(word_text));
            wcsncpy(word_text, line->wString + word->start, word->end - word->start + 1);
            gprintfDirty(word->x_start + x, y, Wiz8ToSgpWideText(g_format_s_006068e4),
                         Wiz8ToSgpWideText(word_text));
            word->flag_09 = 0;
        }
    }
}

// FUNCTION: WIZ8 0x00590150
void ResetUsedNoticeWords(int text_box, unsigned char redraw)
{
    for (int i = 0; i < 0x15e; ++i) {
        W8PList* list = g_message_storage_68f2d8[text_box][i].entries_18;
        if (list != 0) {
            // reinterpret-ok: retail counts the pointer-list through the IList API
            unsigned int count = PLLength(list);
            for (int j = 0; j < static_cast<int>(count); ++j) {
                W8NoticeWord* word = static_cast<W8NoticeWord*>(PLGet(list, j));
                if (word->flag_08 == 2) {
                    word->flag_08 = 0;
                    word->flag_09 = 1;
                }
            }
        }
    }
    if (redraw != 0) {
        RedrawTextBoxBody(1);
    }
}

// FUNCTION: WIZ8 0x005901D0
void ClearNoticeWordHover(int text_box, unsigned char redraw)
{
    for (int i = 0; i < 0x15e; ++i) {
        W8PList* list = g_message_storage_68f2d8[text_box][i].entries_18;
        if (list != 0) {
            // reinterpret-ok: retail counts the pointer-list through the IList API
            unsigned int count = PLLength(list);
            for (int j = 0; j < static_cast<int>(count); ++j) {
                W8NoticeWord* word = static_cast<W8NoticeWord*>(PLGet(list, j));
                if (word->flag_08 != 2) {
                    word->flag_08 = 0;
                    word->flag_09 = 1;
                }
            }
        }
    }
    if (redraw != 0) {
        RedrawTextBoxBody(1);
    }
}

// FUNCTION: WIZ8 0x00590250
void HighlightNoticeWordAt(int text_box, unsigned short x, unsigned short y)
{
    int line;
    int x_base;
    int offset;
    W8PList* list;
    unsigned int count;

    for (int i = 0; i < 0x15e; ++i) {
        list = g_message_storage_68f2d8[text_box][i].entries_18;
        if (list != 0) {
            // reinterpret-ok: retail counts the pointer-list through the IList API
            count = PLLength(list);
            for (int j = 0; j < static_cast<int>(count); ++j) {
                W8NoticeWord* word = static_cast<W8NoticeWord*>(PLGet(list, j));
                if (word->flag_08 != 2) {
                    word->flag_08 = 0;
                    word->flag_09 = 1;
                }
            }
        }
    }
    if (g_level_block->text_box_top <= y && y <= g_level_block->text_box_bottom) {
        line = g_level_block->text_lines[text_box] + (y - g_level_block->text_box_top) / 0xb;
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[text_box])) {
            if (g_status_685170.text_line_cursor_1795 == 3 || g_level_block->flag_271 == 0) {
                x_base = g_level_block->text_box_left;
            } else {
                x_base = g_level_block->text_box_left +
                         ((g_level_block->text_box_right - g_level_block->text_box_left) / 2 -
                          StringPixLength(
                              Wiz8ToSgpWideText(g_message_storage_68f2d8[text_box][line].wString),
                              g_level_block->text_box_font) /
                              2);
            }
            list = g_message_storage_68f2d8[text_box][line].entries_18;
            // reinterpret-ok: retail counts the pointer-list through the IList API
            count = PLLength(list);
            offset = x - x_base;
            for (int i = 0; i < static_cast<int>(count); ++i) {
                W8NoticeWord* word = static_cast<W8NoticeWord*>(PLGet(list, i));
                if (word->x_start <= offset && offset <= word->x_end && word->flag_08 != 2) {
                    word->flag_08 = 1;
                }
            }
        }
        RedrawTextBoxBody(1);
    }
}

// FUNCTION: WIZ8 0x00590410
W8NoticeWord* HitTestNoticeWord(int text_box, unsigned short x, unsigned short y, int* line_out)
{
    int line;
    int x_base;
    int offset;
    W8PList* list;
    unsigned int count;
    W8NoticeWord* word;

    if (g_level_block->text_box_top <= y && y <= g_level_block->text_box_bottom) {
        line = g_level_block->text_lines[text_box] + (y - g_level_block->text_box_top) / 0xb;
        if (line < static_cast<int>(g_status_685170.text_box_lines_shown_49a7[text_box])) {
            if (g_status_685170.text_line_cursor_1795 == 3 || g_level_block->flag_271 == 0) {
                x_base = g_level_block->text_box_left;
            } else {
                x_base = g_level_block->text_box_left +
                         ((g_level_block->text_box_right - g_level_block->text_box_left) / 2 -
                          StringPixLength(
                              Wiz8ToSgpWideText(g_message_storage_68f2d8[text_box][line].wString),
                              g_level_block->text_box_font) /
                              2);
            }
            list = g_message_storage_68f2d8[text_box][line].entries_18;
            // reinterpret-ok: retail counts the pointer-list through the IList API
            count = PLLength(list);
            offset = x - x_base;
            for (int i = 0; i < static_cast<int>(count); ++i) {
                word = static_cast<W8NoticeWord*>(PLGet(list, i));
                if (word->x_start <= offset && offset <= word->x_end) {
                    *line_out = line;
                    return word;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00590560
void CopyNoticeWordText(const W8NoticeWord* word, wchar_t* text, unsigned int size, int text_box,
                        int line)
{
    memset(text, 0, size);
    wcsncpy(text, g_message_storage_68f2d8[text_box][line].wString + word->start,
            word->end - word->start + 1);
}

// FUNCTION: WIZ8 0x00590b40
void PostMonsterNotice(W8MonsterInfo* monster_info, const wchar_t* format, ...)
{
    wchar_t separator[2];
    wchar_t text[4096];
    va_list arguments;
    va_start(arguments, format);
    vswprintf(text, format, arguments);
    va_end(arguments);

    wcscpy(separator, text[0] == L'\'' || text[0] == L':' ? &g_wchar_00689b34 : L" ");
    ShowNoticef(9, L"%s%s%s", GetMonsterName(monster_info, 0, 0), separator, text);
}

/* Re-show the last wrapped entry of `mode`'s message run as a notice line;
   0xffff derives the mode from the live dialogue/camp/combat state. Retail
   only fills `merged` when the run index range is non-empty, so a failing
   (but returning) srAssertFail can leave it uninitialized at ShowNoticeLine;
   preserved intentionally. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
// FUNCTION: WIZ8 0x00590BD0
void RefreshTextBoxMode00590BD0(unsigned short mode)
{
    wchar_t merged[500];
    W8MessageStorageRecord* line;
    unsigned int last_line;
    unsigned int group_lines;
    unsigned int first_line;
    unsigned int index;
    size_t length;

    if (g_current_screen_state.id != W8_SCREEN_MAIN_GAME &&
        g_current_screen_state.id != W8_SCREEN_CAMP) {
        return;
    }
    if (mode == 0xffff) {
        if ((gXStatus.fNpcDialogueMode == 0 || CanOpenNpcDialogue()) && gXStatus.fCampMode == 0) {
            if (GetFlag68F105() == 0) {
                mode = gXStatus.fCombatMode != 0;
            } else {
                mode = 0;
            }
        } else {
            mode = IsNpcDialogueTextBoxActive() ? 0 : 2;
        }
    }
    int text_box = static_cast<short>(mode);
    if (g_status_685170.text_box_lines_used_4997[text_box] == 0) {
        srAssertFail("gStatus.uiTextBoxLinesUsed[iTextBuffer] > 0", MGS_TEXT_BOX_CPP, 0x109e, 0);
    }
    last_line = g_status_685170.text_box_lines_used_4997[text_box] - 1;
    group_lines = g_message_storage_68f2d8[text_box][last_line].link_10;
    if (last_line < group_lines) {
        srAssertFail("uiLastLineIndex >= uiGroupLine", MGS_TEXT_BOX_CPP, 0x10a6, 0);
    }
    first_line = last_line - group_lines;
    line = &g_message_storage_68f2d8[text_box][first_line];
    length = 0;
    for (index = first_line; index <= last_line; index++, line++) {
        if (line->wString == 0) {
            srAssertFail("pTextLine->wString != NULL", MGS_TEXT_BOX_CPP, 0x10ad, 0);
        }
        if (index == first_line) {
            wcscpy(merged, line->wString);
            length = wcslen(line->wString);
        } else {
            if (wcslen(line->wString) + 1 + length > 499) {
                break;
            }
            wcscat(merged, g_W8TextSeparator0060CC74);
            wcscat(merged, line->wString);
            length += 1 + wcslen(line->wString);
        }
    }
    ShowNoticeLine(merged, 0, 1, 0);
}
#pragma clang diagnostic pop
