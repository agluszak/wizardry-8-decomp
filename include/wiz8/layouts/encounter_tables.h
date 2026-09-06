#ifndef WIZ8_LAYOUTS_ENCOUNTER_TABLES_H
#define WIZ8_LAYOUTS_ENCOUNTER_TABLES_H

#include "wiz8/vector.h"

#pragma pack(push, 1)

typedef enum W8EncounterTimeCondition {
    W8_ENCOUNTER_DAY = 0,
    W8_ENCOUNTER_NIGHT = 1,
    W8_ENCOUNTER_ANY_TIME = 2
} W8EncounterTimeCondition;

typedef struct W8EncounterTableDiskHeader {
    unsigned char record_kind;          /* 0x000: four in the reviewed corpus */
    char name[256];                     /* 0x001 */
    unsigned int unknown_101;           /* 0x101 */
    unsigned short version;             /* 0x105: two in the reviewed corpus */
    unsigned char entry_count;          /* 0x107 */
} W8EncounterTableDiskHeader;            /* 0x108 */

typedef struct W8EncounterScriptName {
    char value[64];
} W8EncounterScriptName;                /* 0x40 */

typedef struct W8EncounterTableRuntime {
    ~W8EncounterTableRuntime();

    W8GrowableVector<unsigned short> species_ids; /* 0x000 */
    W8GrowableVector<unsigned char> rarity_class; /* 0x010: 3, 7, 20, or 70 */
    W8GrowableVector<unsigned char> time_condition; /* 0x020 */
    W8GrowableVector<unsigned char> challenge_level; /* 0x030: 1 through 50 */
    W8GrowableVector<W8EncounterScriptName*> script_names; /* 0x040 */
    char name[256];                     /* 0x050 */
    unsigned int unknown_150;           /* 0x150 */
    unsigned char version_two_flags;    /* 0x154 */
    unsigned char padding_155[3];       /* 0x155 */
} W8EncounterTableRuntime;              /* 0x158 */

static_assert(sizeof(W8EncounterTableRuntime) == 0x158,
              "W8EncounterTableRuntime_size_must_be_0x158");

#pragma pack(pop)

#endif
