#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/sr_api.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/3d_code/PList.h"

/* Engine Code\Prop.cpp. The complete destructor at 0x0044BEC0 releases four
   owned members, and each release names the shape of what it owns:

     +0x14  delete through vtable slot 0 with the deleting flag - a class with
            a virtual destructor
     +0x20  a null check and a bare operator delete - a pointer to something
            with no destructor at all
     +0x28  the same virtual-destructor shape as +0x14
     +0x38  its destructor called directly and then operator delete - a class
            with a non-virtual destructor

   Nothing names the class or its members, so all keep positional names, and
   the gaps between the four members stay opaque. */

/* Owned through a virtual destructor; two members share this shape and the
   image does not say whether they share a type. */
class W8PropOwnedPolymorphic {
public:
    virtual ~W8PropOwnedPolymorphic();

    void ToggleAnimation(int argument);   /* 0x0044BA00 */
    int FindSlotByCurrentTag();           /* 0x0044BAE0 */

    unsigned char unknown_004[0x60];
    /* 0x64: the argument the prop's own animation calls pass along. */
    unsigned char setting_64;
    unsigned char unknown_065;
    /* 0x66..0x6f: four settings the prop reads and writes through this member.
       0x6e cycles between one and three; 0x6f having the value two is a state
       one predicate singles out. */
    short setting_66;                     /* 0x66 */
    unsigned char unknown_068[4];
    unsigned char setting_6c;             /* 0x6c */
    unsigned char unknown_06d;
    unsigned char setting_6e;             /* 0x6e */
    unsigned char setting_6f;             /* 0x6f */
    unsigned char unknown_070[0x24];
    /* 0x94: which of the slots below is current, matched against each slot's
       own leading byte rather than used as an index. */
    unsigned char current_tag;
    unsigned char unknown_095[3];
    /* 0x98: the animation the prop drives. */
    W8AnimObj* animation;
    unsigned char unknown_09c[0x18];
    /* 0xb4 and 0xbc: the slot table, count first and data three dwords along -
       the shared growable vector's shape. */
    int slot_count;
    unsigned char unknown_0b8[4];
    unsigned char** slots;
};

/* Released with a null check and then a bare operator delete. A pointer to
   something trivially destructible skips the check entirely, so the check
   without a destructor call is what a declared-but-empty destructor emits. */
class W8PropOwned0020 {
public:
    ~W8PropOwned0020();
};

class W8PropBase004B6B60 {
public:
    virtual ~W8PropBase004B6B60();       /* 0x004B6B60 */

protected:
    unsigned char unknown_004[0x10];
};                                       /* 0x14 */

class W8Prop005EC1E0 : public W8PropBase004B6B60 {
public:
    virtual ~W8Prop005EC1E0() override;           /* 0x0044BEC0 */

    unsigned char GetSetting6C();        /* 0x0044D4F0 */
    void ToggleRepAnimation(int argument);   /* 0x0044D500 */
    void ToggleRepAnimationDefault();        /* 0x0044D550 */
    int PlayRepAnimation(int arg_2, int arg_3);  /* 0x0044D5C0 */
    void SetSetting6E(unsigned char value, unsigned char fallback);  /* 0x0044E1F0 */
    bool CanBeUsedFrom(int arg_2, int arg_3, char notify);  /* 0x0044E0C0 */
    void SetSetting6C(unsigned char value);  /* 0x0044D4B0 */
    void SetSetting66(char value);       /* 0x0044D5B0 */
    bool IsSetting6FTwo();               /* 0x0044E1C0 */
    void ToggleSetting6E();              /* 0x0044E1D0 */
    int GetValue18();                    /* 0x0044D5A0 */
    int GetGDPropValue24();              /* 0x0044E0A0 */

private:
public:
    W8PropOwnedPolymorphic* m_owned_14;  /* 0x14 */
    int value_18;                        /* 0x18 */
    /* 0x1c: bit seven has to be up before the owned GDProp is consulted. */
    unsigned int flags_1c;
private:
    W8PropOwned0020* m_owned_20;         /* 0x20 */
    unsigned char unknown_024[0x4];
    W8PropOwnedPolymorphic* m_owned_28;  /* 0x28 */
    unsigned char unknown_02c[0xc];
public:
    GDProp* m_owned_38;                  /* 0x38 */
};                                       /* 0x3c established */

extern int Function443830(W8World* world, W8Prop005EC1E0* prop);
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
        W8Prop005EC1E0* prop = static_cast<W8Prop005EC1E0*>(PListGetAt(world->plsProps, index));
        int value;

        if (prop != 0 && (value = Function443830(world, prop)) != 0 && prop->m_owned_38 != 0) {
            prop->flags_1c |= 0x80;
            Function4B7470(value);
        }
    }
}

__forceinline W8PropOwned0020::~W8PropOwned0020()
{
}

