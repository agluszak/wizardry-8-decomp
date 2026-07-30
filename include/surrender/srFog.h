#pragma once

#include "srNode.h"

/* SR.DLL exports both vtables and the complete lifecycle. The primary table
   continues srIlluminator's scene-node surface; the secondary table at +0x138
   supplies srVertexProcessor destruction, activation and processing. */
class srFog : public srIlluminator {
public:
    SR_DLL_IMPORT srFog(srNode* parent);
    SR_DLL_IMPORT srFog& operator=(const srFog& other);
    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode) override;

protected:
    virtual SR_DLL_IMPORT ~srFog() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;
    virtual SR_DLL_IMPORT srNode* clone() override;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;
};

static_assert(sizeof(srFog) == 0x168, "srFog_must_be_0x168");
