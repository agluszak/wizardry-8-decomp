#pragma once

class Trigger;

#include <wchar.h>

#include "wiz8/3d_code/PList.h"

struct W8MonsterInfo;

/* Local Screens\MGSTextBox.cpp owns the four message runs at 0x0068F2D8. */
struct W8MessageStorageRecord {
    void* allocation_00;
    unsigned char unknown_04[4];
    int clock_08;
    unsigned char unknown_0c[0x0c];
    W8PList* entries_18;
    unsigned char unknown_1c[8];
};

static_assert(sizeof(W8MessageStorageRecord) == 0x24, "W8MessageStorageRecord_must_be_0x24");

extern W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];

void Function58FD30(void);
void ScrollTextBoxToCursor(void);
unsigned char GetTextBoxMode(void);
void SetTextBoxMode(unsigned char mode, int value);

void Function5905F0(const wchar_t* text, int mode); /* 0x005905F0 */
/* 0x0058FB30: the number of lines the notice pane can scroll. */
int GetTextBoxScrollRange(void);
/* 0x00590950/0x00590B40: the two variadic notice formatters. The binary
   builds a va_list in each and passes the format on. */
void PostCharacterNotice(int party_slot, const wchar_t* format, ...);            /* 0x00590950 */
void PostMonsterNotice(W8MonsterInfo* monster_info, const wchar_t* format, ...); /* 0x00590B40 */
void ScrollTextBoxTo(int line);                                                  /* 0x0058BBC0 */

void Function58F6B0(int value); /* 0x0058F6B0 */

bool CurrentTextLineHasContent(void);                                  /* 0x0058B940 */
bool CurrentDialogueLineHasContent(void);                              /* 0x0058B960 */
int FindStoppedTextLine(void);                                         /* 0x0058D760 */
void SetTextBoxRegionBounds(int left, int top, int right, int bottom); /* 0x0058FA90 */
void ResetMessageStorage(void);                                        /* 0x0058FEE0 */

extern int g_text_line_cursor_00686905;
