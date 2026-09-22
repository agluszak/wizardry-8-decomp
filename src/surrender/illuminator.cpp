#include "surrender/srIlluminator.h"

#include "surrender/srGERD.h"
#include "surrender/srTypeRegistry.h"

// FUNCTION: SURRENDER 0x1004C7D0
srIlluminator::srIlluminator(srNode* parent)
    : srClassSupport<srIlluminator, srNode, false, 0x1200>(static_cast<srNode*>(0))
{
    if (parent != 0) {
        setParent(parent, 0);
    }
    setFlag(FLAG_GLOBAL);
    group_mask_13c = 0x80000000;
}

// FUNCTION: SURRENDER 0x1004C8D0
srIlluminator& srIlluminator::operator=(const srIlluminator& other)
{
    if (this != &other) {
        srNode::operator=(other);
        group_mask_13c = other.group_mask_13c;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004C900
void srIlluminator::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    if (type == static_cast<e_processType>(1) || type == static_cast<e_processType>(3)) {
        applyWorldSpaceMatrix(*renderer);
        srVector4T<float> eye = renderer->getEyeSpaceLocation(srVector3T<float>(0.0f, 0.0f, 0.0f));
        eye_location_140.x = eye.x;
        eye_location_140.y = eye.y;
        eye_location_140.z = eye.z;
        renderer->popMatrix();
        renderer->pushVertexProcessor(*this);
    } else if (type == static_cast<e_processType>(2) || type == static_cast<e_processType>(4)) {
        renderer->popVertexProcessor();
    }
}

// FUNCTION: SURRENDER 0x1004C9E0
void srIlluminator::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (testFlag(FLAG_TERMINATE) == 0) {
        if (testFlag(FLAG_DISABLE) == 0) {
            if (testFlag(FLAG_GLOBAL) == 0) {
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 1;
                info.entry_count++;
            } else {
                if (info.nodes.capacity <= info.node_count) {
                    info.nodes.setCapacity(info.nodes.capacity + 8 + info.node_count);
                }
                info.nodes.data[info.node_count] = this;
                info.node_count++;
            }
            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }
            if (testFlag(FLAG_GLOBAL) == 0) {
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 2;
                info.entry_count++;
            }
        } else if (firstChild() != 0) {
            firstChild()->traverse(info);
        }
    }
}

// FUNCTION: SURRENDER 0x1004CAE0
const char* srIlluminator::sGetClassName()
{
    return "srIlluminator";
}

// FUNCTION: SURRENDER 0x1004CAF0
void srIlluminator::setGroupMask(unsigned long mask)
{
    group_mask_13c = mask;
}

// FUNCTION: SURRENDER 0x1004CB00
unsigned long srIlluminator::getGroupMask() const
{
    return group_mask_13c;
}

// FUNCTION: SURRENDER 0x1004CB20
srIlluminator::srIlluminator(const srIlluminator& other)
    : srClassSupport<srIlluminator, srNode, false, 0x1200>(static_cast<srNode*>(0))
{
    *this = other;
    group_mask_13c = other.group_mask_13c;
    eye_location_140 = other.eye_location_140;
}

// TEMPLATE: SURRENDER 0x1004CB10
// srVertexProcessor secondary-base vptr store emitted by the srIlluminator ctors

// SYNTHETIC: SURRENDER 0x1004CC20
// srIlluminator default constructor closure

// SYNTHETIC: SURRENDER 0x1004CC30
// srIlluminator scalar deleting destructor

// SYNTHETIC: SURRENDER 0x1004CC50
// srIlluminator vector deleting destructor
