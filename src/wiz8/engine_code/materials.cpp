#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srStatisticsManager.h"
#include "surrender/srVertexProcessor.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "wiz8/engine_code/stTextureFile.h"
#include "wiz8/float_constants.h"
#include "wiz8/render_state.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"

#include "DEBUG.H"
#include "FileMan.h"

#include <cstdio>
#include <cstring>
#include <new>

#define MATERIALS_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\materials.cpp"

extern "C" unsigned char g_flag_65beaf;
extern void UpdatePleaseWaitLoadFrame005915A0(void);

/* The global at 0x0065BEA8 is a real zero-storage srVertexProcessor subclass:
   its non-template process body establishes the boundary independently of its
   constructor and vtable. No source or export name survives, so the class
   remains address-qualified. */
// VTABLE: WIZ8 0x005ED0D0
// class W8MaterialMapper004B89A0
class W8MaterialMapper004B89A0 : public srVertexProcessor {
public:
    W8MaterialMapper004B89A0();
    virtual ~W8MaterialMapper004B89A0() override {}
    // FUNCTION: WIZ8 0x004D6190
    virtual int isActive(srVertexPipe&) override { return 1; }
    virtual void process(srVertexPipe& pipe) override;
};

static_assert(sizeof(W8MaterialMapper004B89A0) == 4,
              "W8MaterialMapper004B89A0_must_be_4");

// GLOBAL: WIZ8 0x0065BEA8
W8MaterialMapper004B89A0 g_material_mapper_0065bea8;

// FUNCTION: WIZ8 0x004B89A0
W8MaterialMapper004B89A0::W8MaterialMapper004B89A0()
{
}

/* Convert eye-space normals to the material's first texture-coordinate set.
   The exported srVertexPipe queries preserve the closed renderer's ownership
   of its internal workspace while expressing every operation in this body. */
// FUNCTION: WIZ8 0x004B89B0
void W8MaterialMapper004B89A0::process(srVertexPipe& pipe)
{
    const srVector3T<float>* normals;
    srVector2T<float>* coordinates;
    unsigned long count;
    unsigned long index;

    if (!pipe.isChannelAvailable(
            static_cast<srVertexProcessor::e_channel>(5))) {
        return;
    }
    normals = pipe.getEyeSpaceNormal();
    coordinates = pipe.getST(0, 0);
    count = pipe.getVertexCount();
    srCore.getStatisticsManager()->statistics_00
        .texture_coordinate_operations_34 += count;
    for (index = 0; index < count; ++index) {
        coordinates[index].x =
            (normals[index].x + g_float_005ebb38) * g_float_005ebc7c;
        coordinates[index].y =
            (normals[index].y + g_float_005ebb38) * g_float_005ebc7c;
    }
}

// SYNTHETIC: WIZ8 0x004B8A50
// W8MaterialMapper004B89A0::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004925B0
stMaterial::stMaterial()
{
    srCore.getRegistry()->registerInstance(getClassNode(), this);
    m_field_78 = 0;
}


// Slot 0. The name the constructor also hands to srRegistry::registerClass.
// FUNCTION: WIZ8 0x00492950
const char* stMaterial::getClassName() const
{
    return "stMaterial";
}

// Slot 1. The class id registered for stMaterial.
// FUNCTION: WIZ8 0x00492940
unsigned long stMaterial::getClassID() const
{
    return 0x10002;
}

/* Slot 2. Ensures the class tree this instance registers against exists,
   walking down from stMaterial's own id to whichever ancestor is already
   registered and building back up. The three registry reads are three separate
   loads of srCore, not one cached pointer. */
// FUNCTION: WIZ8 0x00492960
srRegistry::ClassNode* stMaterial::getClassNode() const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x10002);

    if (node == 0) {
        srRegistry* material_registry = srCore.getRegistry();

        node = material_registry->getClassNode(0x2210);
        if (node == 0) {
            srRegistry* interface_registry = srCore.getRegistry();

            node = interface_registry->getClassNode(0x2200);
            if (node == 0) {
                node = interface_registry->registerClass(
                    srMaterialIFace::sGetClassName(),
                    srClass::sGetClassNode(),
                    0x2200,
                    1);
            }
            node = material_registry->registerClass(
                srMaterial::sGetClassName(), node, 0x2210, 0);
        }
        node = registry->registerClass("stMaterial", node, 0x10002, 0);
    }
    return node;
}

