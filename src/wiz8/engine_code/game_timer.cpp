#include "surrender/srTimer.h"

/* The game-timer unit: a small timer object over one shared, reference-counted
   srTimer-derived singleton. The image names neither the unit nor the classes -
   the assertion anchors only bound this code to the gap between
   Engine Code\Octree.cpp and Engine Code\BitArray.cpp - so both classes carry
   address-qualified positional names and the file name is descriptive.

   The globals live at 0x006598B8..0x006598D2 and are declared, not defined:
   their addresses in the original data segment are the authority. */

extern srTimer* g_shared_timer_base;   /* 0x006598B8 */
extern srTimer* g_shared_timer;        /* 0x006598C0 */
extern int g_shared_timer_pause_base;                /* 0x006598C4 */
extern int g_shared_timer_pause_time;                /* 0x006598C8 */
extern int g_shared_timer_refs;                      /* 0x006598CC */
extern unsigned char g_shared_timer_paused;          /* 0x006598D0 */
extern unsigned char g_shared_timer_flag_d1;         /* 0x006598D1 */
extern unsigned char g_shared_timer_flag_d2;         /* 0x006598D2 */
extern int g_game_time_ms;                           /* 0x006874F7 */
extern int g_game_time_days;                         /* 0x00687595 */

/* The 0x24-byte wrapper. Mode 1 reads the game-world clock; mode 0 reads the
   shared timer, frozen while the pause flag is set; the low flag bit bypasses
   the pause bookkeeping and reads the shared timer raw. */
class W8Timer005EC0A4 {
public:
    W8Timer005EC0A4();

    // W8Timer005EC0A4::~W8Timer005EC0A4 is the 94-byte body at 0x00439A00.
    // Defined in the class so VC6 also folds it into the deleting destructor
    // at 0x00439750, whose 120 bytes are this teardown plus the conditional
    // free; an out-of-line definition emits a call there instead.
    // FUNCTION: WIZ8 0x00439A00
    virtual ~W8Timer005EC0A4()
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

typedef char W8Timer005EC0A4_must_be_0x24[
    sizeof(W8Timer005EC0A4) == 0x24 ? 1 : -1];

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
