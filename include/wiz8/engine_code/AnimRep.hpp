#ifndef WIZ8_ENGINE_CODE_ANIM_REP_H
#define WIZ8_ENGINE_CODE_ANIM_REP_H

#include "surrender/srMath.h"

#include <stddef.h>

/* Direction state at AnimRep+0x6e. Props record completion at either
   endpoint; GrCycle advances only the two running directions. Byte storage
   is retained because prop save data reads/writes this field directly. */
enum W8AnimationDirection {
    W8_ANIMATION_DIRECTION_NONE = 0,
    W8_ANIMATION_FORWARD = 1,
    W8_ANIMATION_FORWARD_COMPLETE = 2,
    W8_ANIMATION_REVERSE = 3,
    W8_ANIMATION_REVERSE_COMPLETE = 4
};

/* Proven frame methods. The serialized domain also accepts four; its
   original name remains unknown. */
enum W8AnimationFrameMethod {
    W8_ANIMATION_WRAP = 1,
    W8_ANIMATION_PING_PONG = 2,
    W8_ANIMATION_RANDOM_FRAME = 3
};

/* The 2D instance has a different sixteen-byte block at the same class offset. */
struct W8ModelInstance2DRenderState {
    unsigned long render_depth;
    unsigned short width;
    unsigned short height;
    short position_x;
    short position_y;
    unsigned char display_state;
    /* 0x0d: enables the pulsing glow pass over mesh.materials[0]. */
    bool glow_enabled;
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

    void SetLocation(const srVector3T<float>* location);
    void GetLocation(srVector3T<float>* location) const;
    void GetLocalLocation(srVector3T<float>* location) const;
    void SetRotation(const srMatrix3T<float>* rotation);
    void GetRotation(srMatrix3T<float>* rotation);

public:
    srVector3T<float> location;
    srVector3T<float> local_location;
    srVector3T<float> parent_location;
    srMatrix3T<float> rotation;
    /* RGBA highlight colour; GrCycle copies it into the 3D mesh instance,
       which renders it as its highlight material's emissive colour. */
    srVector4T<float> highlight_colour;
    /* Set by monster scale transitions; GrCycle copies it to model instances
       only when +0x61 enables that path. A value of one clears the instance
       scale flag instead of storing a redundant scale. */
    float instance_scale;
    bool flag;
    bool apply_instance_scale;
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
    void SetFrameMethod(signed char method);

public:
    /* Current frame/subcycle. GrCycle advances it and all derived renderers
       use it to select the live mesh, event, particle, and light state. */
    unsigned char subcycle;
    unsigned char padding_065;
    /* 0xffff means no queued subcycle; ApplyPendingCycle consumes and clears
       this only after the pending cycle is accepted. */
    unsigned short pending_subcycle;
    /* Frame-advance timestamp; GrCycle subtracts it from the current time. */
    unsigned int timer;
    unsigned char
        active; /* bool-byte-ok: SetSetting6C stores and GetSetting6C returns the caller's byte. */
    unsigned char
        animation_playing; /* bool-byte-ok: copied directly from file-backed W8AnimObj byte. */
    /* W8AnimationDirection, retained as a byte in the representation. */
    unsigned char frame_direction;
    /* SetFrameMethod checks the retail DIR_FIRST..DIR_LAST range; endpoint
       behavior 1 wraps and 2 reverses in AdvanceAnimationFrame. */
    unsigned char frame_method;
    unsigned char animation_behaviour;
    /* 0xff means no pending change. ApplyPendingCycle applies it to the
       selected representation, then clears the old object's slot. */
    unsigned char pending_behaviour;
    unsigned char padding_072[2];
    /* Prop computes this pair from the animation's bounds, and stores the
       scaled extent in +0x8c. Other representation families inherit the
       storage even when their own use is not yet established. */
    srVector3T<float> bounds_min;
    srVector3T<float> bounds_max;
    float bounds_extent;
    unsigned int value;
    /* The ordered frame range GrCycle plays between. */
    unsigned char first_frame;
    unsigned char last_frame;
    unsigned char padding_096[2];
};

static_assert(sizeof(W8ModelInstance2DRenderState) == 0x10,
              "W8ModelInstance2DRenderState_size_must_be_0x10");
static_assert(offsetof(W8AnimRepBase, highlight_colour) == 0x4c,
              "W8AnimRepBase_render_state_offset");
static_assert(sizeof(W8AnimRepBase) == 0x64, "W8AnimRepBase_size_must_be_0x64");
static_assert(sizeof(W8AnimRep) == 0x98, "W8AnimRep_size_must_be_0x98");

extern float g_lod_range_default0;
extern float g_lod_range_default1;

#endif
