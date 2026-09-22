#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/sgp_narrow_text.h"

#include <string.h>

#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"
#include "wiz8/xstatus.h"
#include "surrender/srCamera.h"
#include "surrender/srModelInstance.h"
#include "surrender/srNode.h"

#include "FileMan.h"

#include "DEBUG.H"

#include <math.h>
#include <new>
#include <stdlib.h>
#include <windows.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/geometry.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "random.h"

/* Engine Code\Prop.cpp. The complete destructor at 0x0044BEC0 releases four
   owned members, and each release names the shape of what it owns:

     +0x14  delete through vtable slot 0 with the deleting flag - a class with
            a virtual destructor
     +0x20  a null check and a bare operator delete - the owned name string
     +0x28  the same virtual-destructor shape as +0x14
     +0x38  its destructor called directly and then operator delete - a class
            with a non-virtual destructor

   FindPropByName independently proves that +0x20 is the owned prop name.
   Unresolved members and the gaps between them remain positional. */

/* This byte is reset before the world Prop update and set when a collidable
   Prop rebuilds its pathing geometry.  Its three retail references establish
   the process-wide storage; no broader state model is yet proved. */
// GLOBAL: WIZ8 0x00659A64
unsigned char g_byte_00659a64;

#define PROP_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp"

// VTABLE: WIZ8 0x005ec1e0
// class W8Prop

// VTABLE: WIZ8 0x005ec1c8
// class W8PropRepresentation

// VTABLE: WIZ8 0x005ec1d0
// class W8GrowableVector<W8PropAnimationSegment*>

// SYNTHETIC: WIZ8 0x0044ef60
// W8GrowableVector<W8PropAnimationSegment*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0044ef30
// W8GrowableVector<W8PropAnimationSegment*>::`vector deleting destructor' (companion table 0x005EC1D4)

// TEMPLATE: WIZ8 0x0044ef00
// W8GrowableVector<W8PropAnimationSegment*>::~W8GrowableVector<W8PropAnimationSegment*>

// TEMPLATE: WIZ8 0x0044efe0
// W8GrowableVector<W8PropAnimationSegment*>::W8GrowableVector

/* Prop::Prop() - GrObject base, then m_pRep / m_pTimer and two identity
   rotation bases.  Retail expands PropRep after the AnimRep constructor:
   scalar field stores, the capacity-5 slot vector, then the PropRep vtable. */
// FUNCTION: WIZ8 0x0044bc00
W8Prop::W8Prop()
{
    trigger_18 = 0;
    flags_1c = 0;
    m_name = 0;
    anim_frame_fraction_024 = 0;
    kind_004 = 4;
    id_008 = IncrementValue60DFAC();
    m_pRep = new W8PropRepresentation();
    m_pTimer = new W8GameTimer();
    position_02c.SetZero();
    position_03c.SetZero();
    rotation_048.SetIdentity();
    rotation_06c.SetIdentity();
    m_gd_prop = 0;
    if (m_pRep == 0) {
        srAssertFail("m_pRep", PROP_CPP, 0x30e, "Prop::Prop() out of memory allocating m_pRep");
    }
    if (m_pTimer == 0) {
        srAssertFail("m_pTimer", PROP_CPP, 0x30f, "Prop::Prop() out of memory allocating m_pTimer");
    }
}

/* Copy keeps animation through CloneAnimObj and rebuilds an empty slot vector
   with the source capacity.  Clone's vtable slot allocates 0xc4 and lands here. */
// FUNCTION: WIZ8 0x0044ad10
W8PropRepresentation::W8PropRepresentation(const W8PropRepresentation& other)
    : W8AnimRep005ED050(other), animation_speed(other.animation_speed),
      frame_index_0a0(other.frame_index_0a0), animation_running_0a4(other.animation_running_0a4),
      random_play_0a5(other.random_play_0a5), play_chance_0a8(other.play_chance_0a8),
      saved_subcycle_0ac(other.saved_subcycle_0ac), frame_steps_0ad(other.frame_steps_0ad),
      slots(5), footstep_surface_0c0(other.footstep_surface_0c0),
      footstep_material_0c1(other.footstep_material_0c1)
{
    animation = CloneAnimObj004A0320(other.animation);
}

// SYNTHETIC: WIZ8 0x0044acf0
// W8PropRepresentation::`scalar deleting destructor'
// FUNCTION: WIZ8 0x0044ae30
W8PropRepresentation::~W8PropRepresentation()
{
    int index;

    for (index = 0; index < slots.count; ++index) {
        delete slots.data[index];
    }
    slots.count = 0;
    if (animation != 0) {
        DestroyAnimObj004A01E0(animation);
        animation = 0;
    }
}

// FUNCTION: WIZ8 0x0044ef80
W8AnimRepBase005EC1D8* W8PropRepresentation::Clone()
{
    return new W8PropRepresentation(*this);
}

/* Visit every prop attached to the world and activate those whose companion
   object can be resolved and whose owned GDProp exists. */
// FUNCTION: WIZ8 0x0044e010
void UpdateWorldProps0044E010(W8World* world)
{
    unsigned int count;
    int index;

    if (world == 0 || world->plsProps == 0) {
        srAssertFail("pWorld && pWorld->plsProps",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 0x969, 0);
    }
    count = PLLength(world->plsProps);
    for (index = 0; index < static_cast<int>(count); ++index) {
        W8Prop* prop = static_cast<W8Prop*>(PLGet(world->plsProps, index));
        Trigger* trigger;

        if (prop != 0 && (trigger = FindTriggerForProp00443830(world, prop)) != 0 &&
            prop->m_gd_prop != 0) {
            prop->flags_1c |= 0x80;
            prop->m_gd_prop->BindTrigger(trigger);
        }
    }
}

/* VC6 emits the scalar-deleting wrapper from this ordinary virtual
   destructor. */
// SYNTHETIC: WIZ8 0x0044BEA0
// W8Prop::`scalar deleting destructor'
// FUNCTION: WIZ8 0x0044bec0
W8Prop::~W8Prop()
{
    delete static_cast<W8PropRepresentation*>(m_pRep);
    m_pRep = 0;
    if (m_name != 0) {
        delete[] m_name;
        m_name = 0;
    }
    delete m_pTimer;
    m_pTimer = 0;
    delete m_gd_prop;
    m_gd_prop = 0;
}

