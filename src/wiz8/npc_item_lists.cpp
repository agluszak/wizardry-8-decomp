#include "gameplay_boundaries.h"

// FUNCTION: WIZ8 0x0050B830
W8NPCItemList* GetNPCItemListByID(int npc_record_id)
{
    int count = g_npc_item_lists->count;
    W8PtrVector* item_lists = g_npc_item_lists;
    int index = 0;

    if (count > 0) {
        do {
            W8NPCItemList** element;
            W8NPCItemList* entry;

            if (index < count) {
                element = (W8NPCItemList**)&item_lists->data[index];
            } else {
                element = (W8NPCItemList**)item_lists->data;
            }
            entry = *element;

            if (entry->npc_record->record_id == npc_record_id) {
                return entry;
            }
            ++index;
        } while (index < count);
    }
    return 0;
}
