#include "surrender/srNode.h"

#include "surrender/srCore.h"

// GLOBAL: SURRENDER 0x100A49E0
srCriticalSection srNode::sceneGraphCSect;

// GLOBAL: SURRENDER 0x100A49FC
long srNode::sceneGraphLockCount;

/* Comma-separated flag/notify name lists dumped beside the bit values. Both
   are zero-initialized on disk; the constructor lazily installs the flag
   names. */
// GLOBAL: SURRENDER 0x100A4A00
static const char* s_flag_names_100a4a00;

// GLOBAL: SURRENDER 0x100A4A04
static const char* s_notify_names_100a4a04;

// FUNCTION: SURRENDER 0x10050340
void srNode::lockSceneGraph()
{
    sceneGraphCSect.getAccess();
    ++sceneGraphLockCount;
}

// FUNCTION: SURRENDER 0x10050360
void srNode::unlockSceneGraph()
{
    sceneGraphCSect.releaseAccess();
    --sceneGraphLockCount;
}

// FUNCTION: SURRENDER 0x10050380
int srNode::isSceneGraphLocked()
{
    return sceneGraphLockCount != 0;
}

// FUNCTION: SURRENDER 0x10050390
void srNode::notifyDependent()
{
    notifyParents(srFlags<e_notify>(0xffffffff));
    notifyChildren(srFlags<e_notify>(0xffffffff));
}

// FUNCTION: SURRENDER 0x100503C0
void srNode::notifyChildren(const srFlags<e_notify>& notifications)
{
    /* Only the bits this node has not seen yet propagate; the new set is
       re-masked at every recursion level. */
    unsigned long pending = ~notifications_120.value & notifications.value;
    if (pending != 0) {
        notifications_120.value |= pending;
        for (srNode* child = first_child_; child != 0; child = child->next_sibling_) {
            child->notifyChildren(srFlags<e_notify>(pending));
        }
    }
}

// FUNCTION: SURRENDER 0x10050410
void srNode::notifyParents(const srFlags<e_notify>& notifications)
{
    unsigned long pending = ~notifications_120.value & notifications.value;
    if (pending != 0) {
        notifications_120.value |= pending;
        if (parent_ != 0) {
            parent_->notifyParents(srFlags<e_notify>(pending));
        }
    }
}

// FUNCTION: SURRENDER 0x10050450
void srNode::setNotify(e_notify notification)
{
    notifications_120.value |= 1 << notification;
}

// FUNCTION: SURRENDER 0x10050470
void srNode::clearNotify(e_notify notification)
{
    notifications_120.value &= ~(1 << notification);
}

// FUNCTION: SURRENDER 0x10050490
int srNode::testNotify(e_notify notification) const
{
    return (notifications_120.value & (1 << notification)) != 0;
}

// FUNCTION: SURRENDER 0x100504B0
void srNode::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE) == 0 && first_child_ != 0) {
        first_child_->traverse(info);
    }
}

// FUNCTION: SURRENDER 0x100504F0
void srNode::process(const ProcessInfo& info, e_processType type) {}

// FUNCTION: SURRENDER 0x10050500
void srNode::updateBounds()
{
    clearNotify(NOTIFY_POSITIONAL_0);
}

// FUNCTION: SURRENDER 0x10050510
void srNode::getLocalBounds(BoundInfo& bounds)
{
    bounds.minimum.SetZero();
    bounds.maximum.SetZero();
    bounds.center.SetZero();
    bounds.radius = 0.0f;
    if (testFlag(FLAG_GLOBAL) != 0) {
        bounds.state_28 = 2;
        return;
    }
    bounds.state_28 = 0;
}

