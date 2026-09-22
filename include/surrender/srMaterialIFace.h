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
    /* Retail exports standalone copies of the in-class lifecycle members while
       also folding them into callers: srMaterial's constructors inline the
       base registration sequence and ~srMaterialIFace emits per-TU at
       0x10016260/0x10016310. Member-level dllexport forces the standalone
       emissions; class-level dllexport cannot be used because it eagerly
       instantiates the srClassSupport template base, including forwarding
       constructors srClass does not provide. */
#if defined(SURRENDER_BUILD)
    // FUNCTION: SURRENDER 0x10034BE0 SYMBOL
    // ??0srMaterialIFace@@QAE@XZ
    __declspec(dllexport) srMaterialIFace() {}
    /* The copy constructor runs the default base construction plus
       registration, not a base copy. */
    // FUNCTION: SURRENDER 0x10034C70 SYMBOL
    // ??0srMaterialIFace@@QAE@ABV0@@Z
    __declspec(dllexport) srMaterialIFace(const srMaterialIFace&) {}
    // FUNCTION: SURRENDER 0x10016310 SYMBOL
    // ??1srMaterialIFace@@UAE@XZ
    __declspec(dllexport) virtual ~srMaterialIFace() {}
#else
    srMaterialIFace();
    srMaterialIFace(const srMaterialIFace& other);
    virtual ~srMaterialIFace();
#endif
    /* Retail exports the out-of-line assignment (srMaterial.cpp TU); it only
       forwards the srClass base assignment. */
    srMaterialIFace& operator=(const srMaterialIFace& other);

    virtual void getMaterialInfo(srVertexProcessor::MaterialInfo& info) = 0;
    virtual void preProcess(srVertexPipe& pipe) = 0;
    virtual void postProcess(srVertexPipe& pipe) = 0;
};
