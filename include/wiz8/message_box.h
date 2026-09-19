#pragma once

#include <stddef.h>

struct W8NpcState;
struct W8NpcQuoteEntry;
struct W8ItemInstance;

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

/* Queued NPC message kinds dispatched by ProcessMessageBoxQueue at 0x00526E90.
   W8_NPC_MSG_QUOTE lines are built by QueueNpcScriptLine; the command kinds are
   built by QueueNpcMessageLine/AddMessageBoxLine, and QUOTE_ENTRY continuations
   are built inline by the dispatcher itself. Kinds 0x11 and 0x3b have no
   dispatch case; 0x3b is still queued and acts as a queue spacer. */
enum W8NpcMessageKind {
    W8_NPC_MSG_QUOTE = 0,                /* run script quote `quote_index` */
    W8_NPC_MSG_CLOSE_DIALOGUE = 1,       /* close dialogue; set g_flag_6109f0 */
    W8_NPC_MSG_QUOTE_ENTRY = 2,          /* quote_entry + continuation_quote */
    W8_NPC_MSG_REOPEN_TRANSCRIPT = 3,    /* CloseNpcDialogueLayout00570A20 + transcript layout */
    W8_NPC_MSG_REMOVE_SCRIPT_ITEM = 4,   /* item: W8ItemInstance* to unscript */
    W8_NPC_MSG_CLOSE_RESUME_NPC = 5,     /* close dialogue; ResumeNpc(npc, 1) */
    W8_NPC_MSG_FOCUS_NPC = 6,            /* argument: npc kind to switch dialogue to */
    W8_NPC_MSG_GROUP_ACTION = 7,         /* argument: NPC party slot to dismiss */
    W8_NPC_MSG_JOURNAL_QUOTE = 8,        /* quote bubble + journal-entry.wav */
    W8_NPC_MSG_PORTRAIT_STRING = 9,      /* argument: gppStringList index */
    W8_NPC_MSG_CALL_4DFAE0 = 0x0a,       /* SpawnAlfieChaos004DFAE0(text) */
    W8_NPC_MSG_CALL_4DFB40 = 0x0b,       /* SpawnAlfieLife004DFB40 */
    W8_NPC_MSG_CALL_4DFB80 = 0x0c,       /* SpawnAlfieKnow004DFB80 */
    W8_NPC_MSG_FINISH_ACTION = 0x0d,     /* text null: clear targets, else scripted action */
    W8_NPC_MSG_PATH2_TRIGGER = 0x0e,     /* run trigger "Path2Trigger" */
    W8_NPC_MSG_MOVE_SAVANT = 0x0f,       /* MoveSavant.msf on monster group 0xc2 */
    W8_NPC_MSG_MOVE_BELA = 0x10,         /* MoveBela.msf + NP_DSExit teleport */
    W8_NPC_MSG_MOVE_GOLEM = 0x12,        /* MoveGolem.msf on monster group 0xb8 */
    W8_NPC_MSG_ALETHEIDES_LEAVES = 0x13, /* RemoveAletheides() */
    W8_NPC_MSG_SKILL_NOTICES = 0x14,     /* text + extra: W8SkillNoticePayload* */
    W8_NPC_MSG_CALL_HENCHMAN = 0x15,     /* monster group 0x112 cycle 0x12 + callback */
    W8_NPC_MSG_HENCHMAN_LEAVES = 0x16,   /* monster group 0xdc cycle 0x12 + callback */
    W8_NPC_MSG_PORTRAIT_EXTRA = 0x17,    /* text + extra for quote bubble */
    W8_NPC_MSG_PORTRAIT_MESSAGE = 0x18,  /* text: owned wide string for quote bubble */
    W8_NPC_MSG_LEVEL_UP = 0x19,          /* text + extra: int party slot; GainLevel.wav */
    W8_NPC_MSG_PILLARGATE_LURE = 0x1a,   /* pillargate05 + fade Al-Lure (npc 0x3f) */
    W8_NPC_MSG_PILLARGATE_MADEUS = 0x1b, /* pillargate04 + fade Al-Madeus (npc 0x3e) */
    W8_NPC_MSG_PILLARGATE_ASAIZ = 0x1c,  /* pillargate01 + fade Al-Asaiz (npc 0x3d) */
    W8_NPC_MSG_RESET_LEVEL_STATE = 0x1d, /* text null: ClearLevelDataFlag6, else reset vectors */
    W8_NPC_MSG_SET_CONDITION_13 = 0x1e,  /* argument: party slot; condition 0x13, 9999 */
    W8_NPC_MSG_SHOW_DIALOGUE_PANEL = 0x1f,
    W8_NPC_MSG_PRINCE_DISAPPEARS = 0x20,   /* fade group 0x1ab; hostile group 0x15d */
    W8_NPC_MSG_REMOVE_SELF = 0x21,         /* fade the speaking NPC's monster */
    W8_NPC_MSG_REMOVE_JANETTE = 0x22,      /* npc kind 0x62 */
    W8_NPC_MSG_REMOVE_MARTEN = 0x23,       /* npc kind 0x64 */
    W8_NPC_MSG_MOVE_GARI = 0x24,           /* MoveGari.msf on monster group 0x162 */
    W8_NPC_MSG_MILANO_RAT_DOOR = 0x25,     /* RatDoor02 + Milano.msf on group 0xcf */
    W8_NPC_MSG_REMOVE_SHAMAN = 0x26,       /* npc kind 0x4d */
    W8_NPC_MSG_PARTY_SPEAKER_EVENT = 0x27, /* argument: event type for a random speaker */
    W8_NPC_MSG_PARTY_MEMBER_EVENT = 0x28,  /* argument: party slot; event g_effect_005ee58c */
    W8_NPC_MSG_MOVE_RUBBLE = 0x29,         /* MoveRubble.msf on monster group 0x83 */
    W8_NPC_MSG_SEDEXUS_LEAVES = 0x2a,      /* LezboDemonAppeared + fade npc 0x40 */
    W8_NPC_MSG_TRIGGER_FIX = 0x2b,         /* run trigger "triggerFix" */
    W8_NPC_MSG_REMOVE_SEDEXUS_RIFT = 0x2c, /* npc kind 0x42 */
    W8_NPC_MSG_BALBRAK_HOME = 0x2d,        /* teleport npc 0x0c to NP_Balbrakhome */
    W8_NPC_MSG_MOOK_COMMENT = 0x2e,        /* race-10 party member event */
    W8_NPC_MSG_SAVANT_HACK = 0x2f,         /* monster group 0x1b6 cycle 0x19 + callback */
    W8_NPC_MSG_MOVE_TO_BOOK = 0x30,        /* belapath1.msf on group 0x1b4 + triggerplanes */
    W8_NPC_MSG_MOVE_TO_BOOK2 = 0x31,       /* CC_TRIGGERPLANE3 + notice npc 0x8d */
    W8_NPC_MSG_SAVANT_APPEARS = 0x32,      /* monster 0x234 at NP_DS1 + notice npc 0x84 */
    W8_NPC_MSG_REMOVE_RPC_VI = 0x33,       /* spawn 0x1b9 at NP_VI1 when Vi (0x18) leads */
    W8_NPC_MSG_PHOONZANG_SPLIT = 0x34,     /* cycle npc 0x84 + NP_PHOONZANGLEE + npc 0x8d */
    W8_NPC_MSG_BEGIN_ENDGAME = 0x35,
    W8_NPC_MSG_CLEAR_NPC_COMBAT = 0x36, /* argument: party slot */
    W8_NPC_MSG_DISPATCH_PENDING_NOTICE = 0x37,
    W8_NPC_MSG_TRAVEL_CONFIRM = 0x38, /* argument: level id for the confirm dialog */
    W8_NPC_MSG_PRINCE_NOT_HOME = 0x39,
    W8_NPC_MSG_PARTY_SLOT_EVENT_18 = 0x3a,  /* argument: party slot; event 0x18 */
    W8_NPC_MSG_SPACER = 0x3b,               /* queued separator; no dispatch case */
    W8_NPC_MSG_PHOONZANG_NOTICE = 0x3c,     /* notice npc kind 0x87 */
    W8_NPC_MSG_TURN_TO_BOOK = 0x3d,         /* BeginScreenFade + NpcScriptTurnToBook */
    W8_NPC_MSG_REMOVE_ALETHEIDES_AD = 0x3e, /* npc kind 0x2d */
    W8_NPC_MSG_REMOVE_ALETHEIDES_CM = 0x3f, /* npc kind 0x2e */
    W8_NPC_MSG_REMOVE_ALETHEIDES_DD = 0x40, /* npc kind 0x2f */
    W8_NPC_MSG_SEDEXUS_PASSOUT = 0x41,      /* ResolveSedexusCapture() */
    W8_NPC_MSG_ENDGAME_SCREEN = 0x42,       /* BeginScreenFade + NpcScriptEndgameScreen */
};

