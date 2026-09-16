#pragma once

#include "wiz8/vector.h"
#include "surrender/srMath.h"

class Trigger;
class W8WorldCursorNode;
struct W8WorldItem;

/* Local Code\search.cpp. The searchable registry owns small records whose
   members name the world item or trigger participating in a search. */
struct W8Searchable {
    W8Searchable() : world_item(0), cursor_node(0), trigger(0) {}

    /* 0x00517080: resolve this searchable's world position. */
    void GetPosition(srVector3T<float>* position);
    /* 0x00516AD0: fire the found consequence (item reveal or trigger run) and
       remove this record from the registry. */
    void Reveal();
    /* 0x00517560: the best searching party member within range of this
       searchable, or -1. Practices the searching skill on a qualified pick. */
    int PickBestSearcher();

    W8WorldItem* world_item;
    /* 0x04: never written in retail; the only reader resolves a world cursor
       node's location through it, so the dead path is modelled on that type. */
    W8WorldCursorNode* cursor_node;
    Trigger* trigger;
};

static_assert(sizeof(W8Searchable) == 0x0c, "W8Searchable_must_be_0x0c");

/* 0x00689FB8: the per-pulse iteration view over the registry. Its constructor
   seeds the cursor before the first element; CollectSearchablesInView refills
   the item list with the in-range, in-view records. */
struct W8SearchableView {
    W8SearchableView();

    int cursor;
    W8GrowableVector<W8Searchable*> items;
};

static_assert(sizeof(W8SearchableView) == 0x14, "W8SearchableView_must_be_0x14");

extern W8GrowableVector<W8Searchable*> g_searchables_00689fa8;
extern W8SearchableView g_search_view_00689fb8;
/* 500ms pulse clock arming the search-mode sweep. */
extern unsigned int g_search_pulse_clock_00689fcc;

void RegisterSearchableWorldItem00516E20(W8WorldItem* item);
void RegisterSearchableTrigger00516F00(Trigger* trigger);
void UnregisterSearchableTrigger00516FE0(Trigger* trigger);
/* 0x00516BA0: refill the view with the in-range, in-view searchables and
   return it, or null when none qualify. */
W8SearchableView* CollectSearchablesInView(void);
void ClearSearchables005171B0();
/* 0x005171C0: the 500ms sweep that picks searchers, queues their events and
   reveals what they found. */
void RunSearchPulse(void);
/* Toggle the party's search mode; blocked outright in combat. */
void ToggleSearchMode(void); /* 0x00517780 */