// FUNCTION: WIZ8 0x0044bec0
W8Prop005EC1E0::~W8Prop005EC1E0()
{
    delete m_owned_14;
    delete m_owned_20;
    delete m_owned_28;
    delete m_owned_38;
}

extern void Function439D80(void);

/* Vtable slot zero: the complete destructor followed by the conditional
   release the deleting flag selects. */
// FUNCTION: WIZ8 0x0044bea0
void* __fastcall W8Prop005EC1E0_ScalarDeletingDestructor(
    W8Prop005EC1E0* self, int /* unused edx */, unsigned char flags)
{
    self->~W8Prop005EC1E0();
    if ((flags & 1) != 0) {
        operator delete(self);
    }
    return self;
}

/* Four accessors reaching through the owned member at 0x14. */
// FUNCTION: WIZ8 0x0044d4f0
unsigned char W8Prop005EC1E0::GetSetting6C()
{
    return this->m_owned_14->setting_6c;
}

// FUNCTION: WIZ8 0x0044d5b0
void W8Prop005EC1E0::SetSetting66(char value)
{
    this->m_owned_14->setting_66 = value;
}

/* Whether the owned member is in the state the value two stands for. */
// FUNCTION: WIZ8 0x0044e1c0
bool W8Prop005EC1E0::IsSetting6FTwo()
{
    return this->m_owned_14->setting_6f == 2;
}

/* Flip the owned member between the only two values it takes: one and three. */
// FUNCTION: WIZ8 0x0044e1d0
void W8Prop005EC1E0::ToggleSetting6E()
{
    this->m_owned_14->setting_6e = this->m_owned_14->setting_6e == 1 ? 3 : 1;
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
    if (m_owned_14->setting_6c == 0) {
        Function439D80();
    }
    m_owned_14->setting_6c = value;
}

extern void AnimationStart(W8AnimObj* animation, int channel, int argument); /* 0x004A14D0 */
extern void AnimationStop(W8AnimObj* animation, int channel, int argument);  /* 0x004A1560 */
extern void AnimationPlayFromTo(
    W8AnimObj* animation, int channel, unsigned char argument, int from, int to); /* 0x004A1710 */
extern unsigned char Function4B75F0(int arg_1, int arg_2);
extern void Function444750(void);

/* Start or stop the prop's own animation, whichever it is not doing. */
// FUNCTION: WIZ8 0x0044ba00
void W8PropOwnedPolymorphic::ToggleAnimation(int argument)
{
    if (AnimationIsRunning(animation)) {
        AnimationStop(animation, 2, 0);
        return;
    }
    AnimationStart(animation, 2, argument);
}

/* Which slot carries the current tag. The tag is matched against each slot's
   own leading byte rather than used as an index, so the slots need not be in
   tag order. */
// FUNCTION: WIZ8 0x0044bae0
int W8PropOwnedPolymorphic::FindSlotByCurrentTag()
{
    int index;

    for (index = 0; index < slot_count; ++index) {
        if ((int)(char)*slots[index] == (unsigned int)current_tag) {
            return index;
        }
    }
    return -1;
}

/* The same toggle reached through the prop rather than through the member. */
// FUNCTION: WIZ8 0x0044d500
void W8Prop005EC1E0::ToggleRepAnimation(int argument)
{
    if (AnimationIsRunning(m_owned_14->animation)) {
        AnimationStop(m_owned_14->animation, 2, 0);
        return;
    }
    AnimationStart(m_owned_14->animation, 2, argument);
}

/* And again with the member's own stored argument instead of a caller's -
   which is what makes 0x64 the animation's default argument. */
// FUNCTION: WIZ8 0x0044d550
void W8Prop005EC1E0::ToggleRepAnimationDefault()
{
    unsigned char argument = m_owned_14->setting_64;

    if (AnimationIsRunning(m_owned_14->animation)) {
        AnimationStop(m_owned_14->animation, 2, 0);
        return;
    }
    AnimationStart(m_owned_14->animation, 2, argument);
}

/* Play the prop's animation between two points, with the same default
   argument. */
// FUNCTION: WIZ8 0x0044d5c0
int W8Prop005EC1E0::PlayRepAnimation(int from, int to)
{
    AnimationPlayFromTo(m_owned_14->animation, 2, m_owned_14->setting_64, from, to);
    return 1;
}

/* Write the member's setting at 0x6e. The assertion names the member m_pRep,
   and the guarded store is written after the assertion rather than instead of
   it, so a null member writes through null on a build with assertions off. */
// FUNCTION: WIZ8 0x0044e1f0
void W8Prop005EC1E0::SetSetting6E(unsigned char value, unsigned char fallback)
{
    if (m_owned_14 == 0) {
        srAssertFail("m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Prop.cpp", 2698, 0);
        m_owned_14->setting_6e = fallback;
        return;
    }
    m_owned_14->setting_6e = value;
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