/* Queue record consumed by ProcessMessageBoxQueue. Field usage is per `type`:
   QUOTE lines use quote_index/mark_pending/suppress_entries; QUOTE_ENTRY
   continuations use quote_entry/continuation_quote; command kinds carry either
   an owned wide string in `text` or a tagged int/pointer in `argument` (same
   dword). `quote_entry` is only live for QUOTE_ENTRY and is always a
   W8NpcQuoteEntry*. */
struct W8MessageBoxLine {
    int quote_index;            /* 0x00: QUOTE script quote; -1 otherwise */
    unsigned char mark_pending; /* 0x04: QUOTE records quote_index pending */
    /* 0x08: QUOTE_ENTRY only; producers always store a W8NpcQuoteEntry*. */
    W8NpcQuoteEntry* quote_entry;
    W8NpcMessageKind type; /* 0x0c */
    /* 0x10: per-kind payload sharing one dword:
       - wchar_t* text: PORTRAIT_*, SKILL_NOTICES, LEVEL_UP (owned; delete[]),
         FINISH_ACTION/RESET_LEVEL_STATE (null vs non-null flag)
       - W8ItemInstance* item: REMOVE_SCRIPT_ITEM
       - int argument: QueueNpcMessageLine tags (npc kind, group/party slot,
         string index, event type, travel level id) and PARTY_SPEAKER_EVENT */
    union {
        wchar_t* text;
        W8ItemInstance* item;
        int argument;
    };
    int continuation_quote;         /* 0x14: QUOTE_ENTRY's owning quote index */
    unsigned char suppress_entries; /* 0x18: QUOTE skips the quote-entry scan */
    void* extra;                    /* 0x1c: caller payload; category-dependent, not one type */
    W8NpcState* npc;                /* 0x20: speaking NPC, copied from g_npc_scripting.npc */
};

