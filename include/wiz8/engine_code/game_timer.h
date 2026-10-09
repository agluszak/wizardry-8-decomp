#pragma once

#include "Types.h"
#include "wiz8/layouts/game_status.h"
#include "surrender/srTimer.h"

extern int g_shared_timer_pause_base;
extern int g_shared_timer_pause_time;
extern bool g_shared_timer_paused;
extern bool g_shared_timer_flag0;
extern bool g_level_motion_resume_pending;
extern srTimer* g_shared_timer_base;

void PauseSharedGameTimers(void);
void ResumeSharedGameTimers(void);

/* Mode 1 samples the saved game-day/millisecond clock. Mode 0 samples the
   shared srTimer; its separate raw-time flag controls pause adjustment. */
enum W8TimerClock { W8_TIMER_CLOCK_SHARED = 0, W8_TIMER_CLOCK_GAME = 1 };

class W8GameTimer {
public:
    struct Flags {
        bool raw_time : 1;
        bool one_shot : 1;
        unsigned char reserved_low : 1;
        bool paused : 1;
        bool slow_scale : 1;
        unsigned char reserved_high : 3;
        unsigned char reserved_byte;
    };

    W8GameTimer();
    W8GameTimer(float duration, unsigned char raw_time);
    virtual ~W8GameTimer();
    int ReadClock() const
    {
        switch (m_clock_mode) {
        case W8_TIMER_CLOCK_GAME:
            return (g_status.game_time_days * 86400000 + g_status.game_time_ms) * 10;
        default:
            break;
        }
        if (!m_flags.raw_time) {
            if (g_shared_timer_paused) {
                return g_shared_timer_pause_time;
            }
            return m_shared->getUTime(srTimer::TIMER_READ_DEFAULT) - g_shared_timer_pause_base;
        }
        return m_shared->getUTime(srTimer::TIMER_READ_DEFAULT);
    }
    int GetTime();
    void SetMode(W8TimerClock mode);
    void SetDuration(float duration);
    void Restart();
    float GetProgress();
    void SetProgress(float progress);
    void SetDurationScale(float scale);
    void ResetDurationScale();
    BOOLEAN Load(int handle);
    float GetElapsedSeconds();

    W8TimerClock m_clock_mode; /* 0x04: 1 reads the game clock */
    Flags m_flags;             /* 0x08: runtime clock state; save/load uses progress and scale */
    srTimer* m_shared;         /* 0x0c */
    int m_start;               /* 0x10 */
    int m_end;                 /* 0x14: start + duration */
    int m_duration;            /* 0x18: 10000 */
    float m_duration_seconds;  /* 0x1c */
    float m_duration_scale;    /* 0x20 */
};

W8GameTimer* CreateGameTimer(float duration, unsigned char raw_time);

static_assert(sizeof(W8GameTimer::Flags) == 2, "W8GameTimer_flags_size");
W8_ABI_ASSERT(sizeof(W8GameTimer) == 0x24, "W8GameTimer_must_be_0x24");
