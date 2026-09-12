#pragma once

#include "srNode.h"
#include "srVertexProcessor.h"

/* SR.DLL's exported primary and secondary vtable names establish the exact
   srNode/srVertexProcessor multiple-inheritance prefix. getGroupMask/setGroupMask
   store a dword at complete +0x13c; operator= copies that dword only.
   Allocation of the most-derived illuminator stops at 0x150: fog and light
   own the bytes that follow. */
#pragma pack(push, 4)
class srIlluminator : public srClassSupport<srIlluminator, srNode, false, 0x1200>,
                      public srVertexProcessor {
public:
    SR_DLL_IMPORT srIlluminator(srNode* parent);
    SR_DLL_IMPORT srIlluminator& operator=(const srIlluminator& other);
    static SR_DLL_IMPORT const char* sGetClassName();
    virtual SR_DLL_IMPORT void traverse(TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(const ProcessInfo& info, e_processType type) override;
    virtual int isActive(srVertexPipe& pipe) override = 0;
    virtual void process(srVertexPipe& pipe) override = 0;
    SR_DLL_IMPORT unsigned long getGroupMask() const;
    SR_DLL_IMPORT void setGroupMask(unsigned long mask);

protected:
    /* Empty and header-visible, not exported: stLight's destructor at
       0x0049C430 expands this level and srLight's inline instead of calling
       either, and reaches SR.DLL only for srNode::~srNode. The registry
       teardown at this level belongs to the srClassSupport base, and the
       vptr store this body would make is dead-stored away by the base's own
       store that immediately follows. */
    virtual ~srIlluminator() override {}

public:
    unsigned long group_mask_13c;       /* 0x13c */
    srVector3T<float> eye_location_140; /* 0x140 */
    unsigned char pad_14c_[4];          /* 0x14c: keep derived doubles at 0x150 */
};
#pragma pack(pop)

static_assert((sizeof(srIlluminator) == 0x150), "srIlluminator_must_be_0x150");
