#include "surrender/srNode.h"

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srGERD.h"

#include <ostream>
#include <string.h>

// GLOBAL: SURRENDER 0x100A49E0
srCriticalSection srNode::sceneGraphCSect;

// GLOBAL: SURRENDER 0x100A49FC
long srNode::sceneGraphLockCount;

/* Comma-separated flag/notify name lists dumped beside the bit values. Both
   are zero-initialized on disk; the constructor lazily installs the flag
   names. */
// GLOBAL: SURRENDER 0x100A4A00
static const char* s_flag_names1;

// GLOBAL: SURRENDER 0x100A4A04
static const char* s_notify_names;

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
    const srFlags<e_notify> notifications(0xffffffff);
    notifyParents(notifications);
    notifyChildren(notifications);
}

// FUNCTION: SURRENDER 0x100503C0
void srNode::notifyChildren(const srFlags<e_notify>& notifications)
{
    /* Only the bits this node has not seen yet propagate; the new set is
       re-masked at every recursion level. */
    unsigned long pending = ~this->notifications.value & notifications.value;
    if (pending != 0) {
        this->notifications.value |= pending;
        for (srNode* child = first_child_; child != 0; child = child->next_sibling_) {
            child->notifyChildren(srFlags<e_notify>(pending));
        }
    }
}

// FUNCTION: SURRENDER 0x10050410
void srNode::notifyParents(const srFlags<e_notify>& notifications)
{
    unsigned long pending = ~this->notifications.value & notifications.value;
    if (pending != 0) {
        this->notifications.value |= pending;
        if (parent_ != 0) {
            parent_->notifyParents(srFlags<e_notify>(pending));
        }
    }
}

// FUNCTION: SURRENDER 0x10050450
void srNode::setNotify(e_notify notification)
{
    notifications.value |= 1 << notification;
}

// FUNCTION: SURRENDER 0x10050470
void srNode::clearNotify(e_notify notification)
{
    notifications.value &= ~(1 << notification);
}

// FUNCTION: SURRENDER 0x10050490
int srNode::testNotify(e_notify notification) const
{
    return (notifications.value & (1 << notification)) != 0;
}

// FUNCTION: SURRENDER 0x100504B0
void srNode::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
    }
    if ((flags.value & (1 << FLAG_TERMINATE)) == 0 && first_child_ != 0) {
        first_child_->traverse(info);
    }
}

// FUNCTION: SURRENDER 0x100504F0
void srNode::process(const ProcessInfo& info, e_processType type) {}

// FUNCTION: SURRENDER 0x10050500
void srNode::updateBounds()
{
    clearNotify(NOTIFY_BOUNDS_DIRTY);
}

// FUNCTION: SURRENDER 0x10050510
void srNode::getLocalBounds(BoundInfo& bounds)
{
    bounds.minimum.SetZero();
    bounds.maximum.SetZero();
    bounds.center.SetZero();
    bounds.radius = 0.0f;
    if (testFlag(FLAG_GLOBAL) != 0) {
        bounds.state = 2;
        return;
    }
    bounds.state = 0;
}

