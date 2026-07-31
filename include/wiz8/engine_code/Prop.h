#pragma once

#include "surrender/srMath.h"

struct GDProp;
struct W8AnimObj;
struct W8ReadLevelInfo;
struct W8World;
class srModelInstance;
class stModelInstance005EC7D0;
class W8GameTimer;
template <class T> class W8GrowableVector;

/* Prop.cpp owns this animation/state object through a virtual destructor.
   Trigger.cpp also reads active to decide whether a prop-backed trigger
   can run, so the proven layout belongs in the shared Prop declaration rather
   than in one consumer's translation unit. */
class W8PropRepresentation {
public:
    virtual ~W8PropRepresentation();

    void ToggleAnimation(int argument);   /* 0x0044BA00 */
    unsigned char SelectAnimationSlot(unsigned char tag);
    int FindCurrentAnimationSlot();           /* 0x0044BAE0 */
    unsigned char AdvanceAnimationSegment();

    unsigned char unknown_004[0x60];
    unsigned char default_animation_tag;
    unsigned char unknown_065;
    short setting_66;
    unsigned char unknown_068[4];
    unsigned char setting_6c;
    unsigned char active;
    unsigned char setting_6e;
    unsigned char setting_6f;
    unsigned char unknown_070[0x24];
    unsigned char current_tag;
    unsigned char unknown_095[3];
    W8AnimObj* animation;
    float animation_speed;
    unsigned char unknown_0a0[0x14];
    int slot_count;
    unsigned char unknown_0b8[4];
    unsigned char** slots;
};

static_assert(sizeof(W8PropRepresentation) == 0xc0,
              "W8PropOwnedPolymorphic_must_be_0xc0");

class W8PropBase004B6B60 {
public:
    virtual ~W8PropBase004B6B60();       /* 0x004B6B60 */

protected:
    unsigned char unknown_004[0x10];
};                                       /* 0x14 */

class W8Prop : public W8PropBase004B6B60 {
public:
    virtual ~W8Prop() override;  /* complete destructor 0x0044BEC0 */

    void Method44D360(W8World* world);
    void Method44C030();
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

    W8PropRepresentation* m_pRep;
    int value_18;
    unsigned int flags_1c;

    char* m_name;
    unsigned char unknown_024[0x4];
    W8GameTimer* m_animation_timer;
    unsigned char unknown_02c[0xc];
    GDProp* m_gd_prop;
};                                       /* 0x3c */

static_assert(sizeof(W8Prop) == 0x3c,
              "W8Prop_must_be_0x3c");

W8Prop* FindPropByName(W8World* world, const char* name);
unsigned char CreateAndLoadProp0044BF50(
    W8ReadLevelInfo* info, W8Prop** prop);
