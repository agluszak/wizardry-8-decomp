#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"

#include <string.h>

#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/float_constants.h"
#include "wiz8/mesh_model.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"
#include "surrender/srModelInstance.h"
#include "surrender/srNode.h"

#include "DEBUG.H"

#include <math.h>
#include <new>
#include <windows.h>


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

extern void* Function443830(W8World* world, W8Prop* prop);
extern int IncrementValue60DFAC(void);
extern const double g_zero_005ebb40;

/* This byte is reset before the world Prop update and set when a collidable
   Prop rebuilds its pathing geometry.  Its three retail references establish
   the process-wide storage; no broader state model is yet proved. */
unsigned char g_byte_00659a64;

/* Same-TU access to srModelInstance's protected alignment fields so the
   loader can write them the way the image does, without going through the
   SurRender setAlignment/setAlignAxis imports.  Retail ORs the enable bit,
   stores the raw axis, normalizes in place, then ORs the enable bit again.
   The body must live on this derived type: VC6 rejects protected access
   through a derived pointer from a free function. */
struct PropModelInstanceAccess : srModelInstance {
    void WriteAlignAxisYUp()
    {
        float length_squared;
        float scale;

        alignment_flags_148 |= 1;
        align_axis_14c.x = 0.0f;
        align_axis_14c.y = 1.0f;
        align_axis_14c.z = 0.0f;
        length_squared = align_axis_14c.z * align_axis_14c.z +
            align_axis_14c.y * align_axis_14c.y +
            align_axis_14c.x * align_axis_14c.x;
        if ((double)length_squared != g_zero_005ebb40) {
            scale = (float)(g_double_005ebc30 / sqrt((double)length_squared));
            align_axis_14c.x *= scale;
            align_axis_14c.y *= scale;
            align_axis_14c.z *= scale;
        }
        alignment_flags_148 |= 1;
    }
};

#define PROP_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp"

// VTABLE: WIZ8 0x005ec1e0
// class W8Prop

// VTABLE: WIZ8 0x005ec1c8
// class W8PropRepresentation

// VTABLE: WIZ8 0x005ec1d0
// class W8GrowableVector<unsigned char*>

// TEMPLATE: WIZ8 0x0044efe0
// W8GrowableVector<unsigned char*>::W8GrowableVector

/* Prop::Prop() - GrObject base, then m_pRep / m_animation_timer and two identity
   rotation bases.  Retail expands PropRep after the AnimRep constructor:
   scalar field stores, the capacity-5 slot vector, then the PropRep vtable. */
// FUNCTION: WIZ8 0x0044bc00
W8Prop::W8Prop()
{
    W8PropRepresentation* rep;
    void* memory;

    trigger_18 = 0;
    flags_1c = 0;
    m_name = 0;
    unknown_024 = 0;
    unknown_004 = 4;
    unknown_008 = IncrementValue60DFAC();
    memory = ::operator new(sizeof(W8PropRepresentation));
    if (memory == 0) {
        m_pRep = 0;
    }
    else {
        rep = static_cast<W8PropRepresentation*>(memory);
        new (rep) W8PropRepresentation();
        m_pRep = reinterpret_cast<W8ItemRep*>(rep);
    }
    m_animation_timer = new W8GameTimer();
    position_02c.x = 0.0f;
    position_02c.y = 0.0f;
    position_02c.z = 0.0f;
    position_03c.x = 0.0f;
    position_03c.y = 0.0f;
    position_03c.z = 0.0f;
    rotation_048.vectors[0].x = 1.0f;
    rotation_048.vectors[0].y = 0.0f;
    rotation_048.vectors[0].z = 0.0f;
    rotation_048.vectors[1].x = 0.0f;
    rotation_048.vectors[1].y = 1.0f;
    rotation_048.vectors[1].z = 0.0f;
    rotation_048.vectors[2].x = 0.0f;
    rotation_048.vectors[2].y = 0.0f;
    rotation_048.vectors[2].z = 1.0f;
    rotation_06c.vectors[0].x = 1.0f;
    rotation_06c.vectors[0].y = 0.0f;
    rotation_06c.vectors[0].z = 0.0f;
    rotation_06c.vectors[1].x = 0.0f;
    rotation_06c.vectors[1].y = 1.0f;
    rotation_06c.vectors[1].z = 0.0f;
    rotation_06c.vectors[2].x = 0.0f;
    rotation_06c.vectors[2].y = 0.0f;
    rotation_06c.vectors[2].z = 1.0f;
    m_gd_prop = 0;
    if (m_pRep == 0) {
        srAssertFail(
            "m_pRep", PROP_CPP, 0x30e,
            "Prop::Prop() out of memory allocating m_pRep");
    }
    if (m_animation_timer == 0) {
        srAssertFail(
            "m_animation_timer", PROP_CPP, 0x30f,
            "Prop::Prop() out of memory allocating m_animation_timer");
    }
}


