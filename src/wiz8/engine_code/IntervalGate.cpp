#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

/* A one-slot polymorphic class at vtable 0x005EBCFC over the timer whose
   destructor is 0x00439A00. Its whole teardown is the two instructions an
   empty derived destructor emits: restore the class vtable, tail-jump to the
   timer destructor.

   The image also holds an adjustor form at 0x00421890 - the same body behind
   `add ecx, 0xc4` - so this class is embedded at +0xc4 of some larger object.
   GDCamera's constructor is now the direct allocation witness for its 0x28
   extent and the three-argument constructor below.

   Nothing names the class, so it is qualified by its vtable address. */

// FUNCTION: WIZ8 0x0043a4e0
W8IntervalGate::W8IntervalGate()
    : W8GameTimer(1.0f, 0), m_finished(0)
{
}

// FUNCTION: WIZ8 0x0043a500
W8IntervalGate::W8IntervalGate(
    float duration, unsigned char raw_time, unsigned char set_flag_2)
    : W8GameTimer(duration, raw_time), m_finished(0)
{
    if (set_flag_2) {
        m_flags |= 2;
    }
}

// FUNCTION: WIZ8 0x004218d0
W8IntervalGate::~W8IntervalGate()
{
}

// FUNCTION: WIZ8 0x0043A530
void W8IntervalGate::Arm()
{
    m_finished = 0;
    m_start = ReadClock();
    m_end = m_duration + m_start;
}

// FUNCTION: WIZ8 0x0043A5D0
unsigned int W8IntervalGate::PollElapsedIntervals()
{
    if (m_finished != 0) {
        return 1;
    }
    unsigned int intervals = (unsigned int)(ReadClock() - m_start)
                             / (unsigned int)(m_end - m_start);
    if ((int)intervals > 0) {
        if ((m_flags & 2) != 0) {
            m_finished = 1;
            return intervals;
        }
        m_start = (intervals - 1) * m_duration + m_end;
        m_end = m_start + m_duration;
    }
    return intervals;
}

// FUNCTION: WIZ8 0x0043a690
unsigned char W8IntervalGate::Load(int handle)
{
    if ((m_flags & 2) == 0) {
        return W8GameTimer::Load(handle);
    }
    unsigned char loaded = ReadVirtualFile(
        handle, &m_duration_seconds, sizeof(m_duration_seconds), 0);
    if (loaded != 0) {
        m_start = ReadClock();
        m_duration = (int)(m_duration_seconds * m_duration_scale * 10000.0f);
        m_end = m_start + m_duration;
    }
    return loaded;
}

// FUNCTION: WIZ8 0x0043a770
unsigned char W8IntervalGate::Save(int handle)
{
    float elapsed;
    if ((m_flags & 2) == 0) {
        int sample = ReadClock();
        float progress = (float)(unsigned int)(sample - m_start) /
                         (float)(unsigned int)(m_end - m_start);
        // The on-disk pair and its OR-combined result mirror W8GameTimer::Load.
        unsigned char saved = FileWrite(handle, &progress, sizeof(progress), 0);
        saved |= FileWrite(handle, &m_duration_scale, sizeof(m_duration_scale), 0);
        return saved;
    }
    if (m_finished != 0) {
        elapsed = 0.0f;
    }
    else {
        elapsed = GetElapsedSeconds();
    }
    return FileWrite(handle, &elapsed, sizeof(elapsed), 0);
}
