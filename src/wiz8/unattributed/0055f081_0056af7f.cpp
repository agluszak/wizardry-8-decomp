#include "wiz8/unattributed/quarantine_common.h"
#include "input.h"
#include "mousesystem_macros.h"

/* Address quarantine 0055f081-0056af7f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0055F2B0
unsigned char GetTable647CCCEntry(char index)
{
    return g_table_647ccc[index];
}

void GetScreenPoint004284F0(W8ScreenPoint* point);

/* Forward the four mouse-button event kinds through SGP's owned mouse-system
   hook at the current game-space cursor position. Keyboard and motion events
   are deliberately left to their own dispatch layers. */
// FUNCTION: WIZ8 0x00568950
unsigned int Function568950(const InputAtom* input)
{
    W8ScreenPoint point;
    GetScreenPoint004284F0(&point);
    switch (input->usEvent) {
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_UP:
    case RIGHT_BUTTON_DOWN:
    case RIGHT_BUTTON_UP:
        MSYS_SGP_Mouse_Handler_Hook(
            input->usEvent,
            static_cast<unsigned short>(point.x),
            static_cast<unsigned short>(point.y),
            gfLeftButtonState,
            gfRightButtonState);
        return 1;
    default:
        return 0;
    }
}
