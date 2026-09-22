// Recovery of sr.dll's srScene implementation against the gog-base retail
// binary.

#include "surrender/srScene.h"

#include "surrender/srCamera.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"
#include "surrender/srTimer.h"

// FUNCTION: SURRENDER 0x10056C10
const char* srScene::sGetClassName()
{
    return "srScene";
}

// FUNCTION: SURRENDER 0x10056F90
srScene::~srScene() {}

// FUNCTION: SURRENDER 0x10056BB0
srClass* srScene::vInstance()
{
    srScene* instance = static_cast<srScene*>(srHeap.allocate(0x190));
    if (instance != 0) {
        return new (instance) srScene(0);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100565D0
srScene::srScene(srNode* parent)
    : srClassSupport<srScene, srNode, 0, 0x1010>(static_cast<srNode*>(0))
{
    ambient_light_174.x = 0.2f;
    ambient_light_174.y = 0.2f;
    ambient_light_174.z = 0.2f;
    fog_color_180.x = 0.1f;
    fog_color_180.y = 0.2f;
    fog_color_180.z = 0.4f;
    traversal_158.entry_count = 0;
    traversal_158.node_count = 0;
    traversal_158.renderer = 0;
    enabled_138.value = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
    resetStatistics();
}

// FUNCTION: SURRENDER 0x10056D60
srScene::srScene(const srScene& other)
    : srClassSupport<srScene, srNode, 0, 0x1010>(static_cast<srNode*>(0))
{
    *this = other;
    enabled_138 = other.enabled_138;
    statistics_140 = other.statistics_140;
    traversal_158 = other.traversal_158;
    ambient_light_174 = other.ambient_light_174;
    fog_color_180 = other.fog_color_180;
}

// FUNCTION: SURRENDER 0x100566F0
srScene& srScene::operator=(const srScene& other)
{
    if (this != &other) {
        srNode::operator=(other);
        ambient_light_174 = other.ambient_light_174;
        fog_color_180 = other.fog_color_180;
        enabled_138 = other.enabled_138;
    }
    return *this;
}

/* The scene walks its own traversal arrays rather than sharing the generic
   node walk: nodes carry the global pre/post passes (types 3 and 4) and the
   typed entries carry their per-node process type in the entry value. */
// FUNCTION: SURRENDER 0x100561F0
void srScene::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (!testFlag(FLAG_DISABLE) && !testFlag(FLAG_TERMINATE) && firstChild() != 0) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }
}

// FUNCTION: SURRENDER 0x100562E0
void srScene::process(const ProcessInfo& info, e_processType type)
{
    if (firstChild() == 0) {
        return;
    }
    traversal_158.entry_count = 0;
    traversal_158.node_count = 0;
    traversal_158.renderer = 0;
    traversal_158.renderer = info.renderer;
    firstChild()->traverse(traversal_158);
    if (traversal_158.nodes.capacity == 0) {
        traversal_158.nodes.setCapacity(8);
    }
    srNode** nodes = traversal_158.nodes.data;
    if (traversal_158.entries.capacity == 0) {
        traversal_158.entries.setCapacity(8);
    }
    TraverseInfo::Entry* entries = traversal_158.entries.data;
    long node_count = traversal_158.node_count;
    long entry_count = traversal_158.entry_count;
    srGERD* renderer = info.renderer;
    unsigned long pick_key = renderer->getPickKey();
    srVector4T<float> fog_color;
    srVector4T<float> ambient_light;
    renderer->getFogColor(fog_color);
    renderer->getAmbientLight(ambient_light);
    renderer->setFogColor(fog_color_180);
    renderer->setAmbientLight(ambient_light_174);
    ProcessInfo process_info = info;
    long count = node_count;
    while (count > 0) {
        (*nodes)->process(process_info, static_cast<e_processType>(3));
        --count;
        ++nodes;
    }
    long remaining = entry_count;
    while (remaining > 0) {
        if ((enabled_138.value & 1) != 0) {
            // reinterpret-ok: the pick key is the node pointer itself.
            renderer->setPickKey(reinterpret_cast<unsigned long>(entries->node));
        }
        entries->node->process(process_info, static_cast<e_processType>(entries->value));
        ++entries;
        --remaining;
    }
    if (node_count - 1 >= 0) {
        nodes = traversal_158.nodes.data + node_count - 1;
        count = node_count;
        do {
            (*nodes)->process(process_info, static_cast<e_processType>(4));
            --nodes;
            --count;
        } while (count != 0);
    }
    renderer->setFogColor(fog_color);
    renderer->setAmbientLight(ambient_light);
    renderer->setPickKey(pick_key);
    statistics_140.value_0c += node_count;
    ++statistics_140.value_08;
    statistics_140.value_10 += entry_count;
}

