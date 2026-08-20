#pragma once

#include "surrender/srLight.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srScene.h"
#include "surrender/srTexture.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/vector.h"

class W8MonsterShakeCallback;
class W8Prop;
class Trigger;
class stTextureAnim;
struct W8PathAI;

/*
 * Classes whose only recovered members so far are the two SurRender class
 * registry slots.
 *
 * Each class is named by the literal its own getClassName slot returns, which
 * is `original-runtime-string` evidence, and each id sits at 0x10000 and up -
 * the range SurRender reserves for classes the host program registers rather
 * than its own. The owning translation unit is corroborated twice over: the
 * registry name matches the unit name, and the body sits inside that unit's
 * assertion-bounded interval.
 *
 * Classes below remain positional until their constructor or vtable supplies
 * the complete inheritance evidence. Closed classes move into their owned
 * headers instead of accumulating here.
 */

/* Engine Code\stParticle.cpp. */
class stParticle
    : public srClassSupport<stParticle, srNode, 0, 0x10009> {
public:
    static const char* sGetClassName() { return "stParticle"; }
    stParticle(srNode* parent, unsigned int count); /* 0x00497AF0 */
    stParticle(const stParticle& other);           /* 0x00498180 */
    void SetActive(unsigned char active);
    void SetTraversalEnabled00498D90(unsigned char enabled);
    void DeactivateParticle00499F70(unsigned int index);
    unsigned char ActivateParticle00499A50(
        unsigned int* out_index, unsigned char replace_when_full);
    void InitializeParticlePosition0049A990(srVector3T<float>* output);
    void SetTexture0049AB00(srTextureIFace* texture);
    void SetRetainedObject0049ACA0(srMaterialIFace* material);
    void SetRenderFlags004925A0(srShader flags);
    void SetFlutter0049AD10(int enabled);
    void Function4994D0(srGERD* renderer);
    /* The per-particle age/cull/move step and billboard-corner expansion used
       by the submitted batch. Their retail names remain unavailable. */
    void Update00499FA0();                          /* 0x00499FA0 */
    void PrepareRenderer00498DD0(srMatrix4T<float>& view); /* 0x00498DD0 */
    srShader GetRenderFlags00498A10() const;
    unsigned char ReplaceTexture0049AC30(
        const char* old_name, srTextureIFace* replacement);
    virtual srClass* vInstance() override;         /* 0x004980E0 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x00498C40 */
    virtual void process(
        const srNode::ProcessInfo& info, srNode::e_processType type) override; /* 0x00498D60 */

protected:
    virtual ~stParticle() override;                /* 0x00498A20 */

public:

    unsigned int value_138;
    unsigned char unknown_13c[4];
    double value_140;
    srVector3T<float>* allocation_148;
    srMaterialIFace* retained_14c;
    srShader render_flags_150;
    srTextureIFace* texture_154;
    unsigned int vertex_count_158;
    unsigned int texture_frame_count_15c;
    srVector3T<float>* allocation_160;
    srVector2T<float>* allocation_164;
    srVector3i* allocation_168;
    void* allocation_16c;
    void* allocation_170;
    float* allocation_174;
    stTextureAnim** texture_frames_178;
    float* m_pflFlutterAngle;                     /* 0x17c */
    unsigned int particle_count_180;
    /* Both unsigned: 0x004994D0 gates the particle off with the unsigned
       `state_184 != 0 && state_184 <= value_188` pair. */
    unsigned int state_184;
    unsigned int value_188;
    unsigned int active_particle_count_18c;
    unsigned char active_190;
    unsigned char unknown_191;
    unsigned char trigger_flag_192;
    unsigned char unknown_193;
    unsigned char* allocation_194;
    srVector3T<float>* allocation_198;
    /* Unsigned millisecond birth ticks, one per particle. */
    unsigned int* allocation_19c;
    unsigned char active_1a0;
    unsigned char flag_1a1;
    unsigned char unknown_1a2[2];
    int value_1a4;
    int value_1a8;
    int value_1ac;
    int value_1b0;
    int value_1b4;
    int value_1b8;
    int value_1bc;
    int value_1c0;
    int value_1c4;
    /* Emission interval; elapsed comparisons use unsigned subtraction. */
    unsigned int value_1c8;
    /* Lifetime added to each absolute unsigned birth tick. */
    unsigned int value_1cc;
    srVector3T<float> minimum_1d0;
    srVector3T<float> maximum_1dc;
    srVector3T<float> direction_1e8;
    srVector3T<float> acceleration_1f4;
    float value_200;
    /* 0x00498DD0 uses this as an unsigned modulus period. */
    unsigned int value_204;
    float value_208;
    float value_20c;
    float value_210;
    float value_214;
    float value_218;
    srVector3T<float> minimum_21c;
    srVector3T<float> maximum_228;
    srVector3T<float> value_234;
    float value_240;
    /* Added to the camera position when value_1c4 selects camera-relative
       placement. */
    srVector3T<float> camera_offset_244;
    unsigned int update_flags_250;
    /* Index pairs, two per still-active particle, rebuilt whenever
       update_flags_250 carries bit 1. */
    unsigned long* allocation_254;
    /* Last accepted particle-integration tick. */
    unsigned int activated_at_258;
    /* Emission schedule tick. */
    unsigned int updated_at_25c;
    short value_260;
    unsigned char unknown_262[2];
    int start_frame_264;
    int end_frame_268;
    W8MonsterShakeCallback* callback_26c;
    unsigned int value_270;
    unsigned int value_274;
    float value_278;
    unsigned char unknown_27c[4];
};

static_assert(sizeof(stParticle) == 0x280, "stParticle_size_must_be_0x280");

stParticle* FindRegisteredParticle0049ADB0(const char* name);
void SaveParticleStates0049B150(unsigned int handle);
void LoadParticleStates0049B3B0(int handle);

/*
 * The same slots for further host-registered classes whose owning translation
 * unit is not established. Their ids are in the 0x10000 range, so each names a
 * class the program registers itself rather than a SurRender base it is
 * presenting as. They remain together as the explicit closed registry-method
 * family in engine_code/registry_classes.cpp until original TU ownership is proved.
 */

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
    W8Prop* m_prop_254;                     /* 0x254 */
};

static_assert(sizeof(stLight) == 0x258, "stLight_must_be_0x258");


/*
 * Classes that present themselves to the scene graph under a SurRender base
 * name instead of one of their own.
 *
 * Their ids sit in SurRender's own 0x1000-0x3110 range rather than the host
 * range above, so the literal each returns names the *base* and not the class.
 * That is why these carry vtable-qualified names: the vtable is the only thing
 * that identifies them, and borrowing the base's name would assert an identity
 * the registry pair explicitly does not establish. The reviewed model reaches
 * the same conclusion for MonsterLight, whose row records that it "presents
 * itself to the SurRender scene graph as an srLight variant rather than under a
 * Wizardry name".
 */
