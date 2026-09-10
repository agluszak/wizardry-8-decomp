#include "wiz8/bringup_gates.h"
#include "wiz8/game_status.h"

// GLOBAL: WIZ8 0x0068de44
unsigned char g_byte_68de44;

/* Retail's shared success return, also used by the screen lifecycle table. */
// FUNCTION: WIZ8 0x005b1740
unsigned char ScreenLifecycleSuccess(void)
{
    return 1;
}

// FUNCTION: WIZ8 0x00443a50
int Function443A50(void)
{
    g_status_685170.next_trigger_id_2356 = 1;
    return 1;
}

// FUNCTION: WIZ8 0x00482740
void Function482740(int value)
{
    g_status_685170.game_time_days = value;
}

// FUNCTION: WIZ8 0x005588e0
void Function5588E0(unsigned char value)
{
    g_byte_68de44 = value;
}

/* Shared folded empty entry; product callers retain their established names. */
// FUNCTION: WIZ8 0x004023a0
void NoOp(void)
{
}
