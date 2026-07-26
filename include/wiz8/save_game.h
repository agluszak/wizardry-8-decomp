#pragma once

#include "wiz8/item_spawning.h"

unsigned char SaveItemFile(int handle, W8WorldItem* item);
W8WorldItem* LoadItem(int handle, char add_to_list);
