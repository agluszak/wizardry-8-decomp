#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/screen_state.h"

/* Address quarantine 00553910-005545f0; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

/* Whether anything holds the screen busy: combat, a modal, the trigger flag,
   or a current state past the idle slot all answer yes; otherwise the idle
   check decides. */
// FUNCTION: WIZ8 0x00554540
unsigned char Function554540(void)
{
    if (g_in_combat_00683f94 != 0) {
        return 1;
    }
    if (IsModalOpen()) {
        return 1;
    }
    if (g_flag_0068506e != 0) {
        return 1;
    }
    if (g_current_screen_state.id != 7) {
        return 1;
    }
    return !IsScreenIdle();
}
