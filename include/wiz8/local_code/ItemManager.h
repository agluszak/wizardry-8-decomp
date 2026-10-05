#pragma once

#include "surrender/srMath.h"

struct W8Item;
struct W8WorldItem;
struct W8ItemInstance;
struct W8MonsterInfo;

/* 0x0064A1CD: when set, world items flagged invisible are not activated;
   MIPE's item-create 'A' key toggles it ("hidden" versus "blue"). */
extern bool g_hide_invisible_items;

W8WorldItem* ItemInfo(unsigned int item_list_index);
void DeactivateWorldItem(W8WorldItem* item); /* 0x004F70D0 */
unsigned int ItemIndex(int runtime_id);
void SetWorldItemHighlight(int runtime_id, bool on); /* 0x004F71E0 */
int PickNearestItemUnderCursor(int cursor_x, int cursor_y, float max_distance); /* 0x004F7370 */
unsigned char InteractWithWorldItem(int runtime_id); /* 0x004F7910 */
W8ItemInstance* CopyWorldItemInstance(const W8WorldItem* item); /* 0x004F9210 */
void SetWorldItemFalling(W8WorldItem* item, bool enabled);
void RebuildAllWorldItemInstances(void);

bool InitializeItemManagerState();

unsigned char ReleaseItemLists(void);

extern int g_world_item_cursor;

void DropHeldItem(int arg_1); /* 0x004F7610 */
void UpdateNearbyWorldItems(void); /* 0x004F7480 */
unsigned char AdvanceFallingWorldItem(W8WorldItem* item); /* 0x004F9240 */
bool IsWorldItemWithinReach(W8Item* owner, const srVector3T<float>* from,
                            float radius); /* 0x004F8560 */
bool AnyWorldItemVisible(void);                               /* 0x004F8650 */
void DropMonsterLoot(W8MonsterInfo* monster_info, int value); /* 0x004F8CB0 */
