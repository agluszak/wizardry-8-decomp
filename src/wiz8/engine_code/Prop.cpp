#include "wiz8/engine_code/AnimRep.hpp"
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

/* Reset before the world Prop update; set when a Prop whose animation
   behaviour is 1 rebuilds its pathing geometry.  GameData keeps the world
   updating while it is set. */
// GLOBAL: WIZ8 0x00659A64
bool g_animated_prop_present;

#define PROP_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp"

// VTABLE: WIZ8 0x005ec1e0
// class W8Prop
// VTABLE: WIZ8 0x005ec1c8
// class W8PropRepresentation
// VTABLE: WIZ8 0x005ec1d0
// class W8Vector<W8PropAnimationSegment*>
/* Prop::Prop() - GrObject base, then m_pRep / m_pTimer and two identity
   rotation bases.  Retail expands PropRep after the AnimRep constructor:
   scalar field stores, the capacity-5 slot vector, then the PropRep vtable. */
// FUNCTION: WIZ8 0x0044bc00
W8Prop::W8Prop()
{
    trigger = 0;
    flags = 0;
    m_name = 0;
    anim_frame_fraction = 0;
    kind = 4;
    id = AllocateGrObjectId();
    m_pRep = new W8PropRepresentation();
    m_pTimer = new W8GameTimer();
    animation_position.SetZero();
    previous_animation_position.SetZero();
    previous_animation_rotation.SetIdentity();
    animation_rotation.SetIdentity();
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
    : W8AnimRep(), animation_speed(other.animation_speed), frame_index(other.frame_index),
      animation_running(other.animation_running), random_play(other.random_play),
      play_chance(other.play_chance), saved_subcycle(other.saved_subcycle),
      frame_steps(other.frame_steps), slots(5), footstep_surface(other.footstep_surface),
      footstep_material(other.footstep_material)
{
    animation = CloneAnimObj(other.animation);
}

// FUNCTION: WIZ8 0x0044ae30
W8PropRepresentation::~W8PropRepresentation()
{
    int index;

    for (index = 0; index < slots.GetCount(); ++index) {
        delete *slots.GetAt(index);
    }
    slots.Clear();
    if (animation != 0) {
        DestroyAnimObj(animation);
    }
}

// FUNCTION: WIZ8 0x0044ef80
W8AnimRepBase* W8PropRepresentation::Clone()
{
    return new W8PropRepresentation(*this);
}

/* Visit every prop attached to the world and activate those whose companion
   object can be resolved and whose owned GDProp exists. */
// FUNCTION: WIZ8 0x0044e010
void UpdateWorldProps(W8World* world)
{
    unsigned int count;
    int index;

    if (world == 0 || world->plsProps == 0) {
        srAssertFail("pWorld && pWorld->plsProps",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 0x969, 0);
    }
    count = PLLength(world->plsProps);
    for (index = 0; index < static_cast<int>(count); ++index) {
        W8Prop* prop = GetWorldProp(world, index);
        Trigger* trigger;

        if (prop != 0 && (trigger = FindTriggerForProp(world, prop)) != 0 && prop->m_gd_prop != 0) {
            prop->flags |= W8_PROP_GD_TRIGGER_BOUND;
            prop->m_gd_prop->BindTrigger(trigger);
        }
    }
}

// FUNCTION: WIZ8 0x0044bec0
W8Prop::~W8Prop()
{
    delete static_cast<W8PropRepresentation*>(m_pRep);
    if (m_name != 0) {
        delete[] m_name;
    }
    delete m_pTimer;
    delete m_gd_prop;
}

// FUNCTION: WIZ8 0x0044db60
W8Prop* FindPropByName(W8World* world, const char* name)
{
    int index;

    if (world != 0 && name != 0) {
        unsigned int count = PLLength(world->plsProps);

        for (index = 0; index < static_cast<int>(count); ++index) {
            W8Prop* prop = GetWorldProp(world, index);
            if (prop->m_name != 0 && _stricmp(prop->m_name, name) == 0) {
                return prop;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0044e2c0
void W8Prop::GetPosition(srVector3T<float>* out)
{
    if (AnimationIsRunning(Rep()->animation) == 1) {
        *out = animation_position;
        return;
    }
    m_pRep->GetLocation(out);
}

// FUNCTION: WIZ8 0x0044e270
void W8Prop::SetAnimationSpeed(float speed)
{
    if (speed > 0.0f) {
        Rep()->animation_speed = speed;
        if (m_pTimer != 0) {
            m_pTimer->SetDuration(1.0f / Rep()->animation_speed);
        }
    }
}

/* Four accessors reaching through the owned member at 0x14. */
// FUNCTION: WIZ8 0x0044d4f0
unsigned char W8Prop::GetActivationState()
{
    return this->Rep()->active;
}

// FUNCTION: WIZ8 0x0044d5b0
void W8Prop::SetPendingAnimationSubcycle(char value)
{
    this->Rep()->pending_subcycle = value;
}

/* Whether the animation bounces between its first and last frames. */
// FUNCTION: WIZ8 0x0044e1c0
bool W8Prop::IsAnimationPingPong()
{
    return this->Rep()->frame_method == W8_ANIMATION_PING_PONG;
}

/* Switch between forward and reverse playback, preserving the retail
   forward fallback for every state other than running forward. */
// FUNCTION: WIZ8 0x0044e1d0
void W8Prop::ReverseAnimationDirection()
{
    this->Rep()->frame_direction = this->Rep()->frame_direction == W8_ANIMATION_FORWARD
                                       ? W8_ANIMATION_REVERSE
                                       : W8_ANIMATION_FORWARD;
}

/* The prop's own trigger at 0x18. */
// FUNCTION: WIZ8 0x0044d5a0
Trigger* W8Prop::GetTrigger()
{
    return this->trigger;
}

/* The trigger that owns this prop's GDProp, once the flag that says the
   GDProp is attached is up. */
// FUNCTION: WIZ8 0x0044e0a0
Trigger* W8Prop::GetGDPropOwnerTrigger()
{
    if ((this->flags & W8_PROP_GD_TRIGGER_BOUND) != 0 && this->m_gd_prop != 0) {
        return this->m_gd_prop->m_trigger;
    }
    return 0;
}

/* Store activation state, restarting the animation clock whenever the old
   state is inactive. The raw byte is retained by save/load. */
// FUNCTION: WIZ8 0x0044d4b0
void W8Prop::SetActivationState(unsigned char value)
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

    AnimObjGetBounds(Rep()->animation, 2, Rep()->subcycle, &first, &second);
    *position = (first + second) * 0.5;
}

// GLOBAL: WIZ8 0x00659A60
static Trigger* g_selected_prop_trigger;
// GLOBAL: WIZ8 0x00607B98
static int g_selected_prop_index = -1;

/* Whether the renderer's currently selected model instance is one of the
   instances this prop's animation dispatches.  With a running animation every
   list entry is checked; otherwise the single dispatched instance for the
   current frame is compared directly. */
// FUNCTION: WIZ8 0x0044d680
bool W8Prop::IsPickedProp(W8World* world)
{
    srModelInstance* instance;
    unsigned char frame;
    unsigned int count;
    int index;

    if (Rep()->active == 0) {
        return false;
    }
    if (AnimationIsRunning(Rep()->animation) == 1) {
        count = AnimObjListCount(Rep()->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            instance = AnimObjDispatchList(Rep()->animation, 2, static_cast<signed char>(index));
            if (GetPickedModelInstance() == instance) {
                return true;
            }
        }
        return false;
    }
    frame = Rep()->subcycle;
    if (AnimationIsRunning(Rep()->animation) == 0) {
        instance = AnimObjDispatch(Rep()->animation, 2, frame);
    } else {
        instance = AnimObjDispatchList(Rep()->animation, 2, 0);
    }
    return GetPickedModelInstance() == instance;
}

/* Resolve the renderer's picked model instance back to the prop and trigger
   that own it. A pick is accepted only while the prop is active, the trigger
   permits selection, and the prop centre lies inside the trigger's distance
   interval. */
// FUNCTION: WIZ8 0x0044d760
bool ResolvePickedProp(W8World* world)
{
    srModelInstance* selected;
    srVector3T<float> camera_position;
    unsigned int prop_count;
    int prop_index;
    bool valid;

    g_selected_prop_trigger = 0;
    g_selected_prop_index = -1;
    selected = GetPickedModelInstance();
    if (selected == 0) {
        return false;
    }

    valid = true;
    GetCameraPosition(&camera_position);
    prop_count = PLLength(world->plsProps);
    for (prop_index = 0; prop_index < static_cast<int>(prop_count); ++prop_index) {
        W8Prop* prop;
        W8PropRepresentation* representation;
        srModelInstance* instance;
        int instance_index;

        if (!valid) {
            return false;
        }
        if (g_selected_prop_trigger != 0) {
            return valid;
        }
        prop = GetWorldProp(world, prop_index);
        representation = prop->Rep();
        if (representation->active == 0) {
            continue;
        }

        instance = 0;
        if (AnimationIsRunning(representation->animation) == 1) {
            int count = static_cast<int>(AnimObjListCount(representation->animation, 2));
            for (instance_index = 0; instance_index < count; ++instance_index) {
                instance = AnimObjDispatchList(representation->animation, 2,
                                               static_cast<signed char>(instance_index));
                if (GetPickedModelInstance() == instance) {
                    break;
                }
            }
            if (instance_index == count) {
                continue;
            }
        } else {
            instance = representation->ToggleAnimation(representation->subcycle);
            if (GetPickedModelInstance() != instance) {
                continue;
            }
        }

        {
            Trigger* trigger = prop->trigger;
            g_selected_prop_trigger = trigger;
            if (trigger != 0 && (trigger->flags & W8_TRIGGER_ENABLED) != 0 &&
                ((trigger->flags & W8_TRIGGER_ONCE) == 0 ||
                 (trigger->flags & W8_TRIGGER_FIRED) == 0) &&
                (g_combat_inactive ||
                 (trigger->m_pActionData != 0 &&
                  trigger->m_pActionData->type == W8_TRIGGER_PAYLOAD_DOOR &&
                  !static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->open)) &&
                representation->active != 0) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;
                float distance;

                AnimObjGetBounds(representation->animation, 2, representation->subcycle, &minimum,
                                 &maximum);
                distance = ((minimum + maximum) * 0.5 - camera_position).Length();
                if (trigger->range_minimum <= distance) {
                    g_selected_prop_index = prop_index;
                    if (distance <= trigger->range_maximum) {
                        continue;
                    }
                }
            }
            valid = false;
            SetPickedModelInstance(0);
            g_selected_prop_trigger = 0;
            g_selected_prop_index = -1;
        }
    }
    return valid;
}

/* Start or stop the prop's own animation, whichever it is not doing. */
// FUNCTION: WIZ8 0x0044ba00
srModelInstance* W8PropRepresentation::ToggleAnimation(int argument)
{
    if (AnimationIsRunning(animation) == 0) {
        return AnimObjDispatch(animation, 2, argument);
    }
    return AnimObjDispatchList(animation, 2, 0);
}

/* Select the animation slot whose second byte carries the requested tag.
   The slot's first byte is the target frame; the current subcycle and that
   frame become the ordered range the animation plays through. */
// FUNCTION: WIZ8 0x0044ba50
unsigned char W8PropRepresentation::SelectAnimationSlot(unsigned char tag)
{
    int index;

    for (index = 0; index < slots.GetCount(); ++index) {
        W8PropAnimationSegment* slot = *slots.GetAt(index);

        if (slot->tag == tag) {
            signed char selected = slot->frame;

            if (selected < 0) {
                return 0;
            }
            first_frame = subcycle;
            last_frame = static_cast<unsigned char>(selected);
            if (static_cast<unsigned char>(selected) < subcycle) {
                last_frame = subcycle;
                first_frame = static_cast<unsigned char>(selected);
            }
            if (last_frame > subcycle) {
                frame_direction = W8_ANIMATION_FORWARD;
            } else {
                frame_direction = W8_ANIMATION_REVERSE;
            }
            animation_playing = 1;
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
    int index;

    for (index = 0; index < slots.GetCount(); ++index) {
        if ((*slots.GetAt(index))->frame == first_frame) {
            return index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x0044bb20
unsigned char W8PropRepresentation::AdvanceAnimationSegment()
{
    int count;
    int index;
    int segment;

    count = slots.GetCount();
    if (count < 3) {
        return 0;
    }
    segment = -1;
    for (index = 0; index < count; ++index) {
        if ((*slots.GetAt(index))->frame == first_frame) {
            segment = index;
            break;
        }
    }
    if (segment == -1) {
        srAssertFail("lSegment!=(-1)", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 0x2c3, 0);
    }
    if (segment == count - 2) {
        segment = 0;
    } else {
        ++segment;
    }
    first_frame = (*slots.GetAt(segment))->frame;
    last_frame = (*slots.GetAt(segment + 1))->frame;
    frame_direction = W8_ANIMATION_FORWARD;
    animation_playing = 1;
    subcycle = first_frame;
    return static_cast<unsigned char>(segment);
}

/* The same toggle reached through the prop rather than through the member. */
// FUNCTION: WIZ8 0x0044d500
srModelInstance* W8Prop::ToggleRepAnimation(int argument)
{
    W8PropRepresentation* rep = Rep();

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch(rep->animation, 2, argument);
    }
    return AnimObjDispatchList(rep->animation, 2, 0);
}

/* And again with the member's own stored argument instead of a caller's -
   which is what makes 0x64 the animation's default argument. */
// FUNCTION: WIZ8 0x0044d550
srModelInstance* W8Prop::ToggleRepAnimationDefault()
{
    W8PropRepresentation* rep = Rep();
    unsigned char argument = rep->subcycle;

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch(rep->animation, 2, argument);
    }
    return AnimObjDispatchList(rep->animation, 2, 0);
}

/* Resolve the bounds of the prop's current animation frame. */
// FUNCTION: WIZ8 0x0044d5c0
unsigned char W8Prop::PlayRepAnimation(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    AnimObjGetBounds(Rep()->animation, 2, Rep()->subcycle, minimum, maximum);
    return 1;
}

/* Set the running direction or completed endpoint state of the animation. */
// FUNCTION: WIZ8 0x0044e1f0
void W8Prop::SetAnimationDirection(W8AnimationDirection direction)
{
    if (m_pRep == 0) {
        srAssertFail("m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 2698, 0);
    }
    Rep()->frame_direction = direction;
}

/* Set the live representation state. When requested, choose the direction
   and endpoint of the transition from the current and target animation tags. */
// FUNCTION: WIZ8 0x0044da80
void W8Prop::SetRepresentationActive(unsigned char active, bool update_animation)
{
    Rep()->animation_playing = active;
    if (active == 0) {
        return;
    }

    m_pTimer->Restart();
    if (!update_animation) {
        return;
    }

    if (Rep()->animation_behaviour == W8_ANIMATION_PLAY_ONCE ||
        Rep()->animation_behaviour == W8_ANIMATION_REPEAT) {
        if ((Rep()->frame_direction == W8_ANIMATION_FORWARD_COMPLETE &&
             Rep()->frame_method != W8_ANIMATION_PING_PONG) ||
            (Rep()->frame_direction != W8_ANIMATION_FORWARD_COMPLETE &&
             Rep()->frame_method == W8_ANIMATION_PING_PONG)) {
            Rep()->frame_direction = W8_ANIMATION_FORWARD;
            Rep()->subcycle = Rep()->first_frame;
            return;
        }
        Rep()->frame_direction = W8_ANIMATION_REVERSE;
        Rep()->subcycle = Rep()->last_frame;
    }
}

/* Per-frame prop animation service called from WorldUpdateProps.  While the
   rep is animating it converts the game timer's progress into whole elapsed
   frames and drives AdvanceAnimationValue; a finished run leaves the
   0x20 pathing-dirty bit set for the next call to rebuild.  A pending
   pending_subcycle frame is latched into subcycle first, and an idle prop with the
   random flag may start a fresh run on its own. */
// FUNCTION: WIZ8 0x0044c030
void W8Prop::UpdatePropAnimation()
{
    W8PropRepresentation* rep = Rep();
    unsigned int total;
    int frames;

    if (rep->active == 0 &&
        (rep->animation_behaviour != W8_ANIMATION_PLAY_ONCE || rep->animation_playing == 0) &&
        (flags & W8_PROP_ANIMATION_GEOMETRY_DIRTY) == 0) {
        return;
    }
    total = AnimObjValue(rep->animation, 2);
    anim_frame_fraction = 0.0f;
    if (rep->pending_subcycle != 0xffff) {
        if (static_cast<short>(rep->pending_subcycle) < static_cast<int>(total)) {
            rep->subcycle = static_cast<unsigned char>(rep->pending_subcycle);
        }
        rep->pending_subcycle = 0xffff;
    }
    if (static_cast<int>(total) < 2 || rep->animation_running) {
        return;
    }
    if (rep->animation_playing == 0 && rep->random_play &&
        rand() * (1.0f / RAND_MAX) < rep->play_chance) {
        if (rep->frame_direction == W8_ANIMATION_FORWARD_COMPLETE) {
            rep->subcycle = rep->first_frame;
            rep->frame_direction = W8_ANIMATION_FORWARD;
        } else {
            rep->subcycle = rep->last_frame;
            rep->frame_direction = W8_ANIMATION_REVERSE;
        }
        m_pTimer->Restart();
        rep->animation_playing = 1;
    }
    if (rep->animation_playing == 0) {
        if ((flags & W8_PROP_ANIMATION_GEOMETRY_DIRTY) == 0) {
            return;
        }
        ApplyAnimationPaths(GetWorld());
        BuildOrRefreshPathingRepresentation();
        flags &= ~W8_PROP_ANIMATION_GEOMETRY_DIRTY;
        return;
    }
    anim_frame_fraction = m_pTimer->GetProgress();
    BuildOrRefreshPathingRepresentation();
    frames = static_cast<int>(anim_frame_fraction);
    anim_frame_fraction -= frames;
    rep->frame_steps = static_cast<unsigned char>(frames);
    if (frames != 0) {
        W8AnimObj* animation;

        AdvanceAnimationValue(frames, static_cast<char>(total));
        animation = Rep()->animation;
        if (animation->path != 0) {
            if ((flags & W8_PROP_ACCUMULATE_PATH_FRAMES) != 0) {
                rep->frame_index += frames;
                if (rep->frame_index >= animation->frame_count) {
                    rep->frame_index = animation->frame_count;
                }
            } else {
                rep->frame_index = rep->subcycle;
            }
            PathAISetValue(animation->path, static_cast<float>(rep->frame_index));
        }
    }
    if (rep->animation_playing == 0) {
        flags |= W8_PROP_ANIMATION_GEOMETRY_DIRTY;
    }
}

/* Walk every path list entry, apply the path to its mesh, and roll the prop's
   position snapshots: the previous current becomes the old position and the
   path's location becomes the new current. */
// FUNCTION: WIZ8 0x0044c200
void W8Prop::ApplyAnimationPaths(W8World* world)
{
    unsigned int count;
    int index;

    if (world == 0) {
        srAssertFail("pWorld", PROP_CPP, 0x3db, 0);
    }
    if (AnimationIsRunning(Rep()->animation) == 1) {
        count = AnimObjListCount(Rep()->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            stModelInstance* mesh = static_cast<stModelInstance*>(
                AnimObjDispatchList(Rep()->animation, 2, static_cast<signed char>(index)));
            W8PathAI* path;

            if (mesh == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x3ea, 0);
            }
            path = AnimObjListEntry(Rep()->animation, 2, static_cast<signed char>(index));
            if (path != 0) {
                srVector3T<float> location;

                PathAIApply(path, mesh);
                static_cast<srNode*>(mesh)->getLocation(location);
                previous_animation_position = animation_position;
                animation_position = location;
            }
        }
    }
}

/* Advance the rep's animation value by `frames` in the rep's direction.
   Transitive animations clamp at either end and complete: the run stops, the
   linked triggers fire and the global redraw flag goes up.  Looping kinds
   wrap or, when frame_method is two, bounce off the ends and flip direction; a
   step larger than the counter range folds through whole trips.  The new
   value is clamped and pushed to every bound path. */
// FUNCTION: WIZ8 0x0044c310
void W8Prop::AdvanceAnimationValue(int frames, char total)
{
    W8PropRepresentation* rep = Rep();
    unsigned int frame = rep->subcycle;
    char behaviour = rep->frame_method;
    int end = rep->last_frame;
    int start = rep->first_frame;
    unsigned int count;
    int index;

    if (behaviour == W8_ANIMATION_RANDOM_FRAME) {
        rep->subcycle = static_cast<unsigned char>(Random(total));
    } else if (rep->animation_behaviour == W8_ANIMATION_PLAY_ONCE) {
        if (rep->frame_direction == W8_ANIMATION_FORWARD) {
            if (static_cast<int>(frame) + frames < end) {
                rep->subcycle = static_cast<unsigned char>(frame + frames);
            } else {
                rep->subcycle = rep->last_frame;
                rep->animation_playing = 0;
                rep->frame_direction = W8_ANIMATION_FORWARD_COMPLETE;
                if (trigger != 0) {
                    trigger->RunLinkedTriggers();
                }
                gXStatus.sight_refresh_pending = true;
            }
        } else if (rep->frame_direction == W8_ANIMATION_REVERSE) {
            if (static_cast<int>(frame) - frames > start) {
                rep->subcycle = static_cast<unsigned char>(frame - frames);
            } else {
                rep->subcycle = rep->first_frame;
                rep->animation_playing = 0;
                rep->frame_direction = W8_ANIMATION_REVERSE_COMPLETE;
                if (trigger != 0) {
                    trigger->RunLinkedTriggers();
                }
                gXStatus.sight_refresh_pending = true;
            }
        }
    } else {
        int range = end - start;

        if (frames < range) {
            if (rep->frame_direction == W8_ANIMATION_FORWARD) {
                frame += frames;
                if (static_cast<int>(frame) <= end) {
                    rep->subcycle = static_cast<unsigned char>(frame);
                } else if (behaviour == W8_ANIMATION_PING_PONG) {
                    rep->frame_direction = W8_ANIMATION_REVERSE;
                    rep->subcycle = static_cast<unsigned char>(2 * end - static_cast<int>(frame));
                } else {
                    rep->subcycle = static_cast<unsigned char>(static_cast<int>(frame) - range - 1);
                }
            } else if (rep->frame_direction == W8_ANIMATION_REVERSE) {
                frame -= frames;
                if (static_cast<int>(frame) >= start) {
                    rep->subcycle = static_cast<unsigned char>(frame);
                } else if (behaviour == W8_ANIMATION_PING_PONG) {
                    rep->subcycle = static_cast<unsigned char>(2 * start - static_cast<int>(frame));
                    rep->frame_direction = W8_ANIMATION_FORWARD;
                } else {
                    rep->subcycle = static_cast<unsigned char>(range + static_cast<int>(frame) + 1);
                }
            }
        } else if (rep->frame_direction == W8_ANIMATION_FORWARD) {
            int effective = static_cast<int>(frame) - start + frames;

            if (behaviour == W8_ANIMATION_PING_PONG) {
                int trips = effective / range;
                int remainder = effective - trips * range;

                if (trips % 2 == 0) {
                    rep->subcycle = static_cast<unsigned char>(start + remainder);
                } else {
                    rep->frame_direction = W8_ANIMATION_REVERSE;
                    rep->subcycle = static_cast<unsigned char>(end - remainder);
                }
            } else {
                rep->subcycle = static_cast<unsigned char>(start + effective -
                                                           effective / (range + 1) * (range + 1));
            }
        } else if (rep->frame_direction == W8_ANIMATION_REVERSE) {
            int effective = 2 * start - static_cast<int>(frame) + frames;

            if (behaviour == W8_ANIMATION_PING_PONG) {
                int trips = effective / range;
                int remainder = effective - trips * range;

                if (trips % 2 == 0) {
                    rep->subcycle = static_cast<unsigned char>(start + remainder);
                    rep->frame_direction = W8_ANIMATION_FORWARD;
                } else {
                    rep->subcycle = static_cast<unsigned char>(end - remainder);
                }
            } else {
                int folded = effective - 1;

                rep->subcycle =
                    static_cast<unsigned char>(end + folded / (range + 1) * (range + 1) - folded);
            }
        }
    }
    if (rep->subcycle < rep->first_frame) {
        rep->subcycle = rep->first_frame;
    } else if (rep->subcycle > rep->last_frame) {
        rep->subcycle = rep->last_frame;
    }
    if (AnimationIsRunning(rep->animation) == 1) {
        count = AnimObjListCount(rep->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            W8PathAI* path = AnimObjListEntry(rep->animation, 2, static_cast<signed char>(index));

            if (path != 0) {
                PathAISetValue(path, static_cast<float>(rep->subcycle));
            }
        }
    }
}

/* The animation value one step ahead, exactly as
   AdvanceAnimationValue would compute a single step: transitive kinds
   clamp at the ends, looping kinds wrap to the opposite end, and behaviour
   two steps back instead.  Retail returns the literal one rather than
   first_frame + 1 when a bouncing run sits at the start. */
// FUNCTION: WIZ8 0x0044c600
char W8Prop::NextAnimationValue()
{
    W8PropRepresentation* rep = Rep();
    char direction = rep->frame_direction;

    if (rep->animation_behaviour == W8_ANIMATION_PLAY_ONCE) {
        if (direction == W8_ANIMATION_FORWARD) {
            if (rep->subcycle < rep->last_frame) {
                return rep->subcycle + 1;
            }
        } else if (direction == W8_ANIMATION_REVERSE && rep->first_frame < rep->subcycle) {
            return rep->subcycle - 1;
        }
        return rep->subcycle;
    }
    if (direction == W8_ANIMATION_FORWARD) {
        if (rep->subcycle != rep->last_frame) {
            return rep->subcycle + 1;
        }
        if (rep->frame_method == W8_ANIMATION_PING_PONG) {
            return rep->subcycle - 1;
        }
        return rep->first_frame;
    }
    if (direction != W8_ANIMATION_REVERSE) {
        return rep->subcycle;
    }
    if (rep->subcycle > rep->first_frame) {
        return rep->subcycle - 1;
    }
    if (rep->frame_method == W8_ANIMATION_PING_PONG) {
        return 1;
    }
    return rep->last_frame;
}

/* When the next animation step moves the rep exactly one frame, write the
   delta between the current and previous positions into `out` and return the
   elapsed frame count; otherwise `out` is zeroed.  `point` is accepted but
   never read. */
// FUNCTION: WIZ8 0x0044e130
char W8Prop::GetDelta(srVector3T<float>* out, const srVector3T<float>* point)
{
    unsigned char next = static_cast<unsigned char>(NextAnimationValue());

    if (abs(next - Rep()->subcycle) == 1) {
        *out = animation_position - previous_animation_position;
        return Rep()->frame_steps;
    }
    out->SetZero();
    return 0;
}

/* Whether the prop can be used from where the caller is. Retail clears the
   action pointer for non-door actions, then dereferences it when the lock
   state does not short-circuit the test. */
// FUNCTION: WIZ8 0x0044e0c0
bool W8Prop::CanBeUsedFrom(int path_x, int path_z, bool notify)
{
    Trigger* owner;
    W8TriggerActionData* action;

    if ((flags & W8_PROP_GD_TRIGGER_BOUND) == 0 || m_gd_prop == 0) {
        return false;
    }
    owner = m_gd_prop->m_trigger;
    if (owner == 0) {
        return false;
    }

    action = owner->m_pActionData;
    if (action == 0 || action->type != W8_TRIGGER_PAYLOAD_DOOR) {
        action = 0;
    }
    if ((owner->lock_state.lock_type != 0 && owner->lock_state.device_state.completed == 0) ||
        static_cast<W8DoorTriggerActionData*>(action)->open ||
        static_cast<W8DoorTriggerActionData*>(action)->locked) {
        return false;
    }
    if (!m_gd_prop->ContainsPathCoordinate(static_cast<unsigned short>(path_x),
                                           static_cast<unsigned short>(path_z))) {
        return false;
    }
    if (notify) {
        owner->Activate();
    }
    return true;
}

/* After a successful load, bind the current animation frame's mesh to its
   path and, when the animation is already running, snapshot the live
   position into the prop. */
// FUNCTION: WIZ8 0x0044c670
void W8Prop::ApplyAnimationFrame()
{
    unsigned int count;
    int index;

    if (AnimationIsRunning(static_cast<W8PropRepresentation*>(m_pRep)->animation) != 1) {
        W8PropRepresentation* rep = static_cast<W8PropRepresentation*>(m_pRep);
        unsigned char frame = rep->subcycle;
        srModelInstance* mesh;
        W8PathAI* path;

        if (AnimationIsRunning(rep->animation) == 0) {
            mesh = AnimObjDispatch(rep->animation, 2, frame);
        } else {
            mesh = AnimObjDispatchList(rep->animation, 2, 0);
        }
        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x581, 0);
        }
        path = static_cast<W8PropRepresentation*>(m_pRep)->animation->path;
        if (path != 0) {
            PathAISetValue(
                path, static_cast<float>(static_cast<W8PropRepresentation*>(m_pRep)->subcycle));
            PathAIApply(static_cast<W8PropRepresentation*>(m_pRep)->animation->path, mesh);
        }
        return;
    }

    count = AnimObjListCount(static_cast<W8PropRepresentation*>(m_pRep)->animation, 2);
    for (index = 0; index < static_cast<int>(count); ++index) {
        srModelInstance* mesh =
            AnimObjDispatchList(static_cast<W8PropRepresentation*>(m_pRep)->animation, 2,
                                static_cast<signed char>(index));
        W8PathAI* path;

        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x56f, 0);
        }
        path = AnimObjListEntry(static_cast<W8PropRepresentation*>(m_pRep)->animation, 2,
                                static_cast<signed char>(index));
        if (path != 0) {
            srVector3T<float> location;

            PathAISetValue(
                path, static_cast<float>(static_cast<W8PropRepresentation*>(m_pRep)->subcycle));
            PathAIApply(path, mesh);
            static_cast<srNode*>(mesh)->getLocation(location);
            animation_position = location;
            previous_animation_position = location;
        }
    }
    flags |= W8_PROP_ANIMATION_GEOMETRY_DIRTY;
    BuildOrRefreshPathingRepresentation();
}

/* Per-frame prop update: while the animation is running this binds every
   dispatched instance to the world's dynamic scene, interpolates between the
   current and next keyframe records (position lerp, quaternion slerp for
   rotations - the same algorithm as PathAIApply), pushes the transform
   onto the instance or its child chain, and rolls the position snapshots
   forward. A stopped animation binds the single current instance and applies
   either the rep's path or its stored transform. */
// FUNCTION: WIZ8 0x0044c830
void W8Prop::AttachAnimationInstances(W8World* world)
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
        count = static_cast<int>(AnimObjListCount(Rep()->animation, 2));
        for (index = 0; index < count; ++index) {
            instance = static_cast<stModelInstance*>(
                AnimObjDispatchList(Rep()->animation, 2, static_cast<signed char>(index)));
            if (instance == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x5b4, 0);
            }
            instance->clearFlag(srNode::FLAG_DISABLE);
            instance->setParent(world->dynamic_scene, 1);
            instance->light_scale = zero;
            if (g_settings.smooth_world_animations != 0) {
                instance->frame_interpolation = anim_frame_fraction;
            } else {
                instance->frame_interpolation = 0.0f;
            }
            mesh = static_cast<stMeshModel*>(instance->getModel());
            if (mesh != 0 && (mesh->flags & W8_MESH_SORTED_RENDERING) != 0 && trigger == 0) {
                instance->render_flags |= stModelInstance::RENDER_NO_PICK;
            }
            path = AnimObjListEntry(Rep()->animation, 2, static_cast<signed char>(index));
            if (path == 0) {
                continue;
            }
            next_frame = static_cast<unsigned char>(NextAnimationValue());
            if (next_frame > Rep()->last_frame) {
                anim_frame_fraction = 0.0f;
            }
            rotation = path->rotations[Rep()->subcycle];
            next = path->rotations[next_frame];
            instance->getRotation(previous_animation_rotation);
            if (!(rotation == next)) {
                W8Quaternion::InterpolateRotation(rotation, next, anim_frame_fraction, &rotation);
            }
            animation_rotation = rotation;
            current = **path->nodes->GetAt(Rep()->subcycle);
            next_pos = **path->nodes->GetAt(next_frame);
            inv = g_float_one - anim_frame_fraction;
            position = current * inv + next_pos * anim_frame_fraction;
            if (path->scales != 0) {
                has_scales = true;
                current_scale = path->scales[Rep()->subcycle];
                next_scale = path->scales[next_frame];
                scale_vector = current_scale * inv + next_scale * anim_frame_fraction;
            }
            node = instance->first_child_;
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
                    node = node->next_sibling_;
                } while (node != 0);
            }
            previous_animation_position = animation_position;
            animation_position = position;
        }
    } else {
        instance = static_cast<stModelInstance*>(Rep()->ToggleAnimation(Rep()->subcycle));
        if (instance == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x624, 0);
        }
        instance->clearFlag(srNode::FLAG_DISABLE);
        instance->setParent(world->dynamic_scene, 1);
        instance->light_scale.SetZero();
        if (g_settings.smooth_world_animations != 0) {
            instance->frame_interpolation = anim_frame_fraction;
        } else {
            instance->frame_interpolation = 0.0f;
        }
        mesh = static_cast<stMeshModel*>(instance->getModel());
        if (mesh != 0 && (mesh->flags & W8_MESH_SORTED_RENDERING) != 0 && trigger == 0) {
            instance->render_flags |= stModelInstance::RENDER_NO_PICK;
        }
        if (Rep()->animation->path != 0) {
            PathAIApply(Rep()->animation->path, instance);
        } else {
            Rep()->GetLocation(&rep_position);
            Rep()->GetRotation(&rep_rotation);
            node = instance->first_child_;
            if (node == 0) {
                location.SetFromFloat(&rep_position);
                instance->setLocation(location);
                instance->setRotation(rep_rotation);
            } else {
                do {
                    location.SetFromFloat(&rep_position);
                    node->setLocation(location);
                    node->setRotation(rep_rotation);
                    node = node->next_sibling_;
                } while (node != 0);
            }
        }
        BakeInstanceVertexLightingIfNeeded(instance, world->dynamic_scene);
    }
    if (m_gd_prop == 0) {
        BuildOrRefreshPathingRepresentation();
    }
}

