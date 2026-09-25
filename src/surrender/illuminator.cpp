#include "surrender/srIlluminator.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"

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
    /* The recovered e_processType currently names only 0; retail still
       compares this override against 1, 2, 3, and 4. Enumerator names remain
       unknown, so keep the integer tests rather than inventing them. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
    if (type == 1 || type == 3) {
        applyWorldSpaceMatrix(*renderer);
        srVector3T<float> origin;
        origin.x = 0.0f;
        origin.y = 0.0f;
        origin.z = 0.0f;
        const srVector4T<float> location = renderer->getEyeSpaceLocation(origin);
        eye_location_140.x = location.x;
        eye_location_140.y = location.y;
        eye_location_140.z = location.z;
        renderer->popMatrix();
        renderer->pushVertexProcessor(*static_cast<srVertexProcessor*>(this));
        return;
    }
    if (type == 2 || type == 4) {
        renderer->popVertexProcessor();
    }
#pragma clang diagnostic pop
}

// FUNCTION: SURRENDER 0x1004C9E0
void srIlluminator::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (!testFlag(FLAG_TERMINATE)) {
        if (!testFlag(FLAG_DISABLE)) {
            if (!testFlag(FLAG_GLOBAL)) {
                TraverseInfo::Entry& entry = info.entries[info.entry_count];
                entry.node = this;
                entry.value = 1;
                ++info.entry_count;
            } else {
                info.nodes[info.node_count] = this;
                ++info.node_count;
            }
            if (firstChild() != 0) {
                firstChild()->traverse(info);
            }
            if (!testFlag(FLAG_GLOBAL)) {
                TraverseInfo::Entry& entry = info.entries[info.entry_count];
                entry.node = this;
                entry.value = 2;
                ++info.entry_count;
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
    eye_location_140 = other.eye_location_140;
}

// FUNCTION: SURRENDER 0x1004C6E0
srIlluminator::~srIlluminator()
{
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
}

// SYNTHETIC: SURRENDER 0x1004CC20
// srIlluminator default constructor closure

// SYNTHETIC: SURRENDER 0X1004CB10
// srVertexProcessor subobject destructor emission (vtable restore)

// SYNTHETIC: SURRENDER 0X1004CC30
// srIlluminator scalar deleting destructor
