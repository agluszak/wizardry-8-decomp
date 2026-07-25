#ifndef WIZ8_FORMATS_GAMEPLAY_DATABASES_H
#define WIZ8_FORMATS_GAMEPLAY_DATABASES_H

#include <stdint.h>

#pragma pack(push, 1)

typedef uint16_t W8WideChar;

typedef struct W8RecordDatabaseHeader {
    uint32_t record_count;              /* 0x00 */
} W8RecordDatabaseHeader;               /* 0x04 */

typedef struct W8VersionedRecordDatabaseHeader {
    uint32_t record_count;              /* 0x00 */
    uint32_t version;                   /* 0x04 */
} W8VersionedRecordDatabaseHeader;      /* 0x08 */

typedef struct W8Dice {
    int16_t base;                       /* 0x00 */
    uint8_t count;                      /* 0x02 */
    uint8_t sides;                      /* 0x03 */
} W8Dice;                               /* 0x04 */

typedef struct W8ItemInstance {
    int32_t item_id;                    /* 0x00: -1 is empty */
    uint8_t stack_count;                /* 0x04: quantity-kind 1 */
    uint8_t uses_or_charges;            /* 0x05: quantity-kinds 2 through 4 */
    uint8_t identified;                 /* 0x06 */
    uint8_t unknown_07[4];              /* 0x07 */
    uint8_t unknown_0b;                 /* 0x0b */
} W8ItemInstance;                       /* 0x0c */

typedef struct W8ItemDatabaseRecord {
    W8WideChar display_name[30];         /* 0x000 */
    uint8_t unknown_03c[3];             /* 0x03c */
    uint16_t unidentified_name_index;   /* 0x03f */
    uint8_t flags_041;                  /* 0x041: bit zero starts identified */
    uint8_t unknown_042[0x24];          /* 0x042 */
    uint8_t quantity_kind;              /* 0x066: zero none, one stack, two-four uses */
    W8Dice initial_quantity;             /* 0x067 */
    uint8_t unknown_06b[0x4e];          /* 0x06b */
    int32_t combine_ingredient_a;       /* 0x0b9 */
    int32_t combine_ingredient_b;       /* 0x0bd */
    uint8_t unknown_0c1[8];             /* 0x0c1 */
    uint8_t combine_skill;              /* 0x0c9: 0xff means no skill check */
    uint8_t combine_minimum_skill;      /* 0x0ca */
    uint8_t unknown_0cb[0x42];          /* 0x0cb */
} W8ItemDatabaseRecord;                 /* 0x10d */

typedef struct W8MonsterDatabaseRecord {
    W8WideChar name_00[24];             /* 0x000: suffix after '#' removed at load */
    W8WideChar name_30[24];             /* 0x030: suffix after '#' removed at load */
    W8WideChar name_60[24];             /* 0x060: suffix after '#' removed at load */
    W8WideChar name_90[24];             /* 0x090: suffix after '#' removed at load */
    uint8_t fields_0c0[0x1d7];          /* 0x0c0: not yet field-reconciled */
} W8MonsterDatabaseRecord;              /* 0x297 */

typedef struct W8LevelDatabaseRecord {
    W8WideChar display_name[30];         /* 0x00 */
    uint8_t fields_03c[0x9c];           /* 0x3c: not yet field-reconciled */
} W8LevelDatabaseRecord;                /* 0xd8 */

typedef struct W8FactDatabaseRecord {
    uint32_t identifier;                /* 0x000: first record contains zero */
    char symbolic_name[256];            /* 0x004: FACT_* identifier */
    uint8_t fields_104[0xd4];           /* 0x104: not yet field-reconciled */
} W8FactDatabaseRecord;                 /* 0x1d8 */

typedef struct W8SpellRuntimeRecord {
    uint8_t fields[0x1bf];              /* runtime portion retained by Spells.cpp */
} W8SpellRuntimeRecord;                 /* 0x1bf */

typedef struct W8SpellDiskRecord {
    uint8_t ignored_prefix[0x101];       /* explicitly skipped by InitializeSpellDatabase */
    W8SpellRuntimeRecord runtime;        /* 0x101 */
} W8SpellDiskRecord;                    /* 0x2c0 */

#pragma pack(pop)

#endif