/* The detach counterpart to AttachAnimationInstances: the current frame is stashed in
   saved_subcycle, then every dispatched instance is flagged disabled and detached
   from the scene.  While the animation runs the whole list is walked;
   otherwise only the snapshot frame's instance (or the first dispatch-list
   entry when a run is in progress) is pulled. */
// FUNCTION: WIZ8 0x0044d360
void W8Prop::DetachAnimationInstances(W8World* world)
{
    stModelInstance* instance;
    unsigned int count;
    int index;

    if (world == 0) {
        srAssertFail("pWorld", PROP_CPP, 0x66b, 0);
    }
    Rep()->saved_subcycle = Rep()->subcycle;
    if (AnimationIsRunning(Rep()->animation) == 1) {
        count = AnimObjListCount(Rep()->animation, 2);
        for (index = 0; index < static_cast<int>(count); ++index) {
            instance = static_cast<stModelInstance*>(
                AnimObjDispatchList(Rep()->animation, 2, static_cast<signed char>(index)));
            if (instance == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x678, 0);
            }
            instance->setFlag(srNode::FLAG_DISABLE);
            instance->setParent(0, 1);
        }
    } else {
        unsigned char frame = Rep()->subcycle;

        if (AnimationIsRunning(Rep()->animation) == 0) {
            instance = static_cast<stModelInstance*>(AnimObjDispatch(Rep()->animation, 2, frame));
        } else {
            instance = static_cast<stModelInstance*>(AnimObjDispatchList(Rep()->animation, 2, 0));
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
   while a running animation is active, and ApplyAnimationFrame reapplies the state.
   The read chain's success is reported even though the restore runs either
   way. */
// FUNCTION: WIZ8 0x0044dbd0
bool W8Prop::LoadAnimationState(int hFile)
{
    unsigned char unused;
    unsigned int count;
    int index;
    int total;
    bool success;
    W8PathAI* path;

    success = FileRead(hFile, &Rep()->subcycle, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->first_frame, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->last_frame, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->frame_direction, 1, 0) != 0 &&
              FileRead(hFile, &Rep()->animation_playing, 1, 0) != 0 &&
              FileRead(hFile, &unused, 1, 0) != 0;
    if (Rep()->animation != 0) {
        total = static_cast<int>(AnimObjValue(Rep()->animation, 2));
        if (Rep()->last_frame >= total) {
            Rep()->last_frame = static_cast<unsigned char>(total - 1);
        }
        if (Rep()->first_frame >= total) {
            Rep()->first_frame = static_cast<unsigned char>(total - 1);
        }
        if (Rep()->subcycle >= total) {
            Rep()->subcycle = static_cast<unsigned char>(total - 1);
        }
        if (AnimationIsRunning(Rep()->animation) == 1) {
            count = AnimObjListCount(Rep()->animation, 2);
            for (index = 0; index < static_cast<int>(count); ++index) {
                path = AnimObjListEntry(Rep()->animation, 2, static_cast<signed char>(index));
                if (path != 0) {
                    PathAISetValue(path, static_cast<float>(Rep()->subcycle));
                }
            }
        }
        ApplyAnimationFrame();
    }
    return success;
}

/* Union of the per-frame bounds over every frame of the rep's animation.
   The rep's current frame is saved, the bounds for frame zero seed the merge,
   and each remaining frame expands the result before the frame is restored. */
// FUNCTION: WIZ8 0x0044dd60
void W8Prop::GetBounds(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    srVector3T<float> local_minimum;
    srVector3T<float> local_maximum;
    unsigned char saved_frame;
    int frame;
    int total;

    total = static_cast<int>(AnimObjValue(Rep()->animation, 2));
    saved_frame = Rep()->subcycle;
    Rep()->subcycle = 0;
    AnimObjGetBounds(Rep()->animation, 2, Rep()->subcycle, minimum, maximum);
    for (frame = 1; frame < total; ++frame) {
        Rep()->subcycle = static_cast<unsigned char>(frame);
        AnimObjGetBounds(Rep()->animation, 2, Rep()->subcycle, &local_minimum, &local_maximum);
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
    Rep()->subcycle = saved_frame;
}

/* Build or refresh the pathing representation for a collidable Prop.  Retail
   requires a transitive animation with one mesh, then either constructs the
   owned GDProp or reinitializes it for the current animation frame. */
// FUNCTION: WIZ8 0x0044dea0
int W8Prop::BuildOrRefreshPathingRepresentation()
{
    srModelInstance* instance;

    if ((flags & W8_PROP_COLLIDABLE) == 0) {
        return 0;
    }
    if (AnimationIsRunning(Rep()->animation) != 1) {
        ShutdownWithErrorBox("Collidable props can be of Transform type only.");
    }
    if (AnimObjListCount(Rep()->animation, 2) != 1) {
        ShutdownWithErrorBox("Collideable props should have a single LOD.");
    }
    instance = AnimObjDispatchList(Rep()->animation, 2, 0);
    if (instance == 0) {
        srAssertFail("pstInstance", PROP_CPP, 0x939, 0);
    }

    if (m_gd_prop == 0) {
        m_gd_prop = new GDProp(instance, m_name, static_cast<unsigned short>(Rep()->subcycle),
                               Rep()->footstep_surface, Rep()->footstep_material);
    } else {
        if ((flags & W8_PROP_ANIMATION_GEOMETRY_DIRTY) != 0) {
            m_gd_prop->Initialize(instance, true, static_cast<unsigned short>(Rep()->subcycle),
                                  Rep()->footstep_surface, Rep()->footstep_material);
        } else {
            m_gd_prop->Initialize(instance, false, 0, Rep()->footstep_surface,
                                  Rep()->footstep_material);
        }
        if (Rep()->animation_behaviour == W8_ANIMATION_PLAY_ONCE) {
            g_animated_prop_present = true;
        }
    }
    return m_gd_prop->m_surface_count;
}

/* Run the prop's own trigger for a missile impact.  Only the three prop
   animation actions (0x3a..0x3c) accept the missile's table index as their
   source; every other trigger setup is ignored. */
// FUNCTION: WIZ8 0x0044e230
void W8Prop::RunMissileTrigger(W8AIMissile* record)
{
    if (record != 0 && record->missile != 0 && trigger != 0 &&
        (trigger->initial_action == 0x3a || trigger->initial_action == 0x3b ||
         trigger->initial_action == 0x3c)) {
        trigger->Run(record->missile->missile_table_index);
    }
}

/* Mirror of GetPosition: while the rep node reports itself current
   the position is stored in animation_position, otherwise it goes through the rep
   node's own location. */
// FUNCTION: WIZ8 0x0044e310
void W8Prop::SetPosition(srVector3T<float>* position)
{
    if (AnimationIsRunning(Rep()->animation) == 1) {
        animation_position = *position;
        return;
    }
    m_pRep->SetLocation(position);
}

// FUNCTION: WIZ8 0x0044e360
bool W8Prop::TriggerHasActionMessage()
{
    return trigger != 0 && trigger->HasActionMessage();
}

// FUNCTION: WIZ8 0x0044e380
bool W8Prop::TriggerRequiresItem()
{
    return trigger != 0 && trigger->RequiresItem();
}

/* Whether a prop with a selectable trigger is visible from `position`: the
   trigger's distance interval has to contain the centre of the current
   frame's bounds and the centre, minimum or maximum has to project
   on-screen through the active world's camera. */
// FUNCTION: WIZ8 0x0044e3a0
bool W8Prop::IsTriggerInView(srVector3T<float>* position)
{
    Trigger* trigger = this->trigger;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<float> center;
    srVector3T<float> projected;
    float distance;

    if (trigger != 0 && (trigger->flags & W8_TRIGGER_ENABLED) != 0 &&
        ((trigger->flags & W8_TRIGGER_ONCE) == 0 || (trigger->flags & W8_TRIGGER_FIRED) == 0)) {
        AnimObjGetBounds(Rep()->animation, 2, Rep()->subcycle, &minimum, &maximum);
        center.Set((minimum.x + maximum.x) * g_double_half, (minimum.y + maximum.y) * g_double_half,
                   (minimum.z + maximum.z) * g_double_half);
        distance = (center - *position).Length();
        if (distance < trigger->range_maximum && trigger->range_minimum <= distance) {
            {
                srVector3T<double> input(static_cast<double>(center.x),
                                         static_cast<double>(center.y),
                                         static_cast<double>(center.z));

                if (g_world->camera->project(projected, input) ==
                    srCamera::PROJECTION_RESULT_ACCEPTED) {
                    return true;
                }
            }
            {
                srVector3T<double> input(static_cast<double>(minimum.x),
                                         static_cast<double>(minimum.y),
                                         static_cast<double>(minimum.z));

                if (g_world->camera->project(projected, input) ==
                    srCamera::PROJECTION_RESULT_ACCEPTED) {
                    return true;
                }
            }
            {
                srVector3T<double> input(static_cast<double>(maximum.x),
                                         static_cast<double>(maximum.y),
                                         static_cast<double>(maximum.z));

                if (g_world->camera->project(projected, input) ==
                    srCamera::PROJECTION_RESULT_ACCEPTED) {
                    return true;
                }
            }
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x0044bf50
bool CreateAndLoadProp(W8ReadLevelInfo* info, W8Prop** prop_out)
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
    success = static_cast<W8PropRepresentation*>(prop->m_pRep)->LoadProp(info, prop);
    if (success) {
        *prop_out = prop;
        prop->m_pTimer->SetDuration(
            g_float_one / static_cast<W8PropRepresentation*>(prop->m_pRep)->animation_speed);
        prop->ApplyAnimationFrame();
    }
    return success;
}

/* Resource loader rooted at CreateAndLoadProp.  The call site loads
   prop->m_pRep into ECX before the two stack arguments, so this is a
   PropRep method: LoadProp(pInfo, pProp). */
// FUNCTION: WIZ8 0x0044aee0
bool W8PropRepresentation::LoadProp(W8ReadLevelInfo* info, W8Prop* prop)
{
    int hFile;
    bool success;
    signed char version;
    unsigned char frame_count = 0;
    unsigned char option_byte = 0;
    unsigned char attach_flag = 0;
    float playback_scale = 0.0f;
    unsigned int flag_bits = 0;
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
        mesh = CreateAniMesh();
        if (mesh == 0) {
            srAssertFail("pAniMesh", PROP_CPP, 0xd5, 0);
        }
        result = LoadAniMeshFromInfo(info, mesh, 1);
        if (!result) {
            fail_line = 0xd8;
            result = false;
            goto fail_with_result;
        }
        animation = CreateAnimObj();
        animation->entries[2] = mesh;
        animation->group_count = 1;
        animation->animation_playing = b0;
        animation->frame_method = b1;
        animation->behaviour = b2;
        animation->cycle = 0;
        animation->path_lists = 0;
        animation->playback_scale = playback_scale;
        this->animation = animation;
    } else {
        unsigned int entry_index;
        unsigned int list_count;
        signed char slot_count;
        int slot_i;

        flag_bits = 0;
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
            lx *= g_double_five_hundred;
            ly *= g_double_five_hundred;
            lz *= g_double_five_hundred;
            this->local_location.x = lx;
            this->location.x = lx;
            this->local_location.y = ly;
            this->location.y = ly;
            this->local_location.z = lz;
            this->location.z = lz;
        }
        if (version > 5) {
            FileRead(hFile, &flag_bits, 4, 0);
            prop->flags |= flag_bits;
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
                slot->frame = static_cast<signed char>(frame_tmp);
                FileRead(hFile, &tag_tmp, 2, 0);
                slot->tag = static_cast<unsigned char>(tag_tmp);
                if (frame_count <= frame_tmp) {
                    srAssertFail(
                        "(usTemp < (UINT16)ubNumFrames)", /* c-style-cast-ok: verbatim assert text */
                        PROP_CPP, 0x11f,
                        reinterpret_cast<const char*>(
                            String("%s Prop Error:Segment %d frame number is out of range (%d)",
                                   prop->m_name, static_cast<unsigned int>(tag_tmp),
                                   static_cast<unsigned int>(frame_tmp))));
                }
                this->slots.Add(slot);
            }
        }
        animation = CreateAnimObj();
        result = AnimObjReadFromFile(info, animation, 1, 0, 1);
        this->animation = animation;
        if (AnimationIsRunning(animation) == 1) {
            animation->frame_count = frame_count;
        }
        this->play_chance = animation->play_chance;
        this->random_play = animation->random_play != 0;
        list_count = AnimObjValue(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            srModelInstance* instance;
            stMeshModel* mesh_model;
            char* named;

            if (AnimationIsRunning(this->animation) == 0) {
                instance =
                    AnimObjDispatch(this->animation, 2, static_cast<unsigned char>(entry_index));
            } else {
                instance = AnimObjDispatchList(this->animation, 2, 0);
            }
            named = reinterpret_cast<char*>(String("Prop: %s", prop->m_name));
            instance->setName(named);
            mesh_model = static_cast<stMeshModel*>(instance->getModel());
            for (; mesh_model != 0; mesh_model = mesh_model->next) {
                if (AnimationIsRunning(animation) == 0) {
                    mesh_model->GetVertexSunlight(true);
                }
            }
            if (option_byte != 0) {
                srNode* child;
                srVector3T<float> axis;

                axis.Set(0.0f, 1.0f, 0.0f);
                instance->setAlignment(1);
                instance->setAlignAxis(axis);
                child = instance->first_child_;
                if (child != 0) {
                    srModelInstance* child_instance = static_cast<srModelInstance*>(child);
                    child_instance->setAlignment(1);
                    child_instance->setAlignAxis(axis);
                }
            }
        }
        list_count = AnimObjListCount(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            W8PathAI* path = AnimObjListEntry(animation, 2, static_cast<signed char>(entry_index));
            if (path != 0) {
                PathAISetLooping(path, 1);
                PathAISetDiscreteMode(path, 1);
                PathAISetScale(path, animation->playback_scale);
            }
        }
    }

    this->active = 1;
    this->animation_behaviour = animation->behaviour;
    this->frame_method = animation->frame_method;
    this->animation_playing = animation->animation_playing;
    this->frame_direction = W8_ANIMATION_FORWARD;
    this->animation_speed = animation->playback_scale;
    this->timer = GetTickCount();
    if (this->animation_behaviour == W8_ANIMATION_PLAY_ONCE ||
        this->animation_behaviour == W8_ANIMATION_REPEAT) {
        this->animation_playing = 0;
        this->frame_direction = this->frame_method == W8_ANIMATION_PING_PONG
                                    ? W8_ANIMATION_REVERSE_COMPLETE
                                    : W8_ANIMATION_FORWARD_COMPLETE;
    }

    if (animation->path_lists == 0) {
        W8AniMesh* mesh = AnimObjEntry(animation, 2, 0);
        unsigned char value_count = AniMeshValue(mesh);
        stModelInstance* frame = GetAniMeshFrame(mesh, 0);
        srVector3T<float> minimum;
        srVector3T<float> maximum;
        unsigned int frame_i;
        float extent;

        if (frame == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x182, 0);
        }
        frame->getModel()->getBoundingBox(minimum, maximum);
        for (frame_i = 0; frame_i < value_count; ++frame_i) {
            srVector3T<float> frame_min;
            srVector3T<float> frame_max;

            frame = GetAniMeshFrame(mesh, frame_i);
            if (frame == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x192, 0);
            }
            frame->getModel()->getBoundingBox(frame_min, frame_max);
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
            this->bounds_min.y = minimum.y;
            this->bounds_max.x = maximum.x;
            this->bounds_min.x = minimum.x;
            this->bounds_max.z = maximum.z;
            this->bounds_min.z = minimum.z;
            this->bounds_max.y = maximum.y;
        }
        extent = maximum.x - minimum.x;
        if (extent < maximum.y - minimum.y) {
            extent = maximum.y - minimum.y;
        }
        if (extent < maximum.z - minimum.z) {
            extent = maximum.z - minimum.z;
        }
        this->bounds_extent = extent * g_float_half;
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
            trigger->m_bRepType = W8_TRIGGER_REP_PROP;
            trigger->m_pProp = prop;
            if (trigger->initial_action == 0x40) {
                InitializeStateDrivenPropVariables(trigger);
            }
            if (trigger->trigger_kind == 1 && trigger->initial_action == 8) {
                unsigned int path_count;
                unsigned int path_i;

                this->animation_running = true;
                if (AnimationIsRunning(this->animation) == 1) {
                    path_count = AnimObjListCount(this->animation, 2);
                    for (path_i = 0; path_i < path_count; ++path_i) {
                        W8PathAI* path =
                            AnimObjListEntry(this->animation, 2, static_cast<signed char>(path_i));
                        path->step_by_node = 1;
                    }
                }
            }
            switch (trigger->initial_action) {
            case 1:
            case 2:
            case 3:
            case 0x2c:
            case 0x32:
            case 0x33:
            case 0x40:
                this->animation_playing = 0;
                break;
            }
            prop->trigger = trigger;
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
            if (result && (success = FileRead(hFile, &this->footstep_surface, 1, 0), success) &&
                (success = FileRead(hFile, &this->footstep_material, 1, 0), success)) {
                result = true;
            } else {
                result = false;
            }
        }
    }

    this->first_frame = 0;
    this->subcycle = 0;
    {
        unsigned int frames = AnimObjValue(animation, 2);
        this->last_frame = static_cast<unsigned char>(frames) - 1;
    }
    return result;

