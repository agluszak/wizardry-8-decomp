#pragma once

class Trigger;

#include <wchar.h>
#include "input.h"
#include "timer.h"
#include "wiz8/fonts.h"
#include "wiz8/dialog_code/DialogBase.h"

#include "wiz8/3d_code/PList.h"

struct W8MonsterInfo;
struct W8Region;

enum W8NoticeWordState {
    W8_NOTICE_WORD_NORMAL = 0,
    W8_NOTICE_WORD_HOVERED = 1,
    W8_NOTICE_WORD_SELECTED = 2
};

struct W8NoticeWord {
    short start;
    short end;
    short x_start;
    short x_end;
    unsigned char keyword; /* 0 none, 1 keyword, 2 selected */
    bool redraw;           /* repaint once after deselection */
};
static_assert(sizeof(W8NoticeWord) == 10, "W8NoticeWord_must_be_10");

void AdvanceNoticeLine(short text_box);

struct W8MessageStorageRecord {
    void ClearEntries();
    void RebuildEntries();

    wchar_t* wString;
    unsigned char font_palette;
    /* The recoloured span [start, stop) of this line. */
    unsigned char highlight_color;
    unsigned char highlight_start;
    unsigned char highlight_stop;
    TIMER clock;
    /* SaveGame snapshots the unsigned milliseconds remaining from
       ClockIsTicking; load rearms the countdown with this duration. */
    UINT32 saved_remaining_ms;
    /* Continuation link count of a wrapped entry; -1 when unlinked. */
    int link;
    int length; /* wString length, -1 when unset */
    /* Live list pointer; save preserves its 32-bit representation and
       load discards the serialized word instead of reconstructing a pointer. */
    W8PList* entries;
    unsigned char unknown_1c[8];
};

static_assert(sizeof(W8MessageStorageRecord) == 0x24, "W8MessageStorageRecord_must_be_0x24");

extern W8MessageStorageRecord g_message_storage[4][0x15e];

/* Reset one editor status line; -1 selects the current line. */
void ResetEditorStatusLine(short line);
/* The font currently bound to the main text box. */
int GetTextBoxFont(void);
/* Repaint dirty dialogue text-input lines and the caret. */
void RedrawDialogueTextInput(void);
/* Clear one text_lines slot on the level block. */
void ClearTextLineEntry(int index);
void FormatNotice(int channel, short text_box, const wchar_t* format, ...);

void ReleaseMessageStorage(void);
/* The TEXT section pair - the four message-storage
   runs persisted around the game-status record. */
unsigned char SaveTextBoxState(unsigned int file);
unsigned char HandleDialogueTextInput(const InputAtom* input);
char TextBoxHandleKey(const InputAtom* event);
int GetSelectedTextLine(int index);
void ClearSelectedTextLine(int index);
/* Hover and selection track one wrapped-line start per box. Set operations
   snap a visible wrapped index back to its entry's first line. */
int GetHoveredTextLine(int index);
void ClearHoveredTextLine(int index);
void SetHoveredTextLine(int line, int box);
void SetSelectedTextLine(int line, int box);
/* Walk every notice-word list of a box, clearing the
   flag_08 mark and raising flag_09; the first touches only selected (2)
   words, the second everything else. Nonzero redraw repaints the body
   through RedrawTextBoxBody(1). */
void ClearNoticeWordHover(int box, bool redraw);
void ResetUsedNoticeWords(int text_box, bool redraw);
/* Refresh the hover mark on the notice word under (x, y). */
void HighlightNoticeWordAt(int box, unsigned short x, unsigned short y);
/* The notice word under (x, y) in box, or 0; the word's line
   slot is written through line_out. */
W8NoticeWord* HitTestNoticeWord(int text_box, unsigned short x, unsigned short y, int* line_out);
/* Copy a word's text span out of its source line. */
void CopyNoticeWordText(const W8NoticeWord* word, wchar_t* out, unsigned int capacity, int box,
                        int start);
void ScrollTextBoxToCursor(void);
unsigned char GetTextBoxMode(void);
void SetTextBoxMode(unsigned char mode, int value);
/* Re-derive the text-box mode from the live screen state; 0xffff
   asks for the automatic choice. */
void RefreshTextBoxMode(unsigned short mode);
/* The lock-interaction call convention - its caller passes the
   same (level, flag, backfire) triple CastSpellAtLockInteraction
   takes; the body reads the target only. */
void SetKnockKnockTarget(int target, int flag, int backfire);
/* The trap-mode half of CastSpellAtLockInteraction;
   same (level, flag, backfire) triple. */
void AttemptTrapDisarm(int level, int flag, char backfire);

/* Merge text onto a box's last used line, re-posting the combined
   line so wrapping, highlighting and the link counts rebuild; -1 picks the box
   the current game mode writes to. */
void AppendToLastTextLine(const wchar_t* text, short text_box);
/* Wrapped line count of the notice ShowNotice last displayed;
   only maintained while game_status.quote_audit is raised. */
extern int g_notice_line_count;
/* The number of lines the notice pane can scroll. */
int GetTextBoxScrollRange(void);
/* The two variadic notice formatters. */
void PostCharacterNotice(int party_slot, const wchar_t* format, ...);
void PostMonsterNotice(W8MonsterInfo* monster_info, const wchar_t* format, ...);
void ScrollTextBoxTo(int line);
void ScrollDialogueTextBoxToLine(void);
void ScrollTextBoxUp(int lines);
void ScrollTextBoxDown(int lines);
int GetTextBoxVisibleLineCount(void);

/* Recolour the character span [start, stop) of the most recent
   line in one text box; -1 picks the box the current game mode writes to. */
void HighlightTextBoxRange(unsigned char color, unsigned char start, unsigned char stop,
                           short text_box);

void SelectTextBox(short value);
/* Ask for the text box to be redrawn without changing anything. */
void RedrawTextBox(void);
/* Repaint visible text-box lines; skip_invalidate nonzero skips
   InvalidateRegion of the text rectangle. */
void RedrawTextBoxBody(bool skip_invalidate);
/* Redraw text-box scroll up/down buttons and thumb, then body. */
void RedrawTextBoxScrollChrome(void);
/* Append text to a box's current line; the box argument is optional. */
void AppendTextBoxLine(const wchar_t* text, ...);

bool CurrentTextLineHasContent(void);
bool CurrentDialogueLineHasContent(void);
int FindStoppedTextLine(void);
void SetTextBoxRegionBounds(int left, int top, int right, int bottom);
void ResetMessageStorage(void);
/* Write the four message runs into the open TEXT chunk. */
unsigned char SaveMessageStorage(int file);
/* Rebuild the four message runs from the open TEXT chunk. */
unsigned char LoadMessageStorage(int file);
/* Defaults select the live mode's text box and its available line width. */
const short W8_NOTICE_TEXT_BOX_AUTOMATIC = -1;
const unsigned int W8_NOTICE_WRAP_AUTOMATIC = ~0U;

void ShowNotice(unsigned int font_palette, const wchar_t* text,
                short text_box = W8_NOTICE_TEXT_BOX_AUTOMATIC,
                unsigned int wrap_width = W8_NOTICE_WRAP_AUTOMATIC, bool force_dialog = false);
/* Vswprintf the format into a scratch buffer and ShowNotice it,
   choosing the text-box slot from the current dialogue/camp/combat mode. */
void ShowNoticef(unsigned int font_palette, const wchar_t* format, ...);

void HighlightNoticeRow(int line);
