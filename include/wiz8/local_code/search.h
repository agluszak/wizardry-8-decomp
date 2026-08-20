#pragma once

#include "wiz8/vector.h"

class Trigger;
struct W8WorldItem;

/* Local Code\search.cpp. Searchable entries are a three-owner tagged record:
   item registration writes +0, while trigger registration writes +8. */
struct W8Searchable {
    W8WorldItem* item_00;
    void* object_04;
    Trigger* trigger_08;
};

extern W8GrowableVector<W8Searchable*> g_searchables_00689fa8;

void AddSearchableItem00516E20(W8WorldItem* item);
void AddSearchableTrigger00516F00(Trigger* trigger);
