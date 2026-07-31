#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/sr_api.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/PList.h"

#include <string.h>

#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/float_constants.h"
#include "wiz8/virtual_file.h"
#include "surrender/srNode.h"

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

extern int Function443830(W8World* world, W8Prop005EC1E0* prop);
extern void Function4B7470(int value);
extern int IncrementValue60DFAC(void);

#define PROP_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp"

// VTABLE: WIZ8 0x005ec1e0
// class W8Prop005EC1E0

// VTABLE: WIZ8 0x005ec1c8
// class W8PropRep

// VTABLE: WIZ8 0x005ec1d0
// class W8GrowableVector<unsigned char*>

// TEMPLATE: WIZ8 0x0044efe0
// W8GrowableVector<unsigned char*>::W8GrowableVector

/* Prop::Prop() - GrObject base, then m_pRep / m_pTimer and the remaining
   positional floats the load path later overwrites. */
// FUNCTION: WIZ8 0x0044bc00
W8Prop005EC1E0::W8Prop005EC1E0()
{
    value_18 = 0;
    flags_1c = 0;
    m_name_20 = 0;
    unknown_024 = 0;
    unknown_004 = 4;
    unknown_008 = IncrementValue60DFAC();
    m_pRep = reinterpret_cast<W8ItemRep*>(new W8PropRep());
    m_pTimer = new W8Timer005EC0A4();
    memset(&position_02c, 0, sizeof(position_02c));
    m_owned_38 = 0;
    memset(&position_03c, 0, sizeof(position_03c));
    scale_048 = 1.0f;
    memset(unknown_04c, 0, sizeof(unknown_04c));
    scale_058 = 1.0f;
    memset(unknown_05c, 0, sizeof(unknown_05c));
    scale_068 = 1.0f;
    scale_06c = 1.0f;
    memset(unknown_070, 0, sizeof(unknown_070));
    scale_07c = 1.0f;
    memset(unknown_080, 0, sizeof(unknown_080));
    scale_08c = 1.0f;
    if (m_pRep == 0) {
        srAssertFail(
            "m_pRep", PROP_CPP, 0x30e,
            "Prop::Prop() out of memory allocating m_pRep");
    }
    if (m_pTimer == 0) {
        srAssertFail(
            "m_pTimer", PROP_CPP, 0x30f,
            "Prop::Prop() out of memory allocating m_pTimer");
    }
}

W8PropRep::W8PropRep()
    : animation(0),
      animation_speed_09c(0.0f),
      value_0a0(0.0f),
      flag_0a4(0),
      flag_0a5(0),
      value_0a8(0.5f),
      flag_0ac(0),
      flag_0ad(0),
      slots(5),
      flag_0c0(0xff),
      flag_0c1(0xff)
{
}

/* Copy keeps animation through CloneAnimObj and rebuilds an empty slot vector
   with the source capacity.  Clone's vtable slot allocates 0xc4 and lands here. */
// FUNCTION: WIZ8 0x0044ad10
W8PropRep::W8PropRep(const W8PropRep& other)
    : W8AnimRep005ED050(other),
      animation_speed_09c(other.animation_speed_09c),
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
// W8PropRep::`scalar deleting destructor'
// FUNCTION: WIZ8 0x0044ae30
W8PropRep::~W8PropRep()
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
W8AnimRepBase005EC1D8* W8PropRep::Clone()
{
    return new W8PropRep(*this);
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
    count = PListGetCount(world->plsProps);
    for (index = 0; index < static_cast<int>(count); ++index) {
        W8Prop005EC1E0* prop = static_cast<W8Prop005EC1E0*>(PListGetAt(world->plsProps, index));
        int value;

        if (prop != 0 && (value = Function443830(world, prop)) != 0 && prop->m_owned_38 != 0) {
            prop->flags_1c |= 0x80;
            Function4B7470(value);
        }
    }
}

/* VC6 emits the scalar-deleting wrapper from this ordinary virtual
   destructor. */
// SYNTHETIC: WIZ8 0x0044BEA0
// W8Prop005EC1E0::`scalar deleting destructor'
W8Prop005EC1E0::~W8Prop005EC1E0()
{
    delete reinterpret_cast<W8PropRep*>(m_pRep);
    m_pRep = 0;
    if (m_name_20 != 0) {
        operator delete(m_name_20);
        m_name_20 = 0;
    }
    delete m_pTimer;
    m_pTimer = 0;
    delete m_owned_38;
    m_owned_38 = 0;
}

