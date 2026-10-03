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
    /* Provider construction/destruction are consistent with base-only lifecycle.
       Wiz8 imports the standalone public symbols. */

#if !defined(SURRENDER_BUILD)
    srMaterialIFace();
    virtual ~srMaterialIFace();
#endif
    /* Provider assignment is consistent with srClassSupport/base assignment.
       Wiz8 imports the standalone symbol, so the consumer keeps only the
       dllimport declaration. */

#if !defined(SURRENDER_BUILD)
    srMaterialIFace& operator=(const srMaterialIFace& other);
#endif

    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) = 0;
    virtual void preProcess(srVertexPipe& pipe) = 0;
    virtual void postProcess(srVertexPipe& pipe) = 0;
};
