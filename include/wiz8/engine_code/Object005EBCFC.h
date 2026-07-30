#pragma once

#include "wiz8/engine_code/game_timer.h"

/* The 0x28-byte timer-derived helper constructed by GDCamera. Its original
   name is not exposed; the vtable address remains the stable identity. */
class W8Object005EBCFC : public W8Timer005EC0A4 {
public:
    W8Object005EBCFC(
        float duration, unsigned char raw_time, unsigned char set_flag_2);
                                                        /* 0x0043A500 */
    virtual ~W8Object005EBCFC() override;               /* 0x004218D0 */
    void Method0043A530();                              /* 0x0043A530 */

private:
    unsigned char m_positional_024;                     /* 0x024 */
    unsigned char m_padding_025[3];
};

static_assert(sizeof(W8Object005EBCFC) == 0x28,
              "W8Object005EBCFC_must_be_0x28");
