#pragma once

#include "srHeap.h"
#include "srMath.h"

class srVertexPipe;

/*
 * SR.DLL exports secondary vtables qualified as srVertexProcessor for srFog,
 * srIlluminator and srLight.  Each has exactly three slots: a destructor,
 * isActive and process.  In srLight-derived Wiz8 objects this subobject begins
 * at +0x138 and the complete renderer base ends at +0x228.
 */
class srVertexProcessor {
public:
    struct MaterialInfo;

protected:
    virtual ~srVertexProcessor() {}

public:
    virtual int isActive(srVertexPipe& pipe) = 0;
    virtual void process(srVertexPipe& pipe) = 0;

protected:
    unsigned char unknown_04_[0x14];
    int m_positional_18;
    unsigned char unknown_1c_[0x40];
    unsigned int m_positional_flags_5c;
    unsigned char unknown_60_[0x0c];
    srVector3T<float> m_color_6c;
    unsigned char unknown_78_[0x20];

protected:
    /* stLight::process temporarily scales this renderer-owned value. */
    float m_positional_98;

protected:
    unsigned char unknown_9c_[0x54];
};

static_assert(
    sizeof(srVertexProcessor) == 0xf0,
    "srVertexProcessor_must_be_0xf0");
