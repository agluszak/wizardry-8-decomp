#pragma once

#include "srIlluminator.h"

/* SR.DLL exports both vtables and the complete lifecycle. Dump/assert strings
   name Fog start, Fog end and Density; ctor defaults density 0.5 and fogEnd
   1000.0. Wizardry writes these fields on the static and dynamic scene fogs. */
#pragma pack(push, 4)
class srFog : public srIlluminator {
public:
    typedef srClassSupport<srFog, srFog, false, 0x1210> ClientType;

    SR_DLL_IMPORT srFog(srNode* parent);
    SR_DLL_IMPORT srFog& operator=(const srFog& other);
    static SR_DLL_IMPORT const char* sGetClassName()
    {
        return "srFog";
    }

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode) override;

    /* Retail exports the destructor under its public spelling. */
    virtual SR_DLL_IMPORT ~srFog() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;
    virtual SR_DLL_IMPORT srClass* clone() override;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;

    double fog_start_150; /* 0x150 */
    double fog_end_158;   /* 0x158 */
    float density_160;    /* 0x160 */
    unsigned char pad_164_[4];
};
#pragma pack(pop)

static_assert(sizeof(srFog) == 0x168, "srFog_must_be_0x168");
