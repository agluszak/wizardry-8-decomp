#ifndef WIZ8_ENGINE_CODE_ANIM_REP_H
#define WIZ8_ENGINE_CODE_ANIM_REP_H

#include "surrender/srMath.h"

#include <stddef.h>

/* The 2D instance has a different sixteen-byte block at the same class offset. */
struct W8ModelInstance2DRenderState {
    unsigned long render_depth;
    short left;
    short top;
    short right;
    short bottom;
    unsigned char display_state;
    /* 0x0d: enables the pulsing glow pass over mesh.materials_70[0]. */
    unsigned char glow_enabled_0d;
    unsigned char padding_0e[2];
};

/* Prop.cpp writes the triples at 0x074 and 0x080 as x/y/z bounds and the word
   at 0x08c as a float extent, so those members take their actual scalar types. */

/* The copy path at 0x004B87C0 and clone slot at 0x0044EDF0 establish the
   0x64-byte polymorphic root below. The local point at +0x10 combines with
   the parent point at +0x1c to produce the world point at +0x04; rotation
   is stored separately at +0x28. Copy/clone preserve the transforms and
   render block but reset the transient scale and two flags. Its original
   name is not available. */
class W8AnimRepBase {
public:
    W8AnimRepBase();
    W8AnimRepBase(const W8AnimRepBase& other);
    virtual ~W8AnimRepBase() {}
    virtual W8AnimRepBase* Clone();

    void SetLocation004B8850(const srVector3T<float>* location);
    void GetLocation004B8890(srVector3T<float>* location) const;
    void GetLocalLocation(srVector3T<float>* location) const;
    void SetRotation004B88D0(const srMatrix3T<float>* rotation);
    void GetRotation(srMatrix3T<float>* rotation);

public:
    srVector3T<float> location_004;
    srVector3T<float> local_location_010;
    srVector3T<float> parent_location_01c;
    srMatrix3T<float> rotation_028;
    /* RGBA highlight colour; GrCycle copies it into the 3D mesh instance,
       which renders it as its highlight material's emissive colour. */
    srVector4T<float> highlight_colour_04c;
    /* Set by monster scale transitions; GrCycle copies it to model instances
       only when +0x61 enables that path. A value of one clears the instance
       scale flag instead of storing a redundant scale. */
    float instance_scale_05c;
    bool flag_060;
    unsigned char apply_instance_scale_061;
    unsigned char padding_062[2];
};

/* AnimRep.cpp's constructor and copy constructor extend the root through
   0x98. The derived copy preserves animation selection and frame bounds,
   then restarts its timer from the shared clock; the base copy resets its
   transient fields. The address suffix preserves the unresolved original
   class name. */
class W8AnimRep : public W8AnimRepBase {
public:
    W8AnimRep();
    W8AnimRep(const W8AnimRep& other);
    virtual ~W8AnimRep() override;
    void SetFrameMethod(signed char method);

public:
    /* Current frame/subcycle. GrCycle advances it and all derived renderers
       use it to select the live mesh, event, particle, and light state. */
    unsigned char subcycle_064;
    unsigned char padding_065;
    /* 0xffff means no queued subcycle; ApplyPendingCycle consumes and clears
       this only after the pending cycle is accepted. */
    unsigned short pending_subcycle_066;
    /* Frame-advance timestamp; GrCycle subtracts it from the current time. */
    unsigned int timer_068;
    unsigned char active;
    unsigned char animation_playing_06d;
    /* Direction 1 advances and 3 reverses in GrCycle. Other direction codes
       also occur in monster completion checks, so this remains a byte. */
    unsigned char frame_direction_06e;
    /* SetFrameMethod checks the retail DIR_FIRST..DIR_LAST range; endpoint
       behavior 1 wraps and 2 reverses in AdvanceAnimationFrame. */
    unsigned char frame_method_06f;
    unsigned char animation_behaviour_070;
    /* 0xff means no pending change. ApplyPendingCycle applies it to the
       selected representation, then clears the old object's slot. */
    unsigned char pending_behaviour_071;
    unsigned char padding_072[2];
    /* Prop computes this pair from the animation's bounds, and stores the
       scaled extent in +0x8c. Other representation families inherit the
       storage even when their own use is not yet established. */
    srVector3T<float> bounds_min_074;
    srVector3T<float> bounds_max_080;
    float bounds_extent_08c;
    unsigned int value_090;
    /* The ordered frame range GrCycle plays between. */
    unsigned char first_frame_094;
    unsigned char last_frame_095;
    unsigned char padding_096[2];
};

static_assert(sizeof(W8ModelInstance2DRenderState) == 0x10,
              "W8ModelInstance2DRenderState_size_must_be_0x10");
static_assert(offsetof(W8AnimRepBase, highlight_colour_04c) == 0x4c,
              "W8AnimRepBase_render_state_offset");
static_assert(sizeof(W8AnimRepBase) == 0x64, "W8AnimRepBase_size_must_be_0x64");
static_assert(sizeof(W8AnimRep) == 0x98, "W8AnimRep_size_must_be_0x98");

extern float g_lod_range_default_0060e608;
extern float g_lod_range_default_0060e60c;

#endif
