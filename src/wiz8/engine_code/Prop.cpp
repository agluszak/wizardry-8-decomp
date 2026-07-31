#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/sr_api.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/PList.h"

#include <string.h>

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

extern int Function443830(W8World* world, W8Prop* prop);
extern void Function4B7470(int value);

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
    count = PListGetCount(world->plsProps);
    for (index = 0; index < static_cast<int>(count); ++index) {
        W8Prop* prop = static_cast<W8Prop*>(PListGetAt(world->plsProps, index));
        int value;

        if (prop != 0 && (value = Function443830(world, prop)) != 0 && prop->m_gd_prop != 0) {
            prop->flags_1c |= 0x80;
            Function4B7470(value);
        }
    }
}

/* VC6 emits the scalar-deleting wrapper from this ordinary virtual
   destructor. */
// SYNTHETIC: WIZ8 0x0044BEA0
// W8Prop::`scalar deleting destructor'
W8Prop::~W8Prop()
{
    delete m_pRep;
    delete[] m_name;
    delete m_animation_timer;
    delete m_gd_prop;
}

// FUNCTION: WIZ8 0x0044db60
W8Prop* FindPropByName(W8World* world, const char* name)
{
    int index;

    if (world != 0 && name != 0) {
        unsigned int count = PListGetCount(world->plsProps);

        for (index = 0; index < (int)count; ++index) {
            W8Prop* prop = static_cast<W8Prop*>(
                PListGetAt(world->plsProps, index));
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
        m_pRep->animation_speed = speed;
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
    return this->m_pRep->setting_6c;
}

// FUNCTION: WIZ8 0x0044d5b0
void W8Prop::SetSetting66(char value)
{
    this->m_pRep->setting_66 = value;
}

/* Whether the owned member is in the state the value two stands for. */
// FUNCTION: WIZ8 0x0044e1c0
bool W8Prop::IsSetting6FTwo()
{
    return this->m_pRep->setting_6f == 2;
}

/* Flip the owned member between the only two values it takes: one and three. */
// FUNCTION: WIZ8 0x0044e1d0
void W8Prop::ToggleSetting6E()
{
    this->m_pRep->setting_6e = this->m_pRep->setting_6e == 1 ? 3 : 1;
}

/* The prop's own value at 0x18. */
// FUNCTION: WIZ8 0x0044d5a0
int W8Prop::GetValue18()
{
    return this->value_18;
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
    if (m_pRep->setting_6c == 0) {
        Function439D80();
    }
    m_pRep->setting_6c = value;
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
        m_pRep->animation, 2, m_pRep->default_animation_tag,
        &first, &second);
    position->x = (first.x + second.x) * 0.5f;
    position->y = (first.y + second.y) * 0.5f;
    position->z = (first.z + second.z) * 0.5f;
}

/* Start or stop the prop's own animation, whichever it is not doing. */
// FUNCTION: WIZ8 0x0044ba00
void W8PropRepresentation::ToggleAnimation(int argument)
{
    if (AnimationIsRunning(animation)) {
        AnimObjDispatchList004A1560(animation, 2, 0);
        return;
    }
    AnimObjDispatch004A14D0(animation, 2, argument);
}

/* Select the animation slot whose second byte carries the requested tag.
   The slot's signed first byte is the new animation tag; the old and new
   values are retained as an ordered range for the transition state. */
// FUNCTION: WIZ8 0x0044ba50
unsigned char W8PropRepresentation::SelectAnimationSlot(unsigned char tag)
{
    int index;

    for (index = 0; index < slot_count; ++index) {
        if (slots[index][1] == tag) {
            signed char selected = (signed char)slots[index][0];

            if (selected < 0) {
                return 0;
            }
            unknown_068[0] = current_tag;
            unknown_068[1] = (unsigned char)selected;
            if (selected < (signed char)current_tag) {
                unknown_068[0] = (unsigned char)selected;
                unknown_068[1] = current_tag;
            }
            if (unknown_068[1] <= current_tag) {
                setting_6e = 3;
            }
            else {
                setting_6e = 1;
            }
            active = 1;
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

    for (index = 0; index < slot_count; ++index) {
        if ((int)(char)*slots[index] == (unsigned int)current_tag) {
            return index;
        }
    }
    return -1;
}

// FUNCTION: WIZ8 0x0044bb20
unsigned char W8PropRepresentation::AdvanceAnimationSegment()
{
    int segment;

    if (slot_count < 3) {
        return 0;
    }
    for (segment = 0; segment < slot_count; ++segment) {
        if ((int)(char)slots[segment][0] == (unsigned int)current_tag) {
            break;
        }
    }
    if (segment == slot_count) {
        segment = -1;
    }
    if (segment == -1) {
        srAssertFail(
            "lSegment!=(-1)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp",
            0x2c3, 0);
    }
    if (segment == slot_count - 2) {
        segment = 0;
    }
    else {
        ++segment;
    }
    current_tag = slots[segment][0];
    unknown_095[0] = slots[segment + 1][0];
    setting_6e = 1;
    active = 1;
    default_animation_tag = current_tag;
    return (unsigned char)segment;
}

/* The same toggle reached through the prop rather than through the member. */
// FUNCTION: WIZ8 0x0044d500
srModelInstance* W8Prop::ToggleRepAnimation(int argument)
{
    W8PropRepresentation* rep = m_pRep;

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
    W8PropRepresentation* rep = m_pRep;
    unsigned char argument = rep->default_animation_tag;

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
        m_pRep->animation, 2, m_pRep->default_animation_tag, from, to);
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
        m_pRep->setting_6e = fallback;
        return;
    }
    m_pRep->setting_6e = value;
}

/* Set the live representation state. When requested, choose the direction
   and endpoint of the transition from the current and target animation tags. */
// FUNCTION: WIZ8 0x0044da80
void W8Prop::SetRepresentationActive(
    unsigned char active, unsigned char update_animation)
{
    m_pRep->active = active;
    if (active == 0) {
        return;
    }

    m_animation_timer->Restart();
    if (update_animation == 0) {
        return;
    }

    if (m_pRep->unknown_070[0] == 1) {
        if ((m_pRep->setting_6e == 2 &&
             m_pRep->setting_6f != 2) ||
            (m_pRep->setting_6e != 2 &&
             m_pRep->setting_6f == 2)) {
            m_pRep->setting_6e = 1;
            m_pRep->default_animation_tag = m_pRep->current_tag;
            return;
        }
        m_pRep->setting_6e = 3;
        m_pRep->default_animation_tag = m_pRep->unknown_095[0];
        return;
    }
    if (m_pRep->unknown_070[0] == 2) {
        if ((m_pRep->setting_6e == 2 &&
             m_pRep->setting_6f != 2) ||
            (m_pRep->setting_6e != 2 &&
             m_pRep->setting_6f == 2)) {
            m_pRep->setting_6e = 1;
            m_pRep->default_animation_tag = m_pRep->current_tag;
            return;
        }
        m_pRep->setting_6e = 3;
        m_pRep->default_animation_tag = m_pRep->unknown_095[0];
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
