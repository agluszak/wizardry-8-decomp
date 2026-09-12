#include "surrender/srBounder.h"

#include "surrender/srCore.h"

// FUNCTION: SURRENDER 0x1004a5a0
srBounder::srBounder(srNode* parent) : srClassSupport(static_cast<srNode*>(0))
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1600);
    if (node == 0) {
        node = registry->getClassNode(0x1000);
        if (node == 0) {
            node = registry->registerClass(srNode::sGetClassName(), srClass::sGetClassNode(),
                                           0x1000, 1);
        }
        node = registry->registerClass(sGetClassName(), node, 0x1600, 0);
    }
    registry->registerInstance(node, this);
    bound_mode_138_ = BOUND_MODE_POSITIONAL_0;
    bounds_13c_.unknown_28_ = 2;
    if (parent != 0) {
        setParent(parent, 0);
    }
    setNotify(NOTIFY_POSITIONAL_0);
}

// FUNCTION: SURRENDER 0x1004b2e0
srBounder::~srBounder()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1600);
    if (node == 0) {
        node = registry->getClassNode(0x1000);
        if (node == 0) {
            node = registry->registerClass(srNode::sGetClassName(), srClass::sGetClassNode(),
                                           0x1000, 1);
        }
        node = registry->registerClass(sGetClassName(), node, 0x1600, 0);
    }
    registry->unregisterInstance(node, this);
}

// FUNCTION: SURRENDER 0x1004b080
srClass* srBounder::vInstance()
{
    return new srBounder(static_cast<srNode*>(0));
}

// FUNCTION: SURRENDER 0x1004b070
const char* srBounder::sGetClassName()
{
    return "srBounder";
}

// FUNCTION: SURRENDER 0x1004b1a0
srBounder::e_boundMode srBounder::getBoundMode() const
{
    return bound_mode_138_;
}

// FUNCTION: SURRENDER 0x1004b160
void srBounder::setBoundMode(e_boundMode mode)
{
    if (testNotify(NOTIFY_POSITIONAL_0)) {
        updateBounds();
    }
    bound_mode_138_ = mode;
}

// FUNCTION: SURRENDER 0x1004b0e0
void srBounder::getBounds(BoundInfo& bounds)
{
    if (bound_mode_138_ == BOUND_MODE_POSITIONAL_0) {
        if (testNotify(NOTIFY_POSITIONAL_0)) {
            updateBounds();
        }
    }
    bounds = bounds_13c_;
}

// FUNCTION: SURRENDER 0x1004b120
void srBounder::setBounds(const BoundInfo& bounds)
{
    if (bound_mode_138_ == BOUND_MODE_POSITIONAL_0) {
        if (testNotify(NOTIFY_POSITIONAL_0)) {
            updateBounds();
        }
    }
    bounds_13c_ = bounds;
}

// FUNCTION: SURRENDER 0x1004b1b0
void srBounder::checkBounds()
{
    if (bound_mode_138_ == BOUND_MODE_POSITIONAL_0) {
        if (testNotify(NOTIFY_POSITIONAL_0)) {
            updateBounds();
        }
    }
}

// FUNCTION: SURRENDER 0x1004aa00
void srBounder::forceUpdateBounds()
{
    e_boundMode mode;

    setNotify(NOTIFY_POSITIONAL_0);
    mode = bound_mode_138_;
    bound_mode_138_ = BOUND_MODE_POSITIONAL_0;
    updateBounds();
    bound_mode_138_ = mode;
}
