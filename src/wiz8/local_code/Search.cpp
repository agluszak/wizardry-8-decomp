#include "wiz8/local_code/Search.h"

#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/layouts/game_status.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/notices.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"
#include "timer.h"

/* Local Code\search.cpp. The assertion in RegisterSearchableTrigger at source
   line 337 establishes the original translation unit. */

// GLOBAL: WIZ8 0x00689fa8
W8GrowableVector<W8Searchable*> g_searchables_00689fa8;
// GLOBAL: WIZ8 0x00689fcc
TIMER g_search_pulse_clock_00689fcc;

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

/* Toggle the party's search mode: leaving it posts the off notice, entering it
   posts the on notice and arms the 500ms pulse that sweeps the searchable
   registry; combat blocks the toggle outright. */
// FUNCTION: WIZ8 0x00517780
void ToggleSearchMode(void)
{
    if (g_status_685170.search_mode != 0) {
        g_status_685170.search_mode = 0;
        ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_MODE_OFF], -1, -1, 0);
        return;
    }
    if (gXStatus.fCombatMode == 0) {
        g_status_685170.search_mode = 1;
        ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_MODE_ON], -1, -1, 0);
        g_search_pulse_clock_00689fcc = SetCountdownClock(0x1f4);
        ClearValue6834D4();
    } else {
        ShowNotice(0xc, gppStringList[W8_NOTICE_SEARCH_BLOCKED_COMBAT], -1, -1, 0);
    }
}
