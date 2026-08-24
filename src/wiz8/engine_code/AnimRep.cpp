#include "surrender/srTimer.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/geometry.h"
#include "wiz8/sr_api.h"

extern float g_lod_range_default_0060e608;
extern float g_lod_range_default_0060e60c;

// VTABLE: WIZ8 0x005ec1d8 W8AnimRepBase005EC1D8
// class W8AnimRepBase005EC1D8

// VTABLE: WIZ8 0x005ed050 W8AnimRep005ED050
// class W8AnimRep005ED050

// VTABLE: WIZ8 0x005ed058 W8EmitterHost
// class W8EmitterHost

// FUNCTION: WIZ8 0x004b86e0
W8AnimRepBase005EC1D8::W8AnimRepBase005EC1D8()
{
    location_004.x = 0.0f;
    location_004.y = 0.0f;
    location_004.z = 0.0f;
    local_location_010.x = 0.0f;
    local_location_010.y = 0.0f;
    local_location_010.z = 0.0f;
    parent_location_01c.x = 0.0f;
    parent_location_01c.y = 0.0f;
    parent_location_01c.z = 0.0f;
    rotation_028.vectors[0].x = 1.0f;
    rotation_028.vectors[0].y = 0.0f;
    rotation_028.vectors[0].z = 0.0f;
    rotation_028.vectors[1].x = 0.0f;
    rotation_028.vectors[1].y = 1.0f;
    rotation_028.vectors[1].z = 0.0f;
    rotation_028.vectors[2].x = 0.0f;
    rotation_028.vectors[2].y = 0.0f;
    rotation_028.vectors[2].z = 1.0f;
    value_04c.value_00 = 0;
    value_04c.value_04 = 0;
    value_04c.value_08 = 0;
    value_04c.value_0c = 0;
    value_05c = 1.0f;
    flag_060 = 0;
    flag_061 = 0;
}

// FUNCTION: WIZ8 0x004b55c0
void W8AnimRep005ED050::SetFrameMethod004B55C0(signed char method)
{
    if (method < 1 || method > 4) {
        srAssertFail(
            "bFrameMethod >= DIR_FIRST && bFrameMethod <= DIR_LAST",
            "C:\\Projects\\Wizardry 8\\Engine Code\\AnimRep.cpp",
            0x6a,
            0);
    }
    flag_06f = method;
}

/* The root copy preserves five aggregate values, but deliberately resets its
   runtime scale and flags instead of copying them.  Bytes 0x62 and 0x63 are
   not touched by the canonical constructor. */
// FUNCTION: WIZ8 0x004b87c0
W8AnimRepBase005EC1D8::W8AnimRepBase005EC1D8(
    const W8AnimRepBase005EC1D8& other)
{
    location_004 = other.location_004;
    local_location_010 = other.local_location_010;
    parent_location_01c = other.parent_location_01c;
    rotation_028 = other.rotation_028;
    value_04c = other.value_04c;
    value_05c = 1.0f;
    flag_060 = 0;
    flag_061 = 0;
}

/* Store the representation's local location, then rebuild the world location
   from its parent location.  The three floating-point additions establish
   these as vectors rather than opaque twelve-byte values. */
// FUNCTION: WIZ8 0x004b8850
void W8AnimRepBase005EC1D8::SetLocation004B8850(
    const srVector3T<float>* location)
{
    local_location_010.x = location->x;
    local_location_010.y = location->y;
    local_location_010.z = location->z;
    location_004.x = parent_location_01c.x + local_location_010.x;
    location_004.y = parent_location_01c.y + local_location_010.y;
    location_004.z = parent_location_01c.z + local_location_010.z;
}

// FUNCTION: WIZ8 0x004b8890
void W8AnimRepBase005EC1D8::GetLocation004B8890(
    srVector3T<float>* location) const
{
    location->x = location_004.x;
    location->y = location_004.y;
    location->z = location_004.z;
}

// FUNCTION: WIZ8 0x004b88b0
void W8AnimRepBase005EC1D8::GetLocalLocation004B88B0(
    srVector3T<float>* location) const
{
    location->x = local_location_010.x;
    location->y = local_location_010.y;
    location->z = local_location_010.z;
}

