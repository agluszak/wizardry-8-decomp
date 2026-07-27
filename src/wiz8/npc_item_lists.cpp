#include "wiz8/gameplay_boundaries.h"

// FUNCTION: WIZ8 0x0050B830
W8NPCItemList* GetNPCItemListByID(int npc_record_id)
{
    int count = g_npc_item_lists->GetCount();
    W8GrowableVector<W8NPCItemList*>* item_lists = g_npc_item_lists;
    int index = 0;

    if (count > 0) {
        do {
            W8NPCItemList* entry = *item_lists->GetAt(index);

            if (entry->npc_record->record_id == npc_record_id) {
                return entry;
            }
            ++index;
        } while (index < count);
    }
    return 0;
}
