#pragma once

#include "wiz8/text_types.h"

struct W8MessageBoxLine {
    int unknown_00;                       /* initialized to -1 */
    int unknown_04;
    int unknown_08;
    int type;                             /* 0x0c: message category */
    W8WideChar* text;                     /* 0x10 */
    int unknown_14;
    int unknown_18;
    void* extra;                          /* 0x1c: caller-owned payload */
    int sequence;                         /* 0x20: running global counter */
};

static_assert(sizeof(W8MessageBoxLine) == 0x24,
              "W8MessageBoxLine_must_be_0x24");

extern W8MessageBoxLine** g_message_box_lines;
extern int g_message_box_line_count;
extern int g_message_box_line_capacity;
extern int g_message_sequence;
void AddMessageBoxLine(int type, W8WideChar* text, void* extra);
