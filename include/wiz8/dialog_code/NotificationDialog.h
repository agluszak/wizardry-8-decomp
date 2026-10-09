#pragma once

#include "wiz8/dialog_code/MessageDialogBase.h"

struct W8DialogCloseListener {
    virtual void OnDialogClosed(bool accepted, int value) = 0;
};

/* Modal Dialog Code class identified by its constructor. The base owns the
   first 0x98 bytes; this class adds the notification payload and target. */
// VTABLE: WIZ8 0x005eef6c
class W8NotificationDialog : public W8MessageDialogBase {
public:
    W8NotificationDialog(int message_index, bool cancel_allowed, int notify_value);
    virtual bool ProcessInput() override;

public:
    /* OptionsScreen installs the notification receiver directly. */
    int notification_value;
    W8DialogCloseListener* notify_target;
};

W8_ABI_ASSERT(sizeof(W8NotificationDialog) == 0xa0, "W8NotificationDialog_must_be_0xa0");
