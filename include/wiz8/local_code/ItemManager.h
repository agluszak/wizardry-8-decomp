#pragma once

struct W8WorldItem;

W8WorldItem* ItemInfo(unsigned int item_list_index);
void SetWorldItemFlag02(W8WorldItem* item, char enabled);
void RebuildAllWorldItemInstances(void);

bool InitializeItemManagerState();

unsigned char ReleaseItemLists(void);

extern int g_world_item_cursor;
