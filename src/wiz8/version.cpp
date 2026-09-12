#include "wiz8/utility.h"
#include "wiz8/version.h"

#include <string.h>

/* Version-banner formatting. Live query: 0x004E3620 is a gap between
   Local Code\Gameloop.cpp (upper 0x004E34B0) and Local Code\MonsterManager.cpp
   (lower 0x004E3820). */

// FUNCTION: WIZ8 0x004E3620
void FormatVersionBanner004E3620(char* out, char with_title, char with_build, char with_date)
{
    out[0] = '\0';
    if (with_title) {
        strcat(out, "Wizardry 8 ");
    }
    strcat(out, FormatString("v%d.%d.%d", 1, 2, 4));
    if (with_build) {
        strcat(out, FormatString(" (build %d)", 0xdb));
    }
    if (with_date) {
        strcat(out, FormatString(" %s", "2001/12/24 15:36"));
    }
}