// FUNCTION: WIZ8 0x0044db60
W8Prop* FindPropByName(W8World* world, const char* name)
{
    int index;

    if (world != 0 && name != 0) {
        unsigned int count = PLLength(world->plsProps);

        for (index = 0; index < (int)count; ++index) {
            W8Prop* prop = static_cast<W8Prop*>(PLGet(world->plsProps, index));
            if (prop->m_name != 0 && _stricmp(prop->m_name, name) == 0) {
                return prop;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0044e2c0
void W8Prop::GetPosition0044E2C0(srVector3T<float>* out)
{
    if (AnimationIsRunning(Rep()->animation) == 1) {
        *out = position_02c;
        return;
    }
    m_pRep->GetLocation004B8890(out);
}

// FUNCTION: WIZ8 0x0044e270
void W8Prop::SetAnimationSpeed(float speed)
{
    if (speed > 0.0f) {
        Rep()->animation_speed = speed;
        if (m_pTimer != 0) {
            m_pTimer->SetDuration(1.0f / speed);
        }
    }
}

/* Four accessors reaching through the owned member at 0x14. */
// FUNCTION: WIZ8 0x0044d4f0
unsigned char W8Prop::GetSetting6C()
{
    return this->Rep()->active;
}

// FUNCTION: WIZ8 0x0044d5b0
void W8Prop::SetSetting66(char value)
{
    this->Rep()->pending_subcycle_066 = value;
}

/* Whether the owned member is in the state the value two stands for. */
// FUNCTION: WIZ8 0x0044e1c0
bool W8Prop::IsSetting6FTwo()
{
    return this->Rep()->frame_method_06f == 2;
}

/* Flip the owned member between the only two values it takes: one and three. */
// FUNCTION: WIZ8 0x0044e1d0
void W8Prop::ToggleSetting6E()
{
    this->Rep()->frame_direction_06e = this->Rep()->frame_direction_06e == 1 ? 3 : 1;
}

/* The prop's own trigger at 0x18. */
// FUNCTION: WIZ8 0x0044d5a0
Trigger* W8Prop::GetValue18()
{
    return this->trigger_18;
}

/* One value out of the owned GDProp, but only once the flag that says it is
   there is up. */
// FUNCTION: WIZ8 0x0044e0a0
Trigger* W8Prop::GetGDPropValue24()
{
    if ((this->flags_1c & 0x80) != 0 && this->m_gd_prop != 0) {
        return this->m_gd_prop->m_owner_24;
    }
    return 0;
}

/* Write the settings block's byte at 0x6c. Going from zero to anything else
   costs an extra call first, so zero is the state that has to be left rather
   than a value like the others. */
// FUNCTION: WIZ8 0x0044d4b0
void W8Prop::SetSetting6C(unsigned char value)
{
    if (Rep()->active == 0) {
        m_pTimer->Restart();
    }
    Rep()->active = value;
}

// FUNCTION: WIZ8 0x0044d5f0
void W8Prop::GetCenterPosition(srVector3T<float>* position)
{
    srVector3T<float> first;
    srVector3T<float> second;

    AnimObjGetBounds004A1710(Rep()->animation, 2, Rep()->subcycle_064, &first, &second);
    *position = (first + second) * 0.5;
}

// GLOBAL: WIZ8 0x00659A60
Trigger* g_selected_prop_trigger_00659a60;
// GLOBAL: WIZ8 0x00607B98
int g_selected_prop_index_00607b98 = -1;

/* Whether the renderer's currently selected model instance is one of the
   instances this prop's animation dispatches.  With a running animation every
   list entry is checked; otherwise the single dispatched instance for the
   current frame is compared directly. */
// FUNCTION: WIZ8 0x0044d680
bool W8Prop::IsPickedProp0044D680(W8World* world)
{
    srModelInstance* instance;
    unsigned char frame;
    unsigned int count;
    int index;

    if (Rep()->active == 0) {
        return false;
    }
    if (AnimationIsRunning(Rep()->animation) == 1) {
        count = AnimObjListCount004A1620(Rep()->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            instance =
                AnimObjDispatchList004A1560(Rep()->animation, 2, static_cast<signed char>(index));
            if (GetPickedModelInstance00427810() == instance) {
                return true;
            }
        }
        return false;
    }
    frame = Rep()->subcycle_064;
    if (AnimationIsRunning(Rep()->animation) == 0) {
        instance = AnimObjDispatch004A14D0(Rep()->animation, 2, frame);
    } else {
        instance = AnimObjDispatchList004A1560(Rep()->animation, 2, 0);
    }
    return GetPickedModelInstance00427810() == instance;
}

/* Resolve the renderer's picked model instance back to the prop and trigger
   that own it. A pick is accepted only while the prop is active, the trigger
   permits selection, and the prop centre lies inside the trigger's distance
   interval. */
// FUNCTION: WIZ8 0x0044d760
char ResolvePickedProp(W8World* world)
{
    srModelInstance* selected;
    srVector3T<float> camera_position;
    unsigned int prop_count;
    int prop_index;
    char valid;

    g_selected_prop_trigger_00659a60 = 0;
    g_selected_prop_index_00607b98 = -1;
    selected = GetPickedModelInstance00427810();
    if (selected == 0) {
        return 0;
    }

    valid = 1;
    GetCameraPosition(&camera_position);
    prop_count = PLLength(world->plsProps);
    for (prop_index = 0; prop_index < static_cast<int>(prop_count); ++prop_index) {
        W8Prop* prop;
        W8PropRepresentation* representation;
        srModelInstance* instance;
        int instance_index;

        if (!valid) {
            return 0;
        }
        if (g_selected_prop_trigger_00659a60 != 0) {
            return valid;
        }
        prop = static_cast<W8Prop*>(PLGet(world->plsProps, prop_index));
        representation = prop->Rep();
        if (representation->active == 0) {
            continue;
        }

        instance = 0;
        if (AnimationIsRunning(representation->animation) == 1) {
            int count = static_cast<int>(AnimObjListCount004A1620(representation->animation, 2));
            for (instance_index = 0; instance_index < count; ++instance_index) {
                instance = AnimObjDispatchList004A1560(representation->animation, 2,
                                                       static_cast<signed char>(instance_index));
                if (GetPickedModelInstance00427810() == instance) {
                    break;
                }
            }
            if (instance_index == count) {
                continue;
            }
        } else {
            instance = representation->ToggleAnimation(representation->subcycle_064);
            if (GetPickedModelInstance00427810() != instance) {
                continue;
            }
        }

        {
            Trigger* trigger = prop->trigger_18;
            g_selected_prop_trigger_00659a60 = trigger;
            if (trigger != 0 && (trigger->flags_0a0 & W8_TRIGGER_ENABLED) != 0 &&
                ((trigger->flags_0a0 & 0x40000) == 0 || (trigger->flags_0a0 & 0x80000) == 0) &&
                (g_combat_inactive_006081e4 ||
                 (trigger->m_pActionData != 0 && trigger->m_pActionData->type_004 == 10 &&
                  (static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->flags_008 & 1) ==
                      0)) &&
                representation->active != 0) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;
                float distance;

                AnimObjGetBounds004A1710(representation->animation, 2, representation->subcycle_064,
                                         &minimum, &maximum);
                distance = ((minimum + maximum) * 0.5 - camera_position).Length();
                if (trigger->range_minimum_0a4 <= distance) {
                    g_selected_prop_index_00607b98 = prop_index;
                    if (distance <= trigger->range_maximum_0a8) {
                        continue;
                    }
                }
            }
            valid = 0;
            SetPickedModelInstance00427820(0);
            g_selected_prop_trigger_00659a60 = 0;
            g_selected_prop_index_00607b98 = -1;
        }
    }
    return valid;
}

/* Start or stop the prop's own animation, whichever it is not doing. */
// FUNCTION: WIZ8 0x0044ba00
srModelInstance* W8PropRepresentation::ToggleAnimation(int argument)
{
    if (AnimationIsRunning(animation) == 0) {
        return AnimObjDispatch004A14D0(animation, 2, argument);
    }
    return AnimObjDispatchList004A1560(animation, 2, 0);
}

/* Select the animation slot whose second byte carries the requested tag.
   The slot's signed first byte is the new animation tag; the old and new
   values are retained as an ordered range for the transition state. */
// FUNCTION: WIZ8 0x0044ba50
unsigned char W8PropRepresentation::SelectAnimationSlot(unsigned char tag)
{
    int index;

    for (index = 0; index < slots.count; ++index) {
        if (slots.data[index]->tag == tag) {
            signed char selected = static_cast<signed char>(slots.data[index]->frame);

            if (selected < 0) {
                return 0;
            }
            frame_lo_068 = first_frame_094;
            frame_hi_069 = static_cast<unsigned char>(selected);
            if (selected < static_cast<signed char>(first_frame_094)) {
                frame_lo_068 = static_cast<unsigned char>(selected);
                frame_hi_069 = first_frame_094;
            }
            if (frame_hi_069 <= first_frame_094) {
                frame_direction_06e = 3;
            } else {
                frame_direction_06e = 1;
            }
            animation_playing_06d = 1;
            return 1;
        }
    }
    return 0;
}

/* Which slot carries the current tag. The tag is matched against each slot's
   own leading byte rather than used as an index, so the slots need not be in
   tag order. */
// FUNCTION: WIZ8 0x0044bae0
int W8PropRepresentation::FindCurrentAnimationSlot()
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int index;

    for (index = 0; index < slots.count; ++index) {
        if (static_cast<int>(static_cast<char>(slots.data[index]->frame)) ==
            static_cast<unsigned int>(first_frame_094)) {
            return index;
        }
    }
    return -1;
#pragma clang diagnostic pop
}

// FUNCTION: WIZ8 0x0044bb20
unsigned char W8PropRepresentation::AdvanceAnimationSegment()
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int segment;

    if (slots.count < 3) {
        return 0;
    }
    for (segment = 0; segment < slots.count; ++segment) {
        if (static_cast<int>(static_cast<char>(slots.data[segment]->frame)) ==
            static_cast<unsigned int>(first_frame_094)) {
            break;
        }
    }
    if (segment == slots.count) {
        segment = -1;
    }
    if (segment == -1) {
        srAssertFail("lSegment!=(-1)", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 0x2c3, 0);
    }
    if (segment == slots.count - 2) {
        segment = 0;
    } else {
        ++segment;
    }
    first_frame_094 = slots.data[segment]->frame;
    last_frame_095 = slots.data[segment + 1]->frame;
    frame_direction_06e = 1;
    animation_playing_06d = 1;
    subcycle_064 = first_frame_094;
    return (unsigned char)segment;
#pragma clang diagnostic pop
}

/* The same toggle reached through the prop rather than through the member. */
// FUNCTION: WIZ8 0x0044d500
srModelInstance* W8Prop::ToggleRepAnimation(int argument)
{
    W8PropRepresentation* rep = Rep();

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch004A14D0(rep->animation, 2, argument);
    }
    return AnimObjDispatchList004A1560(rep->animation, 2, 0);
}

/* And again with the member's own stored argument instead of a caller's -
   which is what makes 0x64 the animation's default argument. */
// FUNCTION: WIZ8 0x0044d550
srModelInstance* W8Prop::ToggleRepAnimationDefault()
{
    W8PropRepresentation* rep = Rep();
    unsigned char argument = rep->subcycle_064;

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch004A14D0(rep->animation, 2, argument);
    }
    return AnimObjDispatchList004A1560(rep->animation, 2, 0);
}

