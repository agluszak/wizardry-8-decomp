#pragma once

#include "surrender/srLight.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

#include <stddef.h>

class W8Prop;
class Trigger;
struct W8PathAI;
extern unsigned int g_light_update_flags;

enum W8LightDefinitionKind {
    W8_LIGHT_DEFINITION_NONE = 0,
    W8_LIGHT_DEFINITION_PARAMETRIC = 1,
    W8_LIGHT_DEFINITION_KEYFRAMED = 2
};
static_assert(sizeof(W8LightDefinitionKind) == 4, "W8LightDefinitionKind_size");

class stLightDefinition {
public:
    stLightDefinition() : kind(W8_LIGHT_DEFINITION_NONE) {}
    virtual ~stLightDefinition();
    virtual stLightDefinition* Clone() const = 0;
    virtual bool IsEnabledForSubcycle(unsigned char subcycle) = 0;

    W8LightDefinitionKind kind;
};

static_assert(sizeof(stLightDefinition) == 0x8, "stLightDefinition_size_must_be_0x8");

/* Type 1: a light animated from parameters (flicker chance, colour and
   intensity ranges, period and rate) over a subcycle window. */
// VTABLE: WIZ8 0x005ecdbc
class stParametricLightDefinition : public stLightDefinition {
public:
    stParametricLightDefinition()
    {
        kind = W8_LIGHT_DEFINITION_PARAMETRIC;
    }
    // FUNCTION: WIZ8 0x004A2140
    virtual stLightDefinition* Clone() const override
    {
        stParametricLightDefinition* copy = new stParametricLightDefinition;
        if (copy == 0) {
            srAssertFail("pNew", "..\\Engine Code\\Include\\stLight.hpp", 0x56, 0);
        }
        copy->flags = flags;
        copy->flicker_chance = flicker_chance;
        copy->color = color;
        copy->color_to = color_to;
        copy->intensity = intensity;
        copy->intensity_to = intensity_to;
        copy->period = period;
        copy->rate = rate;
        copy->path_speed = path_speed;
        copy->subcycle_min = subcycle_min;
        copy->subcycle_max = subcycle_max;
        return copy;
    }

    // FUNCTION: WIZ8 0x004A21E0
    virtual bool IsEnabledForSubcycle(unsigned char subcycle) override
    {
        if (subcycle >= subcycle_min && subcycle <= subcycle_max) {
            return true;
        }
        return false;
    }

    void GetInterpolatedColor(float blend, srVector3T<float>* out) const;

    /* flags bits 0-1 select the update mode (0 oscillating intensity, 1
       flicker, 3 one-way ramp); bit 3 lerps diffuse toward the target color,
       bit 5 ping-pongs the path direction at the ends. */
    unsigned int flags;
    /* Per-update flicker probability, compared against rand()/32768. */
    float flicker_chance;
    srVector3T<float> color;
    /* The diffuse color the intensity sweep lerps toward under flag bit 3. */
    srVector3T<float> color_to;
    float intensity;
    /* The intensity the sweep lerps toward. */
    float intensity_to;
    /* Sweep period divisor; Update clamps it up to 1.0 when below ~0.0001. */
    float period;
    /* Elapsed-time multiplier applied to the sweep step. */
    float rate;
    float path_speed;
    int subcycle_min;
    int subcycle_max;
};

static_assert(sizeof(stParametricLightDefinition) == 0x44,
              "stParametricLightDefinition_size_must_be_0x44");
static_assert(offsetof(stParametricLightDefinition, flags) == 0x08,
              "stParametricLightDefinition_flags");
static_assert(offsetof(stParametricLightDefinition, flicker_chance) == 0x0c,
              "stParametricLightDefinition_flicker_chance");
static_assert(offsetof(stParametricLightDefinition, color) == 0x10,
              "stParametricLightDefinition_color");
static_assert(offsetof(stParametricLightDefinition, color_to) == 0x1c,
              "stParametricLightDefinition_color_to");
static_assert(offsetof(stParametricLightDefinition, intensity) == 0x28,
              "stParametricLightDefinition_intensity");
static_assert(offsetof(stParametricLightDefinition, intensity_to) == 0x2c,
              "stParametricLightDefinition_intensity_to");
static_assert(offsetof(stParametricLightDefinition, period) == 0x30,
              "stParametricLightDefinition_period");
static_assert(offsetof(stParametricLightDefinition, rate) == 0x34,
              "stParametricLightDefinition_rate");
