#pragma once

#include "timer.h"
#include "wiz8/vector.h"
#include "surrender/srMath.h"

class Trigger;
class W8WorldCursorNode;
struct W8WorldItem;

/* Local Code\search.cpp. The searchable registry owns small records whose
   members name the world item or trigger participating in a search. */
struct W8Searchable {
    W8Searchable() : world_item(0), cursor_node(0), trigger(0) {}

    /* Resolve this searchable's world position. */
    void GetPosition(srVector3T<float>* position);
    /* Fire the found consequence (item reveal or trigger run) and
       remove this record from the registry. */
    void Reveal();
    /* The best searching party member within range of this
       searchable, or -1. Practices the searching skill on a qualified pick. */
    int PickBestSearcher();

    W8WorldItem* world_item;
    /* Never written; the only reader resolves a world cursor node's
       location through it. */
    W8WorldCursorNode* cursor_node;
    Trigger* trigger;
};

static_assert(sizeof(W8Searchable) == 0x0c, "W8Searchable_must_be_0x0c");

/* The per-pulse iteration view over the registry. Its constructor
   seeds the cursor before the first element; CollectSearchablesInView refills
   the item list with the in-range, in-view records. */
struct W8SearchableView {
    W8SearchableView();

    int cursor;
    W8Vector<W8Searchable*> items;
};

static_assert(sizeof(W8SearchableView) == 0x14, "W8SearchableView_must_be_0x14");

extern W8Vector<W8Searchable*> g_searchables;
extern W8SearchableView g_search_view;
/* 500ms pulse clock arming the search-mode sweep. */
extern TIMER g_search_pulse_clock;

void RegisterSearchableWorldItem(W8WorldItem* item);
void RegisterSearchableTrigger(Trigger* trigger);
void UnregisterSearchableTrigger(Trigger* trigger);
/* Refill the view with the in-range, in-view searchables and
   return it, or null when none qualify. */
W8SearchableView* CollectSearchablesInView(void);
void ClearSearchables();
/* The 500ms sweep that picks searchers, queues their events and
   reveals what they found. */
void RunSearchPulse(void);
/* Toggle the party's search mode; blocked outright in combat. */
void ToggleSearchMode(void);
