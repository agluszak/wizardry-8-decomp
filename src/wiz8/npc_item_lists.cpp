#include "wiz8/gameplay_boundaries.h"

// FUNCTION: WIZ8 0x0050b830
W8NPCItemList* GetNPCItemListByID(int npc_record_id)
{
    int count = g_npc_item_lists->count;
    W8GrowableVector<W8NPCItemList*>* item_lists = g_npc_item_lists;
    int index = 0;

    if (count > 0) {
        do {
            /* Two things the accessor form loses. The bound is the count read
               once above, not the one GetAt re-reads from the object on every
               pass; and the offset is applied to the storage pointer already
               loaded, not to a fresh read of it, which is what keeps VC6 from
               strength-reducing the walk into a running pointer. The vector
               stays typed as the template instantiation it is. */
            W8NPCItemList** element;
            W8NPCItemList* entry;

            if (index < count) {
                element = &item_lists->data[index];
            } else {
                element = item_lists->data;
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
