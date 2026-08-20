#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/float_constants.h"

/* The game-timer unit: a small timer object over one shared, reference-counted
   srTimer-derived singleton. The image names neither the unit nor the classes -
   the assertion anchors only bound this code to the gap between
   Engine Code\Octree.cpp and Engine Code\BitArray.cpp - so both classes carry
   address-qualified positional names and the file name is descriptive.

   The globals live at 0x006598B8..0x006598D2 and are declared, not defined:
   their addresses in the original data segment are the authority. */

srTimer* g_shared_timer_base;          /* 0x006598B8 */
srTimer* g_shared_timer;               /* 0x006598C0 */
int g_shared_timer_pause_base;                       /* 0x006598C4 */
int g_shared_timer_pause_time;                       /* 0x006598C8 */
int g_shared_timer_refs;                             /* 0x006598CC */
unsigned char g_shared_timer_paused;                 /* 0x006598D0 */
unsigned char g_shared_timer_flag_d1;                /* 0x006598D1 */
unsigned char g_shared_timer_flag_d2;                /* 0x006598D2 */
int g_game_time_ms;                                  /* 0x006874F7 */
int g_game_time_days;                                /* 0x00687595 */

// GLOBAL: WIZ8 0x005ec0a8
extern "C" const float g_float_005ec0a8 = 10000.0f;

// FUNCTION: WIZ8 0x00439bc0
void Function439BC0(void)
{
    g_shared_timer_paused = 1;
    if (g_shared_timer == 0) {
        return;
    }

    g_shared_timer_pause_time =
        g_shared_timer->getUTime(srTimer::TIMER_READ_DEFAULT)
        - g_shared_timer_pause_base;

    if (g_object_6598bc != 0 && (g_object_6598bc->m_flags & 8) == 0) {
        unsigned short flags = g_object_6598bc->m_flags;
        g_object_6598bc->m_flags = flags | 8;
        if (g_object_6598bc->m_clock_mode != 1) {
            if ((flags & 1) != 0) {
                g_object_6598bc->m_start =
                    g_object_6598bc->m_shared->getUTime(
                        srTimer::TIMER_READ_DEFAULT)
                    - g_object_6598bc->m_start;
            }
            else if (g_shared_timer_paused != 0) {
                g_object_6598bc->m_start =
                    g_shared_timer_pause_time - g_object_6598bc->m_start;
            }
            else {
                g_object_6598bc->m_start =
                    g_object_6598bc->m_shared->getUTime(
                        srTimer::TIMER_READ_DEFAULT)
                    - g_shared_timer_pause_base - g_object_6598bc->m_start;
            }
        }
        else {
            g_object_6598bc->m_start =
                (g_game_time_days * 86400000 + g_game_time_ms) * 10
                - g_object_6598bc->m_start;
        }
    }
}

// FUNCTION: WIZ8 0x00439ca0
void Function439CA0(void)
{
    g_shared_timer_paused = 0;
    g_shared_timer_flag_d1 = 0;
    if (g_shared_timer != 0) {
        g_shared_timer_pause_base =
            g_shared_timer->getUTime(srTimer::TIMER_READ_DEFAULT)
            - g_shared_timer_pause_time;
        g_shared_timer_pause_time = 0;
    }

    W8Object0043A910* timer = g_object_6598bc;
    if (timer != 0) {
        timer->m_flags &= ~8;
        int sample = timer->ReadClock();
        float duration = timer->m_duration_scale
                         * timer->m_duration_seconds
                         * g_float_005ec0a8;
        int start = sample - timer->m_start;
        timer->m_start = start;
        timer->m_duration = (int)duration;
        timer->m_end = start + timer->m_duration;
    }
}

// VTABLE: WIZ8 0x005ec0a4
// class W8GameTimer

// SYNTHETIC: WIZ8 0x00439750
// W8GameTimer::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00439a00
W8GameTimer::~W8GameTimer()
{
    srTimer* shared = m_shared;

    if (shared == g_shared_timer) {
        if (--g_shared_timer_refs <= 0) {
            if (g_shared_timer != 0) {
                delete g_shared_timer;
            }
            g_shared_timer = 0;
            g_shared_timer_base = 0;
            g_shared_timer_refs = 0;
        }
    }
    else if (shared != 0) {
        delete shared;
    }
    m_shared = 0;
}

// FUNCTION: WIZ8 0x00439a60
int W8GameTimer::Method00439A60()
{
    return ReadClock();
}

