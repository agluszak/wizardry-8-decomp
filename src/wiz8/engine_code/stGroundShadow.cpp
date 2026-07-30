#include "wiz8/ground_shadow.h"

#include "surrender/srCore.h"
#include "surrender/srTexture.h"
#include "wiz8/engine_code/Material.h"

#include <new>

/* The SR.DLL registry string is the original runtime class identity. */

extern unsigned char g_ground_shadow_enabled_00685110;
extern srTexture* g_ground_shadow_texture_006834cc;
extern srMaterial* g_ground_shadow_material_006834d0;
extern unsigned long g_ground_shadow_shader_006834c8;
extern unsigned char g_ground_shadow_material_parameters_00683430[];

srTexture* LoadTexture004B95D0(
    const char* folder, const char* name, unsigned char required);

// SYNTHETIC: WIZ8 0x004D6340
// stGroundShadow::`scalar deleting destructor'
// SYNTHETIC: WIZ8 0x004D6B50
// W8GroundShadowRegistry005ED3F8::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004D61B0
stGroundShadow::stGroundShadow(srNode* parent)
    : W8GroundShadowRegistry005ED3F8(0)
{
    field_138 = 0;
    value_13c = 500;
    value_140 = 500;
    setParent(parent, 1);

    if (g_ground_shadow_texture_006834cc == 0) {
        g_ground_shadow_texture_006834cc = LoadTexture004B95D0(
            "Data\\Monsters\\Bitmaps\\", "Shadow.tga", 1);
        g_ground_shadow_texture_006834cc->addReference();
        g_ground_shadow_texture_006834cc->setMipmap(
            static_cast<srTextureIFace::e_mipmap>(0));
        g_ground_shadow_texture_006834cc->setWrapS(
            static_cast<srTextureIFace::e_wrap>(1));
        g_ground_shadow_texture_006834cc->setWrapT(
            static_cast<srTextureIFace::e_wrap>(1));

        W8Material005EBDE0* material = new W8Material005EBDE0;
        g_ground_shadow_material_006834d0 = material;
        material->setParameterSource(
            g_ground_shadow_material_parameters_00683430);
        g_ground_shadow_shader_006834c8 =
            (g_ground_shadow_shader_006834c8 & 0xfeff9277UL) |
            0x00808260UL;
    }
}

// FUNCTION: WIZ8 0x004d6430
stGroundShadow::stGroundShadow(const stGroundShadow& other)
    : W8GroundShadowRegistry005ED3F8(0)
{
    setParent(other.parentNode(), 1);
    setName(other.getName());
    field_138 = other.field_138;
    value_13c = other.value_13c;
    value_140 = other.value_140;
}

// FUNCTION: WIZ8 0x004d6540
void stGroundShadow::traverse(TraverseInfo& info)
{
    unsigned int old_capacity;
    unsigned int new_capacity;
    unsigned int copy_count;
    unsigned int index;
    TraverseInfo::Entry* replacement;

    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_POSITIONAL_0)) {
        old_capacity = info.entries.capacity;
        if (old_capacity <= info.entry_count) {
            new_capacity = old_capacity + 8 + info.entry_count;
            if (old_capacity != new_capacity) {
                replacement = 0;
                if (new_capacity != 0) {
                    replacement = static_cast<TraverseInfo::Entry*>(
                        ::operator new(new_capacity * sizeof(TraverseInfo::Entry)));
                    if (info.entries.data != 0 && old_capacity != 0) {
                        copy_count = new_capacity < old_capacity
                            ? new_capacity : old_capacity;
                        for (index = 0; index < copy_count; ++index) {
                            replacement[index] = info.entries.data[index];
                        }
                    }
                }
                ::operator delete(info.entries.data);
                info.entries.data = replacement;
                info.entries.capacity = new_capacity;
            }
        }
        info.entries.data[info.entry_count].node = this;
        info.entries.data[info.entry_count].value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_POSITIONAL_1) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

// FUNCTION: WIZ8 0x004d6640
void stGroundShadow::process(const ProcessInfo& info, e_processType)
{
    if (g_ground_shadow_enabled_00685110 != 0) {
        if (info.renderer->hasPickState()) {
            srGERD::Pick pick;
            info.renderer->popPick(pick);
            renderGroundShadow(info.renderer);
            info.renderer->pushPick(pick);
            return;
        }
        renderGroundShadow(info.renderer);
    }
}

// FUNCTION: WIZ8 0x004d69a0
unsigned long W8GroundShadowRegistry005ED3F8::getClassID() const
{
    return 0x10010;
}

// FUNCTION: WIZ8 0x004d69b0
const char* W8GroundShadowRegistry005ED3F8::getClassName() const
{
    return "stGroundShadow";
}

// FUNCTION: WIZ8 0x004d69c0
srRegistry::ClassNode* W8GroundShadowRegistry005ED3F8::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10010);

    if (node == 0) {
        srRegistry* parent_registry = srCore.getRegistry();
        node = parent_registry->getClassNode(0x1000);
        if (node == 0) {
            node = parent_registry->registerClass(
                srNode::sGetClassName(),
                srClass::sGetClassNode(),
                0x1000,
                1);
        }
        node = registry->registerClass(
            "stGroundShadow",
            node,
            0x10010,
            0);
    }
    return node;
}

// FUNCTION: WIZ8 0x004D6370
stGroundShadow::~stGroundShadow()
{
}

// FUNCTION: WIZ8 0x004d6a30
srClass* W8GroundShadowRegistry005ED3F8::clone()
{
    stGroundShadow* source = static_cast<stGroundShadow*>(this);
    stGroundShadow* instance = static_cast<stGroundShadow*>(source->vInstance());

    *static_cast<srNode*>(instance) = *source;
    instance->field_138 = source->field_138;
    instance->value_13c = source->value_13c;
    instance->value_140 = source->value_140;
    return instance;
}

// FUNCTION: WIZ8 0x004d6bf0
srClass* stGroundShadow::vInstance()
{
    return new stGroundShadow(0);
}
