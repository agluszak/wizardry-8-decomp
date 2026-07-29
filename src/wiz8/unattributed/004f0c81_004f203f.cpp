#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 004f0c81-004f203f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x004F1220
void ReleasePointer689B40(void)
{
    if (g_pointer_689b40) {
        operator delete(g_pointer_689b40);
    }
}
