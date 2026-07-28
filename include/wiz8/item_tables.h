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
    unsigned char unknown_000[0x3e];
    /* 0x03e: the equipment class, zero through twelve. GetItemDefaultEquipSlot
       is a thirteen-way switch on it and is the only body that enumerates the
       whole domain; class four additionally prices and stacks by the bundle. */
    unsigned char equip_class;
    /* 0x03f: groups items that share one generic name while unidentified.
       GetItemDisplayRecord builds the shared name lazily, one cached string per
       index, and two items with equal indices read as the same thing. */
    unsigned short unidentified_name_index;
    /* 0x041: bit one blocks discarding the item, which is the only bit a
       recovered body reads. */
    unsigned char flags_041;
    /* 0x042: the item's kind. Three selects the spell-source items the magic
       code accepts; no other value is established. */
    unsigned char category;
    unsigned char unknown_043[0x23];
    /* 0x066: zero none, one stack, two through four uses or charges. */
    unsigned char quantity_kind;
    unsigned char unknown_067[0x1f];
    /* 0x086: the gold value of one item, except for equip class four, which is
       priced by the bundle of twenty-five. GenerateItemsFromTable filters a
       level-scaled table on the same field. */
    unsigned int value;
    unsigned short weight;                /* 0x08a: the weight of one item */
    unsigned char unknown_08c[0x81];
};                                       /* 0x10d */

#pragma pack(pop)

unsigned int GetAveragePartyLevel(void); /* 0x004EF420 */
int FindItemTableByName(const char* name);
int GenerateItemsFromTable(
    W8GrowableVector<W8WorldItem*>* output_items,
    unsigned int table_id,
    unsigned int maximum_items);          /* 0x004F88F0 */

#endif