/* Resolve the bounds of the prop's current animation frame. */
// FUNCTION: WIZ8 0x0044d5c0
unsigned char W8Prop::PlayRepAnimation(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    AnimObjGetBounds004A1710(Rep()->animation, 2, Rep()->subcycle_064, minimum, maximum);
    return 1;
}

/* Write the member's setting at 0x6e. The assertion names the member m_pRep,
   and the guarded store is written after the assertion rather than instead of
   it, so a null member writes through null on a build with assertions off. */
// FUNCTION: WIZ8 0x0044e1f0
void W8Prop::SetSetting6E(unsigned char value)
{
    if (m_pRep == 0) {
        srAssertFail("m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 2698, 0);
    }
    Rep()->frame_direction_06e = value;
}

/* Set the live representation state. When requested, choose the direction
   and endpoint of the transition from the current and target animation tags. */
// FUNCTION: WIZ8 0x0044da80
void W8Prop::SetRepresentationActive(unsigned char active, unsigned char update_animation)
{
    Rep()->animation_playing_06d = active;
    if (active == 0) {
        return;
    }

    m_pTimer->Restart();
    if (update_animation == 0) {
        return;
    }

    if (Rep()->animation_behaviour_070 == 1) {
        if ((Rep()->frame_direction_06e == 2 && Rep()->frame_method_06f != 2) ||
            (Rep()->frame_direction_06e != 2 && Rep()->frame_method_06f == 2)) {
            Rep()->frame_direction_06e = 1;
            Rep()->subcycle_064 = Rep()->first_frame_094;
            return;
        }
        Rep()->frame_direction_06e = 3;
        Rep()->subcycle_064 = Rep()->last_frame_095;
        return;
    }
    if (Rep()->animation_behaviour_070 == 2) {
        if ((Rep()->frame_direction_06e == 2 && Rep()->frame_method_06f != 2) ||
            (Rep()->frame_direction_06e != 2 && Rep()->frame_method_06f == 2)) {
            Rep()->frame_direction_06e = 1;
            Rep()->subcycle_064 = Rep()->first_frame_094;
            return;
        }
        Rep()->frame_direction_06e = 3;
        Rep()->subcycle_064 = Rep()->last_frame_095;
    }
}

/* Per-frame prop animation service called from WorldUpdateProps.  While the
   rep is animating it converts the game timer's progress into whole elapsed
   frames and drives AdvanceAnimationValue0044C310; a finished run leaves the
   0x20 pathing-dirty bit set for the next call to rebuild.  A pending
   pending_subcycle_066 frame is latched into subcycle_064 first, and an idle prop with the
   random flag may start a fresh run on its own. */
// FUNCTION: WIZ8 0x0044c030
void W8Prop::UpdatePropAnimation0044C030()
{
    W8PropRepresentation* rep = Rep();
    unsigned int total;
    int frames;

    if (rep->active == 0 &&
        (rep->animation_behaviour_070 != 1 || rep->animation_playing_06d == 0) &&
        (flags_1c & 0x20) == 0) {
        return;
    }
    total = AnimObjValue004A15D0(rep->animation, 2);
    anim_frame_fraction_024 = 0.0f;
    if (rep->pending_subcycle_066 != 0xffff) {
        if (static_cast<short>(rep->pending_subcycle_066) < static_cast<int>(total)) {
            rep->subcycle_064 = static_cast<unsigned char>(rep->pending_subcycle_066);
        }
        rep->pending_subcycle_066 = 0xffff;
    }
    if (static_cast<int>(total) < 2 || rep->animation_running_0a4 != 0) {
        return;
    }
    if (rep->animation_playing_06d == 0 && rep->random_play_0a5 != 0 &&
        rand() * (1.0f / RAND_MAX) < rep->play_chance_0a8) {
        if (rep->frame_direction_06e == 2) {
            rep->subcycle_064 = rep->first_frame_094;
            rep->frame_direction_06e = 1;
        } else {
            rep->subcycle_064 = rep->last_frame_095;
            rep->frame_direction_06e = 3;
        }
        m_pTimer->Restart();
        rep->animation_playing_06d = 1;
    }
    if (rep->animation_playing_06d == 0) {
        if ((flags_1c & 0x20) == 0) {
            return;
        }
        ApplyAnimationPaths0044C200(GetWorld());
        BuildOrRefreshPathingRepresentation();
        flags_1c &= ~0x20;
        return;
    }
    anim_frame_fraction_024 = m_pTimer->GetProgress();
    BuildOrRefreshPathingRepresentation();
    frames = static_cast<int>(anim_frame_fraction_024);
    anim_frame_fraction_024 -= frames;
    rep->frame_steps_0ad = static_cast<unsigned char>(frames);
    if (frames != 0) {
        W8AnimObj* animation;

        AdvanceAnimationValue0044C310(frames, static_cast<char>(total));
        animation = Rep()->animation;
        if (animation->path_24 != 0) {
            if ((flags_1c & 2) != 0) {
                rep->frame_index_0a0 += frames;
                if (rep->frame_index_0a0 >= animation->frame_count_16) {
                    rep->frame_index_0a0 = animation->frame_count_16;
                }
            } else {
                rep->frame_index_0a0 = rep->subcycle_064;
            }
            PathAISetValue004A9F60(animation->path_24, static_cast<float>(rep->frame_index_0a0));
        }
    }
    if (rep->animation_playing_06d == 0) {
        flags_1c |= 0x20;
    }
}

/* Walk every path list entry, apply the path to its mesh, and roll the prop's
   position snapshots: the previous current becomes the old position and the
   path's location becomes the new current. */
// FUNCTION: WIZ8 0x0044c200
void W8Prop::ApplyAnimationPaths0044C200(W8World* world)
{
    unsigned int count;
    int index;

    if (world == 0) {
        srAssertFail("pWorld", PROP_CPP, 0x3db, 0);
    }
    if (AnimationIsRunning(Rep()->animation) == 1) {
        count = AnimObjListCount004A1620(Rep()->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            stModelInstance* mesh = static_cast<stModelInstance*>(
                AnimObjDispatchList004A1560(Rep()->animation, 2, static_cast<signed char>(index)));
            W8PathAI* path;

            if (mesh == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x3ea, 0);
            }
            path = AnimObjListEntry004A16C0(Rep()->animation, 2, static_cast<signed char>(index));
            if (path != 0) {
                srVector3T<float> location;

                PathAIApply004AA520(path, mesh);
                static_cast<srNode*>(mesh)->getLocation(location);
                position_03c = position_02c;
                position_02c = location;
            }
        }
    }
}

/* Advance the rep's animation value by `frames` in the rep's direction.
   Transitive animations clamp at either end and complete: the run stops, the
   linked triggers fire and the global redraw flag goes up.  Looping kinds
   wrap or, when frame_method_06f is two, bounce off the ends and flip direction; a
   step larger than the counter range folds through whole trips.  The new
   value is clamped and pushed to every bound path. */
