#pragma once

#include "srIlluminator.h"

// VTABLE: SURRENDER 0x100770F8 srVertexProcessor
// class srClassSupport<srLight, srIlluminator, 0, 4640>

// VTABLE: SURRENDER 0x10077104 srClassSupport<srIlluminator, srNode, 0, 4608>
// class srClassSupport<srLight, srIlluminator, 0, 4640>

// VTABLE: SURRENDER 0x100770B8 srVertexProcessor
// VTABLE: SURRENDER 0x100770C4 srLight
class SR_DLL_IMPORT SR_DLL_EXPORT srLight
    : public srClassSupport<srLight, srIlluminator, false, 0x1220> {
public:
    enum e_preset { PRESET_DIRECTIONAL = 0, PRESET_POINT = 1, PRESET_SPOT = 2 };

    enum e_enable {
        ENABLE_SPOT = 0,
        ENABLE_DIRECTIONAL = 1,

        ENABLE_BOUNDING_SPHERE = 2,
        ENABLE_RANGE_NEAR = 3,
        ENABLE_RANGE_FAR = 4
    };
    enum e_attenuationModel {
        ATTENUATION_NONE = 0,
        ATTENUATION_OPENGL = 1,
        ATTENUATION_3DSTUDIO_MAX = 2
    };

    srLight(srNode* parent = 0, e_preset preset = PRESET_POINT);

    srLight& operator=(const srLight& other);

    // FUNCTION: SURRENDER 0x1004E8F0
    static const char* sGetClassName()
    {
        return "srLight";
    }

    virtual void dump(std::ostream& stream) override;

public:
#if !defined(SURRENDER_BUILD)
    virtual ~srLight() override {}
#endif

public:
    virtual void traverse(srNode::TraverseInfo& info) override;
    virtual void process(const srNode::ProcessInfo& info, srNode::e_processType type) override;
    virtual int isActive(srVertexPipe& pipe) override;
    virtual void process(srVertexPipe& pipe) override;

public:
    void setLinearAttenuation(float range, float attenuation);
    void disable(e_enable option);
    void enable(e_enable option);
    int isEnabled(e_enable option) const;
    srVector3T<float> getAmbient() const;
    srVector3T<float> getDiffuse() const;
    float getIntensity() const;
    srVector3T<float> getSpecular() const;
    float getSpotAngle() const;
    srVector3T<float> getSpotDirection() const;
    float getSpotExponent() const;
    void setAmbient(const srVector3T<float>& color);
    void setDiffuse(const srVector3T<float>& color);
    void setIntensity(float intensity);
    void setSpecular(const srVector3T<float>& color);
    void setSpotAngle(float angle);
    void setSpotDirection(const srVector3T<float>& direction);
    void setSpotExponent(float exponent);
    void setAttenuationModel(e_attenuationModel model);
    e_attenuationModel getAttenuationModel() const;
    void setAttenuation(const srVector3T<float>& attenuation);
    srVector3T<float> getAttenuation() const;
    void getFarAttenuationRange(double& start, double& end) const;
    void getNearAttenuationRange(double& start, double& end) const;
    void setFarAttenuationRange(double start, double end);
    void setNearAttenuationRange(double start, double end);
    void setSafeRange(float range);
    float getSafeRange() const;

    e_attenuationModel attenuation_model; /* 0x150 */
    double near_start;                    /* 0x158 */
    double near_end;                      /* 0x160 */
    double far_start;                     /* 0x168 */
    double far_end;                       /* 0x170 */

    float scaled_near_start; /* 0x178 */
    float scaled_far_end;    /* 0x17c */
    float near_attenuation;  /* 0x180 */
    float far_attenuation;   /* 0x184 */

    srVector3T<float> opengl_attenuation; /* 0x188 */
    w8_ulong enable_flags;                /* 0x194 */
    srVector3T<float> ambient;            /* 0x198 */
    srVector3T<float> diffuse;            /* 0x1a4 */
    srVector3T<float> specular;           /* 0x1b0 */
    srVector3T<float> spot_direction;     /* 0x1bc */
    float spot_angle;                     /* 0x1c8 */
    float spot_exponent;                  /* 0x1cc */
    float intensity;                      /* 0x1d0 */
    float safe_range;                     /* 0x1d4 */
    srVector4T<float> scaled_ambient;     /* 0x1d8: ambient * intensity */
    srVector4T<float> scaled_diffuse;     /* 0x1e8: diffuse * intensity */
    srVector4T<float> scaled_specular;    /* 0x1f8: specular * intensity */
    srVector3T<float> spot_direction_eye; /* 0x208 */
    float spot_cutoff;                    /* 0x214: cos(spot_angle) */
    float attenuation_range;              /* 0x218: scaled far end */
    enum {
        DERIVED_ACTIVE = 0x01u,
        DERIVED_SPOT = 0x02u,
        DERIVED_DIRECTIONAL = 0x04u,
        DERIVED_OPENGL_ATTENUATION = 0x08u,
        DERIVED_CONSTANT_ATTENUATION = 0x10u,
        DERIVED_RANGE_CULL = 0x20u
    };
    w8_ulong derived_flags; /* 0x21c */
    w8_ulong channel_mask;  /* 0x220 */
};

W8_ABI_ASSERT(sizeof(srLight) == 0x228, "srLight_must_be_0x228");
