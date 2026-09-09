#include "wiz8/npc_state.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/GameplayCode.h"

/* Address quarantine 0056b270-00579900; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern void Function56C5E0(void* npc, int value, int line, int suppress, int arg);

/* Forward a monster-script notice to the targeting layer unless the screen is
   busy or this NPC kind suppresses it. The suppress flag travels as an int:
   the body forwards the whole dword without masking. */
// FUNCTION: WIZ8 0x0056C590
void Function56C590(int npc_record, int value, int line, int suppress)
{
    W8NpcState* npc = reinterpret_cast<W8NpcState*>(npc_record);

    if (g_flag_00683f97 == 0 && g_in_combat_00683f94 == 0 &&
        (npc->record->kind != 7 || GetFact(0x1c) != 1)) {
        Function56C5E0(reinterpret_cast<void*>(npc_record), value, line, suppress, 0);
    }
}
