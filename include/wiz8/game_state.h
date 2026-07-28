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

#include "wiz8/gameplay_databases.h"

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

/* One party slot row. Only the three fields the reset touches are established,
   plus the leading flag UtilityFunctions reads as slot-occupied. */
typedef struct W8PartySlotRow {
    unsigned char flag_00;               /* 0x000: slot occupied */
    /* 0x001: the action this slot has chosen this round; -1 is none, and the
       round reset lifts the ninth action specifically. */
    int pending_action;
    /* 0x005: the attack mode chosen per hand, indexed by the combat state's
       current hand. The bound is a partition of the unknown run, not proven. */
    int attack_mode[4];                  /* 0x005 */
    unsigned char unknown_015[8];
    /* 0x01d and 0x04d: the two targeting blocks the slot carries, cleared
       together when the character dies. */
    unsigned char target_block_01d[0x30];
    unsigned char combat_slot_04d[0x20];  /* 0x04d, a W8CombatSlot */
    /* 0x06d and 0x071: what the slot is doing and the detail that qualifies
       it, the character counterpart of the monster's 0x2e1 and 0x2e5. */
    int action_kind;
    int action_detail;
    /* 0x075..0x0a0: the slot's pending spell target - what kind of target it
       is, which one, a cleared word, and the eight-dword target block the
       targeting code hands over. */
    int spell_target_kind;               /* 0x075 */
    int spell_target_value;              /* 0x079 */
    int spell_target_reset;              /* 0x07d */
    int spell_target_block[8];           /* 0x081 */
    unsigned char unknown_0a1[0x2f];
    unsigned char flag_0d0;              /* 0x0d0: reset to 0xff */
    unsigned char unknown_0d1[0x24];
    /* 0x0f5: set while the slot is out of action. The party-wide sweeps skip a
       slot that has it raised, and the targeting guard reads the same byte. */
    unsigned char flag_0f5;
    unsigned char unknown_0f6[4];
    /* 0x0fa: the animation this slot is driving, -1 when none. Death tells it
       to stop. */
    int animation_0fa;
    unsigned char unknown_0fe[6];
    /* 0x104: the slot's action is the first kind, cached beside it. */
    unsigned char action_is_kind_one;
    unsigned char flag_105;              /* 0x105 */
} W8PartySlotRow;                        /* 0x106 */

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
    unsigned char unknown_000c[0x49b6];
} W8GlobalStatus;                        /* 0x49c2 */

#pragma pack(pop)

#endif
