#pragma once

struct W8WorldItem;
struct W8MonsterInfo;

W8WorldItem* ItemInfo(unsigned int item_list_index);
void SetWorldItemFlag02(W8WorldItem* item, char enabled);
void RebuildAllWorldItemInstances(void);

bool InitializeItemManagerState();

unsigned char ReleaseItemLists(void);

extern int g_world_item_cursor;

void DropHeldItem(int arg_1); /* 0x004F7610 */
void Function4F7480(void);
unsigned char Function4F8650(void);
void Function4F8CB0(W8MonsterInfo* monster_info, int value);
