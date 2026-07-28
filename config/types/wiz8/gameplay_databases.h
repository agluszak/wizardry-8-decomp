#ifndef WIZ8_FORMATS_GAMEPLAY_DATABASES_H
#define WIZ8_FORMATS_GAMEPLAY_DATABASES_H

/* Packed records compiled by recovered source have one editable home. This
   catalogue retains only formats without a compiled-source counterpart. */
#include "../../../include/wiz8/layouts/gameplay_databases.h"
#include "../../../include/wiz8/layouts/item_tables.h"

#pragma pack(push, 1)

typedef struct W8RecordDatabaseHeader {
    unsigned int record_count;
} W8RecordDatabaseHeader;               /* 0x04 */

typedef struct W8VersionedRecordDatabaseHeader {
    unsigned int record_count;
    unsigned int version;
} W8VersionedRecordDatabaseHeader;      /* 0x08 */

typedef struct W8AttributeMinimums {
    int values[7];
} W8AttributeMinimums;                  /* 0x1c */

typedef struct W8ProfessionAbilities {
    int ability_ids[3];
} W8ProfessionAbilities;                /* 0x0c */

typedef struct W8RaceAbilities {
    int ability_ids[5];
} W8RaceAbilities;                      /* 0x14 */

typedef struct W8SkillAttributes {
    int category;
    int unknown_04;
    int unknown_08;
    int unknown_0c;
} W8SkillAttributes;                    /* 0x10 */

typedef struct W8ProfessionSkills {
    int skill_ids[4];
} W8ProfessionSkills;                   /* 0x10 */

typedef struct W8StartingEquipment {
    int item_ids[6];
} W8StartingEquipment;                  /* 0x18 */

typedef struct W8SpellDiskRecord {
    unsigned char ignored_prefix[0x101];
    W8SpellRuntimeRecord runtime;
} W8SpellDiskRecord;                    /* 0x2c0 */

#pragma pack(pop)

#endif
