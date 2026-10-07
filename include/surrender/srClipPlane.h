#pragma once

#include "srNode.h"

// VTABLE: SURRENDER 0x10076EF4
// class srClassSupport<srClipPlane, srNode, 0, 5376>

// VTABLE: SURRENDER 0x10076EC0 srClipPlane
// class srClipPlane
class SR_DLL_IMPORT SR_DLL_EXPORT srClipPlane
    : public srClassSupport<srClipPlane, srNode, false, 0x1500> {
public:
    typedef srClientSupport<srClipPlane, 0x1500> ClientType;

#if !defined(SURRENDER_BUILD)
    srClipPlane& operator=(const srClipPlane& other);
    virtual ~srClipPlane() override;
#endif

    enum e_clip { CLIP_POSITIONAL_0 = 0 };

    srClipPlane(srNode* parent = 0);

    // FUNCTION: SURRENDER 0x1004A150
    static const char* sGetClassName()
    {
        return "srClipPlane";
    }

    virtual void dump(std::ostream& stream) override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

#if defined(SURRENDER_BUILD)
    void setClipPlane(const srVector4T<float>& plane);
    void setClipType(e_clip type);
#else
    void setClipPlane(const srVector4T<float>& plane)
    {
        clip_plane_ = plane;
    }

    void setClipType(e_clip type)
    {
        clip_type_ = type;
    }
#endif
    void getClipPlane(srVector4T<float>& plane) const;
    srVector4T<float> getClipPlane() const;
    e_clip getClipType() const;

protected:
    srVector4T<float> clip_plane_; /* 0x138 */
    e_clip clip_type_;             /* 0x148 */
};

static_assert(sizeof(srClipPlane) == 0x150, "srClipPlane_must_be_0x150");
