#include "surrender/srTimer.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/sr_api.h"

#include <string.h>

extern srTimer* g_shared_timer_base;

/* The root copy keeps the opaque animation representation, but deliberately
   resets its runtime scale and flags instead of copying them.  Bytes 0x62 and
   0x63 are not touched by the canonical constructor. */
// FUNCTION: WIZ8 0x004b87c0
W8AnimRepBase005EC1D8::W8AnimRepBase005EC1D8(
    const W8AnimRepBase005EC1D8& other)
{
    memcpy(unknown_004, other.unknown_004, sizeof(unknown_004));
    value_05c = 1.0f;
    flag_060 = 0;
    flag_061 = 0;
}

/* Vtable 0x005EC1D8's clone slot allocates exactly the root's 0x64-byte
   extent, then invokes the copy constructor above. */
// FUNCTION: WIZ8 0x0044edf0
W8AnimRepBase005EC1D8* W8AnimRepBase005EC1D8::Clone()
{
    return new W8AnimRepBase005EC1D8(*this);
}

// SYNTHETIC: WIZ8 0x0044ee50
// W8AnimRepBase005EC1D8::`scalar deleting destructor'
__forceinline W8AnimRepBase005EC1D8::~W8AnimRepBase005EC1D8()
{
}

/* AnimRep.cpp copies persistent animation state, then timestamps the new
   representation from the shared SurRender timer.  The source assertion names
   that global `gpsrTimer`. */
// FUNCTION: WIZ8 0x004b54a0
W8AnimRep005ED050::W8AnimRep005ED050(const W8AnimRep005ED050& other)
    : W8AnimRepBase005EC1D8(other)
{
    flag_064 = other.flag_064;
    value_066 = other.value_066;
    timer_068 = other.timer_068;
    active = other.active;
    flag_06d = other.flag_06d;
    flag_06e = other.flag_06e;
    flag_06f = other.flag_06f;
    flag_070 = other.flag_070;
    behaviour_071 = other.behaviour_071;
    memcpy(values_074, other.values_074, sizeof(values_074));
    counter_094 = other.counter_094;
    counter_095 = other.counter_095;

    if (g_shared_timer_base == 0) {
        srAssertFail(
            "gpsrTimer",
            "C:\\Projects\\Wizardry 8\\Engine Code\\AnimRep.cpp",
            100,
            0);
    }
    timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
}

// SYNTHETIC: WIZ8 0x004b5760
// W8AnimRep005ED050::`scalar deleting destructor'
// FUNCTION: WIZ8 0x0044ef20
__forceinline W8AnimRep005ED050::~W8AnimRep005ED050()
{
}

/* The abstract emitter host copies its stable settings, but starts with no
   selected emitter and the canonical 00 00 FF FF transient byte pattern. */
// FUNCTION: WIZ8 0x004b5680
W8EmitterHost::W8EmitterHost(const W8EmitterHost& other)
    : W8AnimRep005ED050(other)
{
    setting_98 = other.setting_98;
    value_09c = other.value_09c;
    value_0a0 = other.value_0a0;
    emitter_index = 0;
    unknown_0a5[0] = 0;
    unknown_0a5[1] = 0xff;
    unknown_0a5[2] = 0xff;
    value_0a8 = other.value_0a8;
}

// SYNTHETIC: WIZ8 0x004b5660
// W8EmitterHost::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004b56f0
__forceinline W8EmitterHost::~W8EmitterHost()
{
}
