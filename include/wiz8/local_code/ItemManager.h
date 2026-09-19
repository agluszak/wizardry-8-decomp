#pragma once

#include "surrender/srMath.h"

struct W8Item;
struct W8WorldItem;
struct W8ItemInstance;
struct W8MonsterInfo;

W8WorldItem* ItemInfo(unsigned int item_list_index);
unsigned int ItemIndex(int runtime_id);
/* Runtime id of the nearest hovered world item inside `max_distance`, or -1. */
int PickNearestItemUnderCursor004F7370(int cursor_x, int cursor_y,
                                       float max_distance); /* 0x004F7370 */
/* Take or trigger-pick one world item by runtime id; 1 when it stays handled. */
unsigned char InteractWithWorldItem004F7910(int runtime_id); /* 0x004F7910 */
/* Copy a world item's carried item out onto the heap. */
W8ItemInstance* CopyWorldItemInstance(const W8WorldItem* item); /* 0x004F9210 */
void SetWorldItemFlag02(W8WorldItem* item, char enabled);
void RebuildAllWorldItemInstances(void);

bool InitializeItemManagerState();

unsigned char ReleaseItemLists(void);

extern int g_world_item_cursor;

void DropHeldItem(int arg_1); /* 0x004F7610 */
/* Walk the world-item list: settle bad sectors, advance falling items, and
   activate/deactivate relative to the camera. */
void UpdateNearbyWorldItems(void); /* 0x004F7480 */
/* Advance one falling world item (flag bit 1) toward the ground. */
unsigned char AdvanceFallingWorldItem(W8WorldItem* item); /* 0x004F9240 */
/* Whether one world item is close enough to a point and in sight. */
bool IsWorldItemWithinReach(W8Item* owner, const srVector3T<float>* from,
                            float radius); /* 0x004F8560 */
/* Whether any live world item is visible to the camera within reach. */
bool AnyWorldItemVisible(void);                               /* 0x004F8650 */
void DropMonsterLoot(W8MonsterInfo* monster_info, int value); /* 0x004F8CB0 */