// FUNCTION: WIZ8 0x0044c310
void W8Prop::AdvanceAnimationValue0044C310(int frames, char total)
{
    W8PropRepresentation* rep = Rep();
    unsigned int frame = rep->subcycle_064;
    char behaviour = rep->frame_method_06f;
    int end = rep->last_frame_095;
    int start = rep->first_frame_094;
    unsigned int count;
    int index;

    if (behaviour == 3) {
        rep->subcycle_064 = static_cast<unsigned char>(Random(total));
    } else if (rep->animation_behaviour_070 == 1) {
        if (rep->frame_direction_06e == 1) {
            if (static_cast<int>(frame) + frames < end) {
                rep->subcycle_064 = static_cast<unsigned char>(frame + frames);
            } else {
                rep->subcycle_064 = rep->last_frame_095;
                rep->animation_playing_06d = 0;
                rep->frame_direction_06e = 2;
                if (trigger_18 != 0) {
                    trigger_18->RunLinkedTriggers00441590();
                }
                gXStatus.sight_refresh_pending_a03 = 1;
            }
        } else if (rep->frame_direction_06e == 3) {
            if (static_cast<int>(frame) - frames > start) {
                rep->subcycle_064 = static_cast<unsigned char>(frame - frames);
            } else {
                rep->subcycle_064 = rep->first_frame_094;
                rep->animation_playing_06d = 0;
                rep->frame_direction_06e = 4;
                if (trigger_18 != 0) {
                    trigger_18->RunLinkedTriggers00441590();
                }
                gXStatus.sight_refresh_pending_a03 = 1;
            }
        }
    } else {
        int range = end - start;

        if (frames < range) {
            if (rep->frame_direction_06e == 1) {
                frame += frames;
                if (static_cast<int>(frame) <= end) {
                    rep->subcycle_064 = static_cast<unsigned char>(frame);
                } else if (behaviour == 2) {
                    rep->frame_direction_06e = 3;
                    rep->subcycle_064 =
                        static_cast<unsigned char>(2 * end - static_cast<int>(frame));
                } else {
                    rep->subcycle_064 =
                        static_cast<unsigned char>(static_cast<int>(frame) - range - 1);
                }
            } else if (rep->frame_direction_06e == 3) {
                frame -= frames;
                if (static_cast<int>(frame) >= start) {
                    rep->subcycle_064 = static_cast<unsigned char>(frame);
                } else if (behaviour == 2) {
                    rep->subcycle_064 =
                        static_cast<unsigned char>(2 * start - static_cast<int>(frame));
                    rep->frame_direction_06e = 1;
                } else {
                    rep->subcycle_064 =
                        static_cast<unsigned char>(range + static_cast<int>(frame) + 1);
                }
            }
        } else if (rep->frame_direction_06e == 1) {
            int effective = static_cast<int>(frame) - start + frames;

            if (behaviour == 2) {
                int trips = effective / range;
                int remainder = effective - trips * range;

                if (trips % 2 == 0) {
                    rep->subcycle_064 = static_cast<unsigned char>(start + remainder);
                } else {
                    rep->frame_direction_06e = 3;
                    rep->subcycle_064 = static_cast<unsigned char>(end - remainder);
                }
            } else {
                rep->subcycle_064 = static_cast<unsigned char>(
                    start + effective - effective / (range + 1) * (range + 1));
            }
        } else if (rep->frame_direction_06e == 3) {
            int effective = 2 * start - static_cast<int>(frame) + frames;

            if (behaviour == 2) {
                int trips = effective / range;
                int remainder = effective - trips * range;

                if (trips % 2 == 0) {
                    rep->subcycle_064 = static_cast<unsigned char>(start + remainder);
                    rep->frame_direction_06e = 1;
                } else {
                    rep->subcycle_064 = static_cast<unsigned char>(end - remainder);
                }
            } else {
                int folded = effective - 1;

                rep->subcycle_064 =
                    static_cast<unsigned char>(end + folded / (range + 1) * (range + 1) - folded);
            }
        }
    }
    if (rep->subcycle_064 < rep->first_frame_094) {
        rep->subcycle_064 = rep->first_frame_094;
    } else if (rep->subcycle_064 > rep->last_frame_095) {
        rep->subcycle_064 = rep->last_frame_095;
    }
    if (AnimationIsRunning(rep->animation) == 1) {
        count = AnimObjListCount004A1620(rep->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            W8PathAI* path =
                AnimObjListEntry004A16C0(rep->animation, 2, static_cast<signed char>(index));

            if (path != 0) {
                PathAISetValue004A9F60(path, static_cast<float>(rep->subcycle_064));
            }
        }
    }
}

/* The animation value one step ahead, exactly as
   AdvanceAnimationValue0044C310 would compute a single step: transitive kinds
   clamp at the ends, looping kinds wrap to the opposite end, and behaviour
   two steps back instead.  Retail returns the literal one rather than
   first_frame_094 + 1 when a bouncing run sits at the start. */
// FUNCTION: WIZ8 0x0044c600
char W8Prop::NextAnimationValue0044C600()
{
    W8PropRepresentation* rep = Rep();
    char direction = rep->frame_direction_06e;

    if (rep->animation_behaviour_070 == 1) {
        if (direction == 1) {
            if (rep->subcycle_064 < rep->last_frame_095) {
                return rep->subcycle_064 + 1;
            }
        } else if (direction == 3 && rep->first_frame_094 < rep->subcycle_064) {
            return rep->subcycle_064 - 1;
        }
        return rep->subcycle_064;
    }
    if (direction == 1) {
        if (rep->subcycle_064 != rep->last_frame_095) {
            return rep->subcycle_064 + 1;
        }
        if (rep->frame_method_06f == 2) {
            return rep->subcycle_064 - 1;
        }
        return rep->first_frame_094;
    }
    if (direction != 3) {
        return rep->subcycle_064;
    }
    if (rep->subcycle_064 > rep->first_frame_094) {
        return rep->subcycle_064 - 1;
    }
    if (rep->frame_method_06f == 2) {
        return 1;
    }
    return rep->last_frame_095;
}

/* When the next animation step moves the rep exactly one frame, write the
   delta between the current and previous positions into `out` and return the
   elapsed frame count; otherwise `out` is zeroed.  `point` is accepted but
   never read. */
// FUNCTION: WIZ8 0x0044e130
char W8Prop::GetDelta0044E130(srVector3T<float>* out, const srVector3T<float>* point)
{
    unsigned char next = static_cast<unsigned char>(NextAnimationValue0044C600());

    if (abs(next - Rep()->subcycle_064) == 1) {
        *out = position_02c - position_03c;
        return Rep()->frame_steps_0ad;
    }
    out->SetZero();
    return 0;
}

/* Whether the prop can be used from where the caller is. The owned GDProp has
   to be there, its own owner has to be, that owner must not be in the tenth
   state or hold either of two bits, and the reach test has to pass. */
// FUNCTION: WIZ8 0x0044e0c0
bool W8Prop::CanBeUsedFrom(int arg_2, int arg_3, char notify)
{
    Trigger* owner;
    W8TriggerActionData* action;

    if ((flags_1c & 0x80) == 0 || m_gd_prop == 0) {
        return false;
    }
    owner = m_gd_prop->m_owner_24;
    if (owner == 0) {
        return false;
    }

    action = owner->m_pActionData;
    if (action == 0 || action->type_004 != 10) {
        action = 0;
    }
    if ((owner->lock_state.lock_type != 0 && owner->lock_state.device_state.completed == 0) ||
        (action != 0 && (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 5) != 0)) {
        return false;
    }
    if (!m_gd_prop->ContainsPathCoordinate004B75F0(static_cast<unsigned short>(arg_2),
                                                   static_cast<unsigned short>(arg_3))) {
        return false;
    }
    if (notify) {
        owner->Activate00444750();
    }
    return true;
}

/* After a successful load, bind the current animation frame's mesh to its
   path and, when the animation is already running, snapshot the live
   position into the prop. */
// FUNCTION: WIZ8 0x0044c670
void W8Prop::ApplyAnimationFrame0044C670()
{
    unsigned int count;
    int index;

    if (AnimationIsRunning(static_cast<W8PropRepresentation*>(m_pRep)->animation) != 1) {
        W8PropRepresentation* rep = static_cast<W8PropRepresentation*>(m_pRep);
        unsigned char frame = rep->subcycle_064;
        srModelInstance* mesh;
        W8PathAI* path;

        if (AnimationIsRunning(rep->animation) == 0) {
            mesh = AnimObjDispatch004A14D0(rep->animation, 2, frame);
        } else {
            mesh = AnimObjDispatchList004A1560(rep->animation, 2, 0);
        }
        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x581, 0);
        }
        path = static_cast<W8PropRepresentation*>(m_pRep)->animation->path_24;
        if (path != 0) {
            PathAISetValue004A9F60(
                path, static_cast<float>(static_cast<W8PropRepresentation*>(m_pRep)->subcycle_064));
            PathAIApply004AA520(static_cast<W8PropRepresentation*>(m_pRep)->animation->path_24,
                                mesh);
        }
        return;
    }

    count = AnimObjListCount004A1620(static_cast<W8PropRepresentation*>(m_pRep)->animation, 2);
    for (index = 0; index < (int)count; ++index) {
        srModelInstance* mesh = AnimObjDispatchList004A1560(
            static_cast<W8PropRepresentation*>(m_pRep)->animation, 2, (signed char)index);
        W8PathAI* path;

        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x56f, 0);
        }
        path = AnimObjListEntry004A16C0(static_cast<W8PropRepresentation*>(m_pRep)->animation, 2,
                                        (signed char)index);
        if (path != 0) {
            srVector3T<float> location;

            PathAISetValue004A9F60(
                path, static_cast<float>(static_cast<W8PropRepresentation*>(m_pRep)->subcycle_064));
            PathAIApply004AA520(path, mesh);
            static_cast<srNode*>(mesh)->getLocation(location);
            position_02c = location;
            position_03c = location;
        }
    }
    flags_1c |= 0x20;
    BuildOrRefreshPathingRepresentation();
}

/* Per-frame prop update: while the animation is running this binds every
   dispatched instance to the world's dynamic scene, interpolates between the
   current and next keyframe records (position lerp, quaternion slerp for
   rotations - the same algorithm as PathAIApply004AA520), pushes the transform
   onto the instance or its child chain, and rolls the position snapshots
   forward. A stopped animation binds the single current instance and applies
   either the rep's path or its stored transform. */
