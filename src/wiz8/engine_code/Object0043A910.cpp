#include "wiz8/engine_code/Object0043A910.h"

/* An engine object constructed at 0x0043A910 over W8GameTimer's base built by
   0x00439550. Nothing in the image names it - its constructor sits in a gap
   between assertion-anchored translation units and references no naming string
   - so the class carries an address-qualified positional name and its fields
   keep positional names too. What the constructor does establish is the
   layout: a float copied from a global, an integer derived from it, and four
   constants. */

extern float g_rate_006068EC;            /* 0.1f in the shipped image */

// GLOBAL: WIZ8 0x006598bc
W8Object0043A910* g_object_6598bc;

// VTABLE: WIZ8 0x005ec0ac
// class W8Object0043A910

// SYNTHETIC: WIZ8 0x0043ac40
// W8Object0043A910::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0043a910
W8Object0043A910::W8Object0043A910()
{
    m_duration_seconds = g_rate_006068EC;
    m_scale_24 = 2.0f;
    m_value_28 = 0;
    m_elapsed_ticks_2c = 0;
    m_value_30 = 0;
    m_duration_scale = 1.0f;
    m_duration = (int)(m_duration_seconds * 10000.0f);
    m_end = m_duration;
}

// FUNCTION: WIZ8 0x0043a960
void W8Object0043A910::SetDurationScale(float scale)
{
    m_duration_scale = scale;
    if (scale < 0.5f) {
        m_flags |= 0x10;
    }
    m_scale_24 = 2.0f / scale;
    m_duration = (int)(scale * m_duration_seconds * 10000.0f);
    m_start = ReadClock();
    m_end = m_start + m_duration;
    m_value_28 = 0.0f;
}

// FUNCTION: WIZ8 0x0043aa20
void W8Object0043A910::ResetDurationScale()
{
    m_flags &= ~0x10;
    m_scale_24 = 2.0f;
    m_duration_scale = 1.0f;
    m_duration = (int)(m_duration_seconds * 10000.0f);
    m_start = ReadClock();
    m_end = m_start + m_duration;
    m_value_28 = 0.0f;
}

// FUNCTION: WIZ8 0x0043aad0
float W8Object0043A910::Update()
{
    if ((m_flags & 8) != 0 ||
        (g_shared_timer_paused != 0 && (m_flags & 1) == 0) ||
        g_shared_timer_flag_d1 != 0) {
        m_value_28 = 0.0f;
    }
    else {
        int sample = ReadClock();
        m_elapsed_ticks_2c = (unsigned int)(sample - m_start);
        m_start = sample;
        m_value_28 = (float)m_elapsed_ticks_2c / (float)(unsigned int)m_duration;
        if (m_value_28 > m_scale_24) {
            m_value_28 = m_scale_24;
            m_elapsed_ticks_2c = (unsigned int)((float)(unsigned int)m_duration * m_scale_24);
        }
        m_value_30 += m_value_28;
    }
    if (g_shared_timer_flag_d2 != 0 && (m_flags & 1) == 0) {
        return 0.0f;
    }
    return m_value_28;
}

// FUNCTION: WIZ8 0x0043ac60
W8Object0043A910::~W8Object0043A910()
{
}
