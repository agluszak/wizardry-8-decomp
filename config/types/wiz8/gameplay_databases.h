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

typedef struct W8ItemDatabaseRecord {
    W8WideChar display_name[30];         /* 0x000 */
    uint8_t fields_03c[0xd1];           /* 0x03c: not yet field-reconciled */
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
