#include "wiz8/engine_code/Object0043A910.h"

/* An engine object constructed at 0x0043A910 over the 0x14-byte base built by
   0x00439550. Nothing in the image names it - its constructor sits in a gap
   between assertion-anchored translation units and references no naming string
   - so the class carries an address-qualified positional name and its fields
   keep positional names too. What the constructor does establish is the
   layout: a float copied from a global, an integer derived from it, and four
   constants. */

extern float g_rate_006068EC;            /* 0.1f in the shipped image */

// FUNCTION: WIZ8 0x0043a910
W8Object0043A910::W8Object0043A910()
{
    m_rate_1c = g_rate_006068EC;
    m_scale_24 = 2.0f;
    m_value_28 = 0;
    m_value_2c = 0;
    m_value_30 = 0;
    m_scale_20 = 1.0f;
    m_ticks_18 = (int)(m_rate_1c * 10000.0f);
    m_ticks_14 = m_ticks_18;
}
