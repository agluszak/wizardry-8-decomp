#include "wiz8/engine_code/materials.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/local_code/Configuration.h"

#include "surrender/srMaterial.h"
#include "surrender/srTexture.h"

#include <new>

/* The SR.DLL registry string is the original runtime class identity. */

extern srTexture* g_ground_shadow_texture_006834cc;
extern srMaterial* g_ground_shadow_material_006834d0;
// GLOBAL: WIZ8 0x006834cc
srTexture* g_ground_shadow_texture_006834cc;
// GLOBAL: WIZ8 0x006834d0
srMaterial* g_ground_shadow_material_006834d0;
extern unsigned long g_ground_shadow_shader_006834c8;
// GLOBAL: WIZ8 0x006834c8
unsigned long g_ground_shadow_shader_006834c8;
/* 0x00683430: the vertex-processor parameters the material mapper is
   installed with; the next recovered global begins at 0x006834C8. */
// GLOBAL: WIZ8 0x00683430
unsigned char g_ground_shadow_material_parameters_00683430[0x98] = {0};

// SYNTHETIC: WIZ8 0x004D6340
// stGroundShadow::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ed3f8
// class srClassSupport<stGroundShadow,srNode,0,65552>

// SYNTHETIC: WIZ8 0x004D6B50
// srClassSupport<stGroundShadow,srNode,0,65552>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004D6A70
// srClassSupport<stGroundShadow,srNode,0,65552>::~srClassSupport

// FUNCTION: WIZ8 0x004D61B0
stGroundShadow::stGroundShadow(srNode* parent)
    : srClassSupport<stGroundShadow, srNode, false, 0x10010>(static_cast<srNode*>(0))
{
    angle_138 = 0;
    value_13c = 500;
    value_140 = 500;
    setParent(parent, 1);

    if (g_ground_shadow_texture_006834cc == 0) {
        g_ground_shadow_texture_006834cc =
            LoadTexture004B95D0("Data\\Monsters\\Bitmaps\\", "Shadow.tga", 1);
        g_ground_shadow_texture_006834cc->addReference();
        g_ground_shadow_texture_006834cc->setMipmap(static_cast<srTextureIFace::e_mipmap>(0));
        g_ground_shadow_texture_006834cc->setWrapS(srTextureIFace::WRAP_CLAMP);
        g_ground_shadow_texture_006834cc->setWrapT(srTextureIFace::WRAP_CLAMP);

        srMaterial* material = SR_NEW(srMaterial);
        g_ground_shadow_material_006834d0 = material;
        material->setMapper(
            reinterpret_cast<srVertexProcessor*>(g_ground_shadow_material_parameters_00683430));
        g_ground_shadow_shader_006834c8 =
            (g_ground_shadow_shader_006834c8 & 0xfeff9277UL) | 0x00808260UL;
    }
}

// FUNCTION: WIZ8 0x004d6430
stGroundShadow::stGroundShadow(const stGroundShadow& other)
    : srClassSupport<stGroundShadow, srNode, false, 0x10010>(static_cast<srNode*>(0))
{
    setParent(other.parentNode(), 1);
    setName(other.getName());
    angle_138 = other.angle_138;
    value_13c = other.value_13c;
    value_140 = other.value_140;
}

void stGroundShadow::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_OMIT_SELF)) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_SKIP_CHILDREN) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

// FUNCTION: WIZ8 0x004d6640
void stGroundShadow::process(const ProcessInfo& info, e_processType)
{
    if (g_settings_6850c8.monster_shadows != 0) {
        if (!info.renderer->isPickStackEmpty()) {
            srGERD::Pick pick;
            info.renderer->popPick(pick);
            renderGroundShadow(info.renderer);
            info.renderer->pushPick(pick);
            return;
        }
        renderGroundShadow(info.renderer);
    }
}

// TEMPLATE: WIZ8 0x004d69a0
// srClassSupport<stGroundShadow,srNode,0,65552>::getClassID

// TEMPLATE: WIZ8 0x004d69b0
// srClassSupport<stGroundShadow,srNode,0,65552>::getClassName

// TEMPLATE: WIZ8 0x004d69c0
// srClassSupport<stGroundShadow,srNode,0,65552>::getClassNode

// FUNCTION: WIZ8 0x004D6370
stGroundShadow::~stGroundShadow() {}

// TEMPLATE: WIZ8 0x004d6a30
// srClassSupport<stGroundShadow,srNode,0,65552>::clone

// FUNCTION: WIZ8 0x004d6bf0
srClass* stGroundShadow::vInstance()
{
    return new stGroundShadow(0);
}
