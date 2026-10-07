#pragma once

#include "srIlluminator.h"

// VTABLE: SURRENDER 0x10076FE8 srVertexProcessor
// class srClassSupport<srFog, srIlluminator, 0, 4624>
// VTABLE: SURRENDER 0x10076FF4 srClassSupport<srIlluminator, srNode, 0, 4608>
// class srClassSupport<srFog, srIlluminator, 0, 4624>
// VTABLE: SURRENDER 0x10076FA8 srVertexProcessor
// VTABLE: SURRENDER 0x10076FB4 srClassSupport<srIlluminator, srNode, 0, 4608>
// class srFog
class SR_DLL_EXPORT srFog : public srClassSupport<srFog, srIlluminator, false, 0x1210> {
public:
    typedef srClientSupport<srFog, 0x1210> ClientType;

    SR_DLL_IMPORT srFog(srNode* parent = 0);
#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srFog(const srFog& other);
#endif

#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srFog& operator=(const srFog& other);
#endif

    // FUNCTION: SURRENDER 0x1004C1A0
    static const char* sGetClassName()
    {
        return "srFog";
    }
    SR_DLL_IMPORT void setDensity(float density);
    SR_DLL_IMPORT float getDensity() const;
    SR_DLL_IMPORT void setRange(double start, double end);
    SR_DLL_IMPORT void getRange(double& start, double& end);

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode) override;

    virtual SR_DLL_IMPORT ~srFog() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    using srClassSupport<srFog, srIlluminator, false, 0x1210>::process;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;

    double fog_start; /* 0x150 */
    double fog_end;   /* 0x158 */
    float density;    /* 0x160 */
};

static_assert(sizeof(srFog) == 0x168, "srFog_must_be_0x168");