// FUNCTION: WIZ8 0x0044c830
void W8Prop::AttachAnimationInstances0044C830(W8World* world)
{
    int index;
    int count;
    bool has_scales;
    unsigned char next_frame;
    stModelInstance* instance;
    stMeshModel* mesh;
    W8PathAI* path;
    srNode* node;
    srVector3T<float> zero;
    srVector3T<float> position;
    srVector3T<float> current;
    srVector3T<float> next_pos;
    srVector3T<float> current_scale;
    srVector3T<float> next_scale;
    srVector3T<float> scale_vector;
    srVector3T<float> rep_position;
    srVector3T<double> location;
    srVector3T<double> scale_location;
    srMatrix3T<float> rotation;
    srMatrix3T<float> next;
    srMatrix3T<float> rep_rotation;
    float inv;

    if (Rep()->active == 0) {
        return;
    }
    if (world == 0) {
        srAssertFail("pWorld", PROP_CPP, 0x5a1, 0);
    }
    if (AnimationIsRunning(Rep()->animation) == 1) {
        has_scales = false;
        zero.SetZero();
        count = static_cast<int>(AnimObjListCount004A1620(Rep()->animation, 2));
        for (index = 0; index < count; ++index) {
            instance = static_cast<stModelInstance*>(
                AnimObjDispatchList004A1560(Rep()->animation, 2, static_cast<signed char>(index)));
            if (instance == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x5b4, 0);
            }
            instance->clearFlag(srNode::FLAG_DISABLE);
            instance->setParent(world->dynamic_scene, 1);
            instance->light_scale_194 = zero;
            if (g_settings_6850c8.smooth_world_animations != 0) {
                instance->frame_interpolation_1ac = anim_frame_fraction_024;
            } else {
                instance->frame_interpolation_1ac = 0.0f;
            }
            mesh = static_cast<stMeshModel*>(instance->getModel());
            if (mesh != 0 && (mesh->flags_3a0 & 1) != 0 && trigger_18 == 0) {
                instance->render_flags_178 |= 0x10;
            }
            path = AnimObjListEntry004A16C0(Rep()->animation, 2, static_cast<signed char>(index));
            if (path == 0) {
                continue;
            }
            next_frame = static_cast<unsigned char>(NextAnimationValue0044C600());
            if (next_frame > Rep()->last_frame_095) {
                anim_frame_fraction_024 = 0.0f;
            }
            rotation = path->rotations_14[Rep()->subcycle_064];
            next = path->rotations_14[next_frame];
            instance->getRotation(rotation_048);
            if (!(rotation == next)) {
                W8Quaternion::InterpolateRotation(rotation, next, anim_frame_fraction_024,
                                                  &rotation);
            }
            rotation_06c = rotation;
            current = **path->nodes_0c->GetAt(Rep()->subcycle_064);
            next_pos = **path->nodes_0c->GetAt(next_frame);
            inv = g_float_005ebb38 - anim_frame_fraction_024;
            position = current * inv + next_pos * anim_frame_fraction_024;
            if (path->scales_18 != 0) {
                has_scales = true;
                current_scale = path->scales_18[Rep()->subcycle_064];
                next_scale = path->scales_18[next_frame];
                scale_vector = current_scale * inv + next_scale * anim_frame_fraction_024;
            }
            node = instance->firstChild();
            if (node == 0) {
                instance->setRotation(rotation);
                location.SetFromFloat(&position);
                instance->setLocation(location);
                if (has_scales) {
                    scale_location.SetFromFloat(&scale_vector);
                    instance->setScale(scale_location);
                }
            } else {
                do {
                    node->setRotation(rotation);
                    location.SetFromFloat(&position);
                    node->setLocation(location);
                    if (has_scales) {
                        scale_location.SetFromFloat(&scale_vector);
                        node->setScale(scale_location);
                    }
                    node = node->nextSibling();
                } while (node != 0);
            }
            position_03c = position_02c;
            position_02c = position;
        }
    } else {
        instance = static_cast<stModelInstance*>(Rep()->ToggleAnimation(Rep()->subcycle_064));
        if (instance == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x624, 0);
        }
        instance->clearFlag(srNode::FLAG_DISABLE);
        instance->setParent(world->dynamic_scene, 1);
        instance->light_scale_194.SetZero();
        if (g_settings_6850c8.smooth_world_animations != 0) {
            instance->frame_interpolation_1ac = anim_frame_fraction_024;
        } else {
            instance->frame_interpolation_1ac = 0.0f;
        }
        mesh = static_cast<stMeshModel*>(instance->getModel());
        if (mesh != 0 && (mesh->flags_3a0 & 1) != 0 && trigger_18 == 0) {
            instance->render_flags_178 |= 0x10;
        }
        if (Rep()->animation->path_24 != 0) {
            PathAIApply004AA520(Rep()->animation->path_24, instance);
        } else {
            Rep()->GetLocation004B8890(&rep_position);
            Rep()->GetRotation004B88F0(&rep_rotation);
            node = instance->firstChild();
            if (node == 0) {
                location.SetFromFloat(&rep_position);
                instance->setLocation(location);
                instance->setRotation(rep_rotation);
            } else {
                do {
                    location.SetFromFloat(&rep_position);
                    node->setLocation(location);
                    node->setRotation(rep_rotation);
                    node = node->nextSibling();
                } while (node != 0);
            }
        }
        BakeInstanceVertexLightingIfNeeded0046F4A0(instance, world->dynamic_scene);
    }
    if (m_gd_prop == 0) {
        BuildOrRefreshPathingRepresentation();
    }
}

/* The detach counterpart to AttachAnimationInstances0044C830: the current frame is stashed in
   saved_subcycle_0ac, then every dispatched instance is flagged disabled and detached
   from the scene.  While the animation runs the whole list is walked;
   otherwise only the snapshot frame's instance (or the first dispatch-list
   entry when a run is in progress) is pulled. */
// FUNCTION: WIZ8 0x0044d360
void W8Prop::DetachAnimationInstances0044D360(W8World* world)
{
    stModelInstance* instance;
    unsigned int count;
    int index;

    if (world == 0) {
        srAssertFail("pWorld", PROP_CPP, 0x66b, 0);
    }
    Rep()->saved_subcycle_0ac = Rep()->subcycle_064;
    if (AnimationIsRunning(Rep()->animation) == 1) {
        count = AnimObjListCount004A1620(Rep()->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            instance = static_cast<stModelInstance*>(
                AnimObjDispatchList004A1560(Rep()->animation, 2, static_cast<signed char>(index)));
            if (instance == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x678, 0);
            }
            instance->setFlag(srNode::FLAG_DISABLE);
            instance->setParent(0, 1);
        }
    } else {
        unsigned char frame = Rep()->subcycle_064;

        if (AnimationIsRunning(Rep()->animation) == 0) {
            instance =
                static_cast<stModelInstance*>(AnimObjDispatch004A14D0(Rep()->animation, 2, frame));
        } else {
            instance =
                static_cast<stModelInstance*>(AnimObjDispatchList004A1560(Rep()->animation, 2, 0));
        }
        if (instance == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x686, 0);
        }
        instance->setFlag(srNode::FLAG_DISABLE);
        instance->setParent(0, 1);
    }
}

/* Restore the rep's persisted animation state: frame, the two counters, the
   direction flags and one byte the format no longer uses.  Each saved index
   is clamped to the loaded animation's frame count, path values are re-synced
   while a running animation is active, and ApplyAnimationFrame0044C670 reapplies the state.
   The read chain's success is reported even though the restore runs either
   way. */
// FUNCTION: WIZ8 0x0044dbd0
bool W8Prop::LoadAnimationState0044DBD0(int hFile)
{
    unsigned char unused;
    unsigned int count;
    int index;
    int total;
    bool success;
    W8PathAI* path;

    success = FileRead(hFile, &Rep()->subcycle_064, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->first_frame_094, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->last_frame_095, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->frame_direction_06e, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->animation_playing_06d, 1, 0) != 0 &&
              FileRead(hFile, &unused, 1, 0) != 0;
    if (Rep()->animation != 0) {
        total = static_cast<int>(AnimObjValue004A15D0(Rep()->animation, 2));
        if (Rep()->last_frame_095 >= total) {
            Rep()->last_frame_095 = static_cast<unsigned char>(total - 1);
        }
        if (Rep()->first_frame_094 >= total) {
            Rep()->first_frame_094 = static_cast<unsigned char>(total - 1);
        }
        if (Rep()->subcycle_064 >= total) {
            Rep()->subcycle_064 = static_cast<unsigned char>(total - 1);
        }
        if (AnimationIsRunning(Rep()->animation) == 1) {
            count = AnimObjListCount004A1620(Rep()->animation, 2);
            for (index = 0; index < static_cast<int>(count); ++index) {
                path =
                    AnimObjListEntry004A16C0(Rep()->animation, 2, static_cast<signed char>(index));
                if (path != 0) {
                    PathAISetValue004A9F60(path, static_cast<float>(Rep()->subcycle_064));
                }
            }
        }
        ApplyAnimationFrame0044C670();
    }
    return success;
}

