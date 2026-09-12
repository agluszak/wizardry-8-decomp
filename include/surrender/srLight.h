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
    SR_DLL_IMPORT srLight& operator=(const srLight& other);

    /* Pushed as the literal at 0x00606E48 wherever the registry chain runs,
       never called through SR.DLL's import table, so this level's name is
       header-visible unlike srNode's and srIlluminator's. */
    static const char* sGetClassName()
    {
        return "srLight";
    }

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    /* Header-visible for the same reason srIlluminator's is: 0x0049C430
       expands it rather than calling an import. */
    virtual ~srLight() override {}

public:
    virtual SR_DLL_IMPORT void traverse(srNode::TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(const srNode::ProcessInfo& info,
                                       srNode::e_processType type) override;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;

public:
    SR_DLL_IMPORT void setLinearAttenuation(float range, float attenuation);

    e_attenuationModel attenuation_model_150; /* 0x150 */
    unsigned char pad_154_[4];
    double near_start_158; /* 0x158 */
    double near_end_160;   /* 0x160 */
    double far_start_168;  /* 0x168 */
    double far_end_170;    /* 0x170 */
    unsigned char unknown_178_[0x10];
    float opengl_constant_188;            /* 0x188 */
    float opengl_linear_18c;              /* 0x18c */
    float opengl_quadratic_190;           /* 0x190 */
    unsigned long enable_flags_194;       /* 0x194 */
    srVector3T<float> ambient_198;        /* 0x198 */
    srVector3T<float> diffuse_1a4;        /* 0x1a4 */
    srVector3T<float> specular_1b0;       /* 0x1b0 */
    srVector3T<float> spot_direction_1bc; /* 0x1bc */
    float spot_angle_1c8;                 /* 0x1c8 */
    float spot_exponent_1cc;              /* 0x1cc */
    float intensity_1d0;                  /* 0x1d0 */
    float safe_range_1d4;                 /* 0x1d4 */
    unsigned char unknown_1d8_[0x50];
};
#pragma pack(pop)

static_assert(sizeof(srLight) == 0x228, "srLight_must_be_0x228");
