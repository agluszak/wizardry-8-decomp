#include "wiz8/local_screens/MGSSpellIcons.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/screen_state.h"
#include "wiz8/engine_code/Video2.h"

/* Local Screens\MGSSpellIcons.cpp. The vector destructor emission at
   0x005B1B70 is the last one before the compiler's MGSSpellIcons.cpp to
   RCSCommon.cpp boundary at 0x005B1B90; the matching scalar deleting
   destructors are emitted on the RCSCommon side. */

// TEMPLATE: WIZ8 0x005b1b70
// W8GrowableVector<W8CharacterPageEntry*>::~W8GrowableVector<W8CharacterPageEntry*>

/* Clear and invalidate the main-game effect strip while it is up. Moved here
   from Magic Effects.cpp: the TU report proves this hull's range covers
   0x005AF2D0. */
// FUNCTION: WIZ8 0x005af2d0
void InvalidateMainGameEffectHud(void)
{
    if (g_current_screen_state.id == 7) {
        ClearSurfaceRect(0x7f, 0x14, 0x201, 0x28);
        InvalidateRegion(0x7f, 0x14, 0x201, 0x28, 0);
    }
}
