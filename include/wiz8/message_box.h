#pragma once

struct W8NpcState;

/* Heap payloads shared by the NPC message queue and the quote-bubble owner.
   The experience appender at 0x004eef10 allocates 8 bytes; the skill appenders
   at 0x00553f10/0x005542e0 allocate 0x11 bytes with count/slot/skill columns. */
struct W8ExperienceNoticePayload {
    unsigned int amount;
    unsigned char alternate_message;
    unsigned char pad_05[3];
};

struct W8SkillNoticePayload {
    signed char count;
    signed char party_slots[8];
    signed char skills[8];
};

static_assert(sizeof(W8ExperienceNoticePayload) == 8, "W8ExperienceNoticePayload_size");
static_assert(sizeof(W8SkillNoticePayload) == 0x11, "W8SkillNoticePayload_size");

struct W8MessageBoxLine {
    int unknown_00;           /* often -1; some appenders store a typed id here */
    unsigned char unknown_04; /* type-dependent byte payload */
    int unknown_08;           /* type-dependent payload */
    int type;                 /* 0x0c: message category */
    wchar_t* text;            /* 0x10 */
    int unknown_14;           /* type-dependent payload */
    unsigned char unknown_18; /* type-dependent byte payload */
    void* extra;              /* 0x1c: caller payload; category-dependent, not one type */
    W8NpcState* npc;          /* 0x20: speaking NPC, copied from g_npc_scripting.npc */
};

static_assert(sizeof(W8MessageBoxLine) == 0x24, "W8MessageBoxLine_must_be_0x24");

extern W8MessageBoxLine** g_message_box_lines;
extern int g_message_box_line_count;
extern int g_message_box_line_capacity;
void AddMessageBoxLine(int type, wchar_t* text, void* extra);
bool IsMessageBoxLineQueueEmpty(void);
