/* Recoverable from sr.dll */

#include "surrender/srVariableTimer.h"

namespace {

unsigned __int64 quadWord64(const srQuadWord& value)
{
    return ((unsigned __int64)value.hi << 0x20) | value.lo;
}

} // namespace

// FUNCTION: SURRENDER 0x100632C0
srVariableTimer::srVariableTimer(int a0, int a1, int a2, float multiplier, unsigned long step_size)
    : srTimer(a0, a1, a2)
{
    m_multiplier = multiplier;
    m_step_size = step_size;
    m_stepping = 0;
    m_step_scale = 1.0f / step_size;
    m_prev_tick = m_tick;
    m_scaled_base = m_base;
    m_scaled_tick = m_tick;
    m_step_ticks = m_frequency;
    m_step_ticks = static_cast<unsigned __int64>((double)m_step_ticks * m_step_scale);
}

// FUNCTION: SURRENDER 0x10063450
srVariableTimer::srVariableTimer(const srVariableTimer& other) : srTimer(other)
{
    m_multiplier = other.m_multiplier;
    m_step_size = other.m_step_size;
    m_step_scale = other.m_step_scale;
    m_stepping = other.m_stepping;
    m_scaled_tick = other.m_scaled_tick;
    m_scaled_base = other.m_scaled_base;
    m_prev_tick = other.m_prev_tick;
    m_step_ticks = other.m_step_ticks;
}

// FUNCTION: SURRENDER 0x10063550
srVariableTimer::srVariableTimer(const srTimer& other) : srTimer(other)
{
    m_multiplier = 1.0f;
    m_step_size = 0x1e;
    m_step_scale = 1.0f / 30.0f;
    m_stepping = 0;
    m_prev_tick = m_tick;
    m_scaled_base = m_base;
    m_scaled_tick = m_tick;
    m_step_ticks = m_frequency;
    m_step_ticks = static_cast<unsigned __int64>((double)m_step_ticks * m_step_scale);
}

// FUNCTION: SURRENDER 0x10063600
srVariableTimer& srVariableTimer::operator=(const srTimer& other)
{
    int index;
    for (index = 0; index < 0x400; ++index) {
        m_ident[index] = other.m_ident[index];
    }
    for (index = 0; index < 0x400; ++index) {
        m_cpu_ident[index] = other.m_cpu_ident[index];
    }
    m_frequency = other.m_frequency;
    m_base = other.m_base;
    m_tick = other.m_tick;
    m_pause = other.m_pause;
    m_units_per_interval = other.m_units_per_interval;
    m_seconds_per_tick = other.m_seconds_per_tick;
    m_units_per_tick = other.m_units_per_tick;
    m_cpu_count = other.m_cpu_count;
    m_read_tick = other.m_read_tick;
    m_kernel32 = other.m_kernel32 == 0 ? 0 : (void*)LoadLibraryA("kernel32");
    for (index = 0; index < 0xd; ++index) {
        m_cpu_vendor[index] = other.m_cpu_vendor[index];
    }
    m_cpu_max_id = other.m_cpu_max_id;
    m_cpu_signature = other.m_cpu_signature;
    m_cpu_features = other.m_cpu_features;
    m_prev_tick = m_tick;
    m_step_ticks = m_frequency;
    m_step_ticks = static_cast<unsigned __int64>((double)m_step_ticks * m_step_scale);
    return *this;
}

// FUNCTION: SURRENDER 0x100637A0
srVariableTimer& srVariableTimer::operator=(const srVariableTimer& other)
{
    srTimer temp(other);
    *this = temp;
    m_multiplier = other.m_multiplier;
    m_step_size = other.m_step_size;
    m_step_scale = other.m_step_scale;
    m_stepping = other.m_stepping;
    m_scaled_tick = other.m_scaled_tick;
    m_scaled_base = other.m_scaled_base;
    m_prev_tick = other.m_prev_tick;
    m_step_ticks = other.m_step_ticks;
    return *this;
}

// FUNCTION: SURRENDER 0x10063870
int srVariableTimer::reset(int a0, int a1, int a2)
{
    if (!srTimer::reset(a0, a1, a2))
        return 0;
    m_multiplier = 1.0f;
    m_step_size = 0x1e;
    m_step_scale = 1.0f / 30.0f;
    m_stepping = 0;
    m_scaled_base = m_base;
    m_scaled_tick = m_tick;
    m_prev_tick = m_tick;
    m_step_ticks = m_frequency;
    m_step_ticks = static_cast<unsigned __int64>((double)m_step_ticks * m_step_scale);
    return 1;
}

