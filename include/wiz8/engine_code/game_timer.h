#pragma once

#include "wiz8/game_status.h"
#include "surrender/srTimer.h"

extern int g_shared_timer_pause_base;
extern int g_shared_timer_pause_time;
extern unsigned char g_shared_timer_paused;
extern unsigned char g_shared_timer_flag_d1;
extern unsigned char g_shared_timer_flag_d2;
extern srTimer* g_shared_timer_base;

void Function439BC0(void);
void Function439CA0(void);

class W8GameTimer {
public:
    W8GameTimer();
    W8GameTimer(float duration, unsigned char raw_time);
    virtual ~W8GameTimer();
    __forceinline int ReadClock() const
    {
        switch (m_clock_mode) {
        case 1:
            return (g_status_685170.game_time_days * 86400000 + g_status_685170.game_time_ms) * 10;
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
    void SetMode(int mode);
    void SetDuration(float duration);
    void Restart();
    float GetProgress();
    void SetProgress(float progress);

    int m_clock_mode;                          /* 0x04: 1 reads the game clock */
    unsigned short m_flags;              /* 0x08: bit 0 reads the timer raw */
    srTimer* m_shared;                   /* 0x0c */
    int m_start;                         /* 0x10 */
    int m_end;                           /* 0x14: start + duration */
    int m_duration;                      /* 0x18: 10000 */
    float m_duration_seconds;                       /* 0x1c */
    float m_duration_scale;                     /* 0x20 */
};

static_assert(sizeof(W8GameTimer) == 0x24, "W8GameTimer_must_be_0x24");
