#ifndef WIZ8_LAYOUTS_ENCOUNTER_TABLES_H
#define WIZ8_LAYOUTS_ENCOUNTER_TABLES_H

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

/* One instantiation of the shared growable-vector template over a byte
   element - the same vptr/count/capacity/data shape include/wiz8/vector.h
   models as W8GrowableVector<T> and docs/libraries/wiz8-foundation-types.md
   derives. The leading word is a vtable pointer, not data. */
typedef struct W8EncounterByteVector {
    void *vtable;                       /* 0x00 */
    int count;                          /* 0x04 */
    int capacity;                       /* 0x08 */
    unsigned char *values;              /* 0x0c */
} W8EncounterByteVector;                /* 0x10 */

typedef struct W8EncounterTableRuntime {
    /* 0x000-0x00f: an inline W8GrowableVector<uint16_t> instantiation. */
    void *vtable;                       /* 0x000 */
    int species_count;                  /* 0x004 */
    int species_capacity;               /* 0x008 */
    unsigned short *species_ids;        /* 0x00c */
    W8EncounterByteVector rarity_class; /* 0x010: values 3, 7, 20, or 70 */
    W8EncounterByteVector time_condition; /* 0x020: W8EncounterTimeCondition */
    W8EncounterByteVector challenge_level; /* 0x030: values 1 through 50 */
    unsigned char script_names_runtime[0x10]; /* 0x040: runtime string container */
    char name[256];                     /* 0x050 */
    unsigned int unknown_150;           /* 0x150 */
    unsigned char version_two_flags;    /* 0x154 */
    unsigned char padding_155[3];       /* 0x155 */
} W8EncounterTableRuntime;              /* 0x158 */

#ifndef __WIZ8_GHIDRA_LAYOUTS__
static_assert(sizeof(W8EncounterByteVector) == 0x10, "W8EncounterByteVector_size_must_be_0x10");
static_assert(sizeof(W8EncounterTableRuntime) == 0x158, "W8EncounterTableRuntime_size_must_be_0x158");
#endif

#pragma pack(pop)

#endif
