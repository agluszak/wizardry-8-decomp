#ifndef WIZ8_ITEM_TABLES_H
#define WIZ8_ITEM_TABLES_H

#include "wiz8/layouts/item_tables.h"
#include "wiz8/vector.h"

struct W8WorldItem;

unsigned int GetAveragePartyLevel(void); /* 0x004EF420 */
int FindItemTableByName(const char* name);
int GenerateItemsFromTable(
    W8GrowableVector<W8WorldItem*>* output_items,
    unsigned int table_id,
    unsigned int maximum_items);          /* 0x004F88F0 */

#endif