static_assert(sizeof(W8MessageBoxLine) == 0x24, "W8MessageBoxLine_must_be_0x24");
static_assert(offsetof(W8MessageBoxLine, quote_entry) == 0x08, "W8MessageBoxLine_quote_entry");
static_assert(offsetof(W8MessageBoxLine, type) == 0x0c, "W8MessageBoxLine_type");
static_assert(offsetof(W8MessageBoxLine, text) == 0x10, "W8MessageBoxLine_text");
static_assert(offsetof(W8MessageBoxLine, argument) == 0x10, "W8MessageBoxLine_argument");
static_assert(offsetof(W8MessageBoxLine, continuation_quote) == 0x14,
              "W8MessageBoxLine_continuation_quote");
static_assert(offsetof(W8MessageBoxLine, suppress_entries) == 0x18,
              "W8MessageBoxLine_suppress_entries");
static_assert(offsetof(W8MessageBoxLine, extra) == 0x1c, "W8MessageBoxLine_extra");
static_assert(offsetof(W8MessageBoxLine, npc) == 0x20, "W8MessageBoxLine_npc");

extern W8MessageBoxLine** g_message_box_lines;
extern int g_message_box_line_count;
extern int g_message_box_line_capacity;
void AddMessageBoxLine(W8NpcMessageKind kind, wchar_t* text, void* extra);
bool IsMessageBoxLineQueueEmpty(void);
