#ifndef WIZ8_GAME_STATE_H
#define WIZ8_GAME_STATE_H

/*
 * The global runtime status block and the settings and party rows inside it.
 *
 * This is gXStatus and its neighbours: state the running game mutates, as
 * opposed to the records it loads from disk, which live in gameplay_databases.h.
 * The two are separated because they are different kinds of fact - a disk record
 * is a file format that cannot change, while these are a layout the engine
 * happens to have.
 */

#include "wiz8/layouts/gameplay_databases.h"

#pragma pack(push, 1)

/* The game settings block that 0x0054B560 clears and fills with defaults. Every
   address that function writes falls inside the 0xa4 bytes it clears first, so
   this is one structure rather than a run of separate globals. Only offsets are
   established - nothing here names a setting - so the fields keep positional
   names. */
typedef struct W8GameSettings {
    unsigned char field_000;             /* 0x000 */
    unsigned char field_001;             /* 0x001 */
    unsigned char unknown_002[0x4];
    int field_006;                       /* 0x006 */
    unsigned char field_00a;             /* 0x00a */
    unsigned char field_00b;             /* 0x00b */
    unsigned char field_00c;             /* 0x00c */
    int field_00d;                       /* 0x00d */
    int field_011;                       /* 0x011 */
    int field_015;                       /* 0x015 */
    int field_019;                       /* 0x019 */
    int field_01d;                       /* 0x01d */
    int field_021;                       /* 0x021 */
    int field_025;                       /* 0x025 */
    unsigned char field_029;             /* 0x029 */
    unsigned char field_02a;             /* 0x02a */
    unsigned char field_02b;             /* 0x02b */
    unsigned char field_02c;             /* 0x02c */
    unsigned char unknown_02d[0x1];
    unsigned char field_02e;             /* 0x02e */
    unsigned char field_02f;             /* 0x02f */
    unsigned char field_030;             /* 0x030 */
    unsigned char field_031;             /* 0x031 */
    unsigned char field_032;             /* 0x032 */
    unsigned char field_033;             /* 0x033 */
    unsigned char field_034;             /* 0x034 */
    unsigned char field_035;             /* 0x035 */
    unsigned char field_036;             /* 0x036 */
    int field_037;                       /* 0x037 */
    unsigned char field_03b;             /* 0x03b */
    int field_03c;                       /* 0x03c */
    unsigned char field_040;             /* 0x040 */
    unsigned char field_041;             /* 0x041 */
    unsigned char field_042;             /* 0x042 */
    unsigned char field_043;             /* 0x043 */
    unsigned char unknown_044[0x1];
    unsigned char field_045;             /* 0x045 */
    unsigned char unknown_046[0x1];
    unsigned char field_047;             /* 0x047 */
    unsigned char field_048;             /* 0x048 */
    unsigned char field_049;             /* 0x049 */
    unsigned char field_04a;             /* 0x04a */
    unsigned char field_04b;             /* 0x04b */
    unsigned char field_04c;             /* 0x04c */
    unsigned char field_04d;             /* 0x04d */
    unsigned char field_04e;             /* 0x04e */
    unsigned char field_04f;             /* 0x04f */
    unsigned char field_050;             /* 0x050 */
    unsigned char unknown_051[0x53];
} W8GameSettings;                        /* 0x0a4 */


/* The two heap buffers a status block owns. GetSaveGameLevel builds one of
   these on the stack, reads through it and tears it down again; only the two
   pointers this pair manages are established, not the block's full extent. */
typedef struct W8StatusBuffers {
    unsigned char unknown_00[4];
    void* buffer_04;                     /* 0x04: 0xc310 bytes */
    void* buffer_08;                     /* 0x08: 0x830 bytes */
} W8StatusBuffers;

/* The global status block. It opens with the same two heap buffers
   AllocateStatusBuffers manages on a caller-supplied one, and 0x0054AF30 clears
   the whole block - pointers included - before allocating fresh ones. */
