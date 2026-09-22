#pragma once

#include "srTypeRegistry.h"
#include "srVertexProcessor.h"

class srVertexPipe;

/* srMaterialIFace is the 0x2200 node the registry tree puts between srClass and
   srMaterial's 0x2210. srVertexPipe calls its three interface slots directly:
   the srMaterial vftable resolves +0x20/+0x24/+0x28 to getMaterialInfo,
   preProcess and postProcess. */
class SR_DLL_IMPORT srMaterialIFace
    : public srClassSupport<srMaterialIFace, srClass, true, 0x2200> {
public:
    static const char* sGetClassName();
    /* Retail exports the out-of-line assignment (srMaterial.cpp TU); it only
       forwards the srClass base assignment. */
    srMaterialIFace& operator=(const srMaterialIFace& other);

    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) = 0;
    virtual void preProcess(srVertexPipe& pipe) = 0;
    virtual void postProcess(srVertexPipe& pipe) = 0;
};
