#include "surrender/srBounder.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"

// FUNCTION: SURRENDER 0x1004a5a0
srBounder::srBounder(srNode* parent)
    : srClassSupport<srBounder, srNode, false, 0x1600>(static_cast<srNode*>(0))
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
    bounds_13c_.state_28 = 2;
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

// FUNCTION: SURRENDER 0x1004B1E0
srBounder::srBounder(const srBounder& other)
    : srClassSupport<srBounder, srNode, false, 0x1600>(static_cast<srNode*>(0))
{
    *this = other;
    bound_mode_138_ = other.bound_mode_138_;
    bounds_13c_ = other.bounds_13c_;
    inverse_world_168_ = other.inverse_world_168_;
}

// FUNCTION: SURRENDER 0x1004A690
srBounder& srBounder::operator=(const srBounder& other)
{
    if (this != &other) {
        srNode::operator=(other);
        bound_mode_138_ = other.bound_mode_138_;
        bounds_13c_ = other.bounds_13c_;
        setNotify(NOTIFY_POSITIONAL_0);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004A6E0
void srBounder::process(const ProcessInfo& info, e_processType type) {}

// FUNCTION: SURRENDER 0x1004A6F0
void srBounder::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE) == 0) {
        srGERD* renderer = info.renderer;
        if (testFlag(FLAG_DISABLE) == 0 && renderer != 0 && firstChild() != 0) {
            if (bound_mode_138_ == BOUND_MODE_POSITIONAL_0 &&
                testNotify(NOTIFY_POSITIONAL_0) != 0) {
                updateBounds();
            }
            if (bounds_13c_.state_28 == 0) {
                return;
            }
            if (bounds_13c_.state_28 == 1) {
                applyWorldSpaceMatrix(*renderer);
                if (renderer->testBoundingSphere(bounds_13c_.center, bounds_13c_.radius) ==
                        srGERD::VISIBILITY_POSITIONAL_0 ||
                    renderer->testBoundingBox(bounds_13c_.minimum, bounds_13c_.maximum) ==
                        srGERD::VISIBILITY_POSITIONAL_0) {
                    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
                    renderer->popMatrix();
                    return;
                }
                renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
                renderer->popMatrix();
            }
        }
        if (firstChild() != 0) {
            firstChild()->traverse(info);
        }
    }
}

