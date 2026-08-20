#include "wiz8/local_code/search.h"

#include "wiz8/engine_code/Trigger.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/sr_api.h"

W8GrowableVector<W8Searchable*> g_searchables_00689fa8;

// FUNCTION: WIZ8 0x00516e20
void AddSearchableItem00516E20(W8WorldItem* item)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail("pSearchable",
                     "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp",
                     0x125, 0);
    }
    searchable->item_00 = 0;
    searchable->object_04 = 0;
    searchable->trigger_08 = 0;
    searchable->item_00 = item;
    g_searchables_00689fa8.Add(searchable);
}

// FUNCTION: WIZ8 0x00516f00
void AddSearchableTrigger00516F00(Trigger* trigger)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail("pSearchable",
                     "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp",
                     0x151, 0);
    }
    searchable->item_00 = 0;
    searchable->object_04 = 0;
    searchable->trigger_08 = 0;
    searchable->trigger_08 = trigger;
    g_searchables_00689fa8.Add(searchable);
}

