#pragma once

#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/vector.h"

#include "surrender/srMath.h"

class GDProp;
struct W8AnimObj;
struct W8ReadLevelInfo;
struct W8World;
class srModelInstance;
class stModelInstance;
class Trigger;
class W8Prop;
struct W8AIMissile;

extern unsigned char g_byte_00659a64;

/* One two-byte animation-slot record: the frame the segment selects and the
   tag that names it.  LoadProp0044AEE0 reads each as a serialized short and
   narrows to a byte; the segment walkers sign-extend the frame, and
   SelectAnimationSlot treats a negative frame as a slot with no animation. */
struct W8PropAnimationSegment {
    signed char frame;
    unsigned char tag;
};

/* Prop.cpp's m_pRep.  Assertions name the member; the constructor allocates
   0xc4 bytes, runs the AnimRep constructor, then installs the Prop-owned
   animation pointer, speed, and the slot vector at 0xb0.  The secondary
   vtable at 0xb0 is the growable-vector specialization at 0x005EC1D0. */
class W8PropRepresentation : public W8AnimRep005ED050 {
public:
    /* Default construction is inlined at Prop::Prop. */
    W8PropRepresentation()
        : animation(0), animation_speed(0.0f), frame_index_0a0(0), animation_running_0a4(0),
          random_play_0a5(0), play_chance_0a8(0.5f), saved_subcycle_0ac(0), frame_steps_0ad(0),
          slots(5), footstep_surface_0c0(0xff), footstep_material_0c1(0xff)
    {
    }
    W8PropRepresentation(const W8PropRepresentation& other);
    virtual ~W8PropRepresentation() override;
    virtual W8AnimRepBase005EC1D8* Clone() override;

    srModelInstance* ToggleAnimation(int argument); /* 0x0044BA00 */
    unsigned char SelectAnimationSlot(unsigned char tag);
    int FindCurrentAnimationSlot(); /* 0x0044BAE0 */
    unsigned char AdvanceAnimationSegment();
    /* CreateAndLoadProp loads m_pRep into ECX, then passes (pInfo, pProp). */
    bool LoadProp0044AEE0(W8ReadLevelInfo* info, W8Prop* prop); /* 0x0044AEE0 */

    W8AnimObj* animation;  /* 0x98 */
    float animation_speed; /* 0x9c */
    /* 0xa0: integer path position accumulator.  UpdatePropAnimation0044C030 adds the elapsed
       frame count to it with a dword add, compares it against the animation's
       value_16, and FILD-converts it for PathAISetValue004A9F60. */
    int frame_index_0a0;
    bool animation_running_0a4; /* 0xa4 */
    bool random_play_0a5;       /* 0xa5 */
    unsigned char padding_0a6[2];
    float play_chance_0a8;            /* 0xa8: constructed as 0.5 */
    unsigned char saved_subcycle_0ac; /* 0xac */
    unsigned char frame_steps_0ad;    /* 0xad */
    unsigned char padding_0ae[2];
    W8GrowableVector<W8PropAnimationSegment*> slots; /* 0xb0 */
    unsigned char footstep_surface_0c0;              /* 0xc0 */
    unsigned char footstep_material_0c1;             /* 0xc1 */
    unsigned char padding_0c2[2];
}; /* 0xc4 */

static_assert(sizeof(W8PropRepresentation) == 0xc4, "W8PropRepresentation_must_be_0xc4");

/* Engine Code\Prop.cpp.  Prop::Prop() calls W8GrObject::W8GrObject and
   allocates operator new(0x90), which proves both the base and the extent.
   m_pRep and m_pTimer are the assertion-backed names; the representation is
   the Prop-owned W8PropRepresentation stored through GrObject's m_pRep slot. */
class W8Prop : public W8GrObject {
public:
    W8Prop();                   /* 0x0044BC00 */
    virtual ~W8Prop() override; /* complete destructor 0x0044BEC0 */

    W8PropRepresentation* Rep() const
    {
        return static_cast<W8PropRepresentation*>(m_pRep);
    }

