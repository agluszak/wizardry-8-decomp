#pragma once

#include "wiz8/dialog_base.h"

struct W8DialogCloseListener {
    virtual void OnDialogClosed(unsigned char reason, int value) = 0;
};

/* Modal Dialog Code class identified by its constructor. The base owns the
   first 0x98 bytes; this class adds the notification payload and target. */
class W8NotificationDialog : public W8ModalDialogBase {
public:
    W8NotificationDialog(
        int message_index,
        int caption_id,
        int notify_value);
    virtual ~W8NotificationDialog() override;         /* 0x005A8190 */
    virtual unsigned char Close() override;       /* 0x005A81A0 */

private:
    int notification_value;                          /* 0x98 */
    W8DialogCloseListener* notify_target;           /* 0x9c */
};

static_assert(sizeof(W8NotificationDialog) == 0xa0,
              "W8NotificationDialog_must_be_0xa0");
