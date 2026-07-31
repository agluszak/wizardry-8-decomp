#pragma once

#include "srNode.h"
#include "srVertexProcessor.h"

/* The imported vtable pair and first-party light field accesses establish an
   srIlluminator prefix followed by a 0xc0-byte light-specific renderer tail. */
class srLight
    : public srClassSupport<srLight, srIlluminator, false, 0x1220> {
public:
    enum e_preset {
        PRESET_POSITIONAL_0 = 0,
        PRESET_POSITIONAL_1 = 1
    };

    /* stLight derives through srClassSupport, whose constructors name only
       Base's parent parameter. Every emitted srClassSupport<stLight,srLight>
       constructor - the out-of-line 0x004CA8B0 emission and the copies
       inlined into 0x0049C2C0 - reaches this one as srLight(0, 1), so both
       parameters carry those defaults here. */
    SR_DLL_IMPORT srLight(
        srNode* parent = 0, e_preset preset = PRESET_POSITIONAL_1);
    SR_DLL_IMPORT srLight& operator=(const srLight& other);

    /* Pushed as the literal at 0x00606E48 wherever the registry chain runs,
       never called through SR.DLL's import table, so this level's name is
       header-visible unlike srNode's and srIlluminator's. */
    static const char* sGetClassName() { return "srLight"; }

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    /* Header-visible for the same reason srIlluminator's is: 0x0049C430
       expands it rather than calling an import. */
    virtual ~srLight() override {}

public:
    virtual SR_DLL_IMPORT void traverse(srNode::TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(
        const srNode::ProcessInfo& info,
        srNode::e_processType type) override;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;

public:
    SR_DLL_IMPORT void setLinearAttenuation(float range, float attenuation);

    float m_positional_168;                         /* complete +0x168 */
    float m_positional_16c;                         /* complete +0x16c */
    double m_range_170;                             /* complete +0x170 */
    unsigned char unknown_178_[0x1c];
    unsigned int m_positional_flags_5c;              /* complete +0x194 */
    srVector3T<float> m_direction_60;                 /* complete +0x198 */
    srVector3T<float> m_color_6c;                    /* complete +0x1a4 */
    srVector3T<float> m_position_78;                  /* complete +0x1b0 */
    unsigned char unknown_1bc_[0x14];
    /* stLight::process temporarily scales this renderer-owned value. */
    float m_positional_98;                           /* complete +0x1d0 */
    float m_positional_1d4;                         /* complete +0x1d4 */
    unsigned char unknown_1d8_[0x50];
};

static_assert(sizeof(srLight) == 0x228, "srLight_must_be_0x228");
