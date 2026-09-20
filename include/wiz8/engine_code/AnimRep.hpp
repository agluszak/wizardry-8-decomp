#ifndef WIZ8_ENGINE_CODE_ANIM_REP_H
#define WIZ8_ENGINE_CODE_ANIM_REP_H

#include "surrender/srMath.h"

#include <stddef.h>

#pragma pack(push, 1)

/* Sixteen-byte render block copied from an AnimRep into a model instance.
   GrCycle assigns the whole 0x10 bytes at once; stModelInstance and
   stModelInstance2D store the same layout at +0x164, including the 2D
   left/top extent and right/bottom position shorts. */
struct W8ModelInstanceRenderState {
    union {
        struct {
            unsigned long render_depth;
            union {
                unsigned long state_04;
                struct {
                    short left;
                    short top;
                };
            };
            union {
                unsigned long state_08;
                struct {
                    short right;
                    short bottom;
                };
            };
            union {
                unsigned long state_0c;
                struct {
                    unsigned char display_state;
                    unsigned char state_0d;
                    unsigned char padding_0e[2];
                };
            };
        };
        struct {
            float highlight_red;
            float highlight_green;
            float highlight_blue;
            float highlight_alpha;
        };
    };
};

/* Prop.cpp writes the triples at 0x074 and 0x080 as x/y/z bounds and the word
   at 0x08c as a float extent, so those members take their actual scalar types. */

/* The copy path at 0x004B87C0 and clone slot at 0x0044EDF0 establish the
   0x64-byte polymorphic root below. The local point at +0x10 combines with
   the parent point at +0x1c to produce the world point at +0x04; rotation
   is stored separately at +0x28. Copy/clone preserve the transforms and
   render block but reset the transient scale and two flags. Its original
   name is not available. */
class W8AnimRepBase005EC1D8 {
public:
    W8AnimRepBase005EC1D8();
    W8AnimRepBase005EC1D8(const W8AnimRepBase005EC1D8& other);
    virtual ~W8AnimRepBase005EC1D8() {}
    virtual W8AnimRepBase005EC1D8* Clone();

    void SetLocation004B8850(const srVector3T<float>* location);
    void GetLocation004B8890(srVector3T<float>* location) const;
    void GetLocalLocation004B88B0(srVector3T<float>* location) const;
    void SetRotation004B88D0(const srMatrix3T<float>* rotation);
    void GetRotation004B88F0(srMatrix3T<float>* rotation);

public:
    srVector3T<float> location_004;
    srVector3T<float> local_location_010;
    srVector3T<float> parent_location_01c;
    srMatrix3T<float> rotation_028;
    W8ModelInstanceRenderState render_state_04c;
    /* Set by monster scale transitions; GrCycle copies it to model instances
       only when +0x61 enables that path. A value of one clears the instance
       scale flag instead of storing a redundant scale. */
    float instance_scale_05c;
    unsigned char flag_060;
    unsigned char apply_instance_scale_061;
    unsigned char unknown_062[2];
};

/* AnimRep.cpp's constructor and copy constructor extend the root through
   0x98. The derived copy preserves animation selection and frame bounds,
   then restarts its timer from the shared clock; the base copy resets its
   transient fields. The address suffix preserves the unresolved original
   class name. */
class W8AnimRep005ED050 : public W8AnimRepBase005EC1D8 {
public:
    W8AnimRep005ED050();
    W8AnimRep005ED050(const W8AnimRep005ED050& other);
    virtual ~W8AnimRep005ED050() override;
    void SetFrameMethod004B55C0(signed char method);

public:
    /* Current frame/subcycle. GrCycle advances it and all derived renderers
       use it to select the live mesh, event, particle, and light state. */
    unsigned char subcycle_064;
    unsigned char unknown_065;
    /* 0xffff means no queued subcycle; ApplyPendingCycle consumes and clears
       this only after the pending cycle is accepted. */
    unsigned short pending_subcycle_066;
    /* 0x68: a millisecond timestamp while the animation runs;
       SelectAnimationSlot reads its low two bytes as the transition's ordered
       animation-value pair. */
    union {
        unsigned int timer_068;
        struct {
            unsigned char value_068;
            unsigned char value_069;
            unsigned char animation_padding_06a[2];
        };
    };
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
    unsigned char unknown_072[2];
    /* Prop computes this pair from the animation's bounds, and stores the
       scaled extent in +0x8c. Other representation families inherit the
       storage even when their own use is not yet established. */
    srVector3T<float> bounds_min_074;
    srVector3T<float> bounds_max_080;
    float bounds_extent_08c;
    unsigned int value_090;
    unsigned char first_frame_094;
    unsigned char last_frame_095;
    unsigned char unknown_096[2];
};

static_assert(sizeof(W8ModelInstanceRenderState) == 0x10,
              "W8ModelInstanceRenderState_size_must_be_0x10");
static_assert(offsetof(W8AnimRepBase005EC1D8, render_state_04c) == 0x4c,
              "W8AnimRepBase_render_state_offset");
static_assert(sizeof(W8AnimRepBase005EC1D8) == 0x64, "W8AnimRepBase005EC1D8_size_must_be_0x64");
static_assert(sizeof(W8AnimRep005ED050) == 0x98, "W8AnimRep005ED050_size_must_be_0x98");

#pragma pack(pop)

extern float g_lod_range_default_0060e608;
extern float g_lod_range_default_0060e60c;

#endif
