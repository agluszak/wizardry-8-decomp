#pragma once


struct W8NpcState;

struct W8MessageBoxLine {
    int unknown_00;   /* often -1; some appenders store a typed id here */
    int unknown_04;   /* type-dependent byte/word payload */
    int unknown_08;   /* type-dependent payload */
    int type;         /* 0x0c: message category */
    wchar_t* text; /* 0x10 */
    int unknown_14;   /* type-dependent payload */
    int unknown_18;   /* type-dependent byte payload */
    void* extra;      /* 0x1c: caller payload; category-dependent, not one type */
    W8NpcState* npc;  /* 0x20: speaking NPC, copied from g_npc_state_68c4ac */
};

static_assert(sizeof(W8MessageBoxLine) == 0x24, "W8MessageBoxLine_must_be_0x24");

extern W8MessageBoxLine** g_message_box_lines;
extern int g_message_box_line_count;
extern int g_message_box_line_capacity;
void AddMessageBoxLine(int type, wchar_t* text, void* extra);
bool IsMessageBoxLineQueueEmpty(void);