/* Slot 5. The complete destructor at 0x00492A30 is not recovered: it is 425
   bytes that tear the instance out of the registry at three levels, and the two
   it unwinds through are levels this class model does not yet have. Only the
   compiler-generated deleting destructor above it is claimed, which is what
   proves the srHeap routing on srClass::operator delete. */
stMaterial::~stMaterial()
{
}

// Slot 7. Copies through the instance slot 6 returns, then carries the one
// field srMaterial's assignment operator cannot know about.
// FUNCTION: WIZ8 0x00492a00
srClass* stMaterial::clone()
{
    /* srClass is srMaterial's base at offset zero, so the original reuses the
       returned pointer without adjusting it. */
    stMaterial* instance = static_cast<stMaterial*>(vInstance());

    if (this != instance) {
        *static_cast<srMaterial*>(instance) = *this;
        instance->m_field_78 = m_field_78;
    }
    return instance;
}

/* Select the first populated texture layer, load its file or IFL animation,
   derive the renderer flags, and retain a registry-cached stMaterial keyed by
   all serialized parameters that affect it. The sixth caller argument is an
   established cdecl extra argument: retail 0x004B8A70 never reads it and
   always passes required=1 to the texture loaders. */
// FUNCTION: WIZ8 0x004B8A70
unsigned char LoadMaterial004B8A70(
    const char* bitmap_folder, const W8MaterialRecord004B8A70* source,
    srMaterialIFace** material, srTextureIFace** texture,
    unsigned long* render_flags, int)
{
    char texture_path[80] = "";
    char material_name[80] = "";
    char drive[_MAX_PATH];
    char directory[_MAX_PATH];
    char file_name[_MAX_PATH];
    char extension[_MAX_PATH];
    char texture_folder[_MAX_PATH];
    char texture_file[_MAX_PATH];
    int texture_index = -1;
    unsigned char has_alpha = 0;
    int index;

    *render_flags = 0x0100a51b;
    for (index = 0; index < 4; ++index) {
        if (source->texture_names_029[index][0] != '\0') {
            if (bitmap_folder[0] == '\0') {
                strcpy(texture_path, source->texture_names_029[index]);
            }
            else {
                sprintf(texture_path, "%s\\%s", bitmap_folder,
                        source->texture_names_029[index]);
            }
            texture_index = index;
            break;
        }
    }

    if (texture_path[0] == '\0') {
        *render_flags &= ~0x8000UL;
    }
    else {
        _splitpath(texture_path, drive, directory, file_name, extension);
        strcpy(texture_folder, drive);
        strcat(texture_folder, directory);
        strcpy(texture_file, file_name);
        strcat(texture_file, extension);

        if (strlen(texture_path) > 3 &&
            _strnicmp(extension, ".IFL", 4) == 0) {
            *texture = LoadAnimatedTexture004B98F0(
                texture_folder, texture_file, source, 1);
        }
        else {
            *texture = LoadTexture004B95D0(
                texture_folder, texture_file, 1);
        }
        if (*texture == 0) {
            return 0;
        }

        if ((*texture)->getClassID() == 0x10001 &&
            static_cast<stTextureFile*>(*texture)->hasAlpha()) {
            has_alpha = 1;
        }
        if ((*texture)->getClassID() == stTextureAnim::CLASS_ID) {
            stTextureAnim* animation = static_cast<stTextureAnim*>(*texture);

            if (animation->Prepare004857B0()) {
                has_alpha = 1;
            }
            if (source->version_00 > 3 &&
                source->texture_modes_11a[texture_index] > 0.0f) {
                float mode = source->texture_modes_11a[texture_index];
                if (mode <= 1.0f) {
                    animation->value_70 = 1;
                }
                else {
                    animation->value_70 = 2;
                    mode -= 1.0f;
                }
                animation->value_74 = mode;
            }
        }
    }

    if (source->opacity_0fd < 1.0f || has_alpha) {
        if (texture_index == 0 || texture_index == 1) {
            *render_flags = (*render_flags & 0xffffdfbfUL) | 0x40a0;
        }
        else if (texture_index == 2 || texture_index == 3) {
            *render_flags = (*render_flags & 0xffffdc3fUL) | 0x4020;
        }
        *render_flags &= ~8UL;
    }

    sprintf(material_name,
            "Mt%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f%1.2f"
            "%1.2f%1.2f%1.2f%1.2f%1.2f%d%c",
            source->ambient_0c9[0], source->ambient_0c9[1],
            source->ambient_0c9[2], source->diffuse_0d5[0],
            source->diffuse_0d5[1], source->diffuse_0d5[2],
            source->specular_0ed[0], source->specular_0ed[1],
            source->specular_0ed[2], source->positional_0f9,
            source->opacity_0fd, source->emission_101,
            source->emission_101, source->emission_101,
            static_cast<int>(source->shader_flags_116),
            texture_path[0] == '\0' ? 'F' : 'T');

    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10002);
        stMaterial* concrete;

        if (node == 0) {
            node = registry->registerClass(
                "stMaterial",
                srClassSupport<srMaterial, srMaterialIFace, false,
                               0x2210>::sGetClassNode(),
                0x10002, 0);
        }
        concrete = static_cast<stMaterial*>(
            registry->find(node, material_name,
                           static_cast<const srRuntimeClass*>(0)));
        *material = concrete;
        if (concrete == 0) {
            concrete = new stMaterial;
            *material = concrete;
            if (concrete == 0) {
                srAssertFail("*ppstMaterial", MATERIALS_CPP, 0xe4, 0);
            }
            concrete->setName(material_name);
            concrete->autoRelease();

            concrete->parms_18.specular.x = source->specular_0ed[0];
            concrete->parms_18.specular.y = source->specular_0ed[1];
            concrete->parms_18.specular.z = source->specular_0ed[2];
            concrete->parms_18.specular.w = 0.0f;
            concrete->dirty_74 = 1;
            concrete->parms_18.shininess = 1.0f;
            concrete->dirty_74 = 1;

            concrete->parms_18.diffuse.x = source->diffuse_0d5[0];
            concrete->parms_18.diffuse.y = source->diffuse_0d5[1];
            concrete->parms_18.diffuse.z = source->diffuse_0d5[2];
            concrete->parms_18.diffuse.w =
                source->opacity_0fd == 0.0f ? 0.7f : source->opacity_0fd;
            concrete->dirty_74 = 1;
            concrete->setOpacity(
                source->opacity_0fd == 0.0f ? 0.7 : source->opacity_0fd);

            if (texture_path[0] == '\0') {
                concrete->parms_18.ambient.x = source->diffuse_0d5[0];
                concrete->parms_18.ambient.y = source->diffuse_0d5[1];
                concrete->parms_18.ambient.z = source->diffuse_0d5[2];
                concrete->parms_18.ambient.w = 1.0f;
                concrete->dirty_74 = 1;
                concrete->parms_18.emissive.x = 0.0f;
                concrete->parms_18.emissive.y = 0.0f;
                concrete->parms_18.emissive.z = 0.0f;
                concrete->parms_18.emissive.w = 0.0f;
            }
            else {
                concrete->parms_18.ambient.x = source->ambient_0c9[0];
                concrete->parms_18.ambient.y = source->ambient_0c9[1];
                concrete->parms_18.ambient.z = source->ambient_0c9[2];
                concrete->parms_18.ambient.w = 0.0f;
                concrete->dirty_74 = 1;
                concrete->parms_18.emissive.x = source->emission_101;
                concrete->parms_18.emissive.y = source->emission_101;
                concrete->parms_18.emissive.z = source->emission_101;
                concrete->parms_18.emissive.w = 1.0f;
            }
            concrete->dirty_74 = 1;
            concrete->m_field_78 = source->shader_flags_116;
            if ((source->shader_flags_116 & 0x1fe) != 0) {
                concrete->setMapper(&g_material_mapper_0065bea8);
            }
        }
    }
    (*material)->addReference();
    return 1;
}

