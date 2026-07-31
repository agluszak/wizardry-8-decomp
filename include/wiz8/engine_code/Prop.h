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

/* Prop.cpp's m_pRep.  Assertions name the member; the constructor allocates
   0xc4 bytes, runs the AnimRep constructor, then installs the Prop-owned
   animation pointer, speed, and the slot vector at 0xb0.  The secondary
   vtable at 0xb0 is the unsigned-char-pointer growable-vector specialization
   at 0x005EC1D0. */
#pragma pack(push, 1)
class W8PropRep : public W8AnimRep005ED050 {
public:
    W8PropRep();
    W8PropRep(const W8PropRep& other);
    virtual ~W8PropRep() override;
    virtual W8AnimRepBase005EC1D8* Clone() override;

    void ToggleAnimation(int argument);   /* 0x0044BA00 */
    unsigned char SelectSlot0044BA50(unsigned char tag);
    int FindSlotByCurrentTag();           /* 0x0044BAE0 */
    unsigned char AdvanceAnimationSegment();

    W8AnimObj* animation;                /* 0x98 */
    float animation_speed_09c;           /* 0x9c */
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

static_assert(sizeof(W8PropRep) == 0xc4, "W8PropRep_must_be_0xc4");

/* Engine Code\Prop.cpp.  Prop::Prop() calls W8GrObject::W8GrObject and
   allocates operator new(0x90), which proves both the base and the extent.
   m_pRep and m_pTimer are the assertion-backed names; the representation is
   the Prop-owned W8PropRep stored through GrObject's m_pRep slot. */
class W8Prop005EC1E0 : public W8GrObject {
public:
    W8Prop005EC1E0();                    /* 0x0044BC00 */
    virtual ~W8Prop005EC1E0() override;  /* complete destructor 0x0044BEC0 */

    W8PropRep* Rep() const
    {
        return reinterpret_cast<W8PropRep*>(m_pRep);
    }

    void Method44D360(W8World* world);
    void Method44C030();
    void Method44C670();                 /* 0x0044C670 */
    void Method44C830(W8World* world);
    unsigned char GetSetting6C();
    srModelInstance* ToggleRepAnimation(int argument);
    srModelInstance* ToggleRepAnimationDefault();
    int PlayRepAnimation(int arg_2, int arg_3);
    void SetSetting6E(unsigned char value, unsigned char fallback);
    void SetRepActive0044DA80(
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
    char* m_name_20;                     /* 0x20 */
    int unknown_024;                     /* 0x24 */
    W8Timer005EC0A4* m_pTimer;           /* 0x28 */
    srVector3T<float> position_02c;      /* 0x2c: written by Method44C670 */
    GDProp* m_owned_38;                  /* 0x38 */
    srVector3T<float> position_03c;      /* 0x3c */
    float scale_048;                     /* 0x48: constructed as 1.0 */
    unsigned char unknown_04c[0xc];
    float scale_058;                     /* 0x58: constructed as 1.0 */
    unsigned char unknown_05c[0xc];
    float scale_068;                     /* 0x68: constructed as 1.0 */
    float scale_06c;                     /* 0x6c: constructed as 1.0 */
    unsigned char unknown_070[0xc];
    float scale_07c;                     /* 0x7c: constructed as 1.0 */
    unsigned char unknown_080[0xc];
    float scale_08c;                     /* 0x8c: constructed as 1.0 */
};                                       /* 0x90 */

static_assert(sizeof(W8Prop005EC1E0) == 0x90,
              "W8Prop005EC1E0_must_be_0x90");

W8Prop005EC1E0* FindPropByName(W8World* world, const char* name);
unsigned char LoadProp0044AEE0(
    W8ReadLevelInfo* info, W8Prop005EC1E0* prop);
unsigned char CreateAndLoadProp0044BF50(
    W8ReadLevelInfo* info, W8Prop005EC1E0** prop);
