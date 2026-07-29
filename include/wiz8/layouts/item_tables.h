#ifndef WIZ8_LAYOUTS_ITEM_TABLES_H
#define WIZ8_LAYOUTS_ITEM_TABLES_H

#include "wiz8/layouts/gameplay_databases.h"

#pragma pack(push, 1)

typedef struct W8ItemTableEntry {
    short selector_00;                    /* 0x00: zero disables the slot */
    unsigned short item_id;               /* 0x02: index into Items.dbs */
    unsigned char weight;                 /* 0x04 */
} W8ItemTableEntry;                       /* 0x05 */

typedef struct W8ItemTableRecord {
    char name[256];                       /* 0x000 */
    unsigned int category_id;             /* 0x100 */
    W8ItemTableEntry entries[40];         /* 0x104 */
    unsigned char level_scaled;           /* 0x1cc */
    unsigned char unknown_1cd[0x24];      /* 0x1cd */
} W8ItemTableRecord;                      /* 0x1f1 */

/* One "you need this much of that" entry. CanCharacterUseItem walks two of
   these for attributes and two for skills, stopping at an id of 0xff. */
typedef struct W8ItemRequirement {
    unsigned char stat_id;                /* 0xff when the entry is unused */
    unsigned char minimum;
} W8ItemRequirement;                      /* 0x02 */

typedef struct W8ItemDatabaseRecord {
    W8WideChar display_name[30];          /* 0x000 */
    unsigned char unknown_03c[2];
    unsigned char equip_class;            /* 0x03e: zero through twelve */
    unsigned short unidentified_name_index; /* 0x03f */
    unsigned char flags_041;              /* 0x041 */
    unsigned char category;               /* 0x042: three is a spell source */
    unsigned char unknown_043[3];
    unsigned char weapon_skill;           /* 0x046 */
    unsigned char wield_group;            /* 0x047 */
    unsigned char unknown_048[0x1b];
    unsigned char spell_id;               /* 0x063 */
    unsigned char unknown_064[2];
    unsigned char quantity_kind;          /* 0x066 */
    W8Dice initial_quantity;              /* 0x067 */
    unsigned char unknown_06b[0xb];
    unsigned short profession_mask;       /* 0x076 */
    unsigned int race_mask;               /* 0x078 */
    unsigned char faction_mask;           /* 0x07c */
    W8ItemRequirement attribute_requirements[2]; /* 0x07d */
    W8ItemRequirement skill_requirements[2]; /* 0x081 */
    unsigned char identify_difficulty;    /* 0x085 */
    unsigned int value;                   /* 0x086 */
    unsigned short weight;                /* 0x08a */
    unsigned char binds_on_equip;         /* 0x08c */
    char internal_name[0x40];             /* 0x08d */
    unsigned char unknown_0cd[0x40];
} W8ItemDatabaseRecord;                   /* 0x10d */

#ifndef __WIZ8_GHIDRA_LAYOUTS__
static_assert(sizeof(W8ItemDatabaseRecord) == 0x10d, "W8ItemDatabaseRecord_size_must_be_0x10d");
static_assert(sizeof(W8ItemTableRecord) == 0x1f1, "W8ItemTableRecord_size_must_be_0x1f1");
#endif

#pragma pack(pop)

#endif
