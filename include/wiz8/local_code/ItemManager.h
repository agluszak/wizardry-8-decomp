#pragma once

#include "surrender/srMath.h"

struct W8Item;
struct W8WorldItem;
struct W8ItemInstance;
struct W8MonsterInfo;

/* When set, world items flagged invisible are not activated;
   MIPE's item-create 'A' key toggles it ("hidden" versus "blue"). */
extern bool g_hide_invisible_items;

W8WorldItem* ItemInfo(unsigned int item_list_index);
void DeactivateWorldItem(W8WorldItem* item);
unsigned int ItemIndex(int runtime_id);
void SetWorldItemHighlight(int runtime_id, bool on);
int PickNearestItemUnderCursor(int cursor_x, int cursor_y, float max_distance);
unsigned char InteractWithWorldItem(int runtime_id);
W8ItemInstance* CopyWorldItemInstance(const W8WorldItem* item);
void SetWorldItemFalling(W8WorldItem* item, bool enabled);
void RebuildAllWorldItemInstances(void);

bool InitializeItemManagerState();

unsigned char ReleaseItemLists(void);

extern int g_world_item_cursor;

void DropHeldItem(int);
void UpdateNearbyWorldItems(void);
unsigned char AdvanceFallingWorldItem(W8WorldItem* item);
bool IsWorldItemWithinReach(W8Item* owner, const srVector3T<float>* from, float radius);
bool AnyWorldItemVisible(void);
void DropMonsterLoot(W8MonsterInfo* monster_info, int value);
