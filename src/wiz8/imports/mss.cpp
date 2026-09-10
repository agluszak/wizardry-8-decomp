#include "Mss.h"
#include <stdlib.h>

/* Miles import-library startup support, not an SGP implementation. The
   published MSS.H names the registrar and its AIL_startup macro; retail
   0x409c5a calls it before starting the driver. */

// FUNCTION: WIZ8 0x0041a800
static void MSSShutdown0041A800(void)
{
    AIL_shutdown();
}

// FUNCTION: WIZ8 0x0041a7f0
int __cdecl MSS_auto_cleanup(void)
{
    atexit(MSSShutdown0041A800);
    return 0;
}