static_assert(offsetof(stParametricLightDefinition, path_speed) == 0x38,
              "stParametricLightDefinition_path_speed");
static_assert(offsetof(stParametricLightDefinition, subcycle_min) == 0x3c,
              "stParametricLightDefinition_subcycle_min");
static_assert(offsetof(stParametricLightDefinition, subcycle_max) == 0x40,
              "stParametricLightDefinition_subcycle_max");

/* Type 2: a light driven by keyframe tables stepped by keyframe_index. */
// VTABLE: WIZ8 0x005ecda0
class stKeyframedLightDefinition : public stLightDefinition {
public:
    stKeyframedLightDefinition() : keyframe_index(0), time(0.0f)
    {
        kind = W8_LIGHT_DEFINITION_KEYFRAMED;
    }
    virtual stLightDefinition* Clone() const override;
    virtual bool IsEnabledForSubcycle(unsigned char subcycle) override;

    W8GrowableVector<int> frame_sums;
    W8GrowableVector<int> key_frames;
    W8GrowableVector<float> key_intensities;
    W8GrowableVector<srVector3T<float> > key_colors;
    int keyframe_index;
    float time;
    int start_frame;
    float end_frame;
};

static_assert(sizeof(stKeyframedLightDefinition) == 0x58,
              "stKeyframedLightDefinition_size_must_be_0x58");
static_assert(offsetof(stKeyframedLightDefinition, frame_sums) == 0x08,
              "stKeyframedLightDefinition_values_08");
static_assert(offsetof(stKeyframedLightDefinition, key_frames) == 0x18,
              "stKeyframedLightDefinition_values_18");
static_assert(offsetof(stKeyframedLightDefinition, key_intensities) == 0x28,
              "stKeyframedLightDefinition_values_28");
static_assert(offsetof(stKeyframedLightDefinition, key_colors) == 0x38,
              "stKeyframedLightDefinition_values_38");
static_assert(offsetof(stKeyframedLightDefinition, keyframe_index) == 0x48,
              "stKeyframedLightDefinition_keyframe_index");
static_assert(offsetof(stKeyframedLightDefinition, time) == 0x4c,
              "stKeyframedLightDefinition_time_4c");
static_assert(offsetof(stKeyframedLightDefinition, start_frame) == 0x50,
              "stKeyframedLightDefinition_start_frame");
static_assert(offsetof(stKeyframedLightDefinition, end_frame) == 0x54,
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
    void Reset();                                              /* 0x0049D070 */
    void SetDefinitionTime(float time);                        /* 0x0049C940 */
    void Update();                                             /* 0x0049C960 */

    float positionalX() const
    {
        return m_position.x;
    }
    float positionalY() const
    {
        return m_position.y;
    }
    float positionalZ() const
    {
        return m_position.z;
    }
    stLightDefinition* definition() const
    {
        return m_definition;
    }
    void ConfigureMonsterCopy()
    {
        attenuation_model = srLight::ATTENUATION_3DSTUDIO_MAX;
        enable_flags |= 0x10; /* ENABLE_RANGE_FAR */
        enable_flags |= 4;    /* ENABLE_BOUNDING_SPHERE */
    }

public:
    /* One value, not three floats: 0x0049C690 copies it through the base-pointer
       form VC6 emits for a class type's memberwise assignment, not through three
       independent displacement loads. */
    srVector3T<float> m_position;    /* 0x228 */
    stLightDefinition* m_definition;     /* 0x234: owned */
    unsigned char m_unknown_238;         /* 0x238 */
    /* Oscillation direction: zero sweeps intensity down, nonzero sweeps up. */
    unsigned char m_direction;
    /* Raised by light-toggle triggers; the save path serializes the names of
       lights carrying it so their toggled state persists in savegames. */
    bool m_save_marked; /* 0x23a */
    unsigned char m_padding_23b;
    /* GetTickCount()/1000 timestamp of the last intensity/color update. */
    float m_level_time;
    /* Current 0..1 sweep level driving intensity and the color lerp. */
    float m_level;
    W8PathAI* path_ai; /* 0x244 */
    /* Current path entry index, advanced by m_path_direction. */
    int m_path_index;
    /* GetTickCount()/1000 timestamp of the last path advance. */
    float m_path_time;
    /* Path step direction, +1 or -1 under the ping-pong flag. */
    int m_path_direction;
    W8Prop* m_prop; /* 0x254 */
};

static_assert(sizeof(stLight) == 0x258, "stLight_must_be_0x258");

void SaveLightStates(int handle);
void LoadLightStates(int handle);