// FUNCTION: WIZ8 0x00439550
W8GameTimer::W8GameTimer()
{
    m_clock_mode = 0;
    m_flags = 0;
    m_shared = 0;
    m_start = 0;
    m_end = 0;
    m_duration_seconds = 0;
    m_duration_scale = 1.0f;

    if (g_shared_timer == 0) {
        g_shared_timer_paused = 0;
        g_shared_timer_flag_d1 = 0;
        g_shared_timer_flag_d2 = 0;

        srTimer* timer = new srTimer(0, 0, 1);

        g_shared_timer = timer;
        g_shared_timer_base = timer;
        /* The original stores through the new pointer without a null check,
           and the null-returning operator new makes that reachable; the port
           keeps the shape. VC6 has no unsigned __int64 to double conversion -
           C2520 - which is why the original splits the frequency into halves
           and rejoins them at 2^32; the ternary re-evaluates its else arm,
           which is what emits the second __aullshr. */
        timer->m_units_per_interval = 10000;
        {
            double frequency;

            if ((double)timer->m_frequency != 0.0) {
                frequency = (double)timer->m_frequency;
            }
            else {
                frequency = 1.0;
            }
            timer->m_units_per_tick = 10000.0 / frequency;
        }
        g_shared_timer_refs = 0;
        g_shared_timer_pause_base = 0;
        g_shared_timer_pause_time = 0;
    }
    m_shared = g_shared_timer;
    ++g_shared_timer_refs;

    m_start = ReadClock();
    m_end = m_start + 10000;
    m_duration_seconds = 1.0f;
    m_duration = 10000;
}

/* The startup status object uses the duration-bearing constructor at
   0x004397F0.  It differs from the default constructor only in the raw-time
   flag and in converting seconds to the timer's 1/10000-second units. */
// FUNCTION: WIZ8 0x004397f0
W8GameTimer::W8GameTimer(float duration, unsigned char raw_time)
{
    m_clock_mode = 0;
    m_flags = raw_time ? 1 : 0;
    m_shared = 0;
    m_start = 0;
    m_end = 0;
    m_duration_seconds = 0;
    m_duration_scale = 1.0f;

    if (g_shared_timer == 0) {
        g_shared_timer_paused = 0;
        g_shared_timer_flag_d1 = 0;
        g_shared_timer_flag_d2 = 0;
        srTimer* timer = new srTimer(0, 0, 1);
        g_shared_timer = timer;
        g_shared_timer_base = timer;
        timer->m_units_per_interval = 10000;
        timer->m_units_per_tick = timer->m_frequency != 0
            ? 10000.0 / (double)timer->m_frequency : 10000.0;
        g_shared_timer_refs = 0;
        g_shared_timer_pause_base = 0;
        g_shared_timer_pause_time = 0;
    }
    m_shared = g_shared_timer;
    ++g_shared_timer_refs;
    m_start = ReadClock();
    m_duration_seconds = duration;
    m_duration = (int)(duration * 10000.0f);
    m_end = m_start + m_duration;
}

// FUNCTION: WIZ8 0x00439b80
void W8GameTimer::SetDuration(float duration)
{
    if (duration > 0.0f) {
        m_duration_seconds = duration;
    }
    m_duration = (int)(m_duration_scale * m_duration_seconds * 10000.0f);
    m_end = m_start + m_duration;
}

// FUNCTION: WIZ8 0x00439ad0
void W8GameTimer::SetMode(int mode)
{
    m_clock_mode = mode;
    m_start = ReadClock();
    m_end = m_start + m_duration;
}

// FUNCTION: WIZ8 0x00439d80
void W8GameTimer::Restart()
{
    m_start = ReadClock();
    m_end = m_start + m_duration;
}

// FUNCTION: WIZ8 0x0043a190
float W8GameTimer::GetProgress()
{
    int sample = ReadClock();
    int start = m_start;
    int end = m_end;
    float progress = (float)(unsigned int)(sample - start) /
                     (float)(unsigned int)(end - start);
    int completed = (int)progress;

    if (completed != 0 && completed > 0) {
        m_start = (completed - 1) * m_duration + end;
        m_end = m_start + m_duration;
    }

    if ((m_flags & 8) == 0 &&
        (g_shared_timer_paused == 0 || (m_flags & 1) != 0) &&
        g_shared_timer_flag_d1 == 0 &&
        (g_shared_timer_flag_d2 == 0 || (m_flags & 1) != 0)) {
        return progress;
    }
    return 0.0f;
}

// FUNCTION: WIZ8 0x0043a290
void W8GameTimer::SetProgress(float progress)
{
    int sample = ReadClock();
    m_start = sample - (int)((float)(unsigned int)m_duration * progress);
    m_end = m_start + m_duration;
}

void* CreateGameTimer005EC0A4(float duration, unsigned char raw_time)
{
    return new W8GameTimer(duration, raw_time);
}
