#ifndef WIZ8_NPC_ITEM_LISTS_H
#define WIZ8_NPC_ITEM_LISTS_H

#include "wiz8/vector.h"

#pragma pack(push, 1)

typedef struct W8NPCRecordRef {
    unsigned char unknown_00[0x58];
    int record_id;
} W8NPCRecordRef;

typedef struct W8NPCItemList {
    int unknown_00;
    unsigned char unknown_04[2];
    W8NPCRecordRef* npc_record;
    unsigned char unknown_0a[0x10];
    unsigned char flag_1a;
} W8NPCItemList;

#pragma pack(pop)

extern W8GrowableVector<W8NPCItemList*>* g_npc_item_lists;
W8NPCItemList* GetNPCItemListByID(int npc_record_id);

#endif