// FUNCTION: SURRENDER 0x100564A0
void srScene::render(srGERD& renderer, srCamera* camera)
{
    srGERD* renderer_pointer = &renderer;
    if (renderer_pointer == 0) {
        return;
    }
    if (testFlag(FLAG_DISABLE) || testFlag(FLAG_TERMINATE) || firstChild() == 0) {
        return;
    }
    srNode::lockSceneGraph();
    ProcessInfo process_info;
    process_info.renderer = &renderer;
    if (camera != 0) {
        camera->process(process_info, static_cast<e_processType>(1));
    }
    process(process_info, static_cast<e_processType>(0));
    if (camera != 0) {
        camera->process(process_info, static_cast<e_processType>(2));
    }
    srNode::unlockSceneGraph();
}

// FUNCTION: SURRENDER 0x10056520
void srScene::getStatistics(Statistics& statistics)
{
    statistics = statistics_140;
    statistics.value_00 =
        srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - statistics.value_00;
}

// FUNCTION: SURRENDER 0x10056550
void srScene::resetStatistics()
{
    memset(&statistics_140, 0, sizeof(statistics_140));
    statistics_140.value_00 = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x10056750
void srScene::dump(std::ostream& stream)
{
    srNode::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    stream.width(0x20);
    stream << "  Ambient light: ";
    stream << '{' << ambient_light_174.x << ',' << ambient_light_174.y << ',' << ambient_light_174.z
           << '}' << '\n';
    stream.width(0x20);
    stream << "  Fog color: ";
    stream << '{' << fog_color_180.x << ',' << fog_color_180.y << ',' << fog_color_180.z << '}'
           << '\n';
    Statistics statistics;
    getStatistics(statistics);
    stream.width(0x20);
    stream << "  Time since stat reset: " << statistics.value_00 << '\n';
    stream.width(0x20);
    stream << "  Render calls/sec: " << statistics.value_08 / statistics.value_00 << '\n';
    stream.width(0x20);
    stream << "  Global calls/sec: " << statistics.value_0c / statistics.value_00 << '\n';
    stream.width(0x20);
    stream << "  Process calls/sec: " << statistics.value_10 / statistics.value_00 << '\n';
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x10056B50
void srScene::disable(e_enable option)
{
    enabled_138.value &= ~(1ul << option);
}

// FUNCTION: SURRENDER 0x10056B70
void srScene::enable(e_enable option)
{
    enabled_138.value |= 1ul << option;
}

// FUNCTION: SURRENDER 0x10056B90
int srScene::isEnabled(e_enable option) const
{
    return (enabled_138.value & (1ul << option)) != 0;
}

// FUNCTION: SURRENDER 0x10056C20
void srScene::getAmbientLight(srVector3T<float>& color) const
{
    color = ambient_light_174;
}

// FUNCTION: SURRENDER 0x10056C40
srVector3T<float> srScene::getAmbientLight() const
{
    return ambient_light_174;
}

// FUNCTION: SURRENDER 0x10056C70
void srScene::getFogColor(srVector3T<float>& color) const
{
    color = fog_color_180;
}

// FUNCTION: SURRENDER 0x10056C90
srVector3T<float> srScene::getFogColor() const
{
    return fog_color_180;
}

// FUNCTION: SURRENDER 0x10056CC0
void srScene::setAmbientLight(float red, float green, float blue)
{
    ambient_light_174.x = red;
    ambient_light_174.y = green;
    ambient_light_174.z = blue;
}

// FUNCTION: SURRENDER 0x10056CF0
void srScene::setAmbientLight(const srVector3T<float>& color)
{
    ambient_light_174 = color;
}

// FUNCTION: SURRENDER 0x10056D10
void srScene::setFogColor(float red, float green, float blue)
{
    fog_color_180.x = red;
    fog_color_180.y = green;
    fog_color_180.z = blue;
}

// FUNCTION: SURRENDER 0x10056D40
void srScene::setFogColor(const srVector3T<float>& color)
{
    fog_color_180 = color;
}
