#pragma once

#include "srNode.h"
#include "srVertexProcessor.h"

// VTABLE: SURRENDER 0x10077028
// class srClassSupport<srIlluminator, srNode, 0, 4608>

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
    SR_DLL_IMPORT w8_ulong getGroupMask() const;
    SR_DLL_IMPORT void setGroupMask(w8_ulong mask);

#if !defined(SURRENDER_BUILD)
    virtual SR_DLL_IMPORT ~srIlluminator() override;
#endif

    w8_ulong group_mask;            /* 0x13c */
    srVector3T<float> eye_location; /* 0x140 */
};

W8_ABI_ASSERT((sizeof(srIlluminator) == 0x150), "srIlluminator_must_be_0x150");

W8_ASSERT_BASE_END(srIlluminator, srVertexProcessor, group_mask, 0x138);
