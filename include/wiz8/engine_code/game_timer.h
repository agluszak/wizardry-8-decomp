#pragma once

#include "surrender/srTimer.h"

class W8Timer005EC0A4 {
public:
    W8Timer005EC0A4();
    W8Timer005EC0A4(float duration, unsigned char raw_time);
    virtual ~W8Timer005EC0A4();
    int Sample() const;

    int m_mode;                          /* 0x04: 1 reads the game clock */
    unsigned short m_flags;              /* 0x08: bit 0 reads the timer raw */
    srTimer* m_shared;                   /* 0x0c */
    int m_start;                         /* 0x10 */
    int m_end;                           /* 0x14: start + duration */
    int m_duration;                      /* 0x18: 10000 */
    float m_speed;                       /* 0x1c */
    float m_speed_2;                     /* 0x20 */
};

static_assert(sizeof(W8Timer005EC0A4) == 0x24, "W8Timer005EC0A4_must_be_0x24");
