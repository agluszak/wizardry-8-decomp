#pragma once

#include "srTypeRegistry.h"
#include "srVertexProcessor.h"

class srVertexPipe;

// VTABLE: SURRENDER 0x100755A8
// class srClassSupport<srMaterialIFace, srClass, 1, 8704>

/* srMaterialIFace is the 0x2200 node the registry tree puts between srClass and
   srMaterial's 0x2210. srVertexPipe calls its three interface slots directly:
   the srMaterial vftable resolves +0x20/+0x24/+0x28 to getMaterialInfo,
   preProcess and postProcess. */
class __declspec(novtable) SR_DLL_IMPORT SR_DLL_EXPORT srMaterialIFace
    : public srClassSupport<srMaterialIFace, srClass, true, 0x2200> {
public:
    static const char* sGetClassName();
    /* Retail exports the lifecycle bundle, with header-visible construction
       and destruction also expanded in derived callers. */
#if defined(SURRENDER_BUILD)
    // FUNCTION: SURRENDER 0x10034BE0 SYMBOL
    // ??0srMaterialIFace@@QAE@XZ
    srMaterialIFace() {}
    // FUNCTION: SURRENDER 0x10016310 SYMBOL
    // ??1srMaterialIFace@@UAE@XZ
    virtual ~srMaterialIFace() {}
#else
    srMaterialIFace();
    virtual ~srMaterialIFace();
#endif
    /* Provider assignment is the implicit srClassSupport/base assignment.
       Wiz8 imports the standalone symbol, so the consumer keeps only the
       dllimport declaration. */
    // SYNTHETIC: SURRENDER 0x10034D80
    // srMaterialIFace::operator=
#if !defined(SURRENDER_BUILD)
    srMaterialIFace& operator=(const srMaterialIFace& other);
#endif

    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) = 0;
    virtual void preProcess(srVertexPipe& pipe) = 0;
    virtual void postProcess(srVertexPipe& pipe) = 0;
};
