#include "wiz8/dialog_code/MessageDialogBase.h"

/* The message-dialog implementation moved to stMessageDialog.cpp. Only the
   distant 0x005AD280 override remains here: it is the gap between
   OptionsScreen.cpp and MGSSpellIcons.cpp, with no evidence for a closer
   original unit. */

// FUNCTION: WIZ8 0x005ad280
int W8MessageDialogBase::GetDialogType()
{
    return 1;
}
