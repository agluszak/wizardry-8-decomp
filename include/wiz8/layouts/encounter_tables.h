#ifndef WIZ8_LAYOUTS_ENCOUNTER_TABLES_H
#define WIZ8_LAYOUTS_ENCOUNTER_TABLES_H

#pragma pack(push, 1)

enum W8EncounterTimeCondition {
    W8_ENCOUNTER_DAY = 0,
    W8_ENCOUNTER_NIGHT = 1,
    W8_ENCOUNTER_ANY_TIME = 2
};

struct W8EncounterTableDiskHeader {
    unsigned char record_kind; /* 0x000: four in the reviewed corpus */
    char name[256];            /* 0x001 */
    unsigned int unknown_101;  /* 0x101 */
    unsigned short version;    /* 0x105: two in the reviewed corpus */
    unsigned char entry_count; /* 0x107 */
}; /* 0x108 */

struct W8EncounterScriptName {
    char value[64];
}; /* 0x40 */

#pragma pack(pop)

#endif
