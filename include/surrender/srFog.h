#pragma once

#include "srIlluminator.h"

/* SR.DLL exports both vtables and the complete lifecycle. Dump/assert strings
   name Fog start, Fog end and Density; ctor defaults density 0.5 and fogEnd
   1000.0. Wizardry writes these fields on the static and dynamic scene fogs. */
// VTABLE: SURRENDER 0x10076FA8 srVertexProcessor
// VTABLE: SURRENDER 0x10076FB4 srClassSupport<srIlluminator, srNode, 0, 4608>
// class srFog
class SR_DLL_EXPORT srFog : public srIlluminator {
public:
    typedef srClientSupport<srFog, 0x1210> ClientType;

    SR_DLL_IMPORT srFog(srNode* parent = 0);
    SR_DLL_IMPORT srFog(const srFog& other);
    /* Provider copy assignment is consistent with srIlluminator assignment
       followed by the three fog fields. Wiz8 imports the standalone symbol,
       so the consumer keeps only the dllimport declaration. */

#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srFog& operator=(const srFog& other);
#endif
    /* Header-visible like srLight's: the client getClassName emission
       (0x00484710) returns the consumer literal directly, the consumer import
       table has no entry, and provider TUs inline the same literal inside the
       srClassSupport registrations. The provider still exports an out-of-line
       copy. */
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

    /* Retail exports the destructor under its public spelling. */
    virtual SR_DLL_IMPORT ~srFog() override;

public:
    virtual SR_DLL_IMPORT srClass* vInstance() override;
    /* Slot 7 binds the inherited srClassSupport<srIlluminator, ...>::clone
       emission - the exported vftable name
       ??_7srFog@@6B?$srClassSupport@VsrIlluminator@@VsrNode@@$0A@$0BCAA@@@@
       proves no srFog-level override exists. */
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;

    double fog_start; /* 0x150 */
    double fog_end;   /* 0x158 */
    float density_160;    /* 0x160 */
};

static_assert(sizeof(srFog) == 0x168, "srFog_must_be_0x168");
