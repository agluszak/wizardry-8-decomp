#pragma once

class Trigger;

#include <wchar.h>
#include "input.h"
#include "wiz8/dialog_code/DialogBase.h"

#include "wiz8/3d_code/PList.h"

struct W8MonsterInfo;
struct W8Region;

struct W8NoticeWord {
    short start;
    short end;
    short x_start;
    short x_end;
    unsigned char keyword_08; /* 0x08: 0 none, 1 keyword, 2 selected */
    unsigned char redraw_09;  /* 0x09: repaint once after deselection */
};
static_assert(sizeof(W8NoticeWord) == 10, "W8NoticeWord_must_be_10");

void AdvanceNoticeLine(short text_box);

/* Local Screens\MGSTextBox.cpp owns the four message runs at 0x0068F2D8. */
struct W8MessageStorageRecord {
    wchar_t* wString; /* 0x00: the assertion at 0x0058B410 names it */
    unsigned char font_palette;
    /* 0x05-0x07: the recoloured span [start, stop) of this line. */
    unsigned char highlight_color;
    unsigned char highlight_start;
    unsigned char highlight_stop;
    int clock_08;
    /* 0x0c: SaveGame snapshots ClockIsTicking(clock_08) here so the TEXT
       chunk records whether the line's countdown was still running. */
    int clock_ticking_0c;
    /* 0x10: continuation link count of a wrapped entry; -1 when unlinked. */
    int link_10;
    int length_14; /* 0x14: wString length, -1 when unset */
    W8PList* entries_18;
    unsigned char unknown_1c[8];
};

static_assert(sizeof(W8MessageStorageRecord) == 0x24, "W8MessageStorageRecord_must_be_0x24");

/* The TEXT-chunk form of W8MessageStorageRecord: the same 0x24 bytes, except
   field 0x00 carries the wide-character count including the terminator where
   the live record keeps its wString pointer. Save copies the record and then
   writes the count over that slot; load reads the record, sizes the fresh
   wString buffer from it and drops the stale entries_18 pointer. */
struct W8MessageStorageDiskRecord {
    unsigned int character_count; /* 0x00: wchar count incl. terminator */
    unsigned char font_palette;
    unsigned char highlight_color;
    unsigned char highlight_start;
    unsigned char highlight_stop;
    int clock_08;
    int clock_ticking_0c;
    int link_10;
    int length_14;
    W8PList* entries_18;
    unsigned char unknown_1c[8];
};

static_assert(sizeof(W8MessageStorageDiskRecord) == 0x24,
              "W8MessageStorageDiskRecord_must_be_0x24");

extern W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];

/* 0x0058AA20: reset one editor status line; -1 selects the current line. */
void ResetEditorStatusLine0058AA20(short line);
/* 0x0058AA10: the font currently bound to the main text box. */
int GetTextBoxValue2E8(void);
/* 0x0058C790: repaint dirty dialogue text-input lines and the caret. */
void RedrawDialogueTextInput0058C790(void);
/* 0x00590D90: clear one text_lines slot on the level block. */
void ClearTextLineEntry00590D90(int index);
void FormatNotice(int channel, short text_box, const wchar_t* format, ...); /* 0x0058AB60 */

void ReleaseMessageStorage(void);
/* 0x0058FB50/0x0058FC30: the TEXT section pair - the four message-storage
   runs persisted around the game-status record. */
unsigned char SaveTextBoxState0058FB50(unsigned int file);
/* 0x0058D7E0-0x0058E010: the dormant typed-dialogue editing helpers are
   translation-unit local; MGSTextBox.cpp declares them static. */
unsigned char HandleDialogueTextInput(const InputAtom* input); /* 0x0058F250 */
char TextBoxHandleKey(const InputAtom* event);                 /* 0x0058A8F0 */
int GetTextSlot1E8(int index);                                 /* 0x0058FA60 */
void ClearTextSlot1E8(int index);                              /* 0x0058FA30 */
/* The 0x1d8/0x1e8 slot tables in W8LevelRuntimeBlock track one selected
   wrapped-line start per box; the Set variants snap a wrapped index back to
   its line start before storing. */
int GetTextSlot1D8(int index);             /* 0x0058F990 */
void ClearTextSlot1D8(int index);          /* 0x0058F960 */
void SelectTextSlot1D8(int line, int box); /* 0x0058F8E0 */
void SelectTextSlot1E8(int line, int box); /* 0x0058F9B0 */
/* 0x00590150/0x005901D0: walk every notice-word list of a box, clearing the
   flag_08 mark and raising flag_09; the first touches only selected (2)
   words, the second everything else. Nonzero redraw repaints the body
   through RedrawTextBoxBody(1). */
