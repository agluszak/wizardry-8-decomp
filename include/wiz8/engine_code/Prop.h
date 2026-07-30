#pragma once

#include "surrender/srMath.h"

struct GDProp;
struct W8AnimObj;
struct W8World;
class W8Timer005EC0A4;

/* Prop.cpp owns this animation/state object through a virtual destructor.
   Trigger.cpp also reads unknown_06d to decide whether a prop-backed trigger
   can run, so the proven layout belongs in the shared Prop declaration rather
   than in one consumer's translation unit. */
class W8PropOwnedPolymorphic {
public:
    virtual ~W8PropOwnedPolymorphic();

    void ToggleAnimation(int argument);   /* 0x0044BA00 */
    unsigned char SelectSlot0044BA50(unsigned char tag);
    int FindSlotByCurrentTag();           /* 0x0044BAE0 */
    unsigned char AdvanceAnimationSegment();

    unsigned char unknown_004[0x60];
    unsigned char setting_64;
    unsigned char unknown_065;
    short setting_66;
    unsigned char unknown_068[4];
    unsigned char setting_6c;
    unsigned char unknown_06d;
    unsigned char setting_6e;
    unsigned char setting_6f;
    unsigned char unknown_070[0x24];
    unsigned char current_tag;
    unsigned char unknown_095[3];
    W8AnimObj* animation;
    float animation_speed_09c;
    unsigned char unknown_0a0[0x14];
    int slot_count;
    unsigned char unknown_0b8[4];
    unsigned char** slots;
};

static_assert(sizeof(W8PropOwnedPolymorphic) == 0xc0,
              "W8PropOwnedPolymorphic_must_be_0xc0");

class W8PropBase004B6B60 {
public:
    virtual ~W8PropBase004B6B60();       /* 0x004B6B60 */

protected:
    unsigned char unknown_004[0x10];
};                                       /* 0x14 */

class W8Prop005EC1E0 : public W8PropBase004B6B60 {
public:
    virtual ~W8Prop005EC1E0() override;  /* complete destructor 0x0044BEC0 */

    void Method44D360(W8World* world);
    void Method44C030();
    void Method44C830(W8World* world);
    unsigned char GetSetting6C();
    void ToggleRepAnimation(int argument);
    void ToggleRepAnimationDefault();
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

    W8PropOwnedPolymorphic* m_owned_14;
    int value_18;
    unsigned int flags_1c;

    char* m_name_20;
    unsigned char unknown_024[0x4];
    W8Timer005EC0A4* m_owned_28;
    unsigned char unknown_02c[0xc];
    GDProp* m_owned_38;
};                                       /* 0x3c */

static_assert(sizeof(W8Prop005EC1E0) == 0x3c,
              "W8Prop005EC1E0_must_be_0x3c");

W8Prop005EC1E0* FindPropByName(W8World* world, const char* name);
