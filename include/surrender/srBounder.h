#pragma once

#include "srNode.h"

// VTABLE: SURRENDER 0x10076F64
// class srClassSupport<srBounder, srNode, 0, 5632>

// VTABLE: SURRENDER 0x10076f30 srBounder
class SR_DLL_EXPORT srBounder : public srClassSupport<srBounder, srNode, false, 0x1600> {
public:
    enum e_boundMode { BOUND_MODE_DYNAMIC = 0 };

    srBounder(srNode* parent = 0);

    srBounder& operator=(const srBounder& other);

    // FUNCTION: SURRENDER 0x1004B070
    static const char* sGetClassName()
    {
        return "srBounder";
    }

    /* Empty derived destruction is the implicit srNode-base teardown. */

    virtual void dump(std::ostream& stream) override;
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

    e_boundMode bound_mode;
    BoundInfo bounds;
    /* updateBounds stores the inverse of this node's world matrix here;
       getChildBoundingBox composes it with each child's world matrix to bring
       child bounding boxes into bounder space. */
    srMatrix4T<float> inverse_world;
};

W8_ABI_ASSERT((sizeof(srBounder) == 0x1a8), "srBounder_must_be_0x1a8");
