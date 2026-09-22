#pragma once

#include "srNode.h"

/* Reconstructed declaration surface for the closed SurRender class. The
   exported constructor and virtual methods establish the ABI; the exported
   ctor/dtor registration dance proves the srClassSupport base (0x1500 under
   srNode's 0x1000). Wiz8's level reader establishes the four-float plane,
   clip mode, and complete size. Wiz8 constructs an ordinary
   srClassSupport<srClipPlane, srClipPlane, ...> instantiation over this
   imported class; its client-emitted registry slots and vtable are not
   evidence for another authored class. */
/* The Wiz8 consumer import table covers only the lifecycle/virtual members;
   the three small accessors below carry no IAT entry, so consumers expand
   them from the in-class bodies while the provider emits the standalone
   exports from clipplane.cpp. */
class SR_DLL_IMPORT srClipPlane : public srClassSupport<srClipPlane, srNode, false, 0x1500> {
public:
    typedef srClassSupport<srClipPlane, srClipPlane, false, 0x1500> ClientType;

    /* Dump prints +0x148 as a decimal "Clip type"; no name table. Wizardry
       always writes 0. */
    enum e_clip { CLIP_POSITIONAL_0 = 0 };

    srClipPlane(srNode* parent);
    srClipPlane(const srClipPlane& other);
    srClipPlane& operator=(const srClipPlane& other);

#if defined(SURRENDER_BUILD)
    static const char* sGetClassName();
#else
    static const char* sGetClassName()
    {
        return "srClipPlane";
    }
#endif

    virtual void dump(std::ostream& stream) override;
    virtual ~srClipPlane() override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

#if defined(SURRENDER_BUILD)
    void setClipPlane(const srVector4T<float>& plane);
#else
    void setClipPlane(const srVector4T<float>& plane)
    {
        clip_plane_ = plane;
    }
#endif
    void getClipPlane(srVector4T<float>& plane) const;
    srVector4T<float> getClipPlane() const;

#if defined(SURRENDER_BUILD)
    void setClipType(e_clip type);
#else
    void setClipType(e_clip type)
    {
        clip_type_ = type;
    }
#endif
    e_clip getClipType() const;

protected:
    srVector4T<float> clip_plane_; /* 0x138 */
    e_clip clip_type_;             /* 0x148 */
    unsigned char unknown_14c_[4];
};

static_assert(sizeof(srClipPlane) == 0x150, "srClipPlane_must_be_0x150");