/* Union of the per-frame bounds over every frame of the rep's animation.
   The rep's current frame is saved, the bounds for frame zero seed the merge,
   and each remaining frame expands the result before the frame is restored. */
// FUNCTION: WIZ8 0x0044dd60
void W8Prop::GetBounds0044DD60(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    srVector3T<float> local_minimum;
    srVector3T<float> local_maximum;
    unsigned char saved_frame;
    int frame;
    int total;

    total = static_cast<int>(AnimObjValue004A15D0(Rep()->animation, 2));
    saved_frame = Rep()->subcycle_064;
    Rep()->subcycle_064 = 0;
    AnimObjGetBounds004A1710(Rep()->animation, 2, Rep()->subcycle_064, minimum, maximum);
    for (frame = 1; frame < total; ++frame) {
        Rep()->subcycle_064 = static_cast<unsigned char>(frame);
        AnimObjGetBounds004A1710(Rep()->animation, 2, Rep()->subcycle_064, &local_minimum,
                                 &local_maximum);
        if (local_minimum.x < minimum->x) {
            minimum->x = local_minimum.x;
        }
        if (local_minimum.y < minimum->y) {
            minimum->y = local_minimum.y;
        }
        if (local_minimum.z < minimum->z) {
            minimum->z = local_minimum.z;
        }
        if (local_maximum.x > maximum->x) {
            maximum->x = local_maximum.x;
        }
        if (local_maximum.y > maximum->y) {
            maximum->y = local_maximum.y;
        }
        if (local_maximum.z > maximum->z) {
            maximum->z = local_maximum.z;
        }
    }
    Rep()->subcycle_064 = saved_frame;
}

/* Build or refresh the pathing representation for a collidable Prop.  Retail
   requires a transitive animation with one mesh, then either constructs the
   owned GDProp or reinitializes it for the current animation frame. */
// FUNCTION: WIZ8 0x0044dea0
int W8Prop::BuildOrRefreshPathingRepresentation()
{
    srModelInstance* instance;

    if ((flags_1c & 1) == 0) {
        return 0;
    }
    if (AnimationIsRunning(Rep()->animation) != 1) {
        ShutdownWithErrorBox("Collidable props can be of Transitive animation type only.");
    }
    if (AnimObjListCount004A1620(Rep()->animation, 2) != 1) {
        ShutdownWithErrorBox("Collideable props should have a single mesh.");
    }
    instance = AnimObjDispatchList004A1560(Rep()->animation, 2, 0);
    if (instance == 0) {
        srAssertFail("pstInstance", PROP_CPP, 0x939, 0);
    }

    if (m_gd_prop == 0) {
        m_gd_prop = new GDProp(instance, m_name, static_cast<unsigned short>(Rep()->subcycle_064),
                               Rep()->footstep_surface_0c0, Rep()->footstep_material_0c1);
    } else {
        if ((flags_1c & 0x20) != 0) {
            m_gd_prop->Initialize(instance, 1, static_cast<unsigned short>(Rep()->subcycle_064),
                                  Rep()->footstep_surface_0c0, Rep()->footstep_material_0c1);
        } else {
            m_gd_prop->Initialize(instance, 0, 0, Rep()->footstep_surface_0c0,
                                  Rep()->footstep_material_0c1);
        }
        if (Rep()->animation_behaviour_070 == 1) {
            g_byte_00659a64 = 1;
        }
    }
    return m_gd_prop->m_surface_count_14;
}

/* Run the prop's own trigger for a missile impact.  Only the three prop
   animation actions (0x3a..0x3c) accept the missile's table index as their
   source; every other trigger setup is ignored. */
// FUNCTION: WIZ8 0x0044e230
void W8Prop::RunMissileTrigger0044E230(W8AIMissile* record)
{
    if (record != 0 && record->missile_0c != 0 && trigger_18 != 0 &&
        (trigger_18->initial_action_22a == 0x3a || trigger_18->initial_action_22a == 0x3b ||
         trigger_18->initial_action_22a == 0x3c)) {
        trigger_18->Run(record->missile_0c->missile_table_index_1d8);
    }
}

/* Mirror of GetPosition0044E2C0: while the rep node reports itself current
   the position is stored in position_02c, otherwise it goes through the rep
   node's own location. */
// FUNCTION: WIZ8 0x0044e310
void W8Prop::SetPosition0044E310(srVector3T<float>* position)
{
    if (AnimationIsRunning(Rep()->animation) == 1) {
        position_02c = *position;
        return;
    }
    m_pRep->SetLocation004B8850(position);
}

// FUNCTION: WIZ8 0x0044e360
bool W8Prop::TriggerHasActionMessage0044E360()
{
    return trigger_18 != 0 && trigger_18->HasActionMessage00441780() != 0;
}

// FUNCTION: WIZ8 0x0044e380
bool W8Prop::TriggerRequiresItem0044E380()
{
    return trigger_18 != 0 && trigger_18->RequiresItem00441790() != 0;
}

/* Whether a prop with a selectable trigger is visible from `position`: the
   trigger's distance interval has to contain the centre of the current
   frame's bounds and the centre, minimum or maximum has to project
   on-screen through the active world's camera. */
