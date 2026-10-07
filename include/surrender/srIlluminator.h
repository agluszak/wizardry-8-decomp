#pragma once

#include "srNode.h"
#include "srVertexProcessor.h"

// VTABLE: SURRENDER 0x10077028
// class srClassSupport<srIlluminator, srNode, 0, 4608>

/* SR.DLL's exported primary and secondary vtable names establish the exact
   srNode/srVertexProcessor multiple-inheritance prefix. getGroupMask/setGroupMask
   store a dword at complete +0x13c; operator= copies that dword only.
   Allocation of the most-derived illuminator stops at 0x150: fog and light
   own the bytes that follow. */
// VTABLE: SURRENDER 0x10077068 srVertexProcessor
// VTABLE: SURRENDER 0x10077074 srClassSupport<srIlluminator, srNode, 0, 4608>
// class srIlluminator
class SR_DLL_EXPORT srIlluminator : public srClassSupport<srIlluminator, srNode, false, 0x1200>,
                                    public srVertexProcessor {
public:
    using srVertexProcessor::process;
    SR_DLL_IMPORT srIlluminator(srNode* parent = 0);

    SR_DLL_IMPORT srIlluminator& operator=(const srIlluminator& other);
    static SR_DLL_IMPORT const char* sGetClassName();
    virtual SR_DLL_IMPORT void traverse(TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(const ProcessInfo& info, e_processType type) override;
    SR_DLL_IMPORT unsigned long getGroupMask() const;
    SR_DLL_IMPORT void setGroupMask(unsigned long mask);

    /* Empty body: the provider uses the implicit base teardown.
       Wiz8 imports the standalone public destructor. */

#if !defined(SURRENDER_BUILD)
    virtual SR_DLL_IMPORT ~srIlluminator() override;
#endif

    unsigned long group_mask;       /* 0x13c */
    srVector3T<float> eye_location; /* 0x140 */
};

static_assert((sizeof(srIlluminator) == 0x150), "srIlluminator_must_be_0x150");
/* The secondary srVertexProcessor subobject sits at +0x138 (retail secondary
   vftable); the group's own members begin at +0x13c. */
W8_ASSERT_BASE_END(srIlluminator, srVertexProcessor, group_mask, 0x138);
