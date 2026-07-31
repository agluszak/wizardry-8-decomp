#include "wiz8/dialog_code/NotificationDialog.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/regions.h"

/* Dialog Code. W8NotificationDialog is a modal popup built on the unnamed base
   constructed by 0x005D25B0 and destroyed by 0x005D2610. Only members touched
   by these bodies are modelled; base storage stays opaque, and the source owns
   the two proven fields at 0x98 and 0x9c.

   The base is the shared W8ModalDialogBase in wiz8/dialog_base.h, whose
   fifteen slots this class inherits; it overrides only slot 9. */

/* Table of message payloads the dialog is constructed against; the caller
   passes an index into it. */

/* Registers the dialog's region set. */
extern void ActivateDialogRegion(int region_set);  /* 0x004F2040 */

/* The base constructor runs first and the vtable install follows it, so the
   body is just the two own fields and the dialog setup sequence. The /GX EH
   frame comes from the base having a destructor: an exception in any setup
   call has to unwind it. */
// FUNCTION: WIZ8 0x005a80a0
W8NotificationDialog::W8NotificationDialog(int message_index, int caption_id, int notify_value)
    : notification_value(notify_value), notify_target(0)
{
    SetExtent(0xf0, 0xbe);
    SetOrigin(0xa0, 100);
    SetBackground("Data\\Dialogs\\DialogBackground.sti", 0);
    SetClientExtent(0xfa, 200);
    SetMessage(gppStringList[message_index], 1, 0x32, 1, caption_id, 1, 1, 0, 0x15e);
    ActivateDialogRegion(0x138);
}

/* Restoring the vtable and tail-jumping to the base destructor is the whole
   body; the compiler generates the scalar deleting destructor at 0x005A8170
   from this same declaration. */
// FUNCTION: WIZ8 0x005a8190
W8NotificationDialog::~W8NotificationDialog()
{
}

// FUNCTION: WIZ8 0x005a81a0
unsigned char W8NotificationDialog::Close()
{
    unsigned char handled;

    W8ModalDialogBase::Close();
    handled = is_open;
    if (!handled) {
        ClearActiveRegionIfMatches(0x138);
        if (notify_target) {
            notify_target->OnDialogClosed(close_result, notification_value);
        }
    }
    return handled;
}
