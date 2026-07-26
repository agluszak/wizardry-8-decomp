#ifndef WIZ8_FORMATS_ITEM_TABLES_H
#define WIZ8_FORMATS_ITEM_TABLES_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct W8ItemTableEntry {
    int16_t selector_00;                /* 0x00: zero disables the slot */
    uint16_t item_id;                   /* 0x02: index into Items.dbs */
    uint8_t weight;                     /* 0x04: weighted random selection */
} W8ItemTableEntry;                     /* 0x05 */

typedef struct W8ItemTableRecord {
    char name[256];                     /* 0x000 */
    uint32_t category_id;               /* 0x100: index into category names */
    W8ItemTableEntry entries[40];        /* 0x104 */
    uint8_t level_scaled;               /* 0x1cc: filters by item level */
    uint8_t unknown_1cd[0x24];          /* 0x1cd */
} W8ItemTableRecord;                    /* 0x1f1 */

#pragma pack(pop)

#endif