// FUNCTION: SURRENDER 0x10063990
int srVariableTimer::reset(float multiplier, unsigned long step_size, int a2, int a3, int a4)
{
    if (!srTimer::reset(a2, a3, a4))
        return 0;
    m_multiplier = multiplier;
    m_step_size = step_size;
    m_stepping = 0;
    m_step_scale = 1.0f / step_size;
    m_scaled_base = m_base;
    m_scaled_tick = m_tick;
    m_prev_tick = m_tick;
    m_step_ticks = m_frequency;
    m_step_ticks = static_cast<unsigned __int64>((double)m_step_ticks * m_step_scale);
    return 1;
}

// FUNCTION: SURRENDER 0x10063AB0
void srVariableTimer::addTime(float time)
{
    unsigned __int64 delta;
    srQuadWord scaled;

    delta = static_cast<unsigned __int64>((double)m_frequency * time);
    scaled.lo = (unsigned long)delta;
    scaled.hi = (unsigned long)(delta >> 0x20);
    m_scaled_tick += scaled;
}

// FUNCTION: SURRENDER 0x10063B10
void srVariableTimer::setTime(float time)
{
    unsigned __int64 delta;
    srQuadWord scaled;

    delta = static_cast<unsigned __int64>((double)m_frequency * time);
    scaled.lo = (unsigned long)delta;
    scaled.hi = (unsigned long)(delta >> 0x20);
    m_scaled_tick = scaled;
    m_scaled_tick += m_scaled_base;
}

/* Retail updates m_step_size and m_step_scale here but deliberately does
   not recompute m_step_ticks (0x10063B90 writes only +0x890/+0x88c after
   setting the in-step flag): the running quantum keeps the step size that
   was current at construction/reset until the next reset derives
   m_step_ticks = m_frequency * m_step_scale. stepForward/stepBack read the
   stale quantum for the remainder of the step session — confirmed retail
   behavior, not a missed update. m_step_scale is consumed only by those
   tick derivations and assignment. */
