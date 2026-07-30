#pragma once

#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/item_instance.h"
#include "surrender/srMath.h"

#pragma pack(push, 1)

/* The owner and the entity it holds are Engine Code\Item.cpp's W8Item and
   W8ItemRep. They were modelled a second time here before that unit was
   recovered, which declared 0x004B8890 twice; the canonical pair now lives in
   engine_code/Item.h and this header uses it. */

typedef struct W8WorldItem {
    int runtime_id;                      /* 0x00 */
    W8Item* owner;                       /* 0x04: owns the live world entity */
    unsigned char unknown_08;
    W8ItemInstance item;                 /* 0x09 */
    srVector3T<float> position;          /* 0x15 */
    unsigned char unknown_21[4];
    /* 0x25 is a second flag word: the save and load paths clear bit 3 of it
       while a level restore is in progress. */
    int entity_flags;
    unsigned int flags;                  /* 0x29: tested by mask by 0x004F8130 */
    /* Both established by the LoadSaveGame.cpp serializers: the marker is set
       to 1 before each record is written, and the chain is walked and rebuilt
       through the link. The link is written to the file and reloaded with the
       record, so on disk it only records that another entry follows. */
    int saved_marker;                    /* 0x2d */
    struct W8WorldItem* next;            /* 0x31 */
    int unknown_35;
    int sector_id;                       /* 0x39 */
    unsigned char unknown_3d[0x70];
} W8WorldItem;                           /* 0xad */

#pragma pack(pop)

extern "C" {

/* Saved in the status header and incremented for every newly created world
   item, with zero normalized to the first valid id during initialization. */
// GLOBAL: WIZ8 0x006874C2
extern int g_next_world_item_id;
/* gXStatus.plsItemList, named by the ItemInfo assertion that bounds an index
   against PLLength(gXStatus.plsItemList). */
// GLOBAL: WIZ8 0x00683FB5
extern W8PList* g_world_item_list;

W8WorldItem* CreateWorldItem(
    const W8ItemInstance* item,
    const srVector3T<float>* position,
    int unknown,
    unsigned char add_to_world);
W8WorldItem* SpawnItem(
    int item_id,
    const srVector3T<float>* position,
    int unknown,
    unsigned char add_to_world);

}

void ReplaceOrCreateItem(
    W8ItemInstance* item,
    int item_id,
    int quantity,
    int charges,
    int identified);

bool ItemHasFlags(W8WorldItem* item, unsigned int mask);
void SetItemFlags(W8WorldItem* item, unsigned int mask, unsigned char enabled);
void SetItemAndEntityFlags(W8WorldItem* item, unsigned int mask, unsigned char enabled);
int ItemInfoGetNumInGroup(W8WorldItem* item);
void ItemInfoAddToGroup(W8WorldItem* group, W8WorldItem* item);
W8WorldItem* ItemInfoRemoveFromGroup(
    W8WorldItem* group, W8WorldItem* item);
void FreeWorldItemGroup(W8WorldItem* item);