// FUNCTION: SURRENDER 0x100505E0
srNode& srNode::operator=(const srNode& other)
{
    if (this != &other) {
        sceneGraphCSect.getAccess();
        srClass::operator=(other);
        flags_124.value = other.flags_124.value;
        location_60 = other.location_60;
        scale_78 = other.scale_78;
        rotation_18 = other.rotation_18;
        notifyDependent();
        sceneGraphCSect.releaseAccess();
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10050690
int srNode::processSignal(unsigned long signal, void* value)
{
    return 1;
}

// FUNCTION: SURRENDER 0x100506A0
void srNode::signalInternal(unsigned long signal, void* value)
{
    /* Depth-first walk: at each level the sibling chain is recursed first,
       then the first-child chain is iterated while processSignal keeps
       returning nonzero. */
    srNode* node = this;
    do {
        if (node->next_sibling_ != 0) {
            node->next_sibling_->signalInternal(signal, value);
        }
        if (node->processSignal(signal, value) == 0) {
            return;
        }
        node = node->first_child_;
    } while (node != 0);
}

// FUNCTION: SURRENDER 0x100506E0
void srNode::signal(unsigned long signal, void* value)
{
    if (processSignal(signal, value) != 0 && first_child_ != 0) {
        first_child_->signalInternal(signal, value);
    }
}

// FUNCTION: SURRENDER 0x10050710
long srNode::getHierarchyLevel() const
{
    long level = 0;
    for (srNode* node = parent_; node != 0; node = node->parent_) {
        ++level;
    }
    return level;
}

// FUNCTION: SURRENDER 0x10050C10
srNode::srNode(srNode* parent)
{
    world_transform_90.SetIdentity();
    world_transform_f0.SetIdentity();
    if (s_flag_names_100a4a00 == 0) {
        s_flag_names_100a4a00 = "DISABLE,TERMINATE,GLOBAL,IGNORE";
    }
    sceneGraphCSect.getAccess();
    next_sibling_ = 0;
    previous_sibling_ = 0;
    parent_ = 0;
    first_child_ = 0;
    location_60.Set(0.0, 0.0, 0.0);
    rotation_18.SetRows(srVector3T<double>(1.0, 0.0, 0.0), srVector3T<double>(0.0, 1.0, 0.0),
                        srVector3T<double>(0.0, 0.0, 1.0));
    scale_78.Set(1.0, 1.0, 1.0);
    notifications_120.value = 0;
    setParent(parent, 0);
    sceneGraphCSect.releaseAccess();
}

/* Destroying a node detaches it and deletes its whole child list through the
   deleting destructor at vtable slot 5. The support base runs the registry
   unregister after the scene-graph lock is released. */
// FUNCTION: SURRENDER 0x10050E20
srNode::~srNode()
{
    sceneGraphCSect.getAccess();
    unlink();
    while (first_child_ != 0) {
        delete first_child_;
    }
    sceneGraphCSect.releaseAccess();
}

/* Reparenting under the scene-graph lock; preserve_world_transform decomposes
   the current world transform into the local slots before unlinking, then
   rebuilds them against the new parent's inverse basis (orthonormalized by
   Gram-Schmidt) so the node's world placement survives the move. */
// FUNCTION: SURRENDER 0x10050F00
int srNode::setParent(srNode* parent, int preserve_world_transform)
{
    sceneGraphCSect.getAccess();
    if (parent == this) {
        sceneGraphCSect.releaseAccess();
        return 0;
    }
    if (parent != 0 && isParentOf(*parent)) {
        sceneGraphCSect.releaseAccess();
        return 0;
    }
    if (preserve_world_transform != 0) {
        getWorldSpaceCoordinates(rotation_18, location_60, scale_78);
    }
    unlink();
    if (parent != 0 || ((parent = srCore.getRootNode()) != this && parent != 0)) {
        parent_ = parent;
        next_sibling_ = parent->first_child_;
        if (next_sibling_ != 0) {
            next_sibling_->previous_sibling_ = this;
        }
        parent->first_child_ = this;
        if (preserve_world_transform != 0) {
            srMatrix3T<double> parent_rotation;
            srVector3T<double> parent_location;
            srVector3T<double> parent_scale;
            parent->getWorldSpaceCoordinates(parent_rotation, parent_location, parent_scale);
            srVector3T<double> columns[3];
            columns[0].x = parent_rotation.vectors[0].x;
            columns[0].y = parent_rotation.vectors[1].x;
            columns[0].z = parent_rotation.vectors[2].x;
            columns[1].x = parent_rotation.vectors[0].y;
            columns[1].y = parent_rotation.vectors[1].y;
            columns[1].z = parent_rotation.vectors[2].y;
            columns[2].x = parent_rotation.vectors[0].z;
            columns[2].y = parent_rotation.vectors[1].z;
            columns[2].z = parent_rotation.vectors[2].z;
            srVector3T<double> rows[3];
            for (int row = 0; row < 3; ++row) {
                rows[row].x = DotProduct(columns[0], rotation_18.vectors[row]);
                rows[row].y = DotProduct(columns[1], rotation_18.vectors[row]);
                rows[row].z = DotProduct(columns[2], rotation_18.vectors[row]);
            }
            rotation_18.SetRows(rows[0], rows[1], rows[2]);
            for (int axis = 0; axis < 3; ++axis) {
                for (int prior = 0; prior < axis; ++prior) {
                    rotation_18.vectors[axis] -=
                        rotation_18.vectors[prior] *
                        DotProduct(rotation_18.vectors[axis], rotation_18.vectors[prior]);
                }
                rotation_18.vectors[axis] /= rotation_18.vectors[axis].Length();
            }
            srVector3T<double> inverse_scale;
            inverse_scale.x = 1.0 / parent_scale.x;
            inverse_scale.y = 1.0 / parent_scale.y;
            inverse_scale.z = 1.0 / parent_scale.z;
            location_60 -= parent_location;
            location_60.x *= inverse_scale.x;
            location_60.y *= inverse_scale.y;
            location_60.z *= inverse_scale.z;
            location_60 = parent_rotation.TransformTransposed(location_60);
            scale_78.x *= inverse_scale.x;
            scale_78.y *= inverse_scale.y;
            scale_78.z *= inverse_scale.z;
        }
    }
    notifyDependent();
    setWSDirty();
    if (testNotify(static_cast<e_notify>(1)) != 0) {
        updateTransformation();
    }
    sceneGraphCSect.releaseAccess();
    return 1;
}

// FUNCTION: SURRENDER 0x10050B70
long srNode::getChildCount() const
{
    long count = 0;
    for (srNode* child = first_child_; child != 0; child = child->next_sibling_) {
        count += 1 + child->getChildCount();
    }
    return count;
}

// FUNCTION: SURRENDER 0x10050BA0
void srNode::unlink()
{
    if (previous_sibling_ != 0) {
        previous_sibling_->next_sibling_ = next_sibling_;
    }
    if (next_sibling_ != 0) {
        next_sibling_->previous_sibling_ = previous_sibling_;
    }
    if (parent_ != 0 && parent_->first_child_ == this) {
        parent_->first_child_ = next_sibling_;
    }
    previous_sibling_ = 0;
    next_sibling_ = 0;
    parent_ = 0;
    notifyDependent();
}

// FUNCTION: SURRENDER 0x10051370
int srNode::isChildOf(const srNode& node) const
{
    const srNode* current = this;
    while (current != 0) {
        if (current == &node) {
            return 1;
        }
        current = current->parent_;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100513A0
int srNode::isParentOf(const srNode& node) const
{
    /* Retail guards a null pointer arriving through the reference. */
    const srNode* target = &node;
    if (target == 0) {
        return 0;
    }
    return target->isChildOf(*this);
}

// FUNCTION: SURRENDER 0x10051A20
srNode* srNode::getChild() const
{
    return first_child_;
}

// FUNCTION: SURRENDER 0x10051A30
srNode* srNode::getNext() const
{
    return next_sibling_;
}

// FUNCTION: SURRENDER 0x10051A40
srNode* srNode::getParent() const
{
    return parent_;
}

// FUNCTION: SURRENDER 0x10051A50
srNode* srNode::getPrev() const
{
    return previous_sibling_;
}

// FUNCTION: SURRENDER 0x10051A60
void srNode::setScale(double scale)
{
    setScale(srVector3T<double>(scale, scale, scale));
}

// FUNCTION: SURRENDER 0x10051A90
void srNode::checkTransformation() const
{
    if ((notifications_120.value & 2) != 0) {
        updateTransformation();
    }
}

// FUNCTION: SURRENDER 0x10051C50
void srNode::setFlag(e_flag flag)
{
    flags_124.value |= 1 << flag;
    if (flag == FLAG_IGNORE_TRANSFORM) {
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10051C80
void srNode::clearFlag(e_flag flag)
{
    flags_124.value &= ~(1 << flag);
    if (flag == FLAG_IGNORE_TRANSFORM) {
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10051CB0
int srNode::testFlag(e_flag flag) const
{
    return (flags_124.value & (1 << flag)) != 0;
}

// FUNCTION: SURRENDER 0x10051CD0
srClass* srNode::vInstance()
{
    return new srNode(0);
}

// FUNCTION: SURRENDER 0x10051D30
const char* srNode::sGetClassName()
{
    return "srNode";
}

// FUNCTION: SURRENDER 0x10051D40
void srNode::setWSDirty()
{
    /* The dirty fan-out to parents and children can rewrite the notification
       word, so retail saves the positional bit first and restores it after. */
    int was_positional = testNotify(NOTIFY_POSITIONAL_0);
    notifyParents(srFlags<e_notify>(1));
    notifyChildren(srFlags<e_notify>(2));
    notifications_120.set(NOTIFY_POSITIONAL_0, was_positional);
}

// FUNCTION: SURRENDER 0x10051DB0
double srNode::getDistance(const srNode& node) const
{
    srVector3T<double> other = node.getWorldSpaceLocation();
    return (getWorldSpaceLocation() - other).Length();
}

// FUNCTION: SURRENDER 0x100536B0
void srNode::setRotation(const srMatrix3T<double>& rotation)
{
    rotation_18 = rotation;
    setWSDirty();
}

// FUNCTION: SURRENDER 0x100536D0
void srNode::setRotation(const srMatrix3T<float>& rotation)
{
    /* Retail widens through a double temporary before assigning. */
    srMatrix3T<double> widened;
    widened.vectors[0].x = rotation.vectors[0].x;
    widened.vectors[0].y = rotation.vectors[0].y;
    widened.vectors[0].z = rotation.vectors[0].z;
    widened.vectors[1].x = rotation.vectors[1].x;
    widened.vectors[1].y = rotation.vectors[1].y;
    widened.vectors[1].z = rotation.vectors[1].z;
    widened.vectors[2].x = rotation.vectors[2].x;
    widened.vectors[2].y = rotation.vectors[2].y;
    widened.vectors[2].z = rotation.vectors[2].z;
    rotation_18 = widened;
    setWSDirty();
}

/* Euler form: reset to identity, then apply each nonzero axis rotation in
   X, Y, Z order through the matrix's own guarded helpers. */
// FUNCTION: SURRENDER 0x10053740
void srNode::setRotation(double x, double y, double z)
{
    rotation_18.SetIdentity();
    rotation_18.RotateAboutX(x);
    rotation_18.RotateAboutY(y);
    rotation_18.RotateAboutZ(z);
    setWSDirty();
}

// FUNCTION: SURRENDER 0x10053D70
double srNode::getLocationX() const
{
    return location_60.x;
}

// FUNCTION: SURRENDER 0x10053D80
double srNode::getLocationY() const
{
    return location_60.y;
}

// FUNCTION: SURRENDER 0x10053D90
double srNode::getLocationZ() const
{
    return location_60.z;
}

// FUNCTION: SURRENDER 0x10053DA0
srVector3T<double> srNode::getLocation() const
{
    return location_60;
}

// FUNCTION: SURRENDER 0x10053DD0
void srNode::getLocation(srVector3T<double>& location) const
{
    location = location_60;
}

// FUNCTION: SURRENDER 0x10053DF0
void srNode::getLocation(srVector3T<float>& location) const
{
    location.Set(location_60.x, location_60.y, location_60.z);
}

// FUNCTION: SURRENDER 0x10053E10
void srNode::getRotation(srMatrix3T<float>& rotation) const
{
    /* Retail narrows into a float temporary, then copy-assigns the result. */
    srMatrix3T<float> narrowed;
    narrowed.vectors[0].x = static_cast<float>(rotation_18.vectors[0].x);
    narrowed.vectors[0].y = static_cast<float>(rotation_18.vectors[0].y);
    narrowed.vectors[0].z = static_cast<float>(rotation_18.vectors[0].z);
    narrowed.vectors[1].x = static_cast<float>(rotation_18.vectors[1].x);
    narrowed.vectors[1].y = static_cast<float>(rotation_18.vectors[1].y);
    narrowed.vectors[1].z = static_cast<float>(rotation_18.vectors[1].z);
    narrowed.vectors[2].x = static_cast<float>(rotation_18.vectors[2].x);
    narrowed.vectors[2].y = static_cast<float>(rotation_18.vectors[2].y);
    narrowed.vectors[2].z = static_cast<float>(rotation_18.vectors[2].z);
    rotation = narrowed;
}

// FUNCTION: SURRENDER 0x10053E70
void srNode::getRotation(srMatrix3T<double>& rotation) const
{
    rotation = rotation_18;
}

// FUNCTION: SURRENDER 0x10053E90
srVector3T<double> srNode::getScale() const
{
    return scale_78;
}

// FUNCTION: SURRENDER 0x10053EB0
void srNode::offsetLocation(const srVector3T<double>& offset)
{
    if (offset.x != 0.0 || offset.y != 0.0 || offset.z != 0.0) {
        location_60.x += offset.x;
        location_60.y += offset.y;
        location_60.z += offset.z;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053F10
void srNode::offsetLocation(double x, double y, double z)
{
    if (x != 0.0 || y != 0.0 || z != 0.0) {
        location_60.x += x;
        location_60.y += y;
        location_60.z += z;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054790
void srNode::setLocation(const srVector3T<double>& location)
{
    if (location.x != location_60.x || location.y != location_60.y || location.z != location_60.z) {
        location_60 = location;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100547E0
void srNode::setLocation(double x, double y, double z)
{
    if (x != location_60.x || y != location_60.y || z != location_60.z) {
        location_60.x = x;
        location_60.y = y;
        location_60.z = z;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054840
void srNode::setLocationX(double x)
{
    if (x != location_60.x) {
        location_60.x = x;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054870
void srNode::setLocationY(double y)
{
    if (y != location_60.y) {
        location_60.y = y;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100548A0
void srNode::setLocationZ(double z)
{
    if (z != location_60.z) {
        location_60.z = z;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100548D0
void srNode::setScale(const srVector3T<double>& scale)
{
    if (scale_78.x != scale.x || scale_78.y != scale.y || scale_78.z != scale.z) {
        scale_78 = scale;
        setWSDirty();
    }
}
