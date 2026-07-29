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
    unsigned char unknown_000c[0x18f4];
    int saved_level;                     /* 0x1900 */
    unsigned char unknown_1904[0x30be];
} W8GlobalStatus;                        /* 0x49c2 */

#pragma pack(pop)

struct W8LevelRuntimeBlock;

#ifdef __cplusplus
extern "C" {
#endif

extern W8LevelRuntimeBlock* g_level_block; /* 0x0068EDCC */
extern int g_current_level;                /* 0x00686A70 */

#ifdef __cplusplus
}
#endif

#endif