// FUNCTION: SURRENDER 0x1004A800
void srBounder::updateBounds()
{
    if (testNotify(NOTIFY_POSITIONAL_0) != 0 && bound_mode_138_ == BOUND_MODE_POSITIONAL_0) {
        bounds_13c_.minimum.SetZero();
        bounds_13c_.maximum.SetZero();
        bounds_13c_.center.SetZero();
        bounds_13c_.radius = 0.0f;
        bounds_13c_.state_28 = 0;
        srMatrix4T<float> world;
        getWorldSpaceMatrix(world);
        inverse_world_168_.Inverse(world);
        for (srNode* child = firstChild(); child != 0; child = child->getNext()) {
            getChildBoundingBox(child);
        }
        if (bounds_13c_.state_28 == 1) {
            bounds_13c_.center.x = (bounds_13c_.maximum.x + bounds_13c_.minimum.x) * 0.5f;
            bounds_13c_.center.y = (bounds_13c_.maximum.y + bounds_13c_.minimum.y) * 0.5f;
            bounds_13c_.center.z = (bounds_13c_.maximum.z + bounds_13c_.minimum.z) * 0.5f;
            float dx = bounds_13c_.minimum.x - bounds_13c_.center.x;
            float dy = bounds_13c_.minimum.y - bounds_13c_.center.y;
            float dz = bounds_13c_.minimum.z - bounds_13c_.center.z;
            bounds_13c_.radius = (float)sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    clearNotify(NOTIFY_POSITIONAL_0);
}

// FUNCTION: SURRENDER 0x1004AC60
void srBounder::getChildBoundingBox(srNode* node)
{
    node->updateBounds();
    if (bounds_13c_.state_28 != 2) {
        BoundInfo child_bounds;
        node->getLocalBounds(child_bounds);
        if (child_bounds.state_28 == 1) {
            srMatrix4T<float> world;
            node->getWorldSpaceMatrix(world);
            srMatrix4T<float> combined;
            inverse_world_168_.Multiply(world, combined);
            srVector3T<float> corners[8];
            corners[0].Set(child_bounds.minimum.x, child_bounds.minimum.y, child_bounds.minimum.z);
            corners[1].Set(child_bounds.minimum.x, child_bounds.minimum.y, child_bounds.maximum.z);
            corners[2].Set(child_bounds.minimum.x, child_bounds.maximum.y, child_bounds.minimum.z);
            corners[3].Set(child_bounds.minimum.x, child_bounds.maximum.y, child_bounds.maximum.z);
            corners[4].Set(child_bounds.maximum.x, child_bounds.minimum.y, child_bounds.minimum.z);
            corners[5].Set(child_bounds.maximum.x, child_bounds.minimum.y, child_bounds.maximum.z);
            corners[6].Set(child_bounds.maximum.x, child_bounds.maximum.y, child_bounds.minimum.z);
            corners[7].Set(child_bounds.maximum.x, child_bounds.maximum.y, child_bounds.maximum.z);
            for (int index = 0; index != 8; ++index) {
                corners[index] = combined.TransformPoint(corners[index]);
            }
            if (bounds_13c_.state_28 == 0) {
                bounds_13c_.minimum = corners[0];
                bounds_13c_.maximum = corners[0];
            }
            for (int corner = 0; corner != 8; ++corner) {
                const float* point = &corners[corner].x;
                float* bound_min = &bounds_13c_.minimum.x;
                float* bound_max = &bounds_13c_.maximum.x;
                for (int axis = 0; axis != 3; ++axis) {
                    if (point[axis] < bound_min[axis]) {
                        bound_min[axis] = point[axis];
                    }
                    if (bound_max[axis] < point[axis]) {
                        bound_max[axis] = point[axis];
                    }
                }
            }
            bounds_13c_.state_28 = 1;
        } else if (child_bounds.state_28 == 2) {
            bounds_13c_.state_28 = 2;
            return;
        }
    }
    for (srNode* child = node->getChild(); child != 0; child = child->getNext()) {
        getChildBoundingBox(child);
    }
}

// FUNCTION: SURRENDER 0x1004AA30
void srBounder::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Bound mode: ";
    stream << (bound_mode_138_ == 1 ? "static\n" : "dynamic\n");
    if (testNotify(NOTIFY_POSITIONAL_0) != 0) {
        stream << "Bounds not calculated.\n";
    } else if (bounds_13c_.state_28 == 0) {
        stream << "Bounding area empty\n";
    } else if (bounds_13c_.state_28 == 2) {
        stream << "Bounds contain an infinite-size child (VOLUME_INFINITE) - bounder "
                  "deactivated\n";
    } else {
        stream.width(0x20);
        stream << "  Bounding box min: ";
        stream << '{' << bounds_13c_.minimum.x << ',' << bounds_13c_.minimum.y << ','
               << bounds_13c_.minimum.z << '}' << '\n';
        stream.width(0x20);
        stream << "  Bounding box max: ";
        stream << '{' << bounds_13c_.maximum.x << ',' << bounds_13c_.maximum.y << ','
               << bounds_13c_.maximum.z << '}' << '\n';
        stream.width(0x20);
        stream << "  Bounding sphere origin: ";
        stream << '{' << bounds_13c_.center.x << ',' << bounds_13c_.center.y << ','
               << bounds_13c_.center.z << '}' << '\n';
        stream.width(0x20);
        stream << "  Bounding sphere radius: " << bounds_13c_.radius << '\n';
    }
    stream.flags(flags & 0x7fff);
}

// SYNTHETIC: SURRENDER 0x1004B3A0
// srBounder default constructor closure

// TEMPLATE: SURRENDER 0X1004AF90
// srClassSupport<srBounder, srNode, false, 0x1600>::clone

// TEMPLATE: SURRENDER 0X1004AFB0
// srClassSupport<srBounder, srNode, false, 0x1600>::~srClassSupport

// SYNTHETIC: SURRENDER 0X1004B3B0
// srBounder scalar deleting destructor

// SYNTHETIC: SURRENDER 0X1004B440
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X1004B450
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X1004B480
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X1004B490
// std::_Winit global atexit registrar

// SYNTHETIC: SURRENDER 0X1004B4B0
// srClassSupport<srBounder, srNode, false, 0x1600> scalar deleting destructor
