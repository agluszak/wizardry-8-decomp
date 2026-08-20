#pragma once

#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/vector.h"

#include "surrender/srMath.h"

struct GDProp;
struct W8AnimObj;
struct W8ReadLevelInfo;
struct W8World;
class srModelInstance;
class stModelInstance005EC7D0;
class W8Prop;

/* Prop.cpp's m_pRep.  Assertions name the member; the constructor allocates
   0xc4 bytes, runs the AnimRep constructor, then installs the Prop-owned
   animation pointer, speed, and the slot vector at 0xb0.  The secondary
   vtable at 0xb0 is the unsigned-char-pointer growable-vector specialization
   at 0x005EC1D0. */
#pragma pack(push, 1)
class W8PropRepresentation : public W8AnimRep005ED050 {
public:
    /* Default construction is inlined at Prop::Prop. */
    W8PropRepresentation()
        : animation(0),
          animation_speed(0.0f),
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
    W8PropRepresentation(const W8PropRepresentation& other);
    virtual ~W8PropRepresentation() override;
    virtual W8AnimRepBase005EC1D8* Clone() override;

    void ToggleAnimation(int argument);   /* 0x0044BA00 */
    unsigned char SelectAnimationSlot(unsigned char tag);
    int FindCurrentAnimationSlot();           /* 0x0044BAE0 */
    unsigned char AdvanceAnimationSegment();
    /* CreateAndLoadProp loads m_pRep into ECX, then passes (pInfo, pProp). */
    unsigned char LoadProp0044AEE0(
        W8ReadLevelInfo* info, W8Prop* prop); /* 0x0044AEE0 */

    W8AnimObj* animation;                /* 0x98 */
    float animation_speed;           /* 0x9c */
    float value_0a0;                     /* 0xa0 */
    unsigned char flag_0a4;              /* 0xa4 */
    unsigned char flag_0a5;              /* 0xa5 */
    unsigned char unknown_0a6[2];
    float value_0a8;                     /* 0xa8: constructed as 0.5 */
    unsigned char flag_0ac;              /* 0xac */
    unsigned char flag_0ad;              /* 0xad */
    unsigned char unknown_0ae[2];
    W8GrowableVector<unsigned char*> slots; /* 0xb0 */
    unsigned char flag_0c0;              /* 0xc0 */
    unsigned char flag_0c1;              /* 0xc1 */
    unsigned char unknown_0c2[2];
};                                       /* 0xc4 */
#pragma pack(pop)

static_assert(sizeof(W8PropRepresentation) == 0xc4, "W8PropRepresentation_must_be_0xc4");

/* Engine Code\Prop.cpp.  Prop::Prop() calls W8GrObject::W8GrObject and
   allocates operator new(0x90), which proves both the base and the extent.
   m_pRep and m_animation_timer are the assertion-backed names; the representation is
   the Prop-owned W8PropRepresentation stored through GrObject's m_pRep slot. */
class W8Prop : public W8GrObject {
public:
    W8Prop();                    /* 0x0044BC00 */
    virtual ~W8Prop() override;  /* complete destructor 0x0044BEC0 */

    W8PropRepresentation* Rep() const
    {
        return reinterpret_cast<W8PropRepresentation*>(m_pRep);
    }

    void Method44D360(W8World* world);
    void Method44C030();
    void Method44C670();                 /* 0x0044C670 */
    int Function44DEA0();                /* 0x0044DEA0 */
    void Method44C830(W8World* world);
    unsigned char GetSetting6C();
    srModelInstance* ToggleRepAnimation(int argument);
    srModelInstance* ToggleRepAnimationDefault();
    int PlayRepAnimation(int arg_2, int arg_3);
    void SetSetting6E(unsigned char value, unsigned char fallback);
    void SetRepresentationActive(
        unsigned char active, unsigned char update_animation);
    bool CanBeUsedFrom(int arg_2, int arg_3, char notify);
    void SetSetting6C(unsigned char value);
    void SetSetting66(char value);
    void SetAnimationSpeed(float speed);
    bool IsSetting6FTwo();
    void ToggleSetting6E();
    int GetValue18();
    int GetGDPropValue24();
    void GetCenterPosition(srVector3T<float>* position);
    void GetBounds0044DD60(
        srVector3T<float>* minimum, srVector3T<float>* maximum);
    void CollectModelInstances0044E570(
        W8GrowableVector<stModelInstance005EC7D0*>* instances);

    int value_18;                        /* 0x18 */
    unsigned int flags_1c;               /* 0x1c */
    char* m_name;                     /* 0x20 */
    int unknown_024;                     /* 0x24 */
    W8GameTimer* m_animation_timer;           /* 0x28 */
    srVector3T<float> position_02c;      /* 0x2c: written by Method44C670 */
    GDProp* m_gd_prop;                  /* 0x38 */
    srVector3T<float> position_03c;      /* 0x3c */
    /* Prop::Prop writes two identity bases here as nine floats each. */
    srMatrix3T<float> rotation_048;      /* 0x48 */
    srMatrix3T<float> rotation_06c;      /* 0x6c */
};                                       /* 0x90 */

static_assert(sizeof(W8Prop) == 0x90,
              "W8Prop_must_be_0x90");

W8Prop* FindPropByName(W8World* world, const char* name);
unsigned char CreateAndLoadProp0044BF50(
    W8ReadLevelInfo* info, W8Prop** prop);
