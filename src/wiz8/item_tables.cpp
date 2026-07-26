#include "wiz8/gameplay_boundaries.h"

#include <string.h>

// FUNCTION: WIZ8 0x004F88A0
int FindItemTableByName(const char* name)
{
    int index;

    for (index = 0; index < (int)g_item_table_count; ++index) {
        if (_stricmp(name, g_item_tables[index]->name) == 0) {
            return index;
        }
    }
    return -1;
}
