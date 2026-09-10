#include "wiz8/local_code/Search.h"

#include "wiz8/engine_code/Trigger.h"
#include "wiz8/sr_api.h"

/* Local Code\search.cpp. The assertion in RegisterSearchableTrigger at source
   line 337 establishes the original translation unit. */

// GLOBAL: WIZ8 0x00689fa8
W8GrowableVector<W8Searchable*> g_searchables_00689fa8;

// FUNCTION: WIZ8 0x00516f00
void RegisterSearchableTrigger00516F00(Trigger* trigger)
{
    W8Searchable* searchable = new W8Searchable;
    if (searchable == 0) {
        srAssertFail(
            "pSearchable",
            "C:\\Projects\\Wizardry 8\\Local Code\\search.cpp",
            0x151, 0);
    }
    searchable->object_08 = trigger;
    g_searchables_00689fa8.Add(searchable);
}

// FUNCTION: WIZ8 0x005171b0
void ClearSearchables005171B0()
{
    g_searchables_00689fa8.Clear();
}
