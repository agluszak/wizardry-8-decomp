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
    unsigned char flag_08;
    unsigned char flag_09;
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
    unsigned char unknown_0c[4];
    /* 0x10: continuation link count of a wrapped entry; -1 when unlinked. */
    int link_10;
    int value_14;
    W8PList* entries_18;
    unsigned char unknown_1c[8];
};

static_assert(sizeof(W8MessageStorageRecord) == 0x24, "W8MessageStorageRecord_must_be_0x24");

extern W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];

/* 0x0058AA20: reset one editor status line; -1 selects the current line. */
void ResetEditorStatusLine0058AA20(short line);
void WriteGameLogAmount(int color, const wchar_t* format, ...);
void FormatNotice(int channel, short text_box, const wchar_t* format, ...); /* 0x0058AB60 */

void ReleaseMessageStorage(void);
/* 0x0058D7E0-0x0058E010: the dormant typed-dialogue editing helpers are
   translation-unit local; MGSTextBox.cpp declares them static. */
unsigned char HandleDialogueTextInput(const InputAtom* input); /* 0x0058F250 */
int GetTextSlot1E8(int index);                                 /* 0x0058FA60 */
void ClearTextSlot1E8(int index);                              /* 0x0058FA30 */
void ScrollTextBoxToCursor(void);
unsigned char GetTextBoxMode(void);
void SetTextBoxMode(unsigned char mode, int value);

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
void ScrollTextBoxUp(int lines);                                                 /* 0x0058BF00 */
void ScrollTextBoxDown(int lines);                                               /* 0x0058C060 */
int GetTextBoxVisibleLineCount(void);                                            /* 0x00590900 */

/* 0x0058B410: recolour the character span [start, stop) of the most recent
   line in one text box; -1 picks the box the current game mode writes to. */
void HighlightTextBoxRange(unsigned char color, unsigned char start, unsigned char stop,
                           short text_box);

void Function58F6B0(int value); /* 0x0058F6B0 */
void Function58CC10(void);      /* 0x0058CC10 */
/* 0x0058B300: append text to a box's current line. Retail callers disagree on
   arity - the box argument is optional. */
void AppendTextBoxLine0058B300(const wchar_t* text, ...);

bool CurrentTextLineHasContent(void);                                  /* 0x0058B940 */
bool CurrentDialogueLineHasContent(void);                              /* 0x0058B960 */
int FindStoppedTextLine(void);                                         /* 0x0058D760 */
void SetTextBoxRegionBounds(int left, int top, int right, int bottom); /* 0x0058FA90 */
void ResetMessageStorage(void);                                        /* 0x0058FEE0 */
void ShowNotice(unsigned int font_palette, const wchar_t* text, short text_box = -1,
                unsigned int wrap_width = ~0U, bool force_dialog = false);
/* 0x0058AAD0: vswprintf the format into a scratch buffer and ShowNotice it,
   choosing the text-box slot from the current dialogue/camp/combat mode. */
void ShowNoticef(unsigned int font_palette, const wchar_t* format, ...);
