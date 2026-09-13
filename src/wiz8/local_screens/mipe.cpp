#include "wiz8/local_screens/mipe.h"
#include "wiz8/local_screens/AutomapScreen.h"

/* Local Screens\mipe.cpp. Only the two flag accessors at the top of the
   0x00579900-0x0057DF80 hull are proven so far; the rest of the unit is
   unrecovered. The flags themselves are defined in AutomapScreen.cpp, whose
   gap code writes them. */

// FUNCTION: WIZ8 0x0057dbb0
unsigned char GetFlag68F105(void)
{
    return g_flag_68f105;
}
// FUNCTION: WIZ8 0x0057dbc0
unsigned char GetFlag68F104(void)
{
    return g_flag_68f104;
}