// FUNCTION: WIZ8 0x004B95D0
srTexture* LoadTexture004B95D0(
    const char* folder, const char* name, unsigned char required)
{
    char path[_MAX_PATH];
    char* extension;
    srRegistry* registry;
    srRegistry::ClassNode* node;
    stTextureFile* texture;

    if (g_screen_state_0068ec78.id == 4) {
        UpdatePleaseWaitLoadFrame005915A0();
    }
    strcpy(path, folder);
    strcat(path, name);
    extension = path + strlen(path) - 3;
    *extension = '\0';

    registry = srCore.getRegistry();
    node = registry->getClassNode(0x10001);
    if (node == 0) {
        node = registry->registerClass(
            "stTextureFile", stTextureFile::sGetClassNode(), 0x10001, 0);
    }
    texture = static_cast<stTextureFile*>(
        registry->find(node, name, static_cast<const srRuntimeClass*>(0)));
    if (texture == 0) {
        strcat(extension, "tga");
        if (required != 0) {
            texture = new stTextureFile(path, g_flag_65beaf);
            if (texture == 0) {
                srAssertFail("psrTexture", MATERIALS_CPP, 0x191, 0);
            }
            if (g_gerd_659634 != 0) {
                g_gerd_659634->setTexture(texture, 0);
                g_gerd_659634->setTexture(0, 0);
            }
            if (texture->getTextureFrameHandle() == 0) {
                *extension = '\0';
                strcat(extension, "jpg");
                texture->setFileName(path);
                if (g_gerd_659634 != 0) {
                    g_gerd_659634->setTexture(texture, 0);
                    g_gerd_659634->setTexture(0, 0);
                }
                if (texture->getTextureFrameHandle() == 0) {
                    ReportError00401920(reinterpret_cast<const char*>(
                        String("Missing texture file: %s", path)));
                }
            }
        }
        else {
            if (!FileExists(path)) {
                *extension = '\0';
                strcat(extension, "jpg");
                if (!FileExists(path)) {
                    ReportError00401920(reinterpret_cast<const char*>(
                        String("Missing texture file: %s", path)));
                }
            }
            texture = new stTextureFile(path, g_flag_65beaf);
            if (texture == 0) {
                srAssertFail("psrTexture", MATERIALS_CPP, 0x1a7, 0);
            }
        }
        texture->autoRelease();
        *extension = '\0';
        texture->setName(name);
    }
    return texture;
}

