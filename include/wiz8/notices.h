#pragma once

#include <wchar.h>

/* Entries in the localized notice table at 0x0068C09C. */
enum W8NoticeId {
    W8_NOTICE_MONSTER_SLAIN = 0x74c / 4,
    W8_NOTICE_CHARACTER_SAVE_FAILED = 0x780 / 4,
    W8_NOTICE_CHARACTER_LOAD_FAILED = 0x784 / 4,
    W8_NOTICE_COMBAT_CANNOT_END = 0x878 / 4,
    W8_NOTICE_COMBAT_CANNOT_END_ENGAGED = 0x87c / 4,
    W8_NOTICE_COMBAT_CANNOT_END_PENDING = 0x880 / 4,
    W8_NOTICE_COMBAT_ENDED = 0x884 / 4,
    W8_NOTICE_COMBAT_STANCE_RELAXED = 0x888 / 4,
    W8_NOTICE_COMBAT_STANCE_READY = 0x88c / 4
};

/* Local Screens\MGSTextBox.cpp's five-argument notice entry point at
   0x0058AC00. The short form is source-level default arguments: the retail
   AddPartyGold caller leaves -1, -1 and 0 from its preceding formatter call
   on the stack and supplies only the channel and formatted line afterward. */
void ShowNotice(
    int channel, const wchar_t* text, int a = -1, int b = -1, int c = 0);

/* 0x0055F260 dispatches one already-built line to the active screen. */
void ShowNoticeLine(const wchar_t* text, int a, int b, int c);
