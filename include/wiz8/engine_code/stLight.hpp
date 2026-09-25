#pragma once

#include "surrender/srLight.h"
#include "wiz8/vector.h"

#include <stddef.h>

class W8Prop;
class Trigger;
struct W8PathAI;
extern unsigned int g_light_update_flags_0060bfdc;

class stLightDefinition {
public:
    virtual ~stLightDefinition();
    virtual stLightDefinition* Clone() const = 0;
    virtual bool IsEnabledForSubcycle(unsigned char subcycle) = 0;

    int type_04;
};

static_assert(sizeof(stLightDefinition) == 0x8, "stLightDefinition_size_must_be_0x8");

/* Type 1: a light animated from parameters (flicker chance, colour and
   intensity ranges, period and rate) over a subcycle window. */
// VTABLE: WIZ8 0x005ecdbc
class stParametricLightDefinition : public stLightDefinition {
public:
    stParametricLightDefinition()
    {
        type_04 = 1;
    }
    stParametricLightDefinition(const stParametricLightDefinition& other)
    {
        type_04 = 1;
        flags_08 = other.flags_08;
        flicker_chance_0c = other.flicker_chance_0c;
        color_10 = other.color_10;
        color_to_1c.x = other.color_to_1c.x;
        color_to_1c.y = other.color_to_1c.y;
        color_to_1c.z = other.color_to_1c.z;
        intensity_28 = other.intensity_28;
        intensity_to_2c = other.intensity_to_2c;
        period_30 = other.period_30;
        rate_34 = other.rate_34;
        path_speed_38 = other.path_speed_38;
        subcycle_min_3c = other.subcycle_min_3c;
        subcycle_max_40 = other.subcycle_max_40;
    }

    // FUNCTION: WIZ8 0x004A2140
    virtual stLightDefinition* Clone() const override
    {
        return new stParametricLightDefinition(*this);
    }

    // FUNCTION: WIZ8 0x004A21E0
    virtual bool IsEnabledForSubcycle(unsigned char subcycle) override
    {
        if (subcycle >= subcycle_min_3c && subcycle <= subcycle_max_40) {
            return true;
        }
        return false;
    }

    /* flags_08 bits 0-1 select the update mode (0 oscillating intensity, 1
       flicker, 3 one-way ramp); bit 3 lerps diffuse toward the target color,
       bit 5 ping-pongs the path direction at the ends. */
    unsigned int flags_08;
    /* Per-update flicker probability, compared against rand()/32768. */
    float flicker_chance_0c;
    srVector3T<float> color_10;
    /* The diffuse color the intensity sweep lerps toward under flag bit 3. */
    srVector3T<float> color_to_1c;
    float intensity_28;
    /* The intensity the sweep lerps toward. */
    float intensity_to_2c;
    /* Sweep period divisor; Update clamps it up to 1.0 when below ~0.0001. */
    float period_30;
    /* Elapsed-time multiplier applied to the sweep step. */
    float rate_34;
    float path_speed_38;
    int subcycle_min_3c;
    int subcycle_max_40;
};

static_assert(sizeof(stParametricLightDefinition) == 0x44,
              "stParametricLightDefinition_size_must_be_0x44");
static_assert(offsetof(stParametricLightDefinition, flags_08) == 0x08,
              "stParametricLightDefinition_flags_08");
static_assert(offsetof(stParametricLightDefinition, flicker_chance_0c) == 0x0c,
              "stParametricLightDefinition_flicker_chance_0c");
static_assert(offsetof(stParametricLightDefinition, color_10) == 0x10,
              "stParametricLightDefinition_color_10");
static_assert(offsetof(stParametricLightDefinition, color_to_1c) == 0x1c,
              "stParametricLightDefinition_color_to_1c");
static_assert(offsetof(stParametricLightDefinition, intensity_28) == 0x28,
              "stParametricLightDefinition_intensity_28");
static_assert(offsetof(stParametricLightDefinition, intensity_to_2c) == 0x2c,
              "stParametricLightDefinition_intensity_to_2c");
static_assert(offsetof(stParametricLightDefinition, period_30) == 0x30,
              "stParametricLightDefinition_period_30");
static_assert(offsetof(stParametricLightDefinition, rate_34) == 0x34,
              "stParametricLightDefinition_rate_34");
static_assert(offsetof(stParametricLightDefinition, path_speed_38) == 0x38,
              "stParametricLightDefinition_path_speed_38");
static_assert(offsetof(stParametricLightDefinition, subcycle_min_3c) == 0x3c,
              "stParametricLightDefinition_subcycle_min_3c");
static_assert(offsetof(stParametricLightDefinition, subcycle_max_40) == 0x40,
              "stParametricLightDefinition_subcycle_max_40");

