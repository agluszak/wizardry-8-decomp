#include "surrender/srClipPlane.h"

#include "surrender/srGERD.h"
#include "surrender/srTypeRegistry.h"

// FUNCTION: SURRENDER 0x10049B90
srClipPlane::srClipPlane(srNode* parent)
    : srClassSupport<srClipPlane, srNode, false, 0x1500>(static_cast<srNode*>(0))
{
    clip_plane_.x = 0.0f;
    clip_plane_.y = 0.0f;
    clip_plane_.z = 1.0f;
    clip_plane_.w = 0.0f;
    clip_type_ = CLIP_POSITIONAL_0;
    if (parent != 0) {
        setParent(parent, 0);
    }
}

// FUNCTION: SURRENDER 0x10049C90
srClipPlane& srClipPlane::operator=(const srClipPlane& other)
{
    if (this != &other) {
        srNode::operator=(other);
        clip_type_ = other.clip_type_;
        clip_plane_ = other.clip_plane_;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10049CE0
void srClipPlane::process(const ProcessInfo& info, e_processType type)
{
    srGERD* renderer = info.renderer;
    if (type == static_cast<e_processType>(1) || type == static_cast<e_processType>(3)) {
        applyWorldSpaceMatrix(*renderer);
        renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
        renderer->pushClipPlane(clip_plane_, static_cast<srGERD::e_clipMode>(clip_type_));
        renderer->popMatrix();
    } else if (type == static_cast<e_processType>(2) || type == static_cast<e_processType>(4)) {
        renderer->popClipPlane();
    }
}

// FUNCTION: SURRENDER 0x10049D40
void srClipPlane::traverse(TraverseInfo& info)
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

// FUNCTION: SURRENDER 0x1004A150
const char* srClipPlane::sGetClassName()
{
    return "srClipPlane";
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

// FUNCTION: SURRENDER 0x1004A1C0
srClipPlane::srClipPlane(const srClipPlane& other)
    : srClassSupport<srClipPlane, srNode, false, 0x1500>(static_cast<srNode*>(0))
{
    *this = other;
    clip_type_ = other.clip_type_;
    clip_plane_ = other.clip_plane_;
}

// FUNCTION: SURRENDER 0x1004A2C0
srClipPlane::~srClipPlane() {}

// TEMPLATE: SURRENDER 0x10049FC0
// srClassSupport<srClipPlane, srNode, 0, 0x1500>::clone

// TEMPLATE: SURRENDER 0x10049FE0
// srClassSupport<srClipPlane, srNode, 0, 0x1500>::~srClassSupport

// SYNTHETIC: SURRENDER 0x1004A380
// srClipPlane default constructor closure

// SYNTHETIC: SURRENDER 0x1004A390
// srClipPlane scalar deleting destructor

// SYNTHETIC: SURRENDER 0x1004A410
// srClassSupport<srClipPlane, srNode, 0, 0x1500> scalar deleting destructor

// TEMPLATE: SURRENDER 0x1004A430
// srArray<srNode*>::setCapacity

// TEMPLATE: SURRENDER 0x1004A4A0
// srArray<srNode::TraverseInfo::Entry>::setCapacity