/* Copy keeps animation through CloneAnimObj and rebuilds an empty slot vector
   with the source capacity.  Clone's vtable slot allocates 0xc4 and lands here. */
// FUNCTION: WIZ8 0x0044ad10
W8PropRepresentation::W8PropRepresentation(const W8PropRepresentation& other)
    : W8AnimRep005ED050(other),
      animation_speed(other.animation_speed),
      value_0a0(other.value_0a0),
      flag_0a4(other.flag_0a4),
      flag_0a5(other.flag_0a5),
      value_0a8(other.value_0a8),
      flag_0ac(other.flag_0ac),
      flag_0ad(other.flag_0ad),
      slots(5),
      flag_0c0(other.flag_0c0),
      flag_0c1(other.flag_0c1)
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
        operator delete(slots.data[index]);
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
        srAssertFail("pWorld && pWorld->plsProps", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 0x969, 0);
    }
    count = ILLength(reinterpret_cast<W8IList*>(world->plsProps));
    for (index = 0; index < static_cast<int>(count); ++index) {
        W8Prop* prop = static_cast<W8Prop*>(PLGet(world->plsProps, index));
        void* value;

        if (prop != 0 && (value = Function443830(world, prop)) != 0 && prop->m_gd_prop != 0) {
            prop->flags_1c |= 0x80;
            prop->m_gd_prop->BindOwner004B7470(value);
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
    delete reinterpret_cast<W8PropRepresentation*>(m_pRep);
    m_pRep = 0;
    if (m_name != 0) {
        operator delete(m_name);
        m_name = 0;
    }
    delete m_animation_timer;
    m_animation_timer = 0;
    delete m_gd_prop;
    m_gd_prop = 0;
}

// FUNCTION: WIZ8 0x0044db60
W8Prop* FindPropByName(W8World* world, const char* name)
{
    int index;

    if (world != 0 && name != 0) {
        unsigned int count = ILLength(
            reinterpret_cast<W8IList*>(world->plsProps));

        for (index = 0; index < (int)count; ++index) {
            W8Prop* prop = static_cast<W8Prop*>(
                PLGet(world->plsProps, index));
            if (prop->m_name != 0 &&
                _stricmp(prop->m_name, name) == 0) {
                return prop;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0044e270
void W8Prop::SetAnimationSpeed(float speed)
{
    if (speed > 0.0f) {
        Rep()->animation_speed = speed;
        if (m_animation_timer != 0) {
            m_animation_timer->SetDuration(1.0f / speed);
        }
    }
}

extern void Function439D80(void);

/* Four accessors reaching through the owned member at 0x14. */
// FUNCTION: WIZ8 0x0044d4f0
unsigned char W8Prop::GetSetting6C()
{
    return this->Rep()->active;
}

// FUNCTION: WIZ8 0x0044d5b0
void W8Prop::SetSetting66(char value)
{
    this->Rep()->value_066 = value;
}

/* Whether the owned member is in the state the value two stands for. */
// FUNCTION: WIZ8 0x0044e1c0
bool W8Prop::IsSetting6FTwo()
{
    return this->Rep()->flag_06f == 2;
}

/* Flip the owned member between the only two values it takes: one and three. */
// FUNCTION: WIZ8 0x0044e1d0
void W8Prop::ToggleSetting6E()
{
    this->Rep()->flag_06e = this->Rep()->flag_06e == 1 ? 3 : 1;
}

/* The prop's own value at 0x18. */
// FUNCTION: WIZ8 0x0044d5a0
int W8Prop::GetValue18()
{
    return reinterpret_cast<int>(this->trigger_18);
}

/* One value out of the owned GDProp, but only once the flag that says it is
   there is up. */
// FUNCTION: WIZ8 0x0044e0a0
int W8Prop::GetGDPropValue24()
{
    if ((this->flags_1c & 0x80) != 0 && this->m_gd_prop != 0) {
        return *(int*)((char*)this->m_gd_prop + 0x24);
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
        Function439D80();
    }
    Rep()->active = value;
}

/* Both of these were separate names for 0x004A1710, which AnimObj.h now owns as
   AnimObjGetBounds004A1710. Its body reads six slots; every recovered caller
   pushes five, so each site keeps its own call shape through a cast rather than
   the prototype being weakened. */
typedef void (*LegacyAnimObjPlayCall)(
    W8AnimObj* animation, int channel, unsigned char argument, int from, int to);
extern unsigned char Function4B75F0(int arg_1, int arg_2);
extern void Function444750(void);

// FUNCTION: WIZ8 0x0044d5f0
void W8Prop::GetCenterPosition(srVector3T<float>* position)
{
    srVector3T<float> first;
    srVector3T<float> second;

    ((LegacyAnimObjBoundsCall)AnimObjGetBounds004A1710)(
        Rep()->animation, 2, Rep()->flag_064,
        &first, &second);
    position->x = (first.x + second.x) * 0.5f;
    position->y = (first.y + second.y) * 0.5f;
    position->z = (first.z + second.z) * 0.5f;
}

extern srModelInstance* GetValue65962C(void);
extern void SetValue65962C(srModelInstance* value);
extern unsigned char g_flag_6081e4;

Trigger* g_selected_prop_trigger_00659a60;
int g_selected_prop_index_00607b98;

/* Resolve the renderer's picked model instance back to the prop and trigger
   that own it. A pick is accepted only while the prop is active, the trigger
   permits selection, and the prop centre lies inside the trigger's distance
   interval. */
// FUNCTION: WIZ8 0x0044d760
char Function44D760(W8World* world)
{
    srModelInstance* selected;
    srVector3T<float> camera_position;
    unsigned int prop_count;
    int prop_index;
    char valid;

    g_selected_prop_trigger_00659a60 = 0;
    g_selected_prop_index_00607b98 = -1;
    selected = GetValue65962C();
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
            int count = static_cast<int>(AnimObjListCount004A1620(
                representation->animation, 2));
            for (instance_index = 0; instance_index < count; ++instance_index) {
                instance = AnimObjDispatchList004A1560(
                    representation->animation, 2,
                    static_cast<signed char>(instance_index));
                if (GetValue65962C() == instance) {
                    break;
                }
            }
            if (instance_index == count) {
                continue;
            }
        } else {
            instance = representation->ToggleAnimation(
                representation->flag_064);
            if (GetValue65962C() != instance) {
                continue;
            }
        }

        {
            Trigger* trigger = prop->trigger_18;
            g_selected_prop_trigger_00659a60 = trigger;
            if (trigger != 0 && (trigger->flags_0a0 & 0x100) != 0 &&
                ((trigger->flags_0a0 & 0x40000) == 0 ||
                 (trigger->flags_0a0 & 0x80000) == 0) &&
                (g_flag_6081e4 ||
                 (trigger->m_pActionData != 0 &&
                  trigger->m_pActionData->type_004 == 10 &&
                  (trigger->m_pActionData->flags_008 & 1) == 0)) &&
                representation->active != 0) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;
                float dx;
                float dy;
                float dz;
                float distance;

                ((LegacyAnimObjBoundsCall)AnimObjGetBounds004A1710)(
                    representation->animation, 2, representation->flag_064,
                    &minimum, &maximum);
                dx = (minimum.x + maximum.x) * 0.5f - camera_position.x;
                dy = (minimum.y + maximum.y) * 0.5f - camera_position.y;
                dz = (minimum.z + maximum.z) * 0.5f - camera_position.z;
                distance = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));
                if (trigger->range_minimum_0a4 <= distance) {
                    g_selected_prop_index_00607b98 = prop_index;
                    if (distance <= trigger->range_maximum_0a8) {
                        continue;
                    }
                }
            }
            valid = 0;
            SetValue65962C(0);
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
    if (AnimationIsRunning(animation)) {
        return AnimObjDispatchList004A1560(animation, 2, 0);
    }
    return AnimObjDispatch004A14D0(animation, 2, argument);
}

/* Select the animation slot whose second byte carries the requested tag.
   The slot's signed first byte is the new animation tag; the old and new
   values are retained as an ordered range for the transition state. */
// FUNCTION: WIZ8 0x0044ba50
unsigned char W8PropRepresentation::SelectAnimationSlot(unsigned char tag)
{
    int index;

    for (index = 0; index < slots.count; ++index) {
        if (slots.data[index][1] == tag) {
            signed char selected = (signed char)slots.data[index][0];

            if (selected < 0) {
                return 0;
            }
            reinterpret_cast<unsigned char*>(&timer_068)[0] = counter_094;
            reinterpret_cast<unsigned char*>(&timer_068)[1] = (unsigned char)selected;
            if (selected < (signed char)counter_094) {
                reinterpret_cast<unsigned char*>(&timer_068)[0] = (unsigned char)selected;
                reinterpret_cast<unsigned char*>(&timer_068)[1] = counter_094;
            }
            if (reinterpret_cast<unsigned char*>(&timer_068)[1] <= counter_094) {
                flag_06e = 3;
            }
            else {
                flag_06e = 1;
            }
            flag_06d = 1;
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

    for (index = 0; index < slots.count; ++index) {
        if ((int)(char)*slots.data[index] == (unsigned int)counter_094) {
            return index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x0044bb20
unsigned char W8PropRepresentation::AdvanceAnimationSegment()
{
    int segment;

    if (slots.count < 3) {
        return 0;
    }
    for (segment = 0; segment < slots.count; ++segment) {
        if ((int)(char)slots.data[segment][0] == (unsigned int)counter_094) {
            break;
        }
    }
    if (segment == slots.count) {
        segment = -1;
    }
    if (segment == -1) {
        srAssertFail(
            "lSegment!=(-1)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp",
            0x2c3, 0);
    }
    if (segment == slots.count - 2) {
        segment = 0;
    }
    else {
        ++segment;
    }
    counter_094 = slots.data[segment][0];
    counter_095 = slots.data[segment + 1][0];
    flag_06e = 1;
    flag_06d = 1;
    flag_064 = counter_094;
    return (unsigned char)segment;
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
    unsigned char argument = rep->flag_064;

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch004A14D0(rep->animation, 2, argument);
    }
    return AnimObjDispatchList004A1560(rep->animation, 2, 0);
}

/* Play the prop's animation between two points, with the same default
   argument. */
// FUNCTION: WIZ8 0x0044d5c0
int W8Prop::PlayRepAnimation(int from, int to)
{
    ((LegacyAnimObjPlayCall)AnimObjGetBounds004A1710)(
        Rep()->animation, 2, Rep()->flag_064, from, to);
    return 1;
}

/* Write the member's setting at 0x6e. The assertion names the member m_pRep,
   and the guarded store is written after the assertion rather than instead of
   it, so a null member writes through null on a build with assertions off. */
// FUNCTION: WIZ8 0x0044e1f0
void W8Prop::SetSetting6E(unsigned char value, unsigned char fallback)
{
    if (m_pRep == 0) {
        srAssertFail("m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 2698, 0);
        Rep()->flag_06e = fallback;
        return;
    }
    Rep()->flag_06e = value;
}

/* Set the live representation state. When requested, choose the direction
   and endpoint of the transition from the current and target animation tags. */
// FUNCTION: WIZ8 0x0044da80
void W8Prop::SetRepresentationActive(
    unsigned char active, unsigned char update_animation)
{
    Rep()->flag_06d = active;
    if (active == 0) {
        return;
    }

    m_animation_timer->Restart();
    if (update_animation == 0) {
        return;
    }

    if (Rep()->flag_070 == 1) {
        if ((Rep()->flag_06e == 2 &&
             Rep()->flag_06f != 2) ||
            (Rep()->flag_06e != 2 &&
             Rep()->flag_06f == 2)) {
            Rep()->flag_06e = 1;
            Rep()->flag_064 = Rep()->counter_094;
            return;
        }
        Rep()->flag_06e = 3;
        Rep()->flag_064 = Rep()->counter_095;
        return;
    }
    if (Rep()->flag_070 == 2) {
        if ((Rep()->flag_06e == 2 &&
             Rep()->flag_06f != 2) ||
            (Rep()->flag_06e != 2 &&
             Rep()->flag_06f == 2)) {
            Rep()->flag_06e = 1;
            Rep()->flag_064 = Rep()->counter_094;
            return;
        }
        Rep()->flag_06e = 3;
        Rep()->flag_064 = Rep()->counter_095;
    }
}

/* Whether the prop can be used from where the caller is. The owned GDProp has
   to be there, its own owner has to be, that owner must not be in the tenth
   state or hold either of two bits, and the reach test has to pass. */
// FUNCTION: WIZ8 0x0044e0c0
bool W8Prop::CanBeUsedFrom(int arg_2, int arg_3, char notify)
{
    char* owner;
    char* state;

    if ((flags_1c & 0x80) == 0 || m_gd_prop == 0) {
        return false;
    }
    owner = *(char**)((char*)m_gd_prop + 0x24);
    if (owner == 0) {
        return false;
    }

    state = *(char**)(owner + 0x234);
    if (state == 0 || state[4] != 10) {
        state = 0;
    }
    if ((*(int*)(owner + 0x368) != 0 && owner[0x370] == 0) ||
        (state != 0 && (state[8] & 5) != 0)) {
        return false;
    }
    if (!Function4B75F0(arg_2, arg_3)) {
        return false;
    }
    if (notify) {
        Function444750();
    }
    return true;
}

extern const float g_monster_script_direction_scale_005ec150;

/* After a successful load, bind the current animation frame's mesh to its
   path and, when the animation is already running, snapshot the live
   position into the prop. */
// FUNCTION: WIZ8 0x0044c670
void W8Prop::Method44C670()
{
    unsigned int count;
    int index;

    if (AnimationIsRunning(
            reinterpret_cast<W8PropRepresentation*>(m_pRep)->animation) != 1) {
        W8PropRepresentation* rep = reinterpret_cast<W8PropRepresentation*>(m_pRep);
        unsigned char frame = rep->flag_064;
        srModelInstance* mesh;
        W8PathAI* path;

        if (AnimationIsRunning(rep->animation) == 0) {
            mesh = AnimObjDispatch004A14D0(rep->animation, 2, frame);
        }
        else {
            mesh = AnimObjDispatchList004A1560(rep->animation, 2, 0);
        }
        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x581, 0);
        }
        path = reinterpret_cast<W8PathAI*>(
            reinterpret_cast<W8PropRepresentation*>(m_pRep)->animation->path_24);
        if (path != 0) {
            PathAISetValue004A9F60(
                path,
                (float)reinterpret_cast<W8PropRepresentation*>(m_pRep)->flag_064);
            PathAIApply004AA520(
                reinterpret_cast<W8PathAI*>(
                    reinterpret_cast<W8PropRepresentation*>(m_pRep)
                        ->animation->path_24),
                reinterpret_cast<stModelInstance005EC7D0*>(mesh));
        }
        return;
    }

    count = AnimObjListCount004A1620(
        reinterpret_cast<W8PropRepresentation*>(m_pRep)->animation, 2);
    for (index = 0; index < (int)count; ++index) {
        srModelInstance* mesh = AnimObjDispatchList004A1560(
            reinterpret_cast<W8PropRepresentation*>(m_pRep)->animation,
            2,
            (signed char)index);
        W8PathAI* path;

        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x56f, 0);
        }
        path = reinterpret_cast<W8PathAI*>(AnimObjListEntry004A16C0(
            reinterpret_cast<W8PropRepresentation*>(m_pRep)->animation,
            2,
            (signed char)index));
        if (path != 0) {
            srVector3T<float> location;

            PathAISetValue004A9F60(
                path,
                (float)reinterpret_cast<W8PropRepresentation*>(m_pRep)->flag_064);
            PathAIApply004AA520(
                path, reinterpret_cast<stModelInstance005EC7D0*>(mesh));
            reinterpret_cast<srNode*>(mesh)->getLocation(location);
            position_02c.x = location.x;
            position_02c.y = location.y;
            position_02c.z = location.z;
            position_03c.x = location.x;
            position_03c.y = location.y;
            position_03c.z = location.z;
        }
    }
    flags_1c |= 0x20;
    Function44DEA0();
}

/* Build or refresh the pathing representation for a collidable Prop.  Retail
   requires a transitive animation with one mesh, then either constructs the
   owned GDProp or reinitializes it for the current animation frame. */
// FUNCTION: WIZ8 0x0044dea0
int W8Prop::Function44DEA0()
{
    srModelInstance* instance;

    if ((flags_1c & 1) == 0) {
        return 0;
    }
    if (AnimationIsRunning(Rep()->animation) != 1) {
        ReportError00401920(
            "Collidable props can be of Transitive animation type only.");
    }
    if (AnimObjListCount004A1620(Rep()->animation, 2) != 1) {
        ReportError00401920("Collideable props should have a single mesh.");
    }
    instance = AnimObjDispatchList004A1560(Rep()->animation, 2, 0);
    if (instance == 0) {
        srAssertFail("pstInstance", PROP_CPP, 0x939, 0);
    }

    if (m_gd_prop == 0) {
        m_gd_prop = new GDProp(
            instance, reinterpret_cast<unsigned char*>(m_name),
            static_cast<unsigned short>(Rep()->flag_064), Rep()->flag_0c0,
            Rep()->flag_0c1);
    }
    else {
        if ((flags_1c & 0x20) != 0) {
            m_gd_prop->Initialize(
                instance, 1, static_cast<unsigned short>(Rep()->flag_064),
                Rep()->flag_0c0, Rep()->flag_0c1);
        }
        else {
            m_gd_prop->Initialize(
                instance, 0, 0, Rep()->flag_0c0, Rep()->flag_0c1);
        }
        if (Rep()->flag_070 == 1) {
            g_byte_00659a64 = 1;
        }
    }
    return m_gd_prop->m_surface_count_14;
}

// FUNCTION: WIZ8 0x0044bf50
unsigned char CreateAndLoadProp0044BF50(
    W8ReadLevelInfo* info, W8Prop** prop_out)
{
    W8Prop* prop;
    unsigned char success;

    if (info == 0) {
        srAssertFail("pInfo", PROP_CPP, 0x344, 0);
    }
    prop = new W8Prop();
    if (prop == 0) {
        srAssertFail("pProp", PROP_CPP, 0x348, 0);
    }
    success = reinterpret_cast<W8PropRepresentation*>(prop->m_pRep)->LoadProp0044AEE0(
        info, prop);
    if (success != 0) {
        *prop_out = prop;
        prop->m_animation_timer->SetDuration(
            g_float_005ebb38 /
            reinterpret_cast<W8PropRepresentation*>(prop->m_pRep)->animation_speed);
        prop->Method44C670();
    }
    return success;
}

/* Resource loader rooted at CreateAndLoadProp.  The call site loads
   prop->m_pRep into ECX before the two stack arguments, so this is a
   PropRep method: LoadProp(pInfo, pProp). */
// FUNCTION: WIZ8 0x0044aee0
unsigned char W8PropRepresentation::LoadProp0044AEE0(
    W8ReadLevelInfo* info, W8Prop* prop)
{
    int hFile;
    unsigned char success;
    signed char version;
    unsigned char frame_count = 0;
    unsigned char option_byte = 0;
    unsigned char attach_flag = 0;
    float playback_scale = 0.0f;
    float flag_bits = 0.0f;
    W8AnimObj* animation = 0;
    unsigned char result = 0;
    long fail_line;


    if (info == 0 || info->hFile == 0 || prop == 0) {
        srAssertFail(
            "pInfo && pInfo->hFile && pProp", PROP_CPP, 0xae, 0);
    }
    hFile = info->hFile;
    success = ReadVirtualFile(hFile, &version, 1, 0);
    if (version < 4) {
        unsigned char b0 = 0;
        unsigned char b1 = 0;
        unsigned char b2 = 0;
        int ok;
        W8AniMesh* mesh;

        if (success == 0 ||
            (success = ReadVirtualFile(hFile, &b0, 1, 0), success == 0) ||
            (success = ReadVirtualFile(hFile, &b1, 1, 0), success == 0) ||
            (success = ReadVirtualFile(hFile, &b2, 1, 0), success == 0)) {
            ok = 0;
        }
        else {
            ok = 1;
        }
        if (version < 2) {
            playback_scale = 15.0f;
            if (!ok) {
                fail_line = 0xcb;
                goto fail;
            }
        }
        else {
            if (!ok) {
                fail_line = 0xcb;
                goto fail;
            }
            success = ReadVirtualFile(hFile, &playback_scale, 4, 0);
            if (success == 0) {
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
        if (result == 0) {
            fail_line = 0xd8;
            result = 0;
            goto fail_with_result;
        }
        animation = CreateAnimObj004A01A0();
        animation->entries_18[2] = mesh;
        animation->unknown_00[0] = 1;
        animation->unknown_00[1] = b0;
        animation->value_02 = b1;
        animation->unknown_03[0] = b2;
        animation->unknown_03[1] = 0;
        animation->flag_05 = 0;
        animation->playback_scale_08 = playback_scale;
        this->animation = animation;
    }
    else {
        unsigned int entry_index;
        unsigned int list_count;
        signed char slot_count;
        int slot_i;

        flag_bits = 0.0f;
        info->mesh_filename = 0;
        if (success != 0) {
            ReadVirtualFile(hFile, &frame_count, 1, 0);
        }
        if (version > 4) {
            float lx = 0.0f;
            float ly = 0.0f;
            float lz = 0.0f;

            ReadVirtualFile(hFile, &option_byte, 1, 0);
            ReadVirtualFile(hFile, &lx, 4, 0);
            ReadVirtualFile(hFile, &ly, 4, 0);
            ReadVirtualFile(hFile, &lz, 4, 0);
            lx *= g_monster_script_direction_scale_005ec150;
            ly *= g_monster_script_direction_scale_005ec150;
            lz *= g_monster_script_direction_scale_005ec150;
            this->local_location_010.x = lx;
            this->location_004.x = lx;
            this->local_location_010.y = ly;
            this->location_004.y = ly;
            this->local_location_010.z = lz;
            this->location_004.z = lz;
        }
        if (version > 5) {
            ReadVirtualFile(hFile, &flag_bits, 4, 0);
            prop->flags_1c |= (unsigned int)flag_bits;
        }
        if (version > 6) {
            char* buffer = static_cast<char*>(operator new(0x40));
            int length = -1;
            char* scan = buffer;

            ReadVirtualFile(hFile, buffer, 0x40, 0);
            do {
                if (length == 0) {
                    break;
                }
                --length;
            } while (*scan++ != '\0');
            if (length == -2) {
                operator delete(buffer);
            }
            else {
                if (prop->m_name != 0) {
                    operator delete(prop->m_name);
                }
                prop->m_name = buffer;
            }
        }
        if (version > 7) {
            slot_count = 0;
            ReadVirtualFile(hFile, &slot_count, 1, 0);
            for (slot_i = 0; slot_i < slot_count; ++slot_i) {
                unsigned short frame_tmp = 0;
                unsigned short tag_tmp = 0;
                unsigned char* slot =
                    static_cast<unsigned char*>(operator new(2));

                ReadVirtualFile(hFile, &frame_tmp, 2, 0);
                slot[0] = (unsigned char)frame_tmp;
                ReadVirtualFile(hFile, &tag_tmp, 2, 0);
                slot[1] = (unsigned char)tag_tmp;
                if (frame_count <= frame_tmp) {
                    srAssertFail(
                        "(usTemp < (UINT16)ubNumFrames)",
                        PROP_CPP,
                        0x11f,
                        reinterpret_cast<const char*>(String(
                            "%s Prop Error Segment %d frame n",
                            prop->m_name,
                            (unsigned int)tag_tmp,
                            (unsigned int)frame_tmp)));
                }
                {
                    int next_count = this->slots.count + 1;

                    if (next_count <= this->slots.capacity ||
                        this->slots.Grow(next_count) != 0) {
                        this->slots.data[this->slots.count] = slot;
                        this->slots.count = next_count;
                    }
                }
            }
        }
        animation = CreateAnimObj004A01A0();
        result = AnimObjReadFromFile004A05C0(info, animation, 1, 0, 1);
        this->animation = animation;
        if (AnimationIsRunning(animation) == 1) {
            animation->value_16 = frame_count;
        }
        this->value_0a8 = *(float*)(animation->unknown_0c + 4);
        this->flag_0a5 = animation->unknown_0c[0] != 0;
        list_count = AnimObjValue004A15D0(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            srModelInstance* instance;
            stMeshModel* mesh_model;
            char* named;

            if (AnimationIsRunning(this->animation) == 0) {
                instance = AnimObjDispatch004A14D0(
                    this->animation, 2, (unsigned char)entry_index);
            }
            else {
                instance = AnimObjDispatchList004A1560(
                    this->animation, 2, 0);
            }
            named = reinterpret_cast<char*>(
                String("Prop: %s", prop->m_name));
            instance->setName(named);
            mesh_model = static_cast<stMeshModel*>(instance->model());
            for (; mesh_model != 0; mesh_model = mesh_model->next) {
                if (AnimationIsRunning(animation) == 0) {
                    mesh_model->InitializeVertexWeights004721E0(1);
                }
            }
            if (option_byte != 0) {
                srNode* child;

                static_cast<PropModelInstanceAccess*>(instance)
                    ->WriteAlignAxisYUp();
                child = instance->firstChild();
                if (child != 0) {
                    static_cast<PropModelInstanceAccess*>(child)
                        ->WriteAlignAxisYUp();
                }
            }
        }
        list_count = AnimObjListCount004A1620(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            W8PathAI* path = reinterpret_cast<W8PathAI*>(
                AnimObjListEntry004A16C0(
                    animation, 2, (signed char)entry_index));
            if (path != 0) {
                PathAISetFlag38004AA9D0(path, 1);
                PathAISetFlag1C004AAA10(path, 1);
                PathAISetScale004AA9C0(
                    path, animation->playback_scale_08);
            }
        }
    }

    this->active = 1;
    this->flag_070 = animation->unknown_03[0];
    this->flag_06f = animation->value_02;
    this->flag_06d = animation->unknown_00[1];
    this->flag_06e = 1;
    this->animation_speed = animation->playback_scale_08;
    this->timer_068 = GetTickCount();
    if (this->flag_070 == 1 || this->flag_070 == 2) {
        this->flag_06d = 0;
        this->flag_06e =
            (unsigned char)(((this->flag_06f != 2) - 1U & 2) + 2);
    }

    if (animation->flag_05 == 0) {
        W8AniMesh* mesh = reinterpret_cast<W8AniMesh*>(
            AnimObjEntry004A1660(animation, 2, 0, 0));
        unsigned char value_count = AniMeshValue004B64F0(mesh);
        stModelInstance005EC7D0* frame =
            GetAniMeshFrame004B6550(mesh, 0);
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
            *reinterpret_cast<float*>(&this->value_074.value_04) = minimum.y;
            *reinterpret_cast<float*>(&this->value_080.value_00) = maximum.x;
            *reinterpret_cast<float*>(&this->value_074.value_00) = minimum.x;
            *reinterpret_cast<float*>(&this->value_080.value_08) = maximum.z;
            *reinterpret_cast<float*>(&this->value_074.value_08) = minimum.z;
            *reinterpret_cast<float*>(&this->value_080.value_04) = maximum.y;
        }
        extent = maximum.x - minimum.x;
        if (extent < maximum.y - minimum.y) {
            extent = maximum.y - minimum.y;
        }
        if (extent < maximum.z - minimum.z) {
            extent = maximum.z - minimum.z;
        }
        *reinterpret_cast<float*>(&this->value_08c) =
            extent * g_float_005ebc7c;
    }

    if (version > 2) {
        if (result == 0) {
            result = 0;
        }
        else {
            success = ReadVirtualFile(hFile, &attach_flag, 1, 0);
            result = success != 0 ? 1 : 0;
        }
        if (attach_flag != 0) {
            Trigger* trigger;

            trigger = Trigger::CreateAndLoadLevelTrigger(hFile, info->world);
            /* Retail writes the attach fields first, then tests type at +0x22a
               (the stores do not touch that word). */
            trigger->m_bRepType = 2;
            trigger->m_pProp = prop;
            if (trigger->initial_action_22a == 0x40) {
                Function445200(trigger);
            }
            if (trigger->trigger_kind_018 == 1 &&
                trigger->initial_action_22a == 8) {
                unsigned int path_count;
                unsigned int path_i;

                this->flag_0a4 = 1;
                if (AnimationIsRunning(this->animation) == 1) {
                    path_count = AnimObjListCount004A1620(
                        this->animation, 2);
                    for (path_i = 0; path_i < path_count; ++path_i) {
                        W8PathAI* path = reinterpret_cast<W8PathAI*>(
                            AnimObjListEntry004A16C0(
                                this->animation, 2,
                                (signed char)path_i));
                        path->flag_39 = 1;
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
                this->flag_06d = 0;
                break;
            }
            prop->trigger_18 = trigger;
            hFile = info->hFile;
        }
    }

    if (version >= 9) {
        unsigned char extra = 0;

        if (result != 0) {
            success = ReadVirtualFile(hFile, &extra, 1, 0);
            result = success != 0 ? 1 : 0;
        }
        if (extra != 0) {
            if (result != 0 &&
                (success = ReadVirtualFile(
                     hFile, &this->flag_0c0, 1, 0),
                 success != 0) &&
                (success = ReadVirtualFile(
                     hFile, &this->flag_0c1, 1, 0),
                 success != 0)) {
                result = 1;
            }
            else {
                result = 0;
            }
        }
    }

    this->counter_094 = 0;
    this->flag_064 = 0;
    {
        unsigned int frames = AnimObjValue004A15D0(animation, 2);
        this->counter_095 = (unsigned char)frames - 1;
    }
    return result;

fail:
    srAssertFail("fSuccess", PROP_CPP, fail_line, 0);
    return 0;

fail_with_result:
    srAssertFail("fSuccess", PROP_CPP, fail_line, 0);
    return result;
}