// FUNCTION: WIZ8 0x0044e3a0
bool W8Prop::IsTriggerInView0044E3A0(srVector3T<float>* position)
{
    Trigger* trigger = trigger_18;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<float> center;
    srVector3T<float> projected;
    float distance;

    if (trigger != 0 && (trigger->flags_0a0 & W8_TRIGGER_ENABLED) != 0 &&
        ((trigger->flags_0a0 & 0x40000) == 0 || (trigger->flags_0a0 & 0x80000) == 0)) {
        AnimObjGetBounds004A1710(Rep()->animation, 2, Rep()->subcycle_064, &minimum, &maximum);
        center.Set((minimum.x + maximum.x) * g_double_005ebe80,
                   (minimum.y + maximum.y) * g_double_005ebe80,
                   (minimum.z + maximum.z) * g_double_005ebe80);
        distance = (center - *position).Length();
        if (distance < trigger->range_maximum_0a8 && trigger->range_minimum_0a4 <= distance) {
            {
                srVector3T<double> input(static_cast<double>(center.x),
                                         static_cast<double>(center.y),
                                         static_cast<double>(center.z));

                if (g_world->camera->project(projected, input) ==
                    srCamera::PROJECTION_RESULT_POSITIONAL_0) {
                    return true;
                }
            }
            {
                srVector3T<double> input(static_cast<double>(minimum.x),
                                         static_cast<double>(minimum.y),
                                         static_cast<double>(minimum.z));

                if (g_world->camera->project(projected, input) ==
                    srCamera::PROJECTION_RESULT_POSITIONAL_0) {
                    return true;
                }
            }
            {
                srVector3T<double> input(static_cast<double>(maximum.x),
                                         static_cast<double>(maximum.y),
                                         static_cast<double>(maximum.z));

                if (g_world->camera->project(projected, input) ==
                    srCamera::PROJECTION_RESULT_POSITIONAL_0) {
                    return true;
                }
            }
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x0044bf50
bool CreateAndLoadProp0044BF50(W8ReadLevelInfo* info, W8Prop** prop_out)
{
    W8Prop* prop;
    bool success;

    if (info == 0) {
        srAssertFail("pInfo", PROP_CPP, 0x344, 0);
    }
    prop = new W8Prop();
    if (prop == 0) {
        srAssertFail("pProp", PROP_CPP, 0x348, 0);
    }
    success = static_cast<W8PropRepresentation*>(prop->m_pRep)->LoadProp0044AEE0(info, prop);
    if (success) {
        *prop_out = prop;
        prop->m_pTimer->SetDuration(
            g_float_005ebb38 / static_cast<W8PropRepresentation*>(prop->m_pRep)->animation_speed);
        prop->ApplyAnimationFrame0044C670();
    }
    return success;
}

/* Resource loader rooted at CreateAndLoadProp.  The call site loads
   prop->m_pRep into ECX before the two stack arguments, so this is a
   PropRep method: LoadProp(pInfo, pProp). */
// FUNCTION: WIZ8 0x0044aee0
bool W8PropRepresentation::LoadProp0044AEE0(W8ReadLevelInfo* info, W8Prop* prop)
{
    int hFile;
    bool success;
    signed char version;
    unsigned char frame_count = 0;
    unsigned char option_byte = 0;
    unsigned char attach_flag = 0;
    float playback_scale = 0.0f;
    float flag_bits = 0.0f;
    W8AnimObj* animation = 0;
    bool result = false;
    long fail_line;

    if (info == 0 || info->hFile == 0 || prop == 0) {
        srAssertFail("pInfo && pInfo->hFile && pProp", PROP_CPP, 0xae, 0);
    }
    hFile = info->hFile;
    success = FileRead(hFile, &version, 1, 0);
    if (version < 4) {
        unsigned char b0 = 0;
        unsigned char b1 = 0;
        unsigned char b2 = 0;
        int ok;
        W8AniMesh* mesh;

        if (!success || (success = FileRead(hFile, &b0, 1, 0), !success) ||
            (success = FileRead(hFile, &b1, 1, 0), !success) ||
            (success = FileRead(hFile, &b2, 1, 0), !success)) {
            ok = 0;
        } else {
            ok = 1;
        }
        if (version < 2) {
            playback_scale = 15.0f;
            if (!ok) {
                fail_line = 0xcb;
                goto fail;
            }
        } else {
            if (!ok) {
                fail_line = 0xcb;
                goto fail;
            }
            success = FileRead(hFile, &playback_scale, 4, 0);
            if (!success) {
                fail_line = 0xcb;
                goto fail;
            }
        }
        info->mesh_filename = 0;
        mesh = CreateAniMesh004B57E0();
        if (mesh == 0) {
            srAssertFail("pAniMesh", PROP_CPP, 0xd5, 0);
        }
        result = LoadAniMeshFromInfo004B5B30(info, mesh, 1);
        if (!result) {
            fail_line = 0xd8;
            result = false;
            goto fail_with_result;
        }
        animation = CreateAnimObj004A01A0();
        animation->entries_18[2] = mesh;
        animation->group_count = 1;
        animation->animation_playing_01 = b0;
        animation->frame_method_02 = b1;
        animation->behaviour_03 = b2;
        animation->cycle = 0;
        animation->path_lists_05 = 0;
        animation->playback_scale_08 = playback_scale;
        this->animation = animation;
    } else {
        unsigned int entry_index;
        unsigned int list_count;
        signed char slot_count;
        int slot_i;

        flag_bits = 0.0f;
        info->mesh_filename = 0;
        if (success) {
            FileRead(hFile, &frame_count, 1, 0);
        }
        if (version > 4) {
            float lx = 0.0f;
            float ly = 0.0f;
            float lz = 0.0f;

            FileRead(hFile, &option_byte, 1, 0);
            FileRead(hFile, &lx, 4, 0);
            FileRead(hFile, &ly, 4, 0);
            FileRead(hFile, &lz, 4, 0);
            lx *= g_double_005ec150;
            ly *= g_double_005ec150;
            lz *= g_double_005ec150;
            this->local_location_010.x = lx;
            this->location_004.x = lx;
            this->local_location_010.y = ly;
            this->location_004.y = ly;
            this->local_location_010.z = lz;
            this->location_004.z = lz;
        }
        if (version > 5) {
            FileRead(hFile, &flag_bits, 4, 0);
            prop->flags_1c |= (unsigned int)flag_bits;
        }
        if (version > 6) {
            char* buffer = new char[0x40];
            int length = -1;
            char* scan = buffer;

            FileRead(hFile, buffer, 0x40, 0);
            do {
                if (length == 0) {
                    break;
                }
                --length;
            } while (*scan++ != '\0');
            if (length == -2) {
                delete[] buffer;
            } else {
                if (prop->m_name != 0) {
                    delete[] prop->m_name;
                }
                prop->m_name = buffer;
            }
        }
        if (version > 7) {
            slot_count = 0;
            FileRead(hFile, &slot_count, 1, 0);
            for (slot_i = 0; slot_i < slot_count; ++slot_i) {
                unsigned short frame_tmp = 0;
                unsigned short tag_tmp = 0;
                W8PropAnimationSegment* slot = new W8PropAnimationSegment;

                FileRead(hFile, &frame_tmp, 2, 0);
                slot->frame = static_cast<unsigned char>(frame_tmp);
                FileRead(hFile, &tag_tmp, 2, 0);
                slot->tag = static_cast<unsigned char>(tag_tmp);
                if (frame_count <= frame_tmp) {
                    srAssertFail("(usTemp < (UINT16)ubNumFrames)", PROP_CPP, 0x11f,
                                 SgpToWiz8NarrowText(String("%s Prop Error Segment %d frame n",
                                                            prop->m_name,
                                                            static_cast<unsigned int>(tag_tmp),
                                                            static_cast<unsigned int>(
                                                                frame_tmp))));
                }
                this->slots.Add(slot);
            }
        }
        animation = CreateAnimObj004A01A0();
        result = AnimObjReadFromFile004A05C0(info, animation, 1, 0, 1);
        this->animation = animation;
        if (AnimationIsRunning(animation) == 1) {
            animation->frame_count_16 = frame_count;
        }
        this->play_chance_0a8 = animation->play_chance_10;
        this->random_play_0a5 = animation->random_play_0c != 0;
        list_count = AnimObjValue004A15D0(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            srModelInstance* instance;
            stMeshModel* mesh_model;
            char* named;

            if (AnimationIsRunning(this->animation) == 0) {
                instance = AnimObjDispatch004A14D0(this->animation, 2, (unsigned char)entry_index);
            } else {
                instance = AnimObjDispatchList004A1560(this->animation, 2, 0);
            }
            named = reinterpret_cast<char*>(String("Prop: %s", prop->m_name));
            instance->setName(named);
            mesh_model = static_cast<stMeshModel*>(instance->model());
            for (; mesh_model != 0; mesh_model = mesh_model->next) {
                if (AnimationIsRunning(animation) == 0) {
                    mesh_model->GetVertexSunlight(1);
                }
            }
            if (option_byte != 0) {
                srNode* child;
                srVector3T<float> axis;

                axis.Set(0.0f, 1.0f, 0.0f);
                instance->setAlignment(1);
                instance->setAlignAxis(axis);
                child = instance->firstChild();
                if (child != 0) {
                    srModelInstance* child_instance = static_cast<srModelInstance*>(child);
                    child_instance->setAlignment(1);
                    child_instance->setAlignAxis(axis);
                }
            }
        }
        list_count = AnimObjListCount004A1620(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            W8PathAI* path = AnimObjListEntry004A16C0(animation, 2, (signed char)entry_index);
            if (path != 0) {
                PathAISetLooping004AA9D0(path, 1);
                PathAISetDiscreteMode004AAA10(path, 1);
                PathAISetScale004AA9C0(path, animation->playback_scale_08);
            }
        }
    }

    this->active = 1;
    this->animation_behaviour_070 = animation->behaviour_03;
    this->frame_method_06f = animation->frame_method_02;
    this->animation_playing_06d = animation->animation_playing_01;
    this->frame_direction_06e = 1;
    this->animation_speed = animation->playback_scale_08;
    this->timer_068 = GetTickCount();
    if (this->animation_behaviour_070 == 1 || this->animation_behaviour_070 == 2) {
        this->animation_playing_06d = 0;
        this->frame_direction_06e =
            static_cast<unsigned char>(((this->frame_method_06f != 2) - 1U & 2) + 2);
    }

    if (animation->path_lists_05 == 0) {
        W8AniMesh* mesh = AnimObjEntry004A1660(animation, 2, 0);
        unsigned char value_count = AniMeshValue004B64F0(mesh);
        stModelInstance* frame = GetAniMeshFrame004B6550(mesh, 0);
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        unsigned int frame_i;
        float extent;

        if (frame == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x182, 0);
        }
        frame->model()->getBoundingBox(minimum, maximum);
        for (frame_i = 0; frame_i < value_count; ++frame_i) {
            srVector3T<float> frame_min;
            srVector3T<float> frame_max;

            frame = GetAniMeshFrame004B6550(mesh, frame_i);
            if (frame == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x192, 0);
            }
            frame->model()->getBoundingBox(frame_min, frame_max);
            if (frame_min.x < minimum.x) {
                minimum.x = frame_min.x;
            }
            if (maximum.x < frame_max.x) {
                maximum.x = frame_max.x;
            }
            if (frame_min.y < minimum.y) {
                minimum.y = frame_min.y;
            }
            if (maximum.y < frame_max.y) {
                maximum.y = frame_max.y;
            }
            if (frame_min.z < minimum.z) {
                minimum.z = frame_min.z;
            }
            if (maximum.z < frame_max.z) {
                maximum.z = frame_max.z;
            }
        }
        {
            /* Retail writes the six floats in this interleaved order. */
            this->bounds_min_074.y = minimum.y;
            this->bounds_max_080.x = maximum.x;
            this->bounds_min_074.x = minimum.x;
            this->bounds_max_080.z = maximum.z;
            this->bounds_min_074.z = minimum.z;
            this->bounds_max_080.y = maximum.y;
        }
        extent = maximum.x - minimum.x;
        if (extent < maximum.y - minimum.y) {
            extent = maximum.y - minimum.y;
        }
        if (extent < maximum.z - minimum.z) {
            extent = maximum.z - minimum.z;
        }
        this->bounds_extent_08c = extent * g_float_005ebc7c;
    }

    if (version > 2) {
        if (!result) {
            result = false;
        } else {
            success = FileRead(hFile, &attach_flag, 1, 0);
            result = success;
        }
        if (attach_flag != 0) {
            Trigger* trigger;

            trigger = Trigger::CreateAndLoadLevelTrigger(hFile, info->world);
            /* Retail writes the attach fields first, then tests type at +0x22a
               (the stores do not touch that word). */
            trigger->m_bRepType = 2;
            trigger->m_pProp = prop;
            if (trigger->initial_action_22a == 0x40) {
                InitializeStateDrivenPropVariables00445200(trigger);
            }
            if (trigger->trigger_kind_018 == 1 && trigger->initial_action_22a == 8) {
                unsigned int path_count;
                unsigned int path_i;

                this->animation_running_0a4 = 1;
                if (AnimationIsRunning(this->animation) == 1) {
                    path_count = AnimObjListCount004A1620(this->animation, 2);
                    for (path_i = 0; path_i < path_count; ++path_i) {
                        W8PathAI* path =
                            AnimObjListEntry004A16C0(this->animation, 2, (signed char)path_i);
                        path->step_by_node_39 = 1;
                    }
                }
            }
            switch (trigger->initial_action_22a) {
            case 1:
            case 2:
            case 3:
            case 0x2c:
            case 0x32:
            case 0x33:
            case 0x40:
                this->animation_playing_06d = 0;
                break;
            }
            prop->trigger_18 = trigger;
            hFile = info->hFile;
        }
    }

    if (version >= 9) {
        unsigned char extra = 0;

        if (result) {
            success = FileRead(hFile, &extra, 1, 0);
            result = success;
        }
        if (extra != 0) {
            if (result && (success = FileRead(hFile, &this->footstep_surface_0c0, 1, 0), success) &&
                (success = FileRead(hFile, &this->footstep_material_0c1, 1, 0), success)) {
                result = 1;
            } else {
                result = false;
            }
        }
    }

    this->first_frame_094 = 0;
    this->subcycle_064 = 0;
    {
        unsigned int frames = AnimObjValue004A15D0(animation, 2);
        this->last_frame_095 = static_cast<unsigned char>(frames) - 1;
    }
    return result;

fail:
    srAssertFail("fSuccess", PROP_CPP, fail_line, 0);
    return 0;

fail_with_result:
    srAssertFail("fSuccess", PROP_CPP, fail_line, 0);
    return result;
}

/* The prop representation's current animation value, or -1 while it owns no
   animation. */
// FUNCTION: WIZ8 0x0044ebe0
int W8Prop::GetAnimationState0044EBE0() const
{
    W8AnimObj* animation = Rep()->animation;
    if (animation != 0) {
        return (int)AnimObjValue004A15D0(animation, 2);
    }
    return -1;
}

/* Every model instance the representation's animation can display: one per
   frame of each mesh entry (a single-instance mesh contributes frame zero
   only), or one per frame of every mesh in each mesh list. */
// FUNCTION: WIZ8 0x0044e570
void W8Prop::CollectModelInstances(W8GrowableVector<stModelInstance*>* instances)
{
    W8AnimObj* animation = Rep()->animation;
    if (animation == 0) {
        return;
    }
    if (!AnimationIsRunning(animation)) {
        for (int group = 0; group < 3; ++group) {
            W8AniMesh* mesh = animation->entries_18[group];
            if (mesh == 0) {
                continue;
            }
            int frame_count = AniMeshValue004B64F0(mesh);
            if (mesh->flags_00 & W8_ANI_MESH_SINGLE_INSTANCE) {
                instances->Add(GetAniMeshFrame004B6550(mesh, 0));
            } else {
                for (int frame = 0; frame < frame_count; ++frame) {
                    instances->Add(GetAniMeshFrame004B6550(mesh, frame));
                }
            }
        }
    } else if (AnimationIsRunning(animation) == 1) {
        for (int group = 0; group < 3; ++group) {
            W8PList* meshes = animation->meshes_28[group];
            if (meshes == 0) {
                continue;
            }
            int mesh_count = PLLength(meshes);
            for (int index = 0; index < mesh_count; ++index) {
                W8AniMesh* mesh = static_cast<W8AniMesh*>(PLGet(meshes, index));
                if (mesh == 0) {
                    continue;
                }
                int frame_count = AniMeshValue004B64F0(mesh);
                for (int frame = 0; frame < frame_count; ++frame) {
                    instances->Add(GetAniMeshFrame004B6550(mesh, frame));
                }
            }
        }
    }
}

/* APST chunk writer: a 0xDEADD00D signature, the format version and the prop
   count, then one record per prop - a fixed 64-byte name plus the six rep
   bytes that LoadAnimationState0044DBD0 reads back (frame, counters, direction
   flags and the active byte). The per-prop byte writes only run while the
   previous writes succeed. */
// FUNCTION: WIZ8 0x0044e830
void SaveWorldProps0044E830(W8World* world, int handle)
{
    int index;
    int count;
    unsigned int signature;
    unsigned int version;
    char name[0x40];
    W8Prop* prop;

    signature = 0xDEADD00D;
    version = 1;
    FileWrite(handle, &signature, 4, 0);
    FileWrite(handle, &version, 4, 0);
    count = static_cast<int>(PLLength(world->plsProps));
    FileWrite(handle, &count, 4, 0);
    for (index = 0; index < count; ++index) {
        prop = static_cast<W8Prop*>(PLGet(world->plsProps, index));
        strcpy(name, prop->m_name);
        FileWrite(handle, name, 0x40, 0);
        if (FileWrite(handle, &prop->Rep()->subcycle_064, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->first_frame_094, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->last_frame_095, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->frame_direction_06e, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->animation_playing_06d, 1, 0) != 0) {
            FileWrite(handle, &prop->Rep()->active, 1, 0);
        }
    }
}

/* APST chunk reader. The 0xDEADD00D signature selects the name-keyed format;
   older saves carry a bare record count followed by each prop's object key.
   Records whose prop cannot be found are consumed by a scratch prop so the
   stream stays aligned. */
// FUNCTION: WIZ8 0x0044e9a0
void LoadWorldProps0044E9A0(W8World* world, int handle)
{
    int index;
    int count;
    int entries;
    int key;
    unsigned int signature;
    unsigned int version;
    char name[0x40];
    W8Prop* prop;
    W8Prop* entry;

    FileRead(handle, &signature, 4, 0);
    if (signature != 0xDEADD00D) {
        count = signature;
        for (index = 0; index < count; ++index) {
            FileRead(handle, &key, 4, 0);
            prop = 0;
            entries = static_cast<int>(PLLength(world->plsProps));
            for (int entry_index = 0; entry_index < entries; ++entry_index) {
                entry = static_cast<W8Prop*>(PLGet(world->plsProps, entry_index));
                if (entry->id_008 == key) {
                    prop = entry;
                    break;
                }
            }
            if (prop != 0) {
                prop->LoadAnimationState0044DBD0(handle);
            } else {
                prop = new W8Prop();
                prop->LoadAnimationState0044DBD0(handle);
                delete prop;
            }
        }
    } else {
        FileRead(handle, &version, 4, 0);
        FileRead(handle, &count, 4, 0);
        for (index = 0; index < count; ++index) {
            FileRead(handle, name, 0x40, 0);
            prop = FindPropByName(g_world, name);
            if (prop != 0) {
                prop->LoadAnimationState0044DBD0(handle);
            } else {
                prop = new W8Prop();
                prop->LoadAnimationState0044DBD0(handle);
                delete prop;
            }
        }
    }
}

/* The renderer owns the selected model instance. Without one, retail also
   resets the cached prop index to -1 before returning it. */
// FUNCTION: WIZ8 0x0044DA60
int GetSelectedPropIndex0044DA60(void)
{
    if (GetPickedModelInstance00427810() == 0) {
        return g_selected_prop_index_00607b98 = -1;
    }
    return g_selected_prop_index_00607b98;
}

/* When the renderer still holds a pick and ResolvePickedProp latched a
   trigger, run that trigger and post the nothing-happened / special-item
   notice. Clearing the pick also clears the latch. */
// FUNCTION: WIZ8 0x0044DA20
unsigned char ActivateSelectedProp0044DA20(void)
{
    if (GetPickedModelInstance00427810() == 0) {
        g_selected_prop_trigger_00659a60 = 0;
        return 0;
    }
    if (g_selected_prop_trigger_00659a60 != 0) {
        g_trigger_feedback_00606994 = 0;
        g_selected_prop_trigger_00659a60->Run(-1);
        g_selected_prop_trigger_00659a60->PrintNothingHappenedOrSpecialItemRequired004456E0();
        return 1;
    }
    return 0;
}
