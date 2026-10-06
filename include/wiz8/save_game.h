#pragma once

#include "wiz8/item_spawning.h"

bool SaveItemFile(int handle, W8WorldItem* item);
W8WorldItem* LoadItem(int handle, bool add_to_list);
bool LoadSavedLevelItems(int level, W8GrowableVector<W8WorldItem*>* items);