void ClearNoticeWordSelection(int box, int redraw);
void ClearNoticeWordHover(int box, unsigned char redraw);
/* 0x00590250: refresh the hover mark on the notice word under (x, y). */
void HighlightNoticeWordAt(int box, unsigned short x, unsigned short y);
/* 0x00590410: the notice word under (x, y) in box, or 0; the word's line
   slot is written through line_out. */
W8NoticeWord* FindNoticeWordAt(int box, int x, int y, int* line_out);
/* 0x00590560: copy a word's text span out of its source line. */
void CopyNoticeWordText(const W8NoticeWord* word, wchar_t* out, unsigned int capacity, int box,
                        int start);
void ScrollTextBoxToCursor(void);
unsigned char GetTextBoxMode(void);
void SetTextBoxMode(unsigned char mode, int value);
/* 0x00590BD0: re-derive the text-box mode from the live screen state; 0xffff
   asks for the automatic choice. */
void RefreshTextBoxMode00590BD0(unsigned short mode);
/* 0x0058A9C0: the lock-interaction call convention - its caller passes the
   same (level, flag, backfire) triple CastSpellAtLockInteraction00587C80
   takes; the body reads the target only. */
void SetKnockKnockTarget(int target, int flag, int backfire);
/* 0x0058A930: the trap-mode half of CastSpellAtLockInteraction00587C80;
   same (level, flag, backfire) triple. */
void AttemptTrapDisarm0058A930(int level, int flag, char backfire);

/* 0x005905F0: merge text onto a box's last used line, re-posting the combined
   line so wrapping, highlighting and the link counts rebuild; -1 picks the box
   the current game mode writes to. */
void AppendToLastTextLine(const wchar_t* text, int text_box);
/* 0x0069B7BC: wrapped line count of the notice ShowNotice last displayed;
   only maintained while game_status.quote_audit_2431 is raised. */
extern int g_notice_line_count_0069b7bc;
/* 0x0058FB30: the number of lines the notice pane can scroll. */
int GetTextBoxScrollRange(void);
/* 0x00590950/0x00590B40: the two variadic notice formatters. The binary
   builds a va_list in each and passes the format on. */
void PostCharacterNotice(int party_slot, const wchar_t* format, ...);            /* 0x00590950 */
void PostMonsterNotice(W8MonsterInfo* monster_info, const wchar_t* format, ...); /* 0x00590B40 */
void ScrollTextBoxTo(int line);                                                  /* 0x0058BBC0 */
void ScrollDialogueTextBoxToLine0058BA60(void);                                  /* 0x0058BA60 */
void ScrollTextBoxUp(int lines);                                                 /* 0x0058BF00 */
void ScrollTextBoxDown(int lines);                                               /* 0x0058C060 */
int GetTextBoxVisibleLineCount(void);                                            /* 0x00590900 */

/* 0x0058B410: recolour the character span [start, stop) of the most recent
   line in one text box; -1 picks the box the current game mode writes to. */
void HighlightTextBoxRange(unsigned char color, unsigned char start, unsigned char stop,
                           short text_box);

void SelectTextBox(short value); /* 0x0058F6B0 */
/* 0x0058AA00: ask for the text box to be redrawn without changing anything. */
void RedrawTextBox(void);
/* 0x0058C3A0: repaint visible text-box lines; skip_invalidate nonzero skips
   InvalidateRegion of the text rectangle. */
void RedrawTextBoxBody(unsigned char skip_invalidate);
/* 0x0058CC10: redraw text-box scroll up/down buttons and thumb, then body. */
void RedrawTextBoxScrollChrome(void);
/* 0x0058B300: append text to a box's current line. Retail callers disagree on
   arity - the box argument is optional. */
void AppendTextBoxLine0058B300(const wchar_t* text, ...);

bool CurrentTextLineHasContent(void);                                  /* 0x0058B940 */
bool CurrentDialogueLineHasContent(void);                              /* 0x0058B960 */
int FindStoppedTextLine(void);                                         /* 0x0058D760 */
void SetTextBoxRegionBounds(int left, int top, int right, int bottom); /* 0x0058FA90 */
void ResetMessageStorage(void);                                        /* 0x0058FEE0 */
/* 0x0058FB50: write the four message runs into the open TEXT chunk. */
unsigned char SaveMessageStorage0058FB50(int file);
/* 0x0058FC30: rebuild the four message runs from the open TEXT chunk. */
unsigned char LoadMessageStorage0058FC30(int file);
void ShowNotice(unsigned int font_palette, const wchar_t* text, short text_box = -1,
                unsigned int wrap_width = ~0U, bool force_dialog = false);
/* 0x0058AAD0: vswprintf the format into a scratch buffer and ShowNotice it,
   choosing the text-box slot from the current dialogue/camp/combat mode. */
void ShowNoticef(unsigned int font_palette, const wchar_t* format, ...);
