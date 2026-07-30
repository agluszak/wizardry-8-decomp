#pragma once

#include "surrender/srTimer.h"

extern int g_shared_timer_pause_base;
extern int g_shared_timer_pause_time;
extern unsigned char g_shared_timer_paused;
extern int g_game_time_ms;
extern int g_game_time_days;

class W8Timer005EC0A4 {
public:
    W8Timer005EC0A4();
    W8Timer005EC0A4(float duration, unsigned char raw_time);
    virtual ~W8Timer005EC0A4();
    __forceinline int Sample() const
    {
        switch (m_mode) {
        case 1:
            return (g_game_time_days * 86400000 + g_game_time_ms) * 10;
        }
        if ((m_flags & 1) == 0) {
            if (g_shared_timer_paused != 0) {
                return g_shared_timer_pause_time;
            }
            return m_shared->getUTime(srTimer::TIMER_READ_DEFAULT)
                   - g_shared_timer_pause_base;
        }
        return m_shared->getUTime(srTimer::TIMER_READ_DEFAULT);
    }
    int Method00439A60();
    float Method0043A190();

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
