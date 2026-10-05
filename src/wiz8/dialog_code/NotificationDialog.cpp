#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/regions.h"

/* Dialog Code. W8NotificationDialog is a modal popup built on the unnamed base
   constructed by 0x005D25B0 and destroyed by 0x005D2610. The source owns the
   two proven fields at 0x98 and 0x9c beyond that recovered base.

   The base is the shared W8MessageDialogBase in wiz8/dialog_code/MessageDialogBase.h, whose
   fifteen slots this class inherits; it overrides only slot 9. */

/* Table of message payloads the dialog is constructed against; the caller
   passes an index into it. */

/* The base constructor runs first and the vtable install follows it, so the
   body is just the two own fields and the dialog setup sequence. The /GX EH
   frame comes from the base having a destructor: an exception in any setup
   call has to unwind it. */
// FUNCTION: WIZ8 0x005a80a0
W8NotificationDialog::W8NotificationDialog(int message_index, bool cancel_allowed, int notify_value)
    : notification_value(notify_value), notify_target(0)
{
    SetOrigin(0xf0, 0xbe);
    SetExtent(0xa0, 100);
    SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    SetClientExtent(0xfa, 200);
    SetMessage(gppStringList[message_index], 1, 0x32, true, cancel_allowed, true, true, 0, 0x15e);
    ActivateDialogRegion(0x138);
}

// FUNCTION: WIZ8 0x005a81a0
bool W8NotificationDialog::ProcessInput()
{
    bool handled;

    W8MessageDialogBase::ProcessInput();
    handled = is_open;
    if (!handled) {
        ClearActiveRegionIfMatches(0x138);
        if (notify_target) {
            notify_target->OnDialogClosed(accepted, notification_value);
        }
    }
    return handled;
}
