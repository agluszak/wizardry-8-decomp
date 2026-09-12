#include "wiz8/bringup_gates.h"
#include "wiz8/fact_state.h"
#include "wiz8/game_status.h"

// GLOBAL: WIZ8 0x0068de44
unsigned char g_fact_notifications_suppressed;

/* Retail's shared success return, also used by the screen lifecycle table. */
// FUNCTION: WIZ8 0x005b1740
unsigned char ScreenLifecycleSuccess(void)
{
    return 1;
}

// FUNCTION: WIZ8 0x005588e0
void SetFactNotificationsSuppressed(unsigned char suppressed)
{
    g_fact_notifications_suppressed = suppressed;
}

/* Shared folded empty entry; product callers retain their established names. */
// FUNCTION: WIZ8 0x004023a0
void NoOp(void) {}
