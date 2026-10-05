#include "surrender/srBounder.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"

#include <ostream>

// FUNCTION: SURRENDER 0x1004a5a0
srBounder::srBounder(srNode* parent)
    : srClassSupport<srBounder, srNode, false, 0x1600>(static_cast<srNode*>(0))
{
    bound_mode = BOUND_MODE_DYNAMIC;
    bounds.state = 2;
    if (parent != 0) {
        setParent(parent, 0);
    }
    setNotify(NOTIFY_BOUNDS_DIRTY);
}

// FUNCTION: SURRENDER 0x1004b080
srClass* srBounder::vInstance()
{
    return new srBounder(static_cast<srNode*>(0));
}

// FUNCTION: SURRENDER 0x1004b1a0
srBounder::e_boundMode srBounder::getBoundMode() const
{
    return bound_mode;
}

// FUNCTION: SURRENDER 0x1004b160
void srBounder::setBoundMode(e_boundMode mode)
{
    if (testNotify(NOTIFY_BOUNDS_DIRTY)) {
        updateBounds();
    }
    bound_mode = mode;
}

// FUNCTION: SURRENDER 0x1004b0e0
void srBounder::getBounds(BoundInfo& bounds)
{
    if (bound_mode == BOUND_MODE_DYNAMIC) {
        if (testNotify(NOTIFY_BOUNDS_DIRTY)) {
            updateBounds();
        }
    }
    bounds = this->bounds;
}

// FUNCTION: SURRENDER 0x1004b120
void srBounder::setBounds(const BoundInfo& bounds)
{
    if (bound_mode == BOUND_MODE_DYNAMIC) {
        if (testNotify(NOTIFY_BOUNDS_DIRTY)) {
            updateBounds();
        }
    }
    this->bounds = bounds;
}

// FUNCTION: SURRENDER 0x1004b1b0
void srBounder::checkBounds()
{
    if (bound_mode == BOUND_MODE_DYNAMIC) {
        if (testNotify(NOTIFY_BOUNDS_DIRTY)) {
            updateBounds();
        }
    }
}

// FUNCTION: SURRENDER 0x1004aa00
void srBounder::forceUpdateBounds()
{
    e_boundMode mode;

    setNotify(NOTIFY_BOUNDS_DIRTY);
    mode = bound_mode;
    bound_mode = BOUND_MODE_DYNAMIC;
    updateBounds();
    bound_mode = mode;
}

// FUNCTION: SURRENDER 0x1004A690
srBounder& srBounder::operator=(const srBounder& other)
{
    if (this != &other) {
        srNode::operator=(other);
        bound_mode = other.bound_mode;
        bounds = other.bounds;
        setNotify(NOTIFY_BOUNDS_DIRTY);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004A6E0
void srBounder::process(const ProcessInfo& info, e_processType type) {}

// FUNCTION: SURRENDER 0x1004A6F0
void srBounder::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE) == 0) {
        srGERD* renderer = info.renderer;
        if (testFlag(FLAG_DISABLE) == 0 && renderer != 0 && first_child_ != 0) {
            if (bound_mode == BOUND_MODE_DYNAMIC && testNotify(NOTIFY_BOUNDS_DIRTY) != 0) {
                updateBounds();
            }
            if (bounds.state == 0) {
                return;
            }
            if (bounds.state == 1) {
                applyWorldSpaceMatrix(*renderer);
                if (renderer->testBoundingSphere(bounds.center, bounds.radius) ==
                    srGERD::VISIBILITY_OUTSIDE) {
                    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
                    renderer->popMatrix();
                    return;
                }
                if (renderer->testBoundingBox(bounds.minimum, bounds.maximum) ==
                    srGERD::VISIBILITY_OUTSIDE) {
                    renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
                    renderer->popMatrix();
                    return;
                }
                renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
                renderer->popMatrix();
            }
        }
        if (first_child_ != 0) {
            first_child_->traverse(info);
        }
    }
}

// FUNCTION: SURRENDER 0x1004A800
void srBounder::updateBounds()
{
    if (testNotify(NOTIFY_BOUNDS_DIRTY) != 0 && bound_mode == BOUND_MODE_DYNAMIC) {
        bounds.minimum.SetZero();
        bounds.maximum.SetZero();
        bounds.center.SetZero();
        bounds.radius = 0.0f;
        bounds.state = 0;
        srMatrix4T<float> world;
        getWorldSpaceMatrix(world);
        inverse_world.Inverse(world);
        for (srNode* child = first_child_; child != 0; child = child->getNext()) {
            getChildBoundingBox(child);
        }
        if (bounds.state == 1) {
            bounds.center.x = (bounds.maximum.x + bounds.minimum.x) * 0.5;
            bounds.center.y = (bounds.maximum.y + bounds.minimum.y) * 0.5;
            bounds.center.z = (bounds.maximum.z + bounds.minimum.z) * 0.5;
            float dx = bounds.minimum.x - bounds.center.x;
            float dy = bounds.minimum.y - bounds.center.y;
            float dz = bounds.minimum.z - bounds.center.z;
            bounds.radius = (float)sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    clearNotify(NOTIFY_BOUNDS_DIRTY);
}

// FUNCTION: SURRENDER 0x1004AC60
void srBounder::getChildBoundingBox(srNode* node)
{
    node->updateBounds();
    if (bounds.state != 2) {
        BoundInfo child_bounds;
        node->getLocalBounds(child_bounds);
        if (child_bounds.state == 1) {
            srMatrix4T<float> world;
            node->getWorldSpaceMatrix(world);
            srMatrix4T<float> combined;
            inverse_world.Multiply(world, combined);
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
            if (bounds.state == 0) {
                bounds.minimum = corners[0];
                bounds.maximum = corners[0];
            }
            for (int corner = 0; corner != 8; ++corner) {
                const float* point = &corners[corner].x;
                float* bound_min = &bounds.minimum.x;
                float* bound_max = &bounds.maximum.x;
                for (int axis = 0; axis != 3; ++axis) {
                    if (point[axis] < bound_min[axis]) {
                        bound_min[axis] = point[axis];
                    }
                    if (bound_max[axis] < point[axis]) {
                        bound_max[axis] = point[axis];
                    }
                }
            }
            bounds.state = 1;
        } else if (child_bounds.state == 2) {
            bounds.state = 2;
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
    stream << (bound_mode == 1 ? "static\n" : "dynamic\n");
    if (testNotify(NOTIFY_BOUNDS_DIRTY) != 0) {
        stream << "Bounds not calculated.\n";
    } else if (bounds.state == 0) {
        stream << "Bounding area empty\n";
    } else if (bounds.state == 2) {
        stream << "Bounds contain an infinite-size child (VOLUME_INFINITE) - bounder "
                  "deactivated\n";
    } else {
        stream.width(0x20);
        stream << "  Bounding box min: ";
        stream << '{' << bounds.minimum.x << ',' << bounds.minimum.y << ',' << bounds.minimum.z
               << '}' << '\n';
        stream.width(0x20);
        stream << "  Bounding box max: ";
        stream << '{' << bounds.maximum.x << ',' << bounds.maximum.y << ',' << bounds.maximum.z
               << '}' << '\n';
        stream.width(0x20);
        stream << "  Bounding sphere origin: ";
        stream << '{' << bounds.center.x << ',' << bounds.center.y << ',' << bounds.center.z << '}'
               << '\n';
        stream.width(0x20);
        stream << "  Bounding sphere radius: " << bounds.radius << '\n';
    }
    stream.flags(flags & 0x7fff);
}
