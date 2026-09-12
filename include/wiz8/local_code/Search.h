#pragma once

#include "wiz8/vector.h"

class Trigger;
struct W8WorldItem;

/* Local Code\search.cpp. The searchable registry owns small records whose
   final pointer names the world item or trigger participating in a search. */
struct W8Searchable {
    W8Searchable() : value_00(0), value_04(0), object_08(0) {}

    W8WorldItem* value_00;
    int value_04;
    void* object_08;
};

static_assert(sizeof(W8Searchable) == 0x0c, "W8Searchable_must_be_0x0c");

extern W8GrowableVector<W8Searchable*> g_searchables_00689fa8;

void RegisterSearchableWorldItem00516E20(W8WorldItem* item);
void RegisterSearchableTrigger00516F00(Trigger* trigger);
void ClearSearchables005171B0();