typedef struct W8GlobalStatus {
    W8StatusBuffers buffers;             /* 0x0000 */
    unsigned char unknown_000c[0x11];
    int active_party_slot_001d;           /* 0x001d */
    unsigned char unknown_0021[0x18df];
    int saved_level;                     /* 0x1900 */
    unsigned char unknown_1904[0x30be];
} W8GlobalStatus;                        /* 0x49c2 */

/* Defined by GameplayDatabase.cpp, which carries the address marker. Declared
   here beside its type so consumers share one declaration instead of repeating
   a local extern each time. */
extern W8GlobalStatus g_status_685170;

typedef struct W8LevelRuntimeBlock {
    unsigned char unknown_000[0xf4];
    /* Everything that changes the screen ORs its redraw bit here. */
    unsigned int redraw_flags;           /* 0x0f4 */
    unsigned char unknown_0f8[8];
    /* Modes one and two use GDCamera's tighter lower framing margin. */
    int camera_mode_100;                 /* 0x100 */
    /* The region currently under the pointer. */
    unsigned int hover_region;           /* 0x104 */
    unsigned char unknown_108[0x4c];
    /* Raised when the party changes its selected monster. */
    unsigned char pick_changed_154;
    /* The final combat regions are only removed while this is clear. */
    unsigned char flag_155;
    unsigned char unknown_156[0x16];
    /* Character whose highlight overrides all others, or -1. */
    int highlight_override;              /* 0x16c */
    unsigned char unknown_170[0x38];
    /* One entry per visible main-game message line. */
    int text_lines[12];                  /* 0x1a8 */
    /* Two four-entry tables the text box clears to -1 by slot. */
    int text_slots_1d8[4];
    int text_slots_1e8[4];
    /* Whether dialogue is open and, when it is, its owner. */
    unsigned char dialogue_open;         /* 0x1f8 */
    unsigned char unknown_1f9[3];
    unsigned char* dialogue_owner;       /* 0x1fc */
    unsigned char unknown_200[0x64];
    /* Item under the pointer and item selected by the interface. */
    int highlighted_item;                /* 0x264 */
    int selected_item;                   /* 0x268 */
    unsigned char unknown_26c[0x10];
    /* Level and entry point for a queued transition. */
    int pending_level;                   /* 0x27c */
    int pending_entry_id;                /* 0x280 */
    unsigned char unknown_284[0x3c];
    /* Redraw requests for the combat and party panels. */
    unsigned char refresh_combat_panel;  /* 0x2c0 */
    unsigned char unknown_2c1[7];
    unsigned char refresh_party_panel;   /* 0x2c8 */
    unsigned char unknown_2c9;
    short combat_end_notification;       /* 0x2ca */
    /* Text-box scroll extent. */
    int scroll_top;                      /* 0x2cc */
    unsigned char unknown_2d0[4];
    int scroll_bottom;                   /* 0x2d4 */
    unsigned char unknown_2d8[4];
    /* Movement budgets restored to one hundred with party control. */
    int move_budget_2dc;
    int move_budget_2e0;
    unsigned char unknown_2e4[4];
    int value_2e8;
    unsigned char unknown_2ec[4];
    /* Current interface selection and whether it has settled. */
    int selection_kind;                  /* 0x2f0 */
    unsigned char unknown_2f4[4];
    unsigned char selection_settled;     /* 0x2f8 */
    unsigned char unknown_2f9[3];
    /* Hover-tooltip timing, state, subject and kind. */
    unsigned int tooltip_since;          /* 0x2fc */
    unsigned char tooltip_pending;       /* 0x300 */
    unsigned char unknown_301[3];
    int tooltip_subject;                 /* 0x304 */
    int tooltip_kind;                    /* 0x308 */
} W8LevelRuntimeBlock;

#ifdef __cplusplus
static_assert(sizeof(W8LevelRuntimeBlock) == 0x30c,
              "W8LevelRuntimeBlock_must_be_0x30c");
#endif

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

extern W8LevelRuntimeBlock* g_level_block; /* 0x0068EDCC */
extern int g_current_level;                /* 0x00686A70 */

#ifdef __cplusplus
}
#endif

#endif