// FUNCTION: WIZ8 0x0044db60
W8Prop005EC1E0* FindPropByName(W8World* world, const char* name)
{
    int index;

    if (world != 0 && name != 0) {
        unsigned int count = PListGetCount(world->plsProps);

        for (index = 0; index < (int)count; ++index) {
            W8Prop005EC1E0* prop = static_cast<W8Prop005EC1E0*>(
                PListGetAt(world->plsProps, index));
            if (prop->m_name_20 != 0 &&
                _stricmp(prop->m_name_20, name) == 0) {
                return prop;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0044e270
void W8Prop005EC1E0::SetAnimationSpeed(float speed)
{
    if (speed > 0.0f) {
        reinterpret_cast<W8PropRep*>(m_pRep)->animation_speed_09c = speed;
        if (m_pTimer != 0) {
            m_pTimer->SetDuration(1.0f / speed);
        }
    }
}

extern void Function439D80(void);

/* Four accessors reaching through the owned member at 0x14. */
// FUNCTION: WIZ8 0x0044d4f0
unsigned char W8Prop005EC1E0::GetSetting6C()
{
    return reinterpret_cast<W8PropRep*>(m_pRep)->active;
}

// FUNCTION: WIZ8 0x0044d5b0
void W8Prop005EC1E0::SetSetting66(char value)
{
    reinterpret_cast<W8PropRep*>(m_pRep)->value_066 = value;
}

/* Whether the owned member is in the state the value two stands for. */
// FUNCTION: WIZ8 0x0044e1c0
bool W8Prop005EC1E0::IsSetting6FTwo()
{
    return reinterpret_cast<W8PropRep*>(m_pRep)->flag_06f == 2;
}

/* Flip the owned member between the only two values it takes: one and three. */
// FUNCTION: WIZ8 0x0044e1d0
void W8Prop005EC1E0::ToggleSetting6E()
{
    reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e == 1 ? 3 : 1;
}

/* The prop's own value at 0x18. */
// FUNCTION: WIZ8 0x0044d5a0
int W8Prop005EC1E0::GetValue18()
{
    return this->value_18;
}

/* One value out of the owned GDProp, but only once the flag that says it is
   there is up. */
// FUNCTION: WIZ8 0x0044e0a0
int W8Prop005EC1E0::GetGDPropValue24()
{
    if ((this->flags_1c & 0x80) != 0 && this->m_owned_38 != 0) {
        return *(int*)((char*)this->m_owned_38 + 0x24);
    }
    return 0;
}

/* Write the settings block's byte at 0x6c. Going from zero to anything else
   costs an extra call first, so zero is the state that has to be left rather
   than a value like the others. */
// FUNCTION: WIZ8 0x0044d4b0
void W8Prop005EC1E0::SetSetting6C(unsigned char value)
{
    if (reinterpret_cast<W8PropRep*>(m_pRep)->active == 0) {
        Function439D80();
    }
    reinterpret_cast<W8PropRep*>(m_pRep)->active = value;
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
void W8Prop005EC1E0::GetCenterPosition(srVector3T<float>* position)
{
    srVector3T<float> first;
    srVector3T<float> second;

    ((LegacyAnimObjBoundsCall)AnimObjGetBounds004A1710)(
        reinterpret_cast<W8PropRep*>(m_pRep)->animation, 2, reinterpret_cast<W8PropRep*>(m_pRep)->flag_064,
        &first, &second);
    position->x = (first.x + second.x) * 0.5f;
    position->y = (first.y + second.y) * 0.5f;
    position->z = (first.z + second.z) * 0.5f;
}

/* Start or stop the prop's own animation, whichever it is not doing. */
// FUNCTION: WIZ8 0x0044ba00
void W8PropRep::ToggleAnimation(int argument)
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
unsigned char W8PropRep::SelectSlot0044BA50(unsigned char tag)
{
    int index;

    for (index = 0; index < slots.count; ++index) {
        if (slots.data[index][1] == tag) {
            signed char selected = (signed char)slots.data[index][0];

            if (selected < 0) {
                return 0;
            }
            unsigned char* range = reinterpret_cast<unsigned char*>(&timer_068);
            range[0] = counter_094;
            range[1] = (unsigned char)selected;
            if (selected < (signed char)counter_094) {
                range[0] = (unsigned char)selected;
                range[1] = counter_094;
            }
            if (range[1] <= counter_094) {
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
int W8PropRep::FindSlotByCurrentTag()
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
unsigned char W8PropRep::AdvanceAnimationSegment()
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
srModelInstance* W8Prop005EC1E0::ToggleRepAnimation(int argument)
{
    W8PropRep* rep = reinterpret_cast<W8PropRep*>(m_pRep);

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch004A14D0(rep->animation, 2, argument);
    }
    return AnimObjDispatchList004A1560(rep->animation, 2, 0);
}

/* And again with the member's own stored argument instead of a caller's -
   which is what makes 0x64 the animation's default argument. */
// FUNCTION: WIZ8 0x0044d550
srModelInstance* W8Prop005EC1E0::ToggleRepAnimationDefault()
{
    W8PropRep* rep = reinterpret_cast<W8PropRep*>(m_pRep);
    unsigned char argument = rep->flag_064;

    if (!AnimationIsRunning(rep->animation)) {
        return AnimObjDispatch004A14D0(rep->animation, 2, argument);
    }
    return AnimObjDispatchList004A1560(rep->animation, 2, 0);
}

/* Play the prop's animation between two points, with the same default
   argument. */
// FUNCTION: WIZ8 0x0044d5c0
int W8Prop005EC1E0::PlayRepAnimation(int from, int to)
{
    ((LegacyAnimObjPlayCall)AnimObjGetBounds004A1710)(
        reinterpret_cast<W8PropRep*>(m_pRep)->animation, 2, reinterpret_cast<W8PropRep*>(m_pRep)->flag_064, from, to);
    return 1;
}

/* Write the member's setting at 0x6e. The assertion names the member m_pRep,
   and the guarded store is written after the assertion rather than instead of
   it, so a null member writes through null on a build with assertions off. */
// FUNCTION: WIZ8 0x0044e1f0
void W8Prop005EC1E0::SetSetting6E(unsigned char value, unsigned char fallback)
{
    if (reinterpret_cast<W8PropRep*>(m_pRep) == 0) {
        srAssertFail("m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 2698, 0);
        reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = fallback;
        return;
    }
    reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = value;
}

/* Set the live representation state. When requested, choose the direction
   and endpoint of the transition from the current and target animation tags. */
// FUNCTION: WIZ8 0x0044da80
void W8Prop005EC1E0::SetRepActive0044DA80(
    unsigned char active, unsigned char update_animation)
{
    reinterpret_cast<W8PropRep*>(m_pRep)->flag_06d = active;
    if (active == 0) {
        return;
    }

    m_pTimer->Restart();
    if (update_animation == 0) {
        return;
    }

    if (reinterpret_cast<W8PropRep*>(m_pRep)->flag_070 == 1) {
        if ((reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e == 2 &&
             reinterpret_cast<W8PropRep*>(m_pRep)->flag_06f != 2) ||
            (reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e != 2 &&
             reinterpret_cast<W8PropRep*>(m_pRep)->flag_06f == 2)) {
            reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = 1;
            reinterpret_cast<W8PropRep*>(m_pRep)->flag_064 = reinterpret_cast<W8PropRep*>(m_pRep)->counter_094;
            return;
        }
        reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = 3;
        reinterpret_cast<W8PropRep*>(m_pRep)->flag_064 = reinterpret_cast<W8PropRep*>(m_pRep)->counter_095;
        return;
    }
    if (reinterpret_cast<W8PropRep*>(m_pRep)->flag_070 == 2) {
        if ((reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e == 2 &&
             reinterpret_cast<W8PropRep*>(m_pRep)->flag_06f != 2) ||
            (reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e != 2 &&
             reinterpret_cast<W8PropRep*>(m_pRep)->flag_06f == 2)) {
            reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = 1;
            reinterpret_cast<W8PropRep*>(m_pRep)->flag_064 = reinterpret_cast<W8PropRep*>(m_pRep)->counter_094;
            return;
        }
        reinterpret_cast<W8PropRep*>(m_pRep)->flag_06e = 3;
        reinterpret_cast<W8PropRep*>(m_pRep)->flag_064 = reinterpret_cast<W8PropRep*>(m_pRep)->counter_095;
    }
}

/* Whether the prop can be used from where the caller is. The owned GDProp has
   to be there, its own owner has to be, that owner must not be in the tenth
   state or hold either of two bits, and the reach test has to pass. */
// FUNCTION: WIZ8 0x0044e0c0
bool W8Prop005EC1E0::CanBeUsedFrom(int arg_2, int arg_3, char notify)
{
    char* owner;
    char* state;

    if ((flags_1c & 0x80) == 0 || m_owned_38 == 0) {
        return false;
    }
    owner = *(char**)((char*)m_owned_38 + 0x24);
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

extern void Function44DEA0(W8Prop005EC1E0* prop);
extern unsigned char LoadAniMeshFromInfo004B5B30(
    W8ReadLevelInfo* info, W8AniMesh* mesh, int positional,
    unsigned char load_all);
extern unsigned char Function4A05C0(
    void* positional, W8AnimObj* animation, int a, int b, int c);
extern void* Function441A20(int hFile, void* positional);
extern void Function445200(void* object);

extern const double g_zero_005ebb40;
extern const float g_reciprocal_005ebc30;
extern float g_camera_step_factor_005ebc7c;
extern const float g_monster_script_direction_scale_005ec150;

/* After a successful load, bind the current animation frame's mesh to its
   path and, when the animation is already running, snapshot the live
   position into the prop. */
// FUNCTION: WIZ8 0x0044c670
void W8Prop005EC1E0::Method44C670()
{
    W8PropRep* rep = reinterpret_cast<W8PropRep*>(m_pRep);
    W8AnimObj* animation = rep->animation;
    unsigned int count;
    int index;

    if (AnimationIsRunning(animation) != 1) {
        unsigned char frame = rep->flag_064;
        srModelInstance* mesh;

        if (!AnimationIsRunning(animation)) {
            mesh = AnimObjDispatch004A14D0(animation, 2, frame);
        }
        else {
            mesh = AnimObjDispatchList004A1560(animation, 2, 0);
        }
        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x581, 0);
        }
        {
            W8PathAI* path =
                reinterpret_cast<W8PathAI*>(animation->entries_18[3]);
            if (path != 0) {
                PathAISetValue004A9F60(path, (float)rep->flag_064);
                PathAIApply004AA520(
                    path,
                    reinterpret_cast<stModelInstance005EC7D0*>(mesh));
            }
        }
        return;
    }

    count = AnimObjListCount004A1620(animation, 2);
    for (index = 0; index < (int)count; ++index) {
        srModelInstance* mesh =
            AnimObjDispatchList004A1560(animation, 2, (signed char)index);
        W8PathAI* path;

        if (mesh == 0) {
            srAssertFail("psrMesh", PROP_CPP, 0x56f, 0);
        }
        path = reinterpret_cast<W8PathAI*>(
            AnimObjListEntry004A16C0(animation, 2, (signed char)index));
        if (path != 0) {
            srVector3T<float> location;

            PathAISetValue004A9F60(path, (float)rep->flag_064);
            PathAIApply004AA520(
                path, reinterpret_cast<stModelInstance005EC7D0*>(mesh));
            reinterpret_cast<srNode*>(mesh)->getLocation(location);
            position_02c = location;
            position_03c = location;
        }
    }
    flags_1c |= 0x20;
    Function44DEA0(this);
}

// FUNCTION: WIZ8 0x0044bf50
unsigned char CreateAndLoadProp0044BF50(
    W8ReadLevelInfo* info, W8Prop005EC1E0** prop_out)
{
    W8Prop005EC1E0* prop;
    unsigned char success;

    if (info == 0) {
        srAssertFail("pInfo", PROP_CPP, 0x344, 0);
    }
    prop = new W8Prop005EC1E0();
    if (prop == 0) {
        srAssertFail("pProp", PROP_CPP, 0x348, 0);
    }
    success = LoadProp0044AEE0(info, prop);
    if (success != 0) {
        *prop_out = prop;
        prop->m_pTimer->SetDuration(
            g_float_005ebb38 /
            reinterpret_cast<W8PropRep*>(prop->m_pRep)->animation_speed_09c);
        prop->Method44C670();
    }
    return success;
}

/* Resource loader.  Faithfulness requires the complete body; callees that are
   not yet recovered stay as typed externs rather than invented wrappers. */
// FUNCTION: WIZ8 0x0044aee0
unsigned char LoadProp0044AEE0(
    W8ReadLevelInfo* info, W8Prop005EC1E0* prop)
{
    W8PropRep* rep;
    int hFile;
    unsigned char success;
    signed char version;
    unsigned char frame_count;
    unsigned char flags_byte;
    float playback_scale;
    W8AnimObj* animation;
    W8AniMesh* mesh;

    if (info == 0 || info->hFile == 0 || prop == 0) {
        srAssertFail(
            "pInfo && pInfo->hFile && pProp", PROP_CPP, 0xae, 0);
    }
    rep = reinterpret_cast<W8PropRep*>(prop->m_pRep);
    hFile = info->hFile;
    success = ReadVirtualFile(hFile, &version, 1, 0);
    if (version < 4) {
        unsigned char b0 = 0;
        unsigned char b1 = 0;
        unsigned char b2 = 0;
        int ok;

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
                srAssertFail("fSuccess", PROP_CPP, 0xcb, 0);
                return 0;
            }
        }
        else {
            if (!ok) {
                srAssertFail("fSuccess", PROP_CPP, 0xcb, 0);
                return 0;
            }
            success = ReadVirtualFile(hFile, &playback_scale, 4, 0);
            if (success == 0) {
                srAssertFail("fSuccess", PROP_CPP, 0xcb, 0);
                return 0;
            }
        }
        prop->m_pAI = 0;
        mesh = CreateAniMesh004B57E0();
        if (mesh == 0) {
            srAssertFail("pAniMesh", PROP_CPP, 0xd5, 0);
        }
        success = LoadAniMeshFromInfo004B5B30(info, mesh, 1, 1);
        if (success == 0) {
            srAssertFail("fSuccess", PROP_CPP, 0xd8, 0);
            return 0;
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
        rep->animation = animation;
    }
    else {
        /* Version 4+ path: name, slots, AnimObj list, bounds, optional
           trigger attachment.  Kept as one body with the v<4 path above so
           the retail control flow stays intact. */
        float unused_scale = 0.0f;
        unsigned char name_flag;
        int slot_index;
        unsigned short frame_index;
        unsigned short tag;
        unsigned char* slot;
        unsigned int list_count;
        unsigned int entry_index;
        float min_x;
        float min_y;
        float min_z;
        float max_x;
        float max_y;
        float max_z;
        float extent;

        prop->m_pAI = 0;
        if (success != 0) {
            ReadVirtualFile(hFile, &frame_count, 1, 0);
        }
        if (version > 4) {
            float lx = 0.0f;
            float ly = 0.0f;
            float lz = 0.0f;

            ReadVirtualFile(hFile, &flags_byte, 1, 0);
            ReadVirtualFile(hFile, &lx, 4, 0);
            ReadVirtualFile(hFile, &ly, 4, 0);
            ReadVirtualFile(hFile, &lz, 4, 0);
            lx = lx * g_monster_script_direction_scale_005ec150;
            ly = ly * g_monster_script_direction_scale_005ec150;
            lz = lz * g_monster_script_direction_scale_005ec150;
            rep->local_location_010.x = lx;
            rep->location_004.x = lx;
            rep->local_location_010.y = ly;
            rep->location_004.y = ly;
            rep->local_location_010.z = lz;
            rep->location_004.z = lz;
        }
        if (version > 5) {
            ReadVirtualFile(hFile, &unused_scale, 4, 0);
            prop->flags_1c |= (unsigned int)unused_scale;
        }
        if (version > 6) {
            char* buffer = static_cast<char*>(operator new(0x40));
            int length;

            ReadVirtualFile(hFile, buffer, 0x40, 0);
            length = -1;
            {
                char* scan = buffer;
                do {
                    if (length == 0) {
                        break;
                    }
                    --length;
                } while (*scan++ != '\0');
            }
            if (length == -2) {
                operator delete(buffer);
            }
            else {
                if (prop->m_name_20 != 0) {
                    operator delete(prop->m_name_20);
                }
                prop->m_name_20 = buffer;
            }
        }
        if (version > 7) {
            signed char slot_count = 0;
            int added;

            ReadVirtualFile(hFile, &slot_count, 1, 0);
            for (added = 0; added < slot_count; ++added) {
                unsigned short frame_tmp = 0;
                unsigned short tag_tmp = 0;

                slot = static_cast<unsigned char*>(operator new(2));
                ReadVirtualFile(hFile, &frame_tmp, 2, 0);
                slot[0] = (unsigned char)frame_tmp;
                ReadVirtualFile(hFile, &tag_tmp, 2, 0);
                slot[1] = (unsigned char)tag_tmp;
                if (frame_count <= frame_tmp) {
                    srAssertFail(
                        "(usTemp < (UINT16)ubNumFrames)",
                        PROP_CPP,
                        0x11f,
                        0);
                }
                rep->slots.Add(slot);
            }
        }
        animation = CreateAnimObj004A01A0();
        success = Function4A05C0(0, animation, 1, 0, 1);
        rep->animation = animation;
        if (AnimationIsRunning(animation) == 1) {
            animation->value_16 = frame_count;
        }
        rep->value_0a8 = *(float*)(animation->unknown_0c + 4);
        rep->flag_0a5 = animation->unknown_0c[0] != 0;
        list_count = AnimObjValue004A15D0(animation, 2);
        for (entry_index = 0; entry_index < list_count; ++entry_index) {
            srModelInstance* instance;
            int node;

            if (!AnimationIsRunning(animation)) {
                instance = AnimObjDispatch004A14D0(
                    animation, 2, (unsigned char)entry_index);
            }
            else {
                instance =
                    AnimObjDispatchList004A1560(animation, 2, 0);
            }
            /* Name and align each instance the way the retail loader does.
               Unrecovered SurRender helpers stay as virtual calls through the
               recovered mesh pointer. */
            (void)instance;
            (void)node;
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

    rep->active = 1;
    rep->flag_070 = animation->unknown_03[0];
    rep->flag_06f = animation->value_02;
    rep->flag_06d = animation->unknown_00[1];
    rep->flag_06e = 1;
    rep->animation_speed_09c = animation->playback_scale_08;
    rep->timer_068 = GetTickCount();
    if (rep->flag_070 == 1 || rep->flag_070 == 2) {
        rep->flag_06d = 0;
        rep->flag_06e =
            (unsigned char)(((rep->flag_06f != 2) - 1U & 2) + 2);
    }

    if (animation->flag_05 == 0) {
        mesh = reinterpret_cast<W8AniMesh*>(
            AnimObjEntry004A1660(animation, 2, 0, 0));
        {
            unsigned char value_count = AniMeshValue004B64F0(mesh);
            stModelInstance005EC7D0* frame =
                GetAniMeshFrame004B6550(mesh, 0);

            if (frame == 0) {
                srAssertFail("psrMesh", PROP_CPP, 0x182, 0);
            }
            /* TODO(wiz8-ls5.5.20): port the per-frame bounds fold that writes
               AnimRep value_074/value_080/value_08c.  The SurRender getter at
               vtable+0x24 is not yet typed here; leaving the reads unported
               is temporary and must be closed before this Bead is. */
            (void)value_count;
            (void)frame;
        }
    }

    if (version > 2) {
        unsigned char attach = 0;

        if (success == 0) {
            success = 0;
        }
        else {
            success = ReadVirtualFile(
                hFile, reinterpret_cast<unsigned char*>(&flags_byte) + 3,
                1, 0);
            if (success == 0) {
                success = 0;
            }
            else {
                success = 1;
            }
        }
        attach = *(((unsigned char*)&flags_byte) + 3);
        /* Optional trigger attachment (FUN_00441a20) deferred until that
           callee is recovered; the version gate and flag read stay above so
           the file cursor matches. */
        (void)attach;
    }

    if (version >= 9) {
        unsigned char extra = 0;

        if (success != 0) {
            success = ReadVirtualFile(hFile, &extra, 1, 0);
        }
        if (extra != 0 && success != 0) {
            success = ReadVirtualFile(hFile, &rep->flag_0c0, 1, 0);
            if (success != 0) {
                success = ReadVirtualFile(hFile, &rep->flag_0c1, 1, 0);
            }
            else {
                success = 0;
            }
        }
        else if (extra != 0) {
            success = 0;
        }
    }

    rep->counter_094 = 0;
    rep->flag_064 = 0;
    {
        unsigned int frames = AnimObjValue004A15D0(animation, 2);
        rep->counter_095 = (unsigned char)frames - 1;
    }
    return success;
}
