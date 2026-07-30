#include "wiz8/engine_code/Object005EBCFC.h"

/* A one-slot polymorphic class at vtable 0x005EBCFC over the timer whose
   destructor is 0x00439A00. Its whole teardown is the two instructions an
   empty derived destructor emits: restore the class vtable, tail-jump to the
   timer destructor.

   The image also holds an adjustor form at 0x00421890 - the same body behind
   `add ecx, 0xc4` - so this class is embedded at +0xc4 of some larger object.
   GDCamera's constructor is now the direct allocation witness for its 0x28
   extent and the three-argument constructor below.

   Nothing names the class, so it is qualified by its vtable address. */

// FUNCTION: WIZ8 0x0043a500
W8Object005EBCFC::W8Object005EBCFC(
    float duration, unsigned char raw_time, unsigned char set_flag_2)
    : W8Timer005EC0A4(duration, raw_time), m_positional_024(0)
{
    if (set_flag_2) {
        m_flags |= 2;
    }
}

// FUNCTION: WIZ8 0x004218d0
W8Object005EBCFC::~W8Object005EBCFC()
{
}

// FUNCTION: WIZ8 0x0043A530
void W8Object005EBCFC::Method0043A530()
{
    m_positional_024 = 0;
    m_start = Sample();
    m_end = m_duration + m_start;
}

// FUNCTION: WIZ8 0x0043A5D0
unsigned int W8Object005EBCFC::Method0043A5D0()
{
    if (m_positional_024 != 0) {
        return 1;
    }
    unsigned int intervals = (unsigned int)(Sample() - m_start)
                             / (unsigned int)(m_end - m_start);
    if ((int)intervals > 0) {
        if ((m_flags & 2) != 0) {
            m_positional_024 = 1;
            return intervals;
        }
        m_start = (intervals - 1) * m_duration + m_end;
        m_end = m_start + m_duration;
    }
    return intervals;
}