// FUNCTION: SURRENDER 0x100505E0
srNode& srNode::operator=(const srNode& other)
{
    if (this != &other) {
        srCriticalSectionAccess access(&sceneGraphCSect);
        srClass::operator=(other);
        flags.value = other.flags.value;
        location = other.location;
        scale = other.scale;
        rotation = other.rotation;
        notifyDependent();
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
    world_transform0.SetIdentity();
    world_transform1.SetIdentity();
    if (s_flag_names1 == 0) {
        s_flag_names1 = "DISABLE,TERMINATE,GLOBAL,IGNORE_TRANSFORM";
    }
    srCriticalSectionAccess access(&sceneGraphCSect);
    next_sibling_ = 0;
    previous_sibling_ = 0;
    parent_ = 0;
    first_child_ = 0;
    location.SetZero();
    rotation.SetIdentity();
    scale = 1.0;
    notifications.value = 0;
    setParent(parent, 0);
}

/* Destroying a node detaches it and deletes its whole child list through the
   deleting destructor at vtable slot 5. The support base runs the registry
   unregister after the scene-graph lock is released. */
// FUNCTION: SURRENDER 0x10050E20
srNode::~srNode()
{
    srCriticalSectionAccess access(&sceneGraphCSect);
    unlink();
    while (first_child_ != 0) {
        delete first_child_;
    }
}

/* Reparenting under the scene-graph lock; preserve_world_transform decomposes
   the current world transform into the local slots before unlinking, then
   rebuilds them against the new parent's inverse basis (orthonormalized by
   Gram-Schmidt) so the node's world placement survives the move. */
// FUNCTION: SURRENDER 0x10050F00
int srNode::setParent(srNode* parent, int preserve_world_transform)
{
    srCriticalSectionAccess access(&sceneGraphCSect);
    if (parent == this) {
        return 0;
    }
    if (parent != 0 && isParentOf(*parent)) {
        return 0;
    }
    if (preserve_world_transform != 0) {
        getWorldSpaceCoordinates(rotation, location, scale);
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
            rotation.MultiplyBy(parent_rotation);
            rotation.OrthonormalizeRows();
            srVector3T<double> inverse_scale;
            inverse_scale.x = 1.0 / parent_scale.x;
            inverse_scale.y = 1.0 / parent_scale.y;
            inverse_scale.z = 1.0 / parent_scale.z;
            location -= parent_location;
            location *= inverse_scale;
            location = parent_rotation.TransformTransposed(location);
            scale *= inverse_scale;
        }
    }
    notifyDependent();
    setWSDirty();
    if (testNotify(static_cast<e_notify>(1)) != 0) {
        updateTransformation();
    }
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

// FUNCTION: SURRENDER 0x10055A00
static std::ostream& operator<<(std::ostream& stream, const srVector3T<double>& vector)
{
    return stream << '{' << vector.x << ',' << vector.y << ',' << vector.z << '}';
}

static void dumpFlags(std::ostream& stream, unsigned long flags, const char* names)
{
    if (flags == 0) {
        stream << "[NONE]";
        return;
    }
    stream << '[';
    bool first = true;
    for (unsigned long bit = 0; bit < 0x20; ++bit) {
        if ((flags & (1 << bit)) != 0) {
            if (first) {
                first = false;
            } else {
                stream << ',';
            }
            if (names == 0 || *names == 0) {
                stream << bit;
            } else {
                while (*names != 0 && *names != ',') {
                    stream << *names++;
                }
                if (*names == ',') {
                    ++names;
                }
            }
        } else if (names != 0) {
            while (*names != 0 && *names != ',') {
                ++names;
            }
            if (*names == ',') {
                ++names;
            }
        }
    }
    stream << ']';
}

// FUNCTION: SURRENDER 0x100513C0
void srNode::dump(std::ostream& stream)
{
    srClass::dump(stream);
    char* path = new char[getFullPathLength() + 1];
    getFullPath(path);
    srMatrix3T<double> rotation;
    getRotation(rotation);
    srMatrix3T<double> ws_rotation;
    srVector3T<double> ws_location;
    srVector3T<double> ws_scale;
    getWorldSpaceCoordinates(ws_rotation, ws_location, ws_scale);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  FullPath: " << path << '\n';
    stream.width(0x20);
    stream << "  Parent: " << (parent_ == 0 ? "No Parent" : parent_->getName()) << '\n';
    stream.width(0x20);
    stream << "  Child count: " << getChildCount() << '\n';
    stream.width(0x20);
    stream << "  Local matrix: " << '{' << rotation.vectors[0] << ',' << rotation.vectors[1] << ','
           << rotation.vectors[2] << '}' << '\n';
    stream.width(0x20);
    srVector3T<double> location = getLocation();
    stream << "  Local translation: " << '{' << location.x << ',' << location.y << ',' << location.z
           << '}' << '\n';
    stream.width(0x20);
    srVector3T<double> scale = getScale();
    stream << "  Local scale: " << '{' << scale.x << ',' << scale.y << ',' << scale.z << '}'
           << '\n';
    stream.width(0x20);
    stream << "  WS matrix: " << '{' << ws_rotation.vectors[0] << ',' << ws_rotation.vectors[1]
           << ',' << ws_rotation.vectors[2] << '}' << '\n';
    stream.width(0x20);
    stream << "  WS location: " << '{' << ws_location.x << ',' << ws_location.y << ','
           << ws_location.z << '}' << '\n';
    stream.width(0x20);
    stream << "  WS scale: " << '{' << ws_scale.x << ',' << ws_scale.y << ',' << ws_scale.z << '}'
           << '\n';
    stream.width(0x20);
    stream << "  Flags: ";
    dumpFlags(stream, this->flags.value, s_flag_names1);
    stream << '\n';
    stream.width(0x20);
    stream << "  Notify: ";
    dumpFlags(stream, notifications.value, s_notify_names);
    stream << '\n';
    stream.flags(flags & 0x7fff);
    delete[] path;
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
    if ((notifications.value & 2) != 0) {
        updateTransformation();
    }
}

// FUNCTION: SURRENDER 0x10051C50
void srNode::setFlag(e_flag flag)
{
    flags.value |= 1 << flag;
    if (flag == FLAG_IGNORE_TRANSFORM) {
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10051C80
void srNode::clearFlag(e_flag flag)
{
    flags.value &= ~(1 << flag);
    if (flag == FLAG_IGNORE_TRANSFORM) {
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10051CB0
int srNode::testFlag(e_flag flag) const
{
    return (flags.value & (1 << flag)) != 0;
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
    int bounds_dirty = notifications.value & (1 << NOTIFY_BOUNDS_DIRTY);
    notifyParents(srFlags<e_notify>(1));
    notifyChildren(srFlags<e_notify>(2));
    notifications.set(NOTIFY_BOUNDS_DIRTY, bounds_dirty);
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
    this->rotation = rotation;
    setWSDirty();
}

// FUNCTION: SURRENDER 0x100536D0
void srNode::setRotation(const srMatrix3T<float>& rotation)
{
    /* Retail widens through a double temporary before assigning. */
    srMatrix3T<double> widened;
    widened.vectors[0] = rotation.vectors[0];
    widened.vectors[1] = rotation.vectors[1];
    widened.vectors[2] = rotation.vectors[2];
    this->rotation = widened;
    setWSDirty();
}

/* Euler form: reset to identity, then apply each nonzero axis rotation in
   X, Y, Z order through the matrix's own guarded helpers. */
// FUNCTION: SURRENDER 0x10053740
void srNode::setRotation(double x, double y, double z)
{
    rotation.SetIdentity();
    rotation.RotateAboutX(x);
    rotation.RotateAboutY(y);
    rotation.RotateAboutZ(z);
    setWSDirty();
}

// FUNCTION: SURRENDER 0x10052750
void srNode::setRotation(double amount, const srVector3T<double>& direction)
{
    srVector3T<double> axis = direction;
    axis.Normalize();
    rotation.SetIdentity();
    if (amount != 0.0) {
        rotation.RotateAroundAxis(sin(amount), cos(amount), axis);
    }
    setWSDirty();
}

// FUNCTION: SURRENDER 0x100528C0
void srNode::setRotation(const srVector3T<double>& direction, double amount)
{
    srVector3T<double> axis = direction;
    axis.Normalize();
    rotation.SetIdentity();
    double angle = atan2(axis.x, axis.z);
    if (angle != 0.0) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }
    axis = rotation.TransformTransposed(axis);
    angle = -atan2(axis.y, axis.z);
    if (angle != 0.0) {
        rotation.RotateAboutX(sin(angle), cos(angle));
    }
    if (amount != 0.0) {
        rotation.RotateAboutZ(amount);
    }
    setWSDirty();
}

// FUNCTION: SURRENDER 0x10052B20
void srNode::setRotation(const srVector3T<double>& first, const srVector3T<double>& second,
                         double amount)
{
    srVector3T<double> axis = first - second;
    axis.Normalize();
    rotation.SetIdentity();
    double angle = atan2(axis.x, axis.z);
    if (angle != 0.0) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }
    axis = rotation.TransformTransposed(axis);
    rotation.RotateAboutX(-atan2(axis.y, axis.z));
    if (amount != 0.0) {
        rotation.RotateAboutZ(amount);
    }
    setWSDirty();
}

// FUNCTION: SURRENDER 0x10053D70
double srNode::getLocationX() const
{
    return location.x;
}

// FUNCTION: SURRENDER 0x10053D80
double srNode::getLocationY() const
{
    return location.y;
}

// FUNCTION: SURRENDER 0x10053D90
double srNode::getLocationZ() const
{
    return location.z;
}

// FUNCTION: SURRENDER 0x10053DA0
srVector3T<double> srNode::getLocation() const
{
    return location;
}

// FUNCTION: SURRENDER 0x10053DF0
void srNode::getLocation(srVector3T<float>& location) const
{
    location.Set(this->location.x, this->location.y, this->location.z);
}

// FUNCTION: SURRENDER 0x10053E10
void srNode::getRotation(srMatrix3T<float>& rotation) const
{
    /* Retail narrows into a float temporary, then copy-assigns the result. */
    srMatrix3T<float> narrowed;
    narrowed.vectors[0] = this->rotation.vectors[0];
    narrowed.vectors[1] = this->rotation.vectors[1];
    narrowed.vectors[2] = this->rotation.vectors[2];
    rotation = narrowed;
}

// FUNCTION: SURRENDER 0x10053E70
void srNode::getRotation(srMatrix3T<double>& rotation) const
{
    rotation = this->rotation;
}

// FUNCTION: SURRENDER 0x10053E90
srVector3T<double> srNode::getScale() const
{
    return scale;
}

// FUNCTION: SURRENDER 0x10053EB0
void srNode::offsetLocation(const srVector3T<double>& offset)
{
    if (offset.x != 0.0 || offset.y != 0.0 || offset.z != 0.0) {
        location += offset;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053F10
void srNode::offsetLocation(double x, double y, double z)
{
    if (x != 0.0 || y != 0.0 || z != 0.0) {
        location.x += x;
        location.y += y;
        location.z += z;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054790
void srNode::setLocation(const srVector3T<double>& location)
{
    if (!(location == this->location)) {
        this->location = location;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100547E0
void srNode::setLocation(double x, double y, double z)
{
    if (x != location.x || y != location.y || z != location.z) {
        location.Set(x, y, z);
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054840
void srNode::setLocationX(double x)
{
    if (x != location.x) {
        location.x = x;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054870
void srNode::setLocationY(double y)
{
    if (y != location.y) {
        location.y = y;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100548A0
void srNode::setLocationZ(double z)
{
    if (z != location.z) {
        location.z = z;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100548D0
void srNode::setScale(const srVector3T<double>& scale)
{
    if (!(this->scale == scale)) {
        this->scale = scale;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054E70
void srNode::getWorldSpaceCoordinates(srMatrix3T<double>& rotation, srVector3T<double>& location,
                                      srVector3T<double>& scale) const
{
    checkTransformation();
    getWorldSpaceRotation(rotation);
    location = getWorldSpaceLocation();
    scale = getWorldSpaceScale();
}

// FUNCTION: SURRENDER 0x10054ED0
void srNode::getWorldSpaceCoordinates(srMatrix3T<float>& rotation, srVector3T<float>& location,
                                      srVector3T<float>& scale) const
{
    checkTransformation();
    getWorldSpaceRotation(rotation);
    srVector3T<double> ws_location = getWorldSpaceLocation();
    location.SetFromDouble(&ws_location);
    srVector3T<double> ws_scale = getWorldSpaceScale();
    scale.SetFromDouble(&ws_scale);
}

// FUNCTION: SURRENDER 0x10054F70
srVector3T<double> srNode::getWorldSpaceLocation() const
{
    checkTransformation();
    return srVector3T<double>(world_transform0.rows[0].w, world_transform0.rows[1].w,
                              world_transform0.rows[2].w);
}

// FUNCTION: SURRENDER 0x10054FC0
void srNode::getWorldSpaceRotation(srMatrix3T<double>& rotation) const
{
    checkTransformation();
    rotation.vectors[0] = world_transform0.rows[0].xyz();
    rotation.vectors[1] = world_transform0.rows[1].xyz();
    rotation.vectors[2] = world_transform0.rows[2].xyz();
    srVector3T<double> scale = getWorldSpaceScale();
    srVector3T<double> inverse_scale(1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z);
    rotation.vectors[0] *= inverse_scale;
    rotation.vectors[1] *= inverse_scale;
    rotation.vectors[2] *= inverse_scale;
}

// FUNCTION: SURRENDER 0x10055110
void srNode::getWorldSpaceRotation(srMatrix3T<float>& rotation) const
{
    checkTransformation();
    rotation.vectors[0] = world_transform1.rows[0].xyz();
    rotation.vectors[1] = world_transform1.rows[1].xyz();
    rotation.vectors[2] = world_transform1.rows[2].xyz();
    srVector3T<double> scale = getWorldSpaceScale();
    float inverse_x = 1.0f / (float)scale.x;
    float inverse_y = 1.0f / (float)scale.y;
    float inverse_z = 1.0f / (float)scale.z;
    rotation.vectors[0].x *= inverse_x;
    rotation.vectors[1].x *= inverse_x;
    rotation.vectors[2].x *= inverse_x;
    rotation.vectors[0].y *= inverse_y;
    rotation.vectors[1].y *= inverse_y;
    rotation.vectors[2].y *= inverse_y;
    rotation.vectors[0].z *= inverse_z;
    rotation.vectors[1].z *= inverse_z;
    rotation.vectors[2].z *= inverse_z;
}

// FUNCTION: SURRENDER 0x10055210
srVector3T<double> srNode::getWorldSpaceDOF() const
{
    checkTransformation();
    srVector3T<double> direction(world_transform0.rows[0].z, world_transform0.rows[1].z,
                                 world_transform0.rows[2].z);
    direction.Normalize();
    return direction;
}

// FUNCTION: SURRENDER 0x100552B0
srVector3T<double> srNode::getWorldSpaceScale() const
{
    checkTransformation();
    srVector3T<double> x_axis(world_transform0.rows[0].x, world_transform0.rows[1].x,
                              world_transform0.rows[2].x);
    srVector3T<double> y_axis(world_transform0.rows[0].y, world_transform0.rows[1].y,
                              world_transform0.rows[2].y);
    srVector3T<double> z_axis(world_transform0.rows[0].z, world_transform0.rows[1].z,
                              world_transform0.rows[2].z);
    return srVector3T<double>(x_axis.Length(), y_axis.Length(), z_axis.Length());
}

// FUNCTION: SURRENDER 0x10054D10
void srNode::getWorldSpaceMatrix(srMatrix4x3T<double>& matrix) const
{
    checkTransformation();
    matrix = world_transform0;
}

// FUNCTION: SURRENDER 0x10054D40
void srNode::getWorldSpaceMatrix(srMatrix4x3T<float>& matrix) const
{
    checkTransformation();
    matrix = world_transform1;
}

// FUNCTION: SURRENDER 0x10054D70
void srNode::getWorldSpaceMatrix(srMatrix4T<double>& matrix) const
{
    checkTransformation();
    matrix.vectors[0] = world_transform0.rows[0];
    matrix.vectors[1] = world_transform0.rows[1];
    matrix.vectors[2] = world_transform0.rows[2];
    matrix.vectors[3].Set(0.0, 0.0, 0.0, 1.0);
}

// FUNCTION: SURRENDER 0x10054DE0
void srNode::getWorldSpaceMatrix(srMatrix4T<float>& matrix) const
{
    checkTransformation();
    matrix.vectors[0] = world_transform1.rows[0];
    matrix.vectors[1] = world_transform1.rows[1];
    matrix.vectors[2] = world_transform1.rows[2];
    matrix.vectors[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
}

// FUNCTION: SURRENDER 0x10054920
void srNode::updateTransformation() const
{
    notifications.value &= ~2;
    srNode* parent = parent_;
    if (parent == 0) {
        if (testFlag(FLAG_IGNORE_TRANSFORM) == 0) {
            world_transform0.SetRotation(rotation);
            world_transform0.SetTranslation(location);
            world_transform0.Scale(scale);
        } else {
            world_transform0.SetIdentity();
        }
    } else if (testFlag(FLAG_IGNORE_TRANSFORM) == 0) {
        if ((parent->notifications.value & 2) != 0) {
            parent->updateTransformation();
        }
        srVector3T<double> x_axis(rotation.vectors[0].x, rotation.vectors[1].x,
                                  rotation.vectors[2].x);
        srVector3T<double> y_axis(rotation.vectors[0].y, rotation.vectors[1].y,
                                  rotation.vectors[2].y);
        srVector3T<double> z_axis(rotation.vectors[0].z, rotation.vectors[1].z,
                                  rotation.vectors[2].z);
        for (int row = 0; row < 3; ++row) {
            srVector3T<double> basis = parent->world_transform0.rows[row].xyz();
            world_transform0.rows[row].x = DotProduct(basis, x_axis) * scale.x;
            world_transform0.rows[row].y = DotProduct(basis, y_axis) * scale.y;
            world_transform0.rows[row].z = DotProduct(basis, z_axis) * scale.z;
        }
        world_transform0.SetTranslation(parent->world_transform0.TransformPoint(location));
    } else {
        parent->getWorldSpaceMatrix(world_transform0);
    }
    world_transform1.rows[0].Set(
        (float)world_transform0.rows[0].x, (float)world_transform0.rows[0].y,
        (float)world_transform0.rows[0].z, (float)world_transform0.rows[0].w);
    world_transform1.rows[1].Set(
        (float)world_transform0.rows[1].x, (float)world_transform0.rows[1].y,
        (float)world_transform0.rows[1].z, (float)world_transform0.rows[1].w);
    world_transform1.rows[2].Set(
        (float)world_transform0.rows[2].x, (float)world_transform0.rows[2].y,
        (float)world_transform0.rows[2].z, (float)world_transform0.rows[2].w);
}

// FUNCTION: SURRENDER 0x10051E10
void srNode::setWorldSpaceLocation(const srVector3T<double>& location)
{
    if (parent_ == 0) {
        this->location = location;
    } else {
        srMatrix4T<double> parent_world;
        parent_->getWorldSpaceMatrix(parent_world);
        srMatrix4T<double> inverse;
        inverse.Inverse(parent_world);
        this->location = inverse.TransformPoint(location);
    }
    setWSDirty();
}

// FUNCTION: SURRENDER 0x100553A0
void srNode::applyWorldSpaceMatrix(srGERD& renderer)
{
    checkTransformation();
    renderer.matrixMode(srGERD::MATRIX_MODELVIEW);
    renderer.pushMultMatrix(world_transform1);
}

// FUNCTION: SURRENDER 0x100505B0
srNode* srNode::cloneHierarchy(srNode* parent)
{
    srNode* node = static_cast<srNode*>(vClone());
    node->setParent(parent, 0);
    if (first_child_ != 0) {
        first_child_->cloneHierarchyInternal(node);
    }
    return node;
}

// FUNCTION: SURRENDER 0x10050560
srNode* srNode::cloneHierarchyInternal(srNode* parent)
{
    if (next_sibling_ != 0) {
        next_sibling_->cloneHierarchyInternal(parent);
    }
    srNode* node = static_cast<srNode*>(vClone());
    node->setParent(parent, 0);
    if (first_child_ != 0) {
        first_child_->cloneHierarchyInternal(node);
    }
    return node;
}

// FUNCTION: SURRENDER 0x10050A30
char* srNode::getFullPath(char* path) const
{
    if (path == 0) {
        return 0;
    }
    *path = '\0';
    if (parent_ != 0) {
        parent_->getFullPathInternal(path);
    }
    strcat(path, getName());
    return path;
}

// FUNCTION: SURRENDER 0x100509B0
void srNode::getFullPathInternal(char* path) const
{
    if (parent_ != 0) {
        parent_->getFullPathInternal(path);
    }
    strcat(path, getName());
    strcat(path, "/");
}

// FUNCTION: SURRENDER 0x10050AD0
long srNode::getFullPathLength() const
{
    return (parent_ != 0 ? parent_->getFullPathLengthInternal() : 0) + strlen(getName());
}

// FUNCTION: SURRENDER 0x10050A90
long srNode::getFullPathLengthInternal() const
{
    return (parent_ != 0 ? parent_->getFullPathLengthInternal() : 0) + strlen(getName()) + 1;
}

// FUNCTION: SURRENDER 0x10050B10
void srNode::dumpHierarchy(std::ostream& stream, long indent) const
{
    const srNode* node = this;
    do {
        srStreamPrintf(stream, "%*c%s (%s)\n", indent, 0x20, node->getName(), node->getClassName());
        if (node->first_child_ != 0) {
            node->first_child_->dumpHierarchy(stream, indent + 2);
        }
        node = node->next_sibling_;
    } while (node != 0);
}

// FUNCTION: SURRENDER 0x100507B0
srNode* srNode::findChild(const char* name) const
{
    if (name != 0 && first_child_ != 0) {
        return first_child_->findChildInternal(name);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10050730
srNode* srNode::findChildInternal(const char* name)
{
    if (strcmp(name, getName()) == 0) {
        return this;
    }
    if (next_sibling_ != 0) {
        srNode* found = next_sibling_->findChildInternal(name);
        if (found != 0) {
            return found;
        }
    }
    srNode* found = 0;
    if (first_child_ != 0) {
        found = first_child_->findChildInternal(name);
    }
    return found;
}

// FUNCTION: SURRENDER 0x10050860
srNode* srNode::findChildByNameAndType(const char* name, unsigned long class_id) const
{
    if (name != 0 && first_child_ != 0) {
        return first_child_->findChildByNameAndTypeInternal(name, class_id);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100507D0
srNode* srNode::findChildByNameAndTypeInternal(const char* name, unsigned long class_id)
{
    if (strcmp(name, getName()) == 0 && matchClassID(class_id) != 0) {
        return this;
    }
    if (next_sibling_ != 0) {
        srNode* found = next_sibling_->findChildByNameAndTypeInternal(name, class_id);
        if (found != 0) {
            return found;
        }
    }
    srNode* found = 0;
    if (first_child_ != 0) {
        found = first_child_->findChildByNameAndTypeInternal(name, class_id);
    }
    return found;
}

// FUNCTION: SURRENDER 0x10050930
srNode* srNode::findParent(const char* name) const
{
    if (name != 0 && parent_ != 0) {
        return parent_->findParentInternal(name);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10050890
srNode* srNode::findParentInternal(const char* name)
{
    srNode* node = this;
    while (strcmp(name, node->getName()) != 0) {
        node = node->parent_;
        if (node == 0) {
            return 0;
        }
    }
    return node;
}

// FUNCTION: SURRENDER 0x10050990
srNode* srNode::findParentByType(unsigned long class_id) const
{
    if (parent_ == 0) {
        return 0;
    }
    return parent_->findParentByTypeInternal(class_id);
}

// FUNCTION: SURRENDER 0x10050950
srNode* srNode::findParentByTypeInternal(unsigned long class_id)
{
    srNode* node = this;
    while (node->matchClassID(class_id) == 0) {
        node = node->parent_;
        if (node == 0) {
            return 0;
        }
    }
    return node;
}

// FUNCTION: SURRENDER 0x10053A20
void srNode::move(const srVector3T<double>& offset)
{
    if (offset.x != 0.0 || offset.y != 0.0 || offset.z != 0.0) {
        location += rotation.Transform(offset);
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053AD0
void srNode::moveForward(double distance)
{
    if (distance != 0.0) {
        srVector3T<double> direction(rotation.vectors[0].z, rotation.vectors[1].z,
                                     rotation.vectors[2].z);
        location += direction * distance;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053B40
void srNode::moveBackward(double distance)
{
    if (distance != 0.0) {
        srVector3T<double> direction(rotation.vectors[0].z, rotation.vectors[1].z,
                                     rotation.vectors[2].z);
        location -= direction * distance;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053BB0
void srNode::moveLeft(double distance)
{
    if (distance != 0.0) {
        srVector3T<double> direction(rotation.vectors[0].x, rotation.vectors[1].x,
                                     rotation.vectors[2].x);
        location -= direction * distance;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053C20
void srNode::moveRight(double distance)
{
    if (distance != 0.0) {
        srVector3T<double> direction(rotation.vectors[0].x, rotation.vectors[1].x,
                                     rotation.vectors[2].x);
        location += direction * distance;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053C90
void srNode::moveUp(double distance)
{
    if (distance != 0.0) {
        srVector3T<double> direction(rotation.vectors[0].y, rotation.vectors[1].y,
                                     rotation.vectors[2].y);
        location += direction * distance;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053D00
void srNode::moveDown(double distance)
{
    if (distance != 0.0) {
        srVector3T<double> direction(rotation.vectors[0].y, rotation.vectors[1].y,
                                     rotation.vectors[2].y);
        location -= direction * distance;
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10053F70
void srNode::rotate(const srMatrix3T<double>& rotation)
{
    srMatrix3T<double> identity;
    identity.SetIdentity();
    if (!(rotation == identity)) {
        this->rotation.MultiplyBy(rotation);
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100540E0
void srNode::rotate(double angle, const srVector3T<double>& axis)
{
    if (angle != 0.0) {
        srVector3T<double> normalized = axis;
        normalized.Normalize();
        rotation.RotateAroundAxis(sin(angle), cos(angle), normalized);
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100542A0
void srNode::rotateX(double angle)
{
    if (angle != 0.0) {
        rotation.RotateAboutX(sin(angle), cos(angle));
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10054440
void srNode::rotateY(double angle)
{
    if (angle != 0.0) {
        rotation.RotateAboutY(sin(angle), cos(angle));
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x100545F0
void srNode::rotateZ(double angle)
{
    if (angle != 0.0) {
        rotation.RotateAboutZ(sin(angle), cos(angle));
        setWSDirty();
    }
}

// FUNCTION: SURRENDER 0x10051FB0
void srNode::setWorldSpaceRotation(const srMatrix3T<double>& rotation)
{
    if (parent_ == 0) {
        this->rotation = rotation;
    } else {
        srMatrix4T<double> world;
        world.vectors[0].x = rotation.vectors[0].x * scale.x;
        world.vectors[0].y = rotation.vectors[0].y * scale.y;
        world.vectors[0].z = rotation.vectors[0].z * scale.z;
        world.vectors[0].w = 0.0;
        world.vectors[1].x = rotation.vectors[1].x * scale.x;
        world.vectors[1].y = rotation.vectors[1].y * scale.y;
        world.vectors[1].z = rotation.vectors[1].z * scale.z;
        world.vectors[1].w = 0.0;
        world.vectors[2].x = rotation.vectors[2].x * scale.x;
        world.vectors[2].y = rotation.vectors[2].y * scale.y;
        world.vectors[2].z = rotation.vectors[2].z * scale.z;
        world.vectors[2].w = 0.0;
        world.vectors[3].Set(0.0, 0.0, 0.0, 1.0);
        srMatrix4T<double> local;
        parent_->getWorldSpaceMatrix(local);
        local.vectors[0].w = 0.0;
        local.vectors[1].w = 0.0;
        local.vectors[2].w = 0.0;
        local.vectors[3].w = 1.0;
        srMatrix4T<double> inverse;
        inverse.Inverse(local);
        local = inverse;
        local.MultiplyBy(world);
        srMatrix3T<double> result;
        result.SetRows(local.vectors[0].xyz(), local.vectors[1].xyz(), local.vectors[2].xyz());
        this->rotation = result;
        srVector3T<double> column_x;
        srVector3T<double> column_y;
        srVector3T<double> column_z;
        column_z = srVector3T<double>(this->rotation.vectors[0].z, this->rotation.vectors[1].z,
                                      this->rotation.vectors[2].z);
        column_y = srVector3T<double>(this->rotation.vectors[0].y, this->rotation.vectors[1].y,
                                      this->rotation.vectors[2].y);
        column_x = srVector3T<double>(this->rotation.vectors[0].x, this->rotation.vectors[1].x,
                                      this->rotation.vectors[2].x);
        srVector3T<double> inverse_scale;
        inverse_scale.Set(1.0 / column_x.Length(), 1.0 / column_y.Length(),
                          1.0 / column_z.Length());
        this->rotation.vectors[0] *= inverse_scale;
        this->rotation.vectors[1] *= inverse_scale;
        this->rotation.vectors[2] *= inverse_scale;
    }
    setWSDirty();
}

// FUNCTION: SURRENDER 0x10052420
void srNode::setWorldSpaceMatrix(const srMatrix4T<double>& matrix)
{
    srMatrix4T<double> local;
    if (parent_ == 0) {
        local = matrix;
    } else {
        parent_->getWorldSpaceMatrix(local);
        srMatrix4T<double> inverse;
        inverse.Inverse(local);
        local = inverse;
        local.MultiplyBy(matrix);
    }
    srMatrix3T<double> result;
    result.SetRows(local.vectors[0].xyz(), local.vectors[1].xyz(), local.vectors[2].xyz());
    rotation = result;
    location = srVector3T<double>(local.vectors[0].w, local.vectors[1].w, local.vectors[2].w);
    srVector3T<double> column_x;
    srVector3T<double> column_y;
    srVector3T<double> column_z;
    column_z =
        srVector3T<double>(rotation.vectors[0].z, rotation.vectors[1].z, rotation.vectors[2].z);
    column_y =
        srVector3T<double>(rotation.vectors[0].y, rotation.vectors[1].y, rotation.vectors[2].y);
    column_x =
        srVector3T<double>(rotation.vectors[0].x, rotation.vectors[1].x, rotation.vectors[2].x);
    scale.Set(column_x.Length(), column_y.Length(), column_z.Length());
    srVector3T<double> inverse_scale;
    inverse_scale.Set(1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z);
    rotation.vectors[0] *= inverse_scale;
    rotation.vectors[1] *= inverse_scale;
    rotation.vectors[2] *= inverse_scale;
    setWSDirty();
}

// FUNCTION: SURRENDER 0x10052D80
void srNode::pitchAt(const srVector3T<double>& target, double amount)
{
    srMatrix3T<double> rotation;
    getWorldSpaceRotation(rotation);
    srVector3T<double> direction = target - getWorldSpaceLocation();
    direction.Normalize();
    srVector3T<double> up(rotation.vectors[0].y, rotation.vectors[1].y, rotation.vectors[2].y);
    srVector3T<double> forward(rotation.vectors[0].z, rotation.vectors[1].z, rotation.vectors[2].z);
    float toward = DotProduct(forward, direction);
    double angle = -atan2(DotProduct(up, direction), toward) * amount;
    if (angle != 0.0) {
        rotation.RotateAboutX(sin(angle), cos(angle));
    }
    rotation.OrthonormalizeRows();
    setWorldSpaceRotation(rotation);
}

// FUNCTION: SURRENDER 0x100533D0
void srNode::pitchAt(const srNode* target, double amount)
{
    pitchAt(target->getWorldSpaceLocation(), amount);
}

// FUNCTION: SURRENDER 0x10052FC0
void srNode::yawAt(const srVector3T<double>& target, double amount)
{
    srMatrix3T<double> rotation;
    getWorldSpaceRotation(rotation);
    srVector3T<double> direction = target - getWorldSpaceLocation();
    direction.Normalize();
    srVector3T<double> right(rotation.vectors[0].x, rotation.vectors[1].x, rotation.vectors[2].x);
    srVector3T<double> forward(rotation.vectors[0].z, rotation.vectors[1].z, rotation.vectors[2].z);
    float toward = DotProduct(forward, direction);
    double angle = atan2(DotProduct(right, direction), toward) * amount;
    if (angle != 0.0) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }
    rotation.OrthonormalizeRows();
    setWorldSpaceRotation(rotation);
}

// FUNCTION: SURRENDER 0x10053420
void srNode::yawAt(const srNode* target, double amount)
{
    yawAt(target->getWorldSpaceLocation(), amount);
}

// FUNCTION: SURRENDER 0x10053210
void srNode::rollUp(double amount)
{
    srMatrix3T<double> rotation;
    getWorldSpaceRotation(rotation);
    float vertical = rotation.vectors[1].y;
    double angle = -atan2(rotation.vectors[1].x, vertical) * amount;
    if (angle != 0.0) {
        rotation.RotateAboutZ(sin(angle), cos(angle));
    }
    rotation.OrthonormalizeRows();
    setWorldSpaceRotation(rotation);
}

// FUNCTION: SURRENDER 0x10053470
void srNode::rollAt(const srVector3T<double>& target, double amount)
{
    srMatrix3T<double> rotation;
    getWorldSpaceRotation(rotation);
    srVector3T<double> right(rotation.vectors[0].x, rotation.vectors[1].x, rotation.vectors[2].x);
    srVector3T<double> up(rotation.vectors[0].y, rotation.vectors[1].y, rotation.vectors[2].y);
    double angle = -atan2(DotProduct(right, target), DotProduct(up, target)) * amount;
    if (angle != 0.0) {
        rotation.RotateAboutZ(sin(angle), cos(angle));
    }
    rotation.OrthonormalizeRows();
    setWorldSpaceRotation(rotation);
}

// FUNCTION: SURRENDER 0x10053660
void srNode::rollAt(const srNode* target, double amount)
{
    srMatrix3T<double> rotation;
    target->getWorldSpaceRotation(rotation);
    rollAt(srVector3T<double>(rotation.vectors[0].x, rotation.vectors[1].x, rotation.vectors[2].x),
           amount);
}
