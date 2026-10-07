#pragma once

#include "srIlluminator.h"

// VTABLE: SURRENDER 0x100770F8 srVertexProcessor
// class srClassSupport<srLight, srIlluminator, 0, 4640>

// VTABLE: SURRENDER 0x10077104 srClassSupport<srIlluminator, srNode, 0, 4608>
// class srClassSupport<srLight, srIlluminator, 0, 4640>

/* Dump strings name attenuation model (No attenuation / OpenGL / 3DStudio Max),
   near/far ranges, OpenGL attenuation factors, safe range, spot, and the
   ambient/diffuse/specular coefficients. Ctor preset 0 sets enable flags
   0x12, preset 1 (Wizardry default) sets 0x10, preset 2 sets bit 0. */
/* Two vtables: the primary srClassSupport table and the srVertexProcessor
   virtual-base sub-table. */
// VTABLE: SURRENDER 0x100770B8 srVertexProcessor
// VTABLE: SURRENDER 0x100770C4 srLight
class SR_DLL_IMPORT SR_DLL_EXPORT srLight
    : public srClassSupport<srLight, srIlluminator, false, 0x1220> {
public:
    enum e_preset { PRESET_DIRECTIONAL = 0, PRESET_POINT = 1, PRESET_SPOT = 2 };
    /* enable/disable/isEnabled take these as bit indices into +0x194.
       Dump's Control-flags name table is unset on disk. process uses bit 3
       as the 3DStudio near-range gate and bit 4 as the far-range gate.
       Ctor always ORs bit 4; Wizardry also ORs ENABLE_BOUNDING_SPHERE. */
    enum e_enable {
        ENABLE_SPOT = 0,
        ENABLE_DIRECTIONAL = 1,
        /* Node process: with RANGE_FAR, run testBoundingSphere on
           safe_range+far_end and clear activity bit 0 when culled. */
        ENABLE_BOUNDING_SPHERE = 2,
        ENABLE_RANGE_NEAR = 3,
        ENABLE_RANGE_FAR = 4
    };
    enum e_attenuationModel {
        ATTENUATION_NONE = 0,
        ATTENUATION_OPENGL = 1,
        ATTENUATION_3DSTUDIO_MAX = 2
    };

    /* stLight derives through srClassSupport, whose constructors name only
       Base's parent parameter. Every emitted srClassSupport<stLight,srLight>
       constructor - the out-of-line 0x004CA8B0 emission and the copies
       inlined into 0x0049C2C0 - reaches this one as srLight(0, 1), so both
       parameters carry those defaults here. */
    srLight(srNode* parent = 0, e_preset preset = PRESET_POINT);

    srLight& operator=(const srLight& other);

    /* Pushed as the literal at 0x00606E48 wherever the registry chain runs,
       never called through SR.DLL's import table, so this level's name is
       header-visible unlike srNode's and srIlluminator's. Provider TUs
       inline the literal inside the srClassSupport registrations; the
       provider still exports an out-of-line copy. */
    // FUNCTION: SURRENDER 0x1004E8F0
    static const char* sGetClassName()
    {
        return "srLight";
    }

    virtual void dump(std::ostream& stream) override;

public:
    /* Wiz8 expands this empty derived level inline; SR.DLL's standalone
       0x1004ED70 emission is the compiler-generated destructor. */

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
    /* Eye-space derived state filled by process(ProcessInfo): the near/far
       attenuation ranges rescaled by the model-view scale, their reciprocal
       slopes, the intensity-scaled colors, the eye-space spot direction and
       cone cutoff, the cull range, the derived light flags and the channel
       mask the vertex processor gates on. */
    float scaled_near_start; /* 0x178 */
    float scaled_far_end;    /* 0x17c */
    float near_attenuation;  /* 0x180 */
    float far_attenuation;   /* 0x184 */
    /* BakeInstanceVertexLighting copies this wholesale into a local vec3;
       setLinearAttenuation stores the linear coefficient in .y. */
    srVector3T<float> opengl_attenuation; /* 0x188 */
    unsigned long enable_flags;           /* 0x194 */
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
    unsigned long derived_flags; /* 0x21c */
    unsigned long channel_mask;  /* 0x220 */
};

static_assert(sizeof(srLight) == 0x228, "srLight_must_be_0x228");