// FUNCTION: SURRENDER 0x10063B90
int srVariableTimer::stepBegin(unsigned long step_size)
{
    if ((m_pause.lo == 0 && m_pause.hi == 0) && m_stepping == 0) {
        m_stepping = 1;
        if (step_size != 0) {
            m_step_size = step_size;
            m_step_scale = 1.0f / step_size;
        }
        (*m_read_tick)(&m_pause);
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10063C00
unsigned long srVariableTimer::stepEnd()
{
    srQuadWord now;
    srQuadWord delta;
    unsigned long result;

    if (m_stepping == 0)
        return 0;
    m_stepping = 0;
    (*m_read_tick)(&now);
    delta = now - m_pause;
    m_scaled_base += delta;
    m_pause.lo = 0;
    m_pause.hi = 0;
    result = (unsigned long)(quadWord64(delta) * static_cast<unsigned long>(m_units_per_interval) /
                             quadWord64(m_frequency));
    return result;
}

// FUNCTION: SURRENDER 0x10063DC0
int srVariableTimer::pause()
{
    if (m_stepping != 0)
        return 0;
    return srTimer::pause();
}

// FUNCTION: SURRENDER 0x10063DE0
unsigned long srVariableTimer::resume()
{
    if (m_stepping != 0)
        return 0;
    return srTimer::resume();
}

// FUNCTION: SURRENDER 0x10063E00
unsigned long srVariableTimer::getUTime(e_timerReadControl control)
{
    srQuadWord delta;
    unsigned __int64 scaled;
    srQuadWord add;

    if (m_stepping == 0 && control == TIMER_READ_DEFAULT) {
        m_prev_tick = m_tick;
        (*m_read_tick)(&m_tick);
        delta = m_tick - m_prev_tick;
        scaled = static_cast<unsigned __int64>((double)delta * m_multiplier);
        add.lo = (unsigned long)scaled;
        add.hi = (unsigned long)(scaled >> 0x20);
        m_scaled_tick += add;
    }
    return (unsigned long)((m_scaled_tick - m_scaled_base) * m_units_per_tick);
}

// FUNCTION: SURRENDER 0x10063EE0
double srVariableTimer::getTime(e_timerReadControl control)
{
    getUTime(control);
    return (m_scaled_tick - m_scaled_base) * m_seconds_per_tick;
}

// FUNCTION: SURRENDER 0x10063F40
srVariableTimer::~srVariableTimer() {}

// FUNCTION: SURRENDER 0x10063F70
int srVariableTimer::is_stepping() const
{
    return m_stepping;
}

// FUNCTION: SURRENDER 0x10063F80
float srVariableTimer::getMultiplier() const
{
    return m_multiplier;
}

// FUNCTION: SURRENDER 0x10063F90
void srVariableTimer::setMultiplier(float multiplier)
{
    m_multiplier = multiplier;
}

// FUNCTION: SURRENDER 0x10063FA0
void srVariableTimer::resetMultiplier()
{
    m_multiplier = 1.0f;
}

// FUNCTION: SURRENDER 0x10063FB0
void srVariableTimer::stepBack(unsigned long steps)
{
    unsigned __int64 delta;
    srQuadWord back;

    delta = quadWord64(m_step_ticks) * steps;
    back.lo = (unsigned long)delta;
    back.hi = (unsigned long)(delta >> 0x20);
    m_scaled_tick = m_scaled_tick - back;
}

// FUNCTION: SURRENDER 0x10063FE0
void srVariableTimer::stepForward(unsigned long steps)
{
    unsigned __int64 delta;
    srQuadWord forward;

    delta = quadWord64(m_step_ticks) * steps;
    forward.lo = (unsigned long)delta;
    forward.hi = (unsigned long)(delta >> 0x20);
    m_scaled_tick += forward;
}

// FUNCTION: SURRENDER 0x10064010
unsigned long srVariableTimer::getMsTime(e_timerReadControl control)
{
    getUTime(control);
    return (unsigned long)(quadWord64(m_scaled_tick - m_scaled_base) * 1000 /
                           quadWord64(m_frequency));
}

// FUNCTION: SURRENDER 0x10064070
unsigned long srVariableTimer::getUTime(srQuadWord& out, e_timerReadControl control)
{
    getUTime(control);
    unsigned __int64 units = (unsigned __int64)((m_scaled_tick - m_scaled_base) * m_units_per_tick);
    out.lo = (unsigned long)units;
    out.hi = (unsigned long)(units >> 0x20);
    return out.lo;
}

// FUNCTION: SURRENDER 0x100640E0
char* srVariableTimer::getAscTime(char* buffer, e_timerReadControl control)
{
    unsigned __int64 units;
    srQuadWord ticks;

    getUTime(control);
    units = quadWord64(m_scaled_tick - m_scaled_base) *
            static_cast<unsigned long>(m_units_per_interval) / quadWord64(m_frequency);
    ticks.lo = (unsigned long)units;
    ticks.hi = (unsigned long)(units >> 0x20);
    return srTimer::getAscTime(buffer, ticks);
}

// FUNCTION: SURRENDER 0x10064150
unsigned long srVariableTimer::getRawTime(e_timerReadControl control)
{
    getUTime(control);
    return m_scaled_tick.lo - m_scaled_base.lo;
}

/* Confirmed unusual retail contract (0x10064190): the out parameter receives
   the absolute m_scaled_tick, while the return value is the low dword of the
   elapsed m_scaled_tick - m_scaled_base. The two outputs describe different
   quantities — this is retail's store sequence, not a recovery slip. */
// FUNCTION: SURRENDER 0x10064190
unsigned long srVariableTimer::getRawTime(srQuadWord& out, e_timerReadControl control)
{
    getUTime(control);
    out = m_scaled_tick;
    return m_scaled_tick.lo - m_scaled_base.lo;
}

// FUNCTION: SURRENDER 0x100641E0
unsigned long srVariableTimer::getBaseMsTime(e_timerReadControl control)
{
    return srTimer::getMsTime(control);
}

// FUNCTION: SURRENDER 0x100641F0
double srVariableTimer::getBaseTime(e_timerReadControl control)
{
    return srTimer::getTime(control);
}

// FUNCTION: SURRENDER 0x10064200
unsigned long srVariableTimer::getBaseUTime(e_timerReadControl control)
{
    return srTimer::getUTime(control);
}

// FUNCTION: SURRENDER 0x10064210
unsigned long srVariableTimer::getBaseUTime(srQuadWord& out, e_timerReadControl control)
{
    return srTimer::getUTime(out, control);
}

// FUNCTION: SURRENDER 0x10064230
char* srVariableTimer::getBaseAscTime(char* buffer, e_timerReadControl control)
{
    return srTimer::getAscTime(buffer, control);
}

// FUNCTION: SURRENDER 0x10064250
unsigned long srVariableTimer::getBaseRawTime(e_timerReadControl control)
{
    return srTimer::getRawTime(control);
}

// FUNCTION: SURRENDER 0x100642A0
unsigned long srVariableTimer::getBaseRawTime(srQuadWord& out, e_timerReadControl control)
{
    return srTimer::getRawTime(out, control);
}

// FUNCTION: SURRENDER 0x10063CA0
std::ostream& operator<<(std::ostream& stream, const srVariableTimer& timer)
{
    int mode = stream.width();
    switch (mode) {
    case 0:
    case 1: {
        stream.width(mode);
        srTimer base(timer);
        stream << base;
    } break;
    case 2:
        stream << timer.m_step_size;
        break;
    case 3: {
        long flags = stream.flags();
        stream.flags((flags & ~0x1000) | 0x2000);
        stream.width(3);
        stream << timer.m_multiplier;
        stream.flags(stream.flags() | (flags & 0x7fff));
    } break;
    }
    stream.width(mode);
    return stream;
}
