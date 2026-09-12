#include "wiz8/local_code/Search.h"

#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/sr_api.h"

/* Local Code\search.cpp. The assertion in RegisterSearchableTrigger at source
   line 337 establishes the original translation unit. */

// GLOBAL: WIZ8 0x00689fa8
W8GrowableVector<W8Searchable*> g_searchables_00689fa8;

/* Register one searchable world item. The item pointer lands in the record's
   first slot, which is what distinguishes the item entries from the trigger
   entries registered by the sibling below. */
// FUNCTION: WIZ8 0x00516e20
void RegisterSearchableWorldItem00516E20(W8WorldItem* item)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail("pSearchable", "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp", 0x125, 0);
    }
    searchable->value_00 = item;
    g_searchables_00689fa8.Add(searchable);
}

// FUNCTION: WIZ8 0x00516f00
void RegisterSearchableTrigger00516F00(Trigger* trigger)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail("pSearchable", "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp", 0x151, 0);
    }
    searchable->object_08 = trigger;
    g_searchables_00689fa8.Add(searchable);
}

// FUNCTION: WIZ8 0x005171b0
void ClearSearchables005171B0()
{
    g_searchables_00689fa8.Clear();
}
