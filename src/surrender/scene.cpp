#include "surrender/srScene.h"

#include <string.h>

#include "surrender/srCamera.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srHeap.h"
#include "surrender/srTimer.h"

#pragma intrinsic(memset)

// FUNCTION: SURRENDER 0x100561F0
void srScene::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }
    if (testFlag(FLAG_DISABLE) == 0 && testFlag(FLAG_TERMINATE) == 0 && firstChild() != 0) {
        if (info.entries.capacity <= info.entry_count) {
            info.entries.setCapacity(info.entries.capacity + 8 + info.entry_count);
        }
        info.entries.data[info.entry_count].node = this;
        info.entries.data[info.entry_count].value = 0;
        info.entry_count++;
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
    process_info_170.renderer = info.renderer;
    firstChild()->traverse(traversal_158);
    if (traversal_158.nodes.capacity == 0) {
        traversal_158.nodes.setCapacity(8);
    }
    srNode** nodes = traversal_158.nodes.data;
    if (traversal_158.entries.capacity == 0) {
        traversal_158.entries.setCapacity(8);
    }
    TraverseInfo::Entry* entries = traversal_158.entries.data;
    int node_count = traversal_158.node_count;
    int entry_count = traversal_158.entry_count;
    unsigned long pick_key = info.renderer->getPickKey();
    srVector4T<float> fog_color;
    srVector4T<float> ambient_light;
    info.renderer->getFogColor(fog_color);
    info.renderer->getAmbientLight(ambient_light);
    info.renderer->setFogColor(fog_color_180);
    info.renderer->setAmbientLight(ambient_light_174);
    int i = node_count;
    srNode** node = nodes;
    if (0 < node_count) {
        do {
            (*node)->process(process_info_170, static_cast<e_processType>(3));
            i = i - 1;
            node = node + 1;
        } while (i != 0);
    }
    if (0 < entry_count) {
        TraverseInfo::Entry* entry = entries;
        int j = entry_count;
        do {
            if ((enabled_138.value & 1) != 0) {
                info.renderer->setPickKey(entry->value);
            }
            entry->node->process(process_info_170, static_cast<e_processType>(entry->value));
            entry++;
            j = j - 1;
        } while (j != 0);
    }
    if (-1 < node_count - 1) {
        srNode** node = nodes + node_count - 1;
        int j = node_count;
        do {
            (*node)->process(process_info_170, static_cast<e_processType>(4));
            node = node - 1;
            j = j - 1;
        } while (j != 0);
    }
    info.renderer->setFogColor(fog_color);
    info.renderer->setAmbientLight(ambient_light);
    info.renderer->setPickKey(pick_key);
    statistics_140.value_0c = statistics_140.value_0c + node_count;
    statistics_140.value_08 = statistics_140.value_08 + 1;
    statistics_140.value_10 = statistics_140.value_10 + entry_count;
}

// FUNCTION: SURRENDER 0x100564A0
void srScene::render(srGERD& renderer, srCamera* camera)
{
    if (testFlag(FLAG_DISABLE) == 0) {
        if (testFlag(FLAG_TERMINATE) == 0 && firstChild() != 0) {
            lockSceneGraph();
            ProcessInfo info;
            info.renderer = &renderer;
            if (camera != 0) {
                camera->process(info, static_cast<e_processType>(1));
            }
            process(info, static_cast<e_processType>(0));
            if (camera != 0) {
                camera->process(info, static_cast<e_processType>(2));
            }
            unlockSceneGraph();
        }
    }
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

// FUNCTION: SURRENDER 0x100565D0
srScene::srScene(srNode* parent)
    : srClassSupport<srScene, srNode, 0, 0x1010>(static_cast<srNode*>(0))
{
    setAmbientLight(0.2f, 0.2f, 0.2f);
    setFogColor(0.1f, 0.2f, 0.4f);
    traversal_158.entry_count = 0;
    traversal_158.node_count = 0;
    process_info_170.renderer = 0;
    enabled_138.value = 0;
    if (parent != 0) {
        setParent(parent, 0);
    }
    resetStatistics();
}

// FUNCTION: SURRENDER 0x100566F0
srScene& srScene::operator=(const srScene& other)
{
    if (this != &other) {
        srNode::operator=(other);
        ambient_light_174 = other.ambient_light_174;
        fog_color_180 = other.fog_color_180;
        enabled_138.value = other.enabled_138.value;
    }
    return *this;
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
    enabled_138.value &= ~(1 << (option & 0x1f));
}

// FUNCTION: SURRENDER 0x10056B70
void srScene::enable(e_enable option)
{
    enabled_138.value |= 1 << (option & 0x1f);
}

// FUNCTION: SURRENDER 0x10056B90
int srScene::isEnabled(e_enable option) const
{
    return (enabled_138.value & (1 << (option & 0x1f))) != 0;
}

// FUNCTION: SURRENDER 0x10056BB0
srClass* srScene::vInstance()
{
    srScene* instance = static_cast<srScene*>(srHeap.allocate(400));
    if (instance != 0) {
        return new (instance) srScene(0);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10056C10
const char* srScene::sGetClassName()
{
    return "srScene";
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

// FUNCTION: SURRENDER 0x10056CF0
void srScene::setAmbientLight(const srVector3T<float>& color)
{
    ambient_light_174 = color;
}

// FUNCTION: SURRENDER 0x10056D40
void srScene::setFogColor(const srVector3T<float>& color)
{
    fog_color_180 = color;
}

// SYNTHETIC: SURRENDER 0x10057080
// srScene default_constructor_closure

// FUNCTION: SURRENDER 0x10056D60
srScene::srScene(const srScene& other)
    : srClassSupport<srScene, srNode, 0, 0x1010>(static_cast<srNode*>(0))
{
    operator=(other);
    enabled_138.value = other.enabled_138.value;
    statistics_140 = other.statistics_140;
    traversal_158 = other.traversal_158;
    process_info_170 = other.process_info_170;
    ambient_light_174 = other.ambient_light_174;
    fog_color_180 = other.fog_color_180;
}

// FUNCTION: SURRENDER 0x10056F90
srScene::~srScene() {}