fail:
    srAssertFail("fSuccess", PROP_CPP, fail_line, 0);
    return false;

fail_with_result:
    srAssertFail("fSuccess", PROP_CPP, fail_line, 0);
    return result;
}

/* The prop representation's current animation value, or -1 while it owns no
   animation. */
// FUNCTION: WIZ8 0x0044ebe0
int W8Prop::GetAnimationState() const
{
    W8AnimObj* animation = Rep()->animation;
    if (animation != 0) {
        return static_cast<int>(AnimObjValue(animation, 2));
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
            W8AniMesh* mesh = animation->entries[group];
            if (mesh == 0) {
                continue;
            }
            int frame_count = AniMeshValue(mesh);
            if (mesh->flags.single_instance) {
                instances->Add(GetAniMeshFrame(mesh, 0));
            } else {
                for (int frame = 0; frame < frame_count; ++frame) {
                    instances->Add(GetAniMeshFrame(mesh, frame));
                }
            }
        }
    } else if (AnimationIsRunning(animation) == 1) {
        for (int group = 0; group < 3; ++group) {
            W8PList* meshes = animation->meshes[group];
            if (meshes == 0) {
                continue;
            }
            int mesh_count = PLLength(meshes);
            for (int index = 0; index < mesh_count; ++index) {
                W8AniMesh* mesh = static_cast<W8AniMesh*>(PLGet(meshes, index));
                if (mesh == 0) {
                    continue;
                }
                int frame_count = AniMeshValue(mesh);
                for (int frame = 0; frame < frame_count; ++frame) {
                    instances->Add(GetAniMeshFrame(mesh, frame));
                }
            }
        }
    }
}

