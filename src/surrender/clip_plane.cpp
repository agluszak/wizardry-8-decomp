#include "surrender/srClipPlane.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"

// FUNCTION: SURRENDER 0x10049B90
srClipPlane::srClipPlane(srNode* parent) : srNode(0)
{
    srCore.getRegistry()->registerInstance(ClientType::sGetClassNode(), this);
    clip_type_ = CLIP_POSITIONAL_0;
    clip_plane_.x = 0.0f;
    clip_plane_.y = 0.0f;
    clip_plane_.z = 1.0f;
    clip_plane_.w = 0.0f;
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
    /* The recovered e_processType currently names only 0; retail still
       compares this override against 1, 2, 3, and 4. Enumerator names remain
       unknown, so keep the integer tests rather than inventing them. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
    if (type == 1 || type == 3) {
        applyWorldSpaceMatrix(*renderer);
        renderer->matrixMode(srGERD::MATRIX_MODELVIEW);
        renderer->pushClipPlane(clip_plane_, static_cast<srGERD::e_clipMode>(clip_type_));
        renderer->popMatrix();
    } else if (type == 2 || type == 4) {
        renderer->popClipPlane();
    }
#pragma clang diagnostic pop
}

// FUNCTION: SURRENDER 0x10049D40
void srClipPlane::traverse(TraverseInfo& info)
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

// FUNCTION: SURRENDER 0x10049E40
void srClipPlane::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "Plane equation: " << '{' << clip_plane_.x << ',' << clip_plane_.y << ','
           << clip_plane_.z << ',' << clip_plane_.w << '}' << '\n';
    stream.width(0x20);
    stream << "Clip type: " << static_cast<long>(clip_type_) << '\n';
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
srClipPlane::srClipPlane(const srClipPlane& other) : srNode(0)
{
    srCore.getRegistry()->registerInstance(ClientType::sGetClassNode(), this);
    /* Retail emits the operator= call followed by the clip field copies. */
    *this = other;
    clip_plane_ = other.clip_plane_;
    clip_type_ = other.clip_type_;
}

// FUNCTION: SURRENDER 0x1004A2C0
srClipPlane::~srClipPlane()
{
    srCore.getRegistry()->unregisterInstance(ClientType::sGetClassNode(), this);
}
