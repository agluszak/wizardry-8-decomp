#pragma once

#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/xstatus.h"
#include "wiz8/vector.h"
#include "surrender/srMath.h"

#pragma pack(push, 1)

/* The owner and the entity it holds are Engine Code\Item.cpp's W8Item and
   W8ItemRep. They were modelled a second time here before that unit was
   recovered, which declared 0x004B8890 twice; the canonical pair now lives in
   engine_code/Item.h and this header uses it. */

struct W8WorldItem {
    int runtime_id; /* 0x00 */
    W8Item* p3D;    /* 0x04: assertion-proven pItemInfo->p3D; owns the live world entity */
    unsigned char fActive; /* 0x08: assertion-proven pItemInfo->fActive */
    W8ItemInstance item;        /* 0x09 */
    srVector3T<float> position; /* 0x15 */
    unsigned char unknown_21[4];
    /* 0x25 is a second flag word: the save and load paths clear bit 3 of it
       while a level restore is in progress. */
    int entity_flags;
    unsigned int flags; /* 0x29: tested by mask by 0x004F8130 */
    /* Both established by the LoadSaveGame.cpp serializers: the marker is set
       to 1 before each record is written, and the chain is walked and rebuilt
       through the link. The link is written to the file and reloaded with the
       record, so on disk it only records that another entry follows. */
    int saved_marker;  /* 0x2d */
    W8WorldItem* next; /* 0x31 */
    /* 0x35: vertical velocity while flag bit 1 (falling) is set; cleared on
       ground settle. */
    float vertical_velocity_35;
    int sector_id; /* 0x39 */
    unsigned char unknown_3d[0x70];
}; /* 0xad */

#pragma pack(pop)

W8WorldItem* CreateWorldItem(W8ItemInstance* item, const srVector3T<float>* position,
                             int entity_flags, unsigned char add_to_world);
W8WorldItem* GetNextWorldItem(char restart);
unsigned char SettleWorldItem(W8WorldItem* item);
W8WorldItem* SpawnItem(int item_id, const srVector3T<float>* position, int entity_flags,
                       unsigned char add_to_world);

bool ItemHasFlags(W8WorldItem* item, unsigned int mask);
void SetItemFlags(W8WorldItem* item, unsigned int mask, bool enabled);
void SetItemAndEntityFlags(W8WorldItem* item, unsigned int mask, bool enabled);
int ItemInfoGetNumInGroup(W8WorldItem* item);
void ItemInfoAddToGroup(W8WorldItem* group, W8WorldItem* item);
W8WorldItem* ItemInfoRemoveFromGroup(W8WorldItem* group, W8WorldItem* item);
W8WorldItem* ItemInfoGroupGetNext(W8WorldItem* item);
int ItemInfoMakeGroupList(W8WorldItem* item, W8GrowableVector<W8WorldItem*>* out);
bool ItemInfoIsWorldPersistent(const W8WorldItem* item);
void FreeWorldItemGroup(W8WorldItem* item);
void ActivateItem(W8WorldItem* item);
int FindItemRecordByName(const char* name);
