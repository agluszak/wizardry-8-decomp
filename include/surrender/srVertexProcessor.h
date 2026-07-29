#pragma once

#include "srHeap.h"

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

private:
    unsigned char unknown_04_[0x94];

protected:
    /* stLight::process temporarily scales this renderer-owned value. */
    float m_positional_98;

private:
    unsigned char unknown_9c_[0x54];
};

static_assert(
    sizeof(srVertexProcessor) == 0xf0,
    "srVertexProcessor_must_be_0xf0");