/* Type 2: a light driven by keyframe tables stepped by keyframe_index_48. */
// VTABLE: WIZ8 0x005ecda0
class stKeyframedLightDefinition : public stLightDefinition {
public:
    stKeyframedLightDefinition()
        : values_08(5), values_18(5), values_28(5), values_38(5), keyframe_index_48(0),
          time_4c(0.0f)
    {
        type_04 = 2;
    }
    virtual ~stKeyframedLightDefinition() override;
    virtual stLightDefinition* Clone() const override;
    virtual bool IsEnabledForSubcycle(unsigned char subcycle) override;

    W8GrowableVector<int> values_08;
    W8GrowableVector<int> values_18;
    W8GrowableVector<float> values_28;
    W8GrowableVector<srVector3T<float> > values_38;
    int keyframe_index_48;
    float time_4c;
    int start_frame_50;
    float end_frame_54;
};

static_assert(sizeof(stKeyframedLightDefinition) == 0x58,
              "stKeyframedLightDefinition_size_must_be_0x58");
static_assert(offsetof(stKeyframedLightDefinition, values_08) == 0x08,
              "stKeyframedLightDefinition_values_08");
static_assert(offsetof(stKeyframedLightDefinition, values_18) == 0x18,
              "stKeyframedLightDefinition_values_18");
static_assert(offsetof(stKeyframedLightDefinition, values_28) == 0x28,
              "stKeyframedLightDefinition_values_28");
static_assert(offsetof(stKeyframedLightDefinition, values_38) == 0x38,
              "stKeyframedLightDefinition_values_38");
static_assert(offsetof(stKeyframedLightDefinition, keyframe_index_48) == 0x48,
              "stKeyframedLightDefinition_keyframe_index_48");
static_assert(offsetof(stKeyframedLightDefinition, time_4c) == 0x4c,
              "stKeyframedLightDefinition_time_4c");
static_assert(offsetof(stKeyframedLightDefinition, start_frame_50) == 0x50,
              "stKeyframedLightDefinition_start_frame_50");
static_assert(offsetof(stKeyframedLightDefinition, end_frame_54) == 0x54,
              "stKeyframedLightDefinition_end_frame_54");

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
    static const char* sGetClassName()
    {
        return "stLight";
    }

    stLight() {}
    explicit stLight(srNode* parent);         /* 0x0049C2C0 */
    stLight& operator=(const stLight& other); /* 0x0049C690 */

protected:
    virtual ~stLight() override; /* 0x0049C430 */

public:
    virtual srClass* vInstance() override;                      /* 0x0049E3A0 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x0049C7A0 */
    virtual void process(const srNode::ProcessInfo& info,
                         srNode::e_processType type) override; /* 0x0049C8D0 */
    void Reset0049D070();                                      /* 0x0049D070 */
    void SetDefinitionTime(float time);                        /* 0x0049C940 */
    void Update0049C960();                                     /* 0x0049C960 */

    float positionalX() const
    {
        return m_position_228.x;
    }
    float positionalY() const
    {
        return m_position_228.y;
    }
    float positionalZ() const
    {
        return m_position_228.z;
    }
    stLightDefinition* definition() const
    {
        return m_definition_234;
    }
    void ConfigureMonsterCopy()
    {
        attenuation_model_150 = srLight::ATTENUATION_3DSTUDIO_MAX;
        enable_flags_194 |= 0x10; /* ENABLE_RANGE_FAR */
        enable_flags_194 |= 4;    /* ENABLE_BOUNDING_SPHERE */
    }

public:
    /* One value, not three floats: 0x0049C690 copies it through the base-pointer
       form VC6 emits for a class type's memberwise assignment, not through three
       independent displacement loads. */
    srVector3T<float> m_position_228;    /* 0x228 */
    stLightDefinition* m_definition_234; /* 0x234: owned */
    unsigned char m_padding_238;         /* 0x238 */
    /* Oscillation direction: zero sweeps intensity down, nonzero sweeps up. */
    unsigned char m_direction_239;
    /* Raised by light-toggle triggers; the save path serializes the names of
       lights carrying it so their toggled state persists in savegames. */
    bool m_save_marked_23a; /* 0x23a */
    unsigned char m_padding_23b;
    /* GetTickCount()/1000 timestamp of the last intensity/color update. */
    float m_level_time_23c;
    /* Current 0..1 sweep level driving intensity_1d0 and the color lerp. */
    float m_level_240;
    W8PathAI* m_owned_244; /* 0x244 */
    /* Current path entry index, advanced by m_path_direction_250. */
    int m_path_index_248;
    /* GetTickCount()/1000 timestamp of the last path advance. */
    float m_path_time_24c;
    /* Path step direction, +1 or -1 under the ping-pong flag. */
    int m_path_direction_250;
    W8Prop* m_prop_254; /* 0x254 */
};

static_assert(sizeof(stLight) == 0x258, "stLight_must_be_0x258");

void SaveLightStates(int handle);
void LoadLightStates(int handle);
