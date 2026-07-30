#pragma once

#include "wiz8/dialog_base.h"

struct W8DialogNotifyTarget {
    virtual void Notify(unsigned char reason, int value) = 0;
};

/* Modal Dialog Code class identified by its constructor. The base owns the
   first 0x98 bytes; this class adds the notification payload and target. */
class W8Dialog005A80A0 : public W8DialogBase005D25B0 {
public:
    W8Dialog005A80A0(
        int message_index,
        int caption_id,
        int notify_value);
    virtual ~W8Dialog005A80A0() override;         /* 0x005A8190 */
    virtual unsigned char Close() override;       /* 0x005A81A0 */

private:
    int notify_value_98;                          /* 0x98 */
    W8DialogNotifyTarget* notify_target;           /* 0x9c */
};

static_assert(sizeof(W8Dialog005A80A0) == 0xa0,
              "W8Dialog005A80A0_must_be_0xa0");
