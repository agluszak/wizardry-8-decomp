#include "wiz8/engine_code/AnimRep.hpp"
#include "surrender/srTimer.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/geometry.h"
#include "wiz8/sr_api.h"

// GLOBAL: WIZ8 0x0060e608
float g_lod_range_default0 = 3500.0f;

// GLOBAL: WIZ8 0x0060e60c
float g_lod_range_default1 = 8500.0f;

// VTABLE: WIZ8 0x005ec1d8 W8AnimRepBase
// class W8AnimRepBase

// VTABLE: WIZ8 0x005ed050 W8AnimRep
// class W8AnimRep

// VTABLE: WIZ8 0x005ed058 W8EmitterHost
// class W8EmitterHost

// FUNCTION: WIZ8 0x004b86e0
W8AnimRepBase::W8AnimRepBase()
{
    location.SetZero();
    local_location.SetZero();
    parent_location.SetZero();
    rotation.SetIdentity();
    highlight_colour.Set(0.0f, 0.0f, 0.0f, 0.0f);
    instance_scale = 1.0f;
    flag = false;
    apply_instance_scale = false;
}

// FUNCTION: WIZ8 0x004b55c0
void W8AnimRep::SetFrameMethod(signed char method)
{
    if (method < 1 || method > 4) {
        srAssertFail("bFrameMethod >= DIR_FIRST && bFrameMethod <= DIR_LAST",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\AnimRep.cpp", 0x6a, 0);
    }
    frame_method = method;
}

/* The root copy preserves five aggregate values, but deliberately resets its
   runtime scale and flags instead of copying them.  Bytes 0x62 and 0x63 are
   not touched by the canonical constructor. */
// FUNCTION: WIZ8 0x004b87c0
W8AnimRepBase::W8AnimRepBase(const W8AnimRepBase& other)
    : location(other.location), local_location(other.local_location),
      parent_location(other.parent_location), rotation(other.rotation),
      highlight_colour(other.highlight_colour), instance_scale(1.0f), flag(false),
      apply_instance_scale(0)
{
}

/* Store the representation's local location, then rebuild the world location
   from its parent location.  The three floating-point additions establish
   these as vectors rather than opaque twelve-byte values. */
// FUNCTION: WIZ8 0x004b8850
void W8AnimRepBase::SetLocation(const srVector3T<float>* location)
{
    local_location = *location;
    this->location = parent_location + local_location;
}

// FUNCTION: WIZ8 0x004b8890
void W8AnimRepBase::GetLocation(srVector3T<float>* location) const
{
    *location = this->location;
}

// FUNCTION: WIZ8 0x004b88b0
void W8AnimRepBase::GetLocalLocation(srVector3T<float>* location) const
{
    *location = local_location;
}

// FUNCTION: WIZ8 0x004b88d0
void W8AnimRepBase::SetRotation(const srMatrix3T<float>* rotation)
{
    this->rotation = *rotation;
}

// FUNCTION: WIZ8 0x004b88f0
void W8AnimRepBase::GetRotation(srMatrix3T<float>* rotation)
{
    *rotation = this->rotation;
}

// FUNCTION: WIZ8 0x004b53d0
W8AnimRep::W8AnimRep()
    : subcycle(0), pending_subcycle(0xffff), timer(0), active(0), animation_playing(0),
      frame_direction(W8_ANIMATION_DIRECTION_NONE), frame_method(0), animation_behaviour(0),
      pending_behaviour(-1), bounds_min(0.0f, 0.0f, 0.0f), bounds_max(0.0f, 0.0f, 0.0f),
      bounds_extent(0), value(0), first_frame(0xff), last_frame(0xff)
{
    if (g_shared_timer_base == 0) {
        srAssertFail("gpsrTimer", "C:\\Projects\\Wizardry 8\\Engine Code\\AnimRep.cpp", 0x4e, 0);
    }
    timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
}

/* Vtable 0x005EC1D8's clone slot allocates exactly the root's 0x64-byte
   extent, then invokes the copy constructor above. */
// FUNCTION: WIZ8 0x0044edf0
W8AnimRepBase* W8AnimRepBase::Clone()
{
    return new W8AnimRepBase(*this);
}

/* AnimRep.cpp copies persistent animation state, then timestamps the new
   representation from the shared SurRender timer.  The source assertion names
   that global `gpsrTimer`. */
// FUNCTION: WIZ8 0x004b54a0
W8AnimRep::W8AnimRep(const W8AnimRep& other)
    : W8AnimRepBase(other), subcycle(other.subcycle),
      pending_subcycle(other.pending_subcycle), timer(other.timer),
      active(other.active), animation_playing(other.animation_playing),
      frame_direction(other.frame_direction), frame_method(other.frame_method),
      animation_behaviour(other.animation_behaviour),
      pending_behaviour(other.pending_behaviour), bounds_min(other.bounds_min),
      bounds_max(other.bounds_max), bounds_extent(other.bounds_extent),
      value(other.value), first_frame(other.first_frame),
      last_frame(other.last_frame)
{
    if (g_shared_timer_base == 0) {
        srAssertFail("gpsrTimer", "C:\\Projects\\Wizardry 8\\Engine Code\\AnimRep.cpp", 100, 0);
    }
    timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
}

/* The abstract emitter host copies its stable settings, but starts with no
   selected emitter and the canonical 00 00 FF FF transient byte pattern. */
// FUNCTION: WIZ8 0x004b5680
W8EmitterHost::W8EmitterHost(const W8EmitterHost& other)
    : W8AnimRep(other), m_bLOD(other.m_bLOD), lod_near(other.lod_near),
      lod_far(other.lod_far), current_cycle(0), current_subcycle(0),
      forced_subcycle(-1), pending_cycle(-1), animation_radius(other.animation_radius)
{
}

// FUNCTION: WIZ8 0x004b5600
W8EmitterHost::W8EmitterHost()
    : m_bLOD(0), lod_near(g_lod_range_default0),
      lod_far(g_lod_range_default1), current_cycle(0), current_subcycle(0),
      forced_subcycle(-1), pending_cycle(-1), animation_radius(0)
{
}
