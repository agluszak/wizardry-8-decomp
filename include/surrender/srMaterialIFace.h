#pragma once

#include "srTypeRegistry.h"
#include "srVertexProcessor.h"

class srVertexPipe;

// VTABLE: SURRENDER 0x100755A8
// class srClassSupport<srMaterialIFace, srClass, 1, 8704>

class __declspec(novtable) SR_DLL_IMPORT SR_DLL_EXPORT srMaterialIFace
    : public srClassSupport<srMaterialIFace, srClass, true, 0x2200> {
public:
    static const char* sGetClassName();

#if !defined(SURRENDER_BUILD)
    srMaterialIFace();
    virtual ~srMaterialIFace();
#endif

#if !defined(SURRENDER_BUILD)
    srMaterialIFace& operator=(const srMaterialIFace& other);
#endif

    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) = 0;
    virtual void preProcess(srVertexPipe& pipe) = 0;
    virtual void postProcess(srVertexPipe& pipe) = 0;
};
