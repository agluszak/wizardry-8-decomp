#pragma once

#include "surrender/srLight.h"
#include "wiz8/vector.h"

class W8Prop;
class Trigger;
struct W8PathAI;

class stLightDefinition {
public:
    virtual ~stLightDefinition();
    virtual stLightDefinition* Clone() const = 0;
    virtual unsigned char IsEnabledForSubcycle(unsigned char subcycle) = 0;

    int type_04;
};

static_assert(sizeof(stLightDefinition) == 0x8,
              "stLightDefinition_size_must_be_0x8");

// VTABLE: WIZ8 0x005ecdbc
class stLightDefinition005ECDBC : public stLightDefinition {
public:
    stLightDefinition005ECDBC() { type_04 = 1; }
    stLightDefinition005ECDBC(
        const stLightDefinition005ECDBC& other)
    {
        type_04 = 1;
        flags_08 = other.flags_08;
        value_0c = other.value_0c;
        color_10.x = other.color_10.x;
        color_10.y = other.color_10.y;
        color_10.z = other.color_10.z;
        value_1c = other.value_1c;
        value_20 = other.value_20;
        value_24 = other.value_24;
        intensity_28 = other.intensity_28;
        value_2c = other.value_2c;
        value_30 = other.value_30;
        value_34 = other.value_34;
        path_value_38 = other.path_value_38;
        value_3c = other.value_3c;
        value_40 = other.value_40;
    }

    // FUNCTION: WIZ8 0x004A2140
    virtual stLightDefinition* Clone() const override
    {
        return new stLightDefinition005ECDBC(*this);
    }

    // FUNCTION: WIZ8 0x004A21E0
    virtual unsigned char IsEnabledForSubcycle(
        unsigned char subcycle) override
    {
        if (subcycle >= value_3c && subcycle <= value_40) {
            return 1;
        }
        return 0;
    }

    unsigned int flags_08;
    unsigned int value_0c;
    srVector3T<float> color_10;
    unsigned int value_1c;
    unsigned int value_20;
    unsigned int value_24;
    float intensity_28;
    float value_2c;
    unsigned int value_30;
    unsigned int value_34;
    float path_value_38;
    int value_3c;
    int value_40;
};

static_assert(sizeof(stLightDefinition005ECDBC) == 0x44,
              "stLightDefinition005ECDBC_size_must_be_0x44");

// VTABLE: WIZ8 0x005ecda0
class stLightDefinition005ECDA0 : public stLightDefinition {
public:
    stLightDefinition005ECDA0()
        : values_08(5), values_18(5), values_28(5), values_38(5),
          value_48(0), time_4c(0.0f)
    {
        type_04 = 2;
    }
    virtual ~stLightDefinition005ECDA0() override;
    virtual stLightDefinition* Clone() const override;
    virtual unsigned char IsEnabledForSubcycle(
        unsigned char subcycle) override;

    W8GrowableVector<int> values_08;
    W8GrowableVector<int> values_18;
    W8GrowableVector<float> values_28;
    W8GrowableVector<srVector3T<float> > values_38;
    int value_48;
    float time_4c;
    int value_50;
    float value_54;
};

static_assert(sizeof(stLightDefinition005ECDA0) == 0x58,
              "stLightDefinition005ECDA0_size_must_be_0x58");

/*
 * stLight owns the 0x10006 registry identity, so the class that supplies it -
 * and the registerInstance/unregisterInstance pair that identity implies - is
 * srClassSupport rather than srLight itself. Both lifecycle bodies prove the
 * intermediate base directly: 0x0049C2C0 writes 0x005ECCA4/0x005ECC98 between
 * the srLight constructor and the 0x10006 registration and only then installs
 * stLight's own pair, and 0x0049C430 unwinds through the same two levels with
 * separate EH states.
 *
 * The default constructor stays implicit-shaped: the `new stLight` site inlined
 * at 0x004BF329 is nothing but a call to the base emission at 0x004CA8B0
 * followed by the two vptr stores, with no member initialisation at all.
 */
// VTABLE: WIZ8 0x005ecc64 stLight
// VTABLE: WIZ8 0x005ecc58 srVertexProcessor
class stLight : public srClassSupport<stLight, srLight, false, 0x10006> {
    friend class W8GrCycle;
    friend class Trigger;

public:
    static const char* sGetClassName() { return "stLight"; }

    stLight() {}
    explicit stLight(srNode* parent);                /* 0x0049C2C0 */
    stLight& operator=(const stLight& other);         /* 0x0049C690 */

protected:
    virtual ~stLight() override;                    /* 0x0049C430 */

public:
    virtual srClass* vInstance() override;          /* 0x0049E3A0 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x0049C7A0 */
    virtual void process(
        const srNode::ProcessInfo& info,
        srNode::e_processType type) override;       /* 0x0049C8D0 */
    void Reset0049D070();                           /* 0x0049D070 */
    void SetDefinitionTime0049C940(float time);      /* 0x0049C940 */
    void Update0049C960();                           /* 0x0049C960 */

    float positionalX() const { return m_positional_228.x; }
    float positionalY() const { return m_positional_228.y; }
    float positionalZ() const { return m_positional_228.z; }
    stLightDefinition* definition() const { return m_definition_234; }
    void ConfigureMonsterCopy()
    {
        m_positional_18 = 2;
        m_positional_flags_5c |= 0x10;
        m_positional_flags_5c |= 4;
    }

public:
    /* One value, not three floats: 0x0049C690 copies it through the base-pointer
       form VC6 emits for a class type's memberwise assignment, not through three
       independent displacement loads. */
    srVector3T<float> m_positional_228;             /* 0x228 */
    stLightDefinition* m_definition_234;            /* 0x234: owned */
    unsigned char m_positional_238;                 /* 0x238 */
    unsigned char m_positional_239;                 /* 0x239 */
    unsigned char m_positional_23a;                 /* 0x23a */
    unsigned char m_padding_23b;
    float m_positional_23c;                         /* 0x23c */
    unsigned long m_positional_240;                 /* 0x240 */
    W8PathAI* m_owned_244;                          /* 0x244 */
    unsigned long m_positional_248;                 /* 0x248 */
    float m_positional_24c;                         /* 0x24c */
    unsigned long m_positional_250;                 /* 0x250 */
    W8Prop* m_prop_254;                             /* 0x254 */
};

static_assert(sizeof(stLight) == 0x258, "stLight_must_be_0x258");
