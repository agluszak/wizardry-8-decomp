#include "wiz8/regions.h"

/* Address quarantine 004f0c81-004f203f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x004F1220
void ReleasePointer689B40(void)
{
    if (g_default_help_text) {
        delete[] g_default_help_text;
    }
}
