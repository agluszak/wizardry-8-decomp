#pragma once

#include "srNode.h"

// VTABLE: SURRENDER 0x10076f30 srBounder
// VTABLE: SURRENDER 0x10076f64 srClassSupport<srBounder, srNode, 0, 5632>
class SR_DLL_IMPORT srBounder : public srClassSupport<srBounder, srNode, false, 0x1600> {
public:
    enum e_boundMode { BOUND_MODE_POSITIONAL_0 = 0 };

    srBounder(srNode* parent);
    srBounder(const srBounder& other);
    srBounder& operator=(const srBounder& other);

    static const char* sGetClassName();

    virtual void dump(std::ostream& stream) override;
    virtual ~srBounder() override;
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;
    virtual void updateBounds() override;

    void forceUpdateBounds();
    e_boundMode getBoundMode() const;
    void setBoundMode(e_boundMode mode);
    void getBounds(BoundInfo& bounds);
    void setBounds(const BoundInfo& bounds);

private:
    void checkBounds();
    void getChildBoundingBox(srNode* node);

    e_boundMode bound_mode_138_;
    BoundInfo bounds_13c_;
    unsigned long unknown_168_[16];
};

static_assert((sizeof(srBounder) == 0x1a8), "srBounder_must_be_0x1a8");
