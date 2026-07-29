#pragma once

struct GDProp;
struct W8World;
class W8PropOwned0020;
class W8PropOwnedPolymorphic;

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
    bool CanBeUsedFrom(int arg_2, int arg_3, char notify);
    void SetSetting6C(unsigned char value);
    void SetSetting66(char value);
    bool IsSetting6FTwo();
    void ToggleSetting6E();
    int GetValue18();
    int GetGDPropValue24();

    W8PropOwnedPolymorphic* m_owned_14;
    int value_18;
    unsigned int flags_1c;

private:
    W8PropOwned0020* m_owned_20;
    unsigned char unknown_024[0x4];
    W8PropOwnedPolymorphic* m_owned_28;
    unsigned char unknown_02c[0xc];

public:
    GDProp* m_owned_38;
};                                       /* 0x3c */

static_assert(sizeof(W8Prop005EC1E0) == 0x3c,
              "W8Prop005EC1E0_must_be_0x3c");

