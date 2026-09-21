#include "surrender/srNode.h"

// GLOBAL: SURRENDER 0x100A49E0
srCriticalSection srNode::sceneGraphCSect;

// GLOBAL: SURRENDER 0x100A49FC
long srNode::sceneGraphLockCount;

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

// FUNCTION: SURRENDER 0x10050710
long srNode::getHierarchyLevel() const
{
    long level = 0;
    for (srNode* node = parent_; node != 0; node = node->parent_) {
        ++level;
    }
    return level;
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
