#include "wiz8/engine_code/game_timer.h"

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

// Defined with forced inlining so VC6 folds the teardown into the deleting
// destructor at 0x00439750; a normal out-of-line definition emits a call.
// FUNCTION: WIZ8 0x00439A00
__forceinline W8Timer005EC0A4::~W8Timer005EC0A4()
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

/* The current time in the wrapper's units. The game clock is carried as days
   and milliseconds and scaled to tenths of a millisecond; the shared timer is
   rebased while unpaused and frozen while paused, except under the raw flag. */
__forceinline int W8Timer005EC0A4::Sample() const
{
    switch (m_mode) {
    case 1:
        return (g_game_time_days * 86400000 + g_game_time_ms) * 10;
    }
    if ((m_flags & 1) == 0) {
        if (g_shared_timer_paused != 0) {
            return g_shared_timer_pause_time;
        }
        return m_shared->getUTime(srTimer::TIMER_READ_DEFAULT) -
               g_shared_timer_pause_base;
    }
    return m_shared->getUTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: WIZ8 0x00439550
W8Timer005EC0A4::W8Timer005EC0A4()
{
    m_mode = 0;
    m_flags = 0;
    m_shared = 0;
    m_start = 0;
    m_end = 0;
    m_speed = 0;
    m_speed_2 = 1.0f;

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

    m_start = Sample();
    m_end = m_start + 10000;
    m_speed = 1.0f;
    m_duration = 10000;
}

/* The startup status object uses the duration-bearing constructor at
   0x004397F0.  It differs from the default constructor only in the raw-time
   flag and in converting seconds to the timer's 1/10000-second units. */
W8Timer005EC0A4::W8Timer005EC0A4(float duration, unsigned char raw_time)
{
    m_mode = 0;
    m_flags = raw_time ? 1 : 0;
    m_shared = 0;
    m_start = 0;
    m_end = 0;
    m_speed = 0;
    m_speed_2 = 1.0f;

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
    m_start = Sample();
    m_speed = duration;
    m_duration = (int)(duration * 10000.0f);
    m_end = m_start + m_duration;
}

void* CreateGameTimer005EC0A4(float duration, unsigned char raw_time)
{
    return new W8Timer005EC0A4(duration, raw_time);
}