// FUNCTION: WIZ8 0x004b88d0
void W8AnimRepBase005EC1D8::SetRotation004B88D0(
    const srMatrix3T<float>* rotation)
{
    rotation_028 = *rotation;
}

// FUNCTION: WIZ8 0x004b88f0
void W8AnimRepBase005EC1D8::GetRotation004B88F0(
    srMatrix3T<float>* rotation)
{
    *rotation = rotation_028;
}

// FUNCTION: WIZ8 0x004b53d0
W8AnimRep005ED050::W8AnimRep005ED050()
{
    flag_064 = 0;
    value_066 = 0xffff;
    timer_068 = 0;
    active = 0;
    flag_06d = 0;
    flag_06e = 0;
    flag_06f = 0;
    flag_070 = 0;
    behaviour_071 = -1;
    value_074.value_00 = 0;
    value_074.value_04 = 0;
    value_074.value_08 = 0;
    value_080.value_00 = 0;
    value_080.value_04 = 0;
    value_080.value_08 = 0;
    value_08c = 0;
    value_090 = 0;
    counter_094 = 0xff;
    counter_095 = 0xff;
    if (g_shared_timer_base == 0) {
        srAssertFail(
            "gpsrTimer",
            "C:\\Projects\\Wizardry 8\\Engine Code\\AnimRep.cpp",
            0x4e,
            0);
    }
    timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
}

/* Vtable 0x005EC1D8's clone slot allocates exactly the root's 0x64-byte
   extent, then invokes the copy constructor above. */
// FUNCTION: WIZ8 0x0044edf0
W8AnimRepBase005EC1D8* W8AnimRepBase005EC1D8::Clone()
{
    return new W8AnimRepBase005EC1D8(*this);
}

/* VC6 emits the scalar-deleting wrapper below from this ordinary source
   destructor.  The synthetic marker identifies the generated wrapper; it is
   not an address claim for the source destructor itself. */
// SYNTHETIC: WIZ8 0x0044ee50
// W8AnimRepBase005EC1D8::`scalar deleting destructor'
W8AnimRepBase005EC1D8::~W8AnimRepBase005EC1D8()
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
    value_074 = other.value_074;
    value_080 = other.value_080;
    value_08c = other.value_08c;
    value_090 = other.value_090;
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

/* The FUNCTION marker owns the complete destructor body.  The separate
   SYNTHETIC marker records the deleting wrapper VC6 generates for vtable slot
   zero. */
// SYNTHETIC: WIZ8 0x004b5760
// W8AnimRep005ED050::`scalar deleting destructor'
// FUNCTION: WIZ8 0x0044ef20
W8AnimRep005ED050::~W8AnimRep005ED050()
{
}

/* The abstract emitter host copies its stable settings, but starts with no
   selected emitter and the canonical 00 00 FF FF transient byte pattern. */
// FUNCTION: WIZ8 0x004b5680
W8EmitterHost::W8EmitterHost(const W8EmitterHost& other)
    : W8AnimRep005ED050(other)
{
    m_bLOD = other.m_bLOD;
    lod_range_09c = other.lod_range_09c;
    lod_range_0a0 = other.lod_range_0a0;
    current_cycle = 0;
    current_subcycle = 0;
    selection_value_0a6 = -1;
    pending_cycle = -1;
    value_0a8 = other.value_0a8;
}

// FUNCTION: WIZ8 0x004b5600
W8EmitterHost::W8EmitterHost()
{
    m_bLOD = 0;
    lod_range_09c = g_lod_range_default_0060e608;
    lod_range_0a0 = g_lod_range_default_0060e60c;
    current_cycle = 0;
    current_subcycle = 0;
    selection_value_0a6 = -1;
    pending_cycle = -1;
    value_0a8 = 0;
}

/* As above, 0x004B56F0 is the complete destructor and 0x004B5660 is its
   compiler-generated scalar-deleting wrapper. */
// SYNTHETIC: WIZ8 0x004b5660
// W8EmitterHost::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004b56f0
W8EmitterHost::~W8EmitterHost()
{
}
