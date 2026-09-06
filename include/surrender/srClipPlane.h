#pragma once

#include "srNode.h"

/* Reconstructed declaration surface for the closed SurRender class. The
   exported constructor and virtual methods establish the ABI; Wiz8's level
   reader establishes the four-float plane, clip mode, and complete size. Wiz8
   constructs an ordinary srClassSupport<srClipPlane, srClipPlane, ...>
   instantiation over this imported base; its client-emitted registry slots
   and vtable are not evidence for another authored class. */
class SR_DLL_IMPORT srClipPlane : public srNode {
public:
    typedef srClassSupport<srClipPlane, srClipPlane, false, 0x1500> ClientType;

    enum e_clip {
        CLIP_POSITIONAL_0 = 0
    };

    srClipPlane(srNode* parent);
    srClipPlane& operator=(const srClipPlane& other);

    static const char* sGetClassName()
    {
        return "srClipPlane";
    }

    virtual void dump(std::ostream& stream) override;
    virtual ~srClipPlane() override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(
        const ProcessInfo& info,
        e_processType type) override;

    void setClipPlane(const srVector4T<float>& plane)
    {
        clip_plane_ = plane;
    }

    void setClipType(e_clip type)
    {
        clip_type_ = type;
    }

protected:
    srVector4T<float> clip_plane_;          /* 0x138 */
    e_clip clip_type_;                      /* 0x148 */
    unsigned char unknown_14c_[4];
};

static_assert(sizeof(srClipPlane) == 0x150,
              "srClipPlane_must_be_0x150");
