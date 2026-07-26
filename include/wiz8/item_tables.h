#ifndef WIZ8_ITEM_TABLES_H
#define WIZ8_ITEM_TABLES_H

#include "wiz8/vector.h"

struct W8WorldItem;

#pragma pack(push, 1)

struct W8ItemTableEntry {
    short selector_00;                    /* 0x00: zero disables the slot */
    unsigned short item_id;               /* 0x02: index into Items.dbs */
    unsigned char weight;                 /* 0x04 */
};                                       /* 0x05 */

struct W8ItemTableRecord {
    char name[256];                       /* 0x000 */
    unsigned int category_id;             /* 0x100 */
    W8ItemTableEntry entries[40];         /* 0x104 */
    unsigned char level_scaled;           /* 0x1cc */
    unsigned char unknown_1cd[0x24];      /* 0x1cd */
};                                       /* 0x1f1 */

struct W8ItemDatabaseRecord {
    unsigned char unknown_000[0x86];
    unsigned int unknown_086;             /* 0x086: level-scaled table filter */
    unsigned char unknown_08a[0x83];
};                                       /* 0x10d */

#pragma pack(pop)

extern "C" {

unsigned int GetAveragePartyLevel(void); /* 0x004EF420 */
int FindItemTableByName(const char* name);
int GenerateItemsFromTable(
    W8GrowableVector<W8WorldItem*>* output_items,
    unsigned int table_id,
    unsigned int maximum_items);          /* 0x004F88F0 */

}

#endif
