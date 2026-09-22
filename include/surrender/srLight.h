#pragma once

#include "srIlluminator.h"

/* Dump strings name attenuation model (No attenuation / OpenGL / 3DStudio Max),
   near/far ranges, OpenGL attenuation factors, safe range, spot, and the
   ambient/diffuse/specular coefficients. Ctor preset 0 sets enable flags
   0x12, preset 1 (Wizardry default) sets 0x10, preset 2 sets bit 0. */
#pragma pack(push, 4)
class srLight : public srClassSupport<srLight, srIlluminator, false, 0x1220> {
public:
    enum e_preset { PRESET_POSITIONAL_0 = 0, PRESET_POSITIONAL_1 = 1, PRESET_POSITIONAL_2 = 2 };
    /* enable/disable/isEnabled take these as bit indices into +0x194.
       Dump's Control-flags name table is unset on disk. process uses bit 3
       as the 3DStudio near-range gate and bit 4 as the far-range gate.
       Ctor always ORs bit 4; Wizardry also ORs ENABLE_BOUNDING_SPHERE. */
    enum e_enable {
        ENABLE_POSITIONAL_0 = 0,
        ENABLE_POSITIONAL_1 = 1,
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
    SR_DLL_IMPORT srLight(srNode* parent = 0, e_preset preset = PRESET_POSITIONAL_1);
    SR_DLL_IMPORT srLight(const srLight& other);
    SR_DLL_IMPORT srLight& operator=(const srLight& other);

#if defined(SURRENDER_BUILD)
    static const char* sGetClassName();
#else
    /* Pushed as the literal at 0x00606E48 wherever the registry chain runs,
       never called through SR.DLL's import table, so this level's name is
       header-visible unlike srNode's and srIlluminator's. The provider
       still emits its own copy from light.cpp. */
    static const char* sGetClassName()
    {
        return "srLight";
    }
#endif

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

public:
    /* Public per the SR.DLL export (??1srLight@@UAE@XZ) and inline so
       0x0049C430 expands the empty body rather than calling an import.
       SR.DLL also emits the out-of-line copy at 0x1004ED70. */
    // FUNCTION: SURRENDER 0x1004ED70
    virtual ~srLight() override {}
    virtual SR_DLL_IMPORT void traverse(srNode::TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(const srNode::ProcessInfo& info,
                                       srNode::e_processType type) override;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;

public:
    SR_DLL_IMPORT void setLinearAttenuation(float range, float attenuation);
    SR_DLL_IMPORT void disable(e_enable option);
    SR_DLL_IMPORT void enable(e_enable option);
    SR_DLL_IMPORT int isEnabled(e_enable option) const;
    SR_DLL_IMPORT srVector3T<float> getAmbient() const;
    SR_DLL_IMPORT srVector3T<float> getDiffuse() const;
    SR_DLL_IMPORT float getIntensity() const;
    SR_DLL_IMPORT srVector3T<float> getSpecular() const;
    SR_DLL_IMPORT float getSpotAngle() const;
    SR_DLL_IMPORT srVector3T<float> getSpotDirection() const;
    SR_DLL_IMPORT float getSpotExponent() const;
    SR_DLL_IMPORT void setAmbient(const srVector3T<float>& color);
    SR_DLL_IMPORT void setDiffuse(const srVector3T<float>& color);
    SR_DLL_IMPORT void setIntensity(float intensity);
    SR_DLL_IMPORT void setSpecular(const srVector3T<float>& color);
    SR_DLL_IMPORT void setSpotAngle(float angle);
    SR_DLL_IMPORT void setSpotDirection(const srVector3T<float>& direction);
    SR_DLL_IMPORT void setSpotExponent(float exponent);
    SR_DLL_IMPORT void setAttenuationModel(e_attenuationModel model);
    SR_DLL_IMPORT e_attenuationModel getAttenuationModel() const;
    SR_DLL_IMPORT void setAttenuation(const srVector3T<float>& attenuation);
    SR_DLL_IMPORT srVector3T<float> getAttenuation() const;
    SR_DLL_IMPORT void getFarAttenuationRange(double& start, double& end) const;
    SR_DLL_IMPORT void getNearAttenuationRange(double& start, double& end) const;
    SR_DLL_IMPORT void setFarAttenuationRange(double start, double end);
    SR_DLL_IMPORT void setNearAttenuationRange(double start, double end);
    SR_DLL_IMPORT void setSafeRange(float range);
    SR_DLL_IMPORT float getSafeRange() const;

    e_attenuationModel attenuation_model_150; /* 0x150 */
    unsigned char pad_154_[4];
    double near_start_158; /* 0x158 */
    double near_end_160;   /* 0x160 */
    double far_start_168;  /* 0x168 */
    double far_end_170;    /* 0x170 */
    /* 3DStudio ranges scaled by the current model-view scale, and the
       inverse range factors (1.0 when the range is degenerate). */
    float near_start_scaled_178; /* 0x178 */
    float far_end_scaled_17c;    /* 0x17c */
    float near_inverse_180;      /* 0x180 */
    float far_inverse_184;       /* 0x184 */
    /* BakeInstanceVertexLighting copies this wholesale into a local vec3;
       setLinearAttenuation stores the linear coefficient in .y. */
    srVector3T<float> opengl_attenuation_188; /* 0x188 */
    unsigned long enable_flags_194;           /* 0x194 */
    srVector3T<float> ambient_198;            /* 0x198 */
    srVector3T<float> diffuse_1a4;            /* 0x1a4 */
    srVector3T<float> specular_1b0;           /* 0x1b0 */
    srVector3T<float> spot_direction_1bc;     /* 0x1bc */
    float spot_angle_1c8;                     /* 0x1c8 */
    float spot_exponent_1cc;                  /* 0x1cc */
    float intensity_1d0;                      /* 0x1d0 */
    float safe_range_1d4;                     /* 0x1d4 */
    /* Eye-space state computed by process(ProcessInfo): the three
       intensity-scaled channel colors (w zero), the eye-space spot
       direction, cos(spot_angle), and the scaled far range end. */
    srVector4T<float> eye_ambient_1d8;  /* 0x1d8 */
    srVector4T<float> eye_diffuse_1e8;  /* 0x1e8 */
    srVector4T<float> eye_specular_1f8; /* 0x1f8 */
    srVector3T<float> eye_spot_dir_208; /* 0x208 */
    float spot_cos_214;                 /* 0x214 */
    float far_end_current_218;          /* 0x218 */
    unsigned long activity_21c;         /* 0x21c: activity bitfield; bit 0 = lit */
    unsigned long channel_bits_220;     /* 0x220: vp channel bits this light feeds */
    unsigned char unknown_224_[4];
};
#pragma pack(pop)

static_assert(sizeof(srLight) == 0x228, "srLight_must_be_0x228");
