#include "surrender/srClipPlane.h"

#include "surrender/srGERD.h"
#include "surrender/srTypeRegistry.h"

#include <ostream>

// FUNCTION: SURRENDER 0x10049B90
srClipPlane::srClipPlane(srNode* parent)
    : srClassSupport<srClipPlane, srNode, false, 0x1500>(static_cast<srNode*>(0))
{
    clip_plane_.Set(0.0f, 0.0f, 1.0f, 0.0f);
    clip_type_ = CLIP_POSITIONAL_0;
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x10049CE0
void srClipPlane::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    if (type == PROCESS_PUSH || type == PROCESS_PUSH_GLOBAL) {
        applyWorldSpaceMatrix(*renderer);
        renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
        renderer->pushClipPlane(clip_plane_, static_cast<srGERD::e_clipMode>(clip_type_));
        renderer->popMatrix();
    } else if (type == PROCESS_POP || type == PROCESS_POP_GLOBAL) {
        renderer->popClipPlane();
    }
}

// FUNCTION: SURRENDER 0x10049D40
void srClipPlane::traverse(TraverseInfo& info)
{
    if (next_sibling_ != 0) {
        next_sibling_->traverse(info);
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
            if (first_child_ != 0) {
                first_child_->traverse(info);
            }
            if (testFlag(FLAG_GLOBAL) == 0) {
                if (info.entries.capacity <= info.entry_count) {
                    info.entries.setCapacity(info.entries.capacity + 8 + info.entry_count);
                }
                info.entries.data[info.entry_count].node = this;
                info.entries.data[info.entry_count].value = 2;
                info.entry_count++;
            }
        } else if (first_child_ != 0) {
            first_child_->traverse(info);
        }
    }
}

// FUNCTION: SURRENDER 0x10049E40
void srClipPlane::dump(std::ostream& stream)
{
    long flags;

    srNode::dump(stream);
    flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Plane equation: ";
    stream << '{' << clip_plane_.x << ',' << clip_plane_.y << ',' << clip_plane_.z << ','
           << clip_plane_.w << '}';
    stream << '\n';
    stream.width(0x20);
    stream << "  Clip type: " << clip_type_ << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1004A0A0
void srClipPlane::setClipType(e_clip type)
{
    clip_type_ = type;
}

// FUNCTION: SURRENDER 0x1004A0B0
srClipPlane::e_clip srClipPlane::getClipType() const
{
    return clip_type_;
}

// FUNCTION: SURRENDER 0x1004A0C0
void srClipPlane::setClipPlane(const srVector4T<float>& plane)
{
    clip_plane_ = plane;
}

// FUNCTION: SURRENDER 0x1004A0F0
void srClipPlane::getClipPlane(srVector4T<float>& plane) const
{
    plane = clip_plane_;
}

// FUNCTION: SURRENDER 0x1004A120
srVector4T<float> srClipPlane::getClipPlane() const
{
    return clip_plane_;
}

// FUNCTION: SURRENDER 0x1004A160
srClass* srClipPlane::vInstance()
{
    srClipPlane* instance = static_cast<srClipPlane*>(srHeap.allocate(0x150));
    if (instance != 0) {
        return new (instance) srClipPlane(0);
    }
    return 0;
}