// FUNCTION: WIZ8 0x004B98F0
stTextureAnim* LoadAnimatedTexture004B98F0(
    const char* folder, const char* name,
    const W8MaterialRecord004B8A70* source, unsigned char required)
{
    char buffer[_MAX_PATH];
    unsigned char more = 1;
    int handle;
    stTextureAnim* animation;

    strcpy(buffer, folder);
    strcat(buffer, name);
    handle = FileOpen(buffer, 0x41, 0);
    if (handle == 0) {
        ReportError00401920(reinterpret_cast<const char*>(
            String("Cannot load/find material: %s", buffer)));
    }

    animation = new stTextureAnim;
    animation->autoRelease();
    animation->setName(name);
    while (more != 0) {
        ReadTextLine004CEE40(handle, buffer, 200, &more);
        if (strlen(buffer) <= 2) {
            more = 0;
        }
        else {
            srTexture* texture = LoadTexture004B95D0(folder, buffer, required);
            if (texture != 0) {
                animation->AddTexture00485420(texture);
            }
            if (more != 0) {
                continue;
            }
        }
        break;
    }
    animation->setupDefaultValues();
    CloseVirtualFile(handle);
    if (source != 0) {
        int frame = source->animation_frame_10e;
        animation->flag_60 = source->animation_mode_10d;
        animation->value_64 = frame;
        animation->frame_58 = frame;
        animation->frame_rate_68 = source->animation_rate_112;
    }
    return animation;
}

/* getPolyTexture selects the layer and table up front, then returns one smart
   pointer per polygon. Report whether any selected texture is animated. */
// FUNCTION: WIZ8 0x004b9aa0
unsigned char MeshHasAnimatedTexture004B9AA0(srMeshModel* model)
{
    if (model != 0) {
        srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 0);

        if (textures != 0) {
            int polygon;

            for (polygon = 0; polygon < model->polygon_count_230; ++polygon) {
                srTextureIFace* texture = textures[polygon].get();

                if (texture != 0 &&
                    texture->getClassID() == stTextureAnim::CLASS_ID) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The model instance's srModel::Client base supplies its mesh model. The first
   polygon texture is the shared animation object whose frame is restarted. */
// FUNCTION: WIZ8 0x004b9b00
void SetModelAnimatedTextureFrame004B9B00(
    srModelInstance* instance, int frame)
{
    if (instance != 0) {
        srMeshModel* model = static_cast<srMeshModel*>(instance->model());

        if (model != 0) {
            srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 0);

            if (textures != 0) {
                srTextureIFace* texture = textures[0].get();

                if (texture != 0 &&
                    texture->getClassID() == stTextureAnim::CLASS_ID) {
                    static_cast<stTextureAnim*>(texture)->SetFrame00485400(frame);
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004B9B50
stTextureAnim* GetModelAnimatedTexture004B9B50(srModelInstance* instance)
{
    if (instance != 0) {
        srMeshModel* model = static_cast<srMeshModel*>(instance->model());

        if (model != 0) {
            srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 0);

            if (textures != 0) {
                srTextureIFace* texture = textures[0].get();

                if (texture != 0 &&
                    texture->getClassID() == stTextureAnim::CLASS_ID) {
                    return static_cast<stTextureAnim*>(texture);
                }
            }
        }
    }
    return 0;
}
