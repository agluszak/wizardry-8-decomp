
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/startup_runtime_state.h"

extern "C" {

// FUNCTION: WIZ8 0x0054b0b0
void DestroyGameplayObjects(void)
{
    W8StartupRuntimeState* owned = g_startup_runtime_state;

    if (owned) {
        owned->~W8StartupRuntimeState();
        operator delete(owned);
        g_startup_runtime_state = 0;
    }
    if (g_gameplay_timer_685067) {
        delete g_gameplay_timer_685067;
        g_gameplay_timer_685067 = 0;
    }
}

}