    void DetachAnimationInstances0044D360(W8World* world);
    void UpdatePropAnimation0044C030();
    /* Re-apply every animation path and roll the position snapshots forward.
       `world` is only used by the pWorld assertion. */
    void ApplyAnimationPaths0044C200(W8World* world);
    /* Advance the rep's animation value by `frames`, honouring the direction,
       bounce and wrap modes; clamps to the counter range and pushes the new
       value to every bound path. `total` is the animation's frame count. */
    void AdvanceAnimationValue0044C310(int frames, char total);
    /* The animation value one step ahead of the current one, without
       committing it - clamped for transitive animations, wrapping or bouncing
       for the looping kinds. */
    char NextAnimationValue0044C600();
    void ApplyAnimationFrame0044C670(); /* 0x0044C670 */
    /* Restore the rep's persisted animation state: five saved bytes plus one
       discarded byte, clamped to the loaded animation's frame count, with
       path values re-synced while a running animation is active. */
    bool LoadAnimationState0044DBD0(int hFile); /* 0x0044DBD0 */
    int BuildOrRefreshPathingRepresentation();  /* 0x0044DEA0 */
    /* When the animation advanced exactly one frame this writes the current
       position minus the home position into `out`; otherwise `out` is zeroed.
       `point` is accepted but never read. */
    char GetDelta0044E130(srVector3T<float>* out, const srVector3T<float>* point); /* 0x0044E130 */
    /* The prop's position for external queries: position_02c when the rep
       node reports itself current, else the rep node's own position. */
    void GetPosition0044E2C0(srVector3T<float>* out); /* 0x0044E2C0 */
    /* Mirror of GetPosition0044E2C0: writes `position` back to the rep node's
       position when the rep is current, else through SetLocation004B8850. */
    void SetPosition0044E310(srVector3T<float>* position); /* 0x0044E310 */
    /* Whether trigger_18 exists and carries the action-message flag. */
    bool TriggerHasActionMessage0044E360(); /* 0x0044E360 */
    /* Whether trigger_18 exists and takes an item (required_item_id >= 0 or a
       type-10 action payload naming item_00a). */
    bool TriggerRequiresItem0044E380(); /* 0x0044E380 */
    /* The prop's current animation value; -1 when it has none. */
    int GetAnimationState0044EBE0() const; /* 0x0044EBE0 */
    void AttachAnimationInstances0044C830(W8World* world);
    unsigned char GetSetting6C();
    srModelInstance* ToggleRepAnimation(int argument);
    srModelInstance* ToggleRepAnimationDefault();
    unsigned char PlayRepAnimation(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void SetSetting6E(unsigned char value);
    void SetRepresentationActive(unsigned char active, unsigned char update_animation);
    bool CanBeUsedFrom(int arg_2, int arg_3, char notify);
    void SetSetting6C(unsigned char value);
    void SetSetting66(char value);
    void SetAnimationSpeed(float speed);
    bool IsSetting6FTwo();
    void ToggleSetting6E();
    Trigger* GetValue18();
    bool IsTriggerInView0044E3A0(srVector3T<float>* position);
    Trigger* GetGDPropValue24();
    void GetCenterPosition(srVector3T<float>* position);
    /* Whether the renderer's currently selected model instance is dispatched
       by this prop's animation - the prop half of ResolvePickedProp's test.
       `world` is accepted but never read. */
    bool IsPickedProp0044D680(W8World* world); /* 0x0044D680 */
    void GetBounds0044DD60(srVector3T<float>* minimum, srVector3T<float>* maximum);
    void CollectModelInstances(W8GrowableVector<stModelInstance*>* instances);
    /* Run trigger_18 when its action is one of the missile-impact kinds
       (0x3a..0x3c); the record hands Run the missile's table index. */
    void RunMissileTrigger0044E230(W8AIMissile* record);

    Trigger* trigger_18;   /* 0x18 */
    unsigned int flags_1c; /* 0x1c */
    char* m_name;          /* 0x20 */
    /* 0x24: UpdatePropAnimation0044C030 stores the animation timer's progress here, then
       reduces it by the whole-frame count - the fractional remainder. */
    float anim_frame_fraction_024;
    W8GameTimer* m_pTimer;          /* 0x28 */
    srVector3T<float> position_02c; /* 0x2c: written by ApplyAnimationFrame0044C670 */
    GDProp* m_gd_prop;              /* 0x38 */
    srVector3T<float> position_03c; /* 0x3c */
    /* Prop::Prop writes two identity bases here as nine floats each. */
    srMatrix3T<float> rotation_048; /* 0x48 */
    srMatrix3T<float> rotation_06c; /* 0x6c */
}; /* 0x90 */

static_assert(sizeof(W8Prop) == 0x90, "W8Prop_must_be_0x90");

W8Prop* FindPropByName(W8World* world, const char* name);
bool CreateAndLoadProp0044BF50(W8ReadLevelInfo* info, W8Prop** prop);

char ResolvePickedProp(W8World* world);
int GetSelectedPropIndex0044DA60(void);
/* Run the latched selected-prop trigger, or clear the latch when the
   renderer has no pick. */
unsigned char ActivateSelectedProp0044DA20(void);
void UpdateWorldProps0044E010(W8World* world);
