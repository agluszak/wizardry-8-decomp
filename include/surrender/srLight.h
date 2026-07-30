#pragma once

#include "srNode.h"
#include "srVertexProcessor.h"

/* The imported vtable pair and first-party light field accesses establish an
   srIlluminator prefix followed by a 0xc0-byte light-specific renderer tail. */
class srLight : public srIlluminator {
public:
    enum e_preset {
        PRESET_POSITIONAL_0 = 0
    };

    SR_DLL_IMPORT srLight(srNode* parent, e_preset preset);
    SR_DLL_IMPORT srLight& operator=(const srLight& other);

    virtual const char* getClassName() const override;
    virtual unsigned long getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    virtual SR_DLL_IMPORT ~srLight() override;

public:
    virtual srNode* clone() override;
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
