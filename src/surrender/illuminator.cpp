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
    group_mask = 0x80000000;
}

// FUNCTION: SURRENDER 0x1004C8D0
srIlluminator& srIlluminator::operator=(const srIlluminator& other)
{
    if (this != &other) {
        srNode::operator=(other);
        group_mask = other.group_mask;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1004C900
void srIlluminator::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    if (type == PROCESS_PUSH || type == PROCESS_PUSH_GLOBAL) {
        applyWorldSpaceMatrix(*renderer);
        srVector3T<float> origin;
        origin.SetZero();
        const srVector4T<float> location = renderer->getEyeSpaceLocation(origin);
        eye_location.x = location.x;
        eye_location.y = location.y;
        eye_location.z = location.z;
        renderer->popMatrix();
        renderer->pushVertexProcessor(*this);
        return;
    }
    if (type == PROCESS_POP || type == PROCESS_POP_GLOBAL) {
        renderer->popVertexProcessor();
    }
}

// FUNCTION: SURRENDER 0x1004C9E0
void srIlluminator::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
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
            if (first_child_ != 0) {
                first_child_->traverse(info);
            }
            if (!testFlag(FLAG_GLOBAL)) {
                TraverseInfo::Entry& entry = info.entries[info.entry_count];
                entry.node = this;
                entry.value = 2;
                ++info.entry_count;
            }
        } else if (first_child_ != 0) {
            first_child_->traverse(info);
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
    group_mask = mask;
}

// FUNCTION: SURRENDER 0x1004CB00
unsigned long srIlluminator::getGroupMask() const
{
    return group_mask;
}