/* APST chunk writer: a 0xDEADD00D signature, the format version and the prop
   count, then one record per prop - a fixed 64-byte name plus the six rep
   bytes that LoadAnimationState reads back (frame, counters, direction
   flags and the active byte). The per-prop byte writes only run while the
   previous writes succeed. */
// FUNCTION: WIZ8 0x0044e830
void SaveWorldProps(W8World* world, int handle)
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
        prop = GetWorldProp(world, index);
        strcpy(name, prop->m_name);
        FileWrite(handle, name, 0x40, 0);
        if (FileWrite(handle, &prop->Rep()->subcycle, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->first_frame, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->last_frame, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->frame_direction, 1, 0) != 0 &&
            FileWrite(handle, &prop->Rep()->animation_playing, 1, 0) != 0) {
            FileWrite(handle, &prop->Rep()->active, 1, 0);
        }
    }
}

/* APST chunk reader. The 0xDEADD00D signature selects the name-keyed format;
   older saves carry a bare record count followed by each prop's object key.
   Records whose prop cannot be found are consumed by a scratch prop so the
   stream stays aligned. */
// FUNCTION: WIZ8 0x0044e9a0
void LoadWorldProps(W8World* world, int handle)
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
                entry = GetWorldProp(world, entry_index);
                if (entry->id == key) {
                    prop = entry;
                    break;
                }
            }
            if (prop != 0) {
                prop->LoadAnimationState(handle);
            } else {
                prop = new W8Prop();
                prop->LoadAnimationState(handle);
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
                prop->LoadAnimationState(handle);
            } else {
                prop = new W8Prop();
                prop->LoadAnimationState(handle);
                delete prop;
            }
        }
    }
}

/* The renderer owns the selected model instance. Without one, retail also
   resets the cached prop index to -1 before returning it. */
// FUNCTION: WIZ8 0x0044DA60
int GetSelectedPropIndex(void)
{
    if (GetPickedModelInstance() == 0) {
        return g_selected_prop_index = -1;
    }
    return g_selected_prop_index;
}

/* When the renderer still holds a pick and ResolvePickedProp latched a
   trigger, run that trigger and post the nothing-happened / special-item
   notice. Clearing the pick also clears the latch. */
// FUNCTION: WIZ8 0x0044DA20
bool ActivateSelectedProp(void)
{
    if (GetPickedModelInstance() == 0) {
        g_selected_prop_trigger = 0;
        return false;
    }
    if (g_selected_prop_trigger != 0) {
        g_trigger_feedback = false;
        g_selected_prop_trigger->Run(-1);
        g_selected_prop_trigger->PrintNothingHappenedOrSpecialItemRequired();
        return true;
    }
    return false;
}
