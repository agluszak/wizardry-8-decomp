#pragma once

struct W8Item;
struct W8WorldItem;
struct W8ItemInstance;
struct W8MonsterInfo;

W8WorldItem* ItemInfo(unsigned int item_list_index);
unsigned int ItemIndex(int runtime_id);
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
bool IsWorldItemWithinReach(W8Item* owner, const float* from,
                                     float radius); /* 0x004F8560 */
/* Whether any live world item is visible to the camera within reach. */
bool AnyWorldItemVisible(void); /* 0x004F8650 */
void Function4F8CB0(W8MonsterInfo* monster_info, int value);
