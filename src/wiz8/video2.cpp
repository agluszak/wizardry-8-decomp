#include "surrender/srColorSurface.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "wiz8/engine_code/ColorSurface.h"
#include "wiz8/engine_code/Material.h"
#include "wiz8/engine_code/MeshModel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"
#include "surrender/srVertexProcessor.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/Scene.h"
#include "wiz8/engine_code/TextureMap.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/sr_api.h"
#include "wiz8/render_state.h"
#include "wiz8/surface2d.h"
#include "wiz8/wiz8_windows.h"
#include "DirectDraw Calls.h"

#include <string.h>

extern "C" void Function425B40(void);
extern srNode* Function424BA0(
    srTextureIFace* texture,
    float width,
    float height,
    unsigned char positional_3);

// TEMPLATE: WIZ8 0x00429B30
// srClassSupport<srMeshModel,srModel,0,8208>::getClassID

// TEMPLATE: WIZ8 0x00429B40
// srClassSupport<srMeshModel,srModel,0,8208>::getClassName

// TEMPLATE: WIZ8 0x00429B50
// srClassSupport<srMeshModel,srModel,0,8208>::getClassNode

// TEMPLATE: WIZ8 0x00429BC0
// srClassSupport<srMeshModel,srModel,0,8208>::clone

// SYNTHETIC: WIZ8 0x00424b70
// W8TextureMap005EBEEC::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00429BE0
// srClassSupport<srTextureMap,srTexture,0,8465>::getClassID

// TEMPLATE: WIZ8 0x00429BF0
// srClassSupport<srTextureMap,srTexture,0,8465>::getClassName

// TEMPLATE: WIZ8 0x00429C00
// srClassSupport<srTextureMap,srTexture,0,8465>::getClassNode

// TEMPLATE: WIZ8 0x00429CA0
// srClassSupport<srTextureMap,srTexture,0,8465>::clone

// TEMPLATE: WIZ8 0x00429CC0
// srClassSupport<srMaterial,srMaterialIFace,0,8720>::getClassID

// TEMPLATE: WIZ8 0x00429CE0
// srClassSupport<srMaterial,srMaterialIFace,0,8720>::getClassNode

// TEMPLATE: WIZ8 0x00429D50
// srClassSupport<srMaterial,srMaterialIFace,0,8720>::clone

// FUNCTION: WIZ8 0x00429E80
unsigned long W8Registered005EBF94::getClassID() const
{
    return 0x2200;
}

// TEMPLATE: WIZ8 0x0042A010
// srClassSupport<srCamera,srNode,0,5120>::getClassID

// TEMPLATE: WIZ8 0x0042A020
// srClassSupport<srCamera,srNode,0,5120>::getClassName

// TEMPLATE: WIZ8 0x0042A030
// srClassSupport<srCamera,srNode,0,5120>::getClassNode

// TEMPLATE: WIZ8 0x0042A0A0
// srClassSupport<srCamera,srNode,0,5120>::clone

// TEMPLATE: WIZ8 0x0042A0C0
// srClassSupport<srScene,srNode,0,4112>::getClassID

// TEMPLATE: WIZ8 0x0042A0D0
// srClassSupport<srScene,srNode,0,4112>::getClassName

// TEMPLATE: WIZ8 0x0042A0E0
// srClassSupport<srScene,srNode,0,4112>::getClassNode

// TEMPLATE: WIZ8 0x0042A150
// srClassSupport<srScene,srNode,0,4112>::clone

// FUNCTION: WIZ8 0x0042A360
srVertexProcessor::~srVertexProcessor()
{
}

// SYNTHETIC: WIZ8 0x0042B890
// srVertexProcessor::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00424A90
srNode* VideoMakePoster(
    srColorSurfaceIFace* surface,
    float width,
    float height,
    unsigned char positional_3)
{
    srTextureIFace::e_hint hint;
    W8TextureMap005EBEEC* texture = new W8TextureMap005EBEEC(0);
    texture->setMipmapBias(-8.0f);
    texture->autoRelease();
    texture->setName("VideoMakePoster");
    texture->setSurfacePtr(surface);
    texture->setWrapS(srTextureIFace::WRAP_POSITIONAL_1);
    texture->setWrapT(srTextureIFace::WRAP_POSITIONAL_1);
    if (positional_3 == 0) {
        hint = srTextureIFace::HINT_POSITIONAL_1;
    } else {
        hint = srTextureIFace::HINT_POSITIONAL_2;
    }
    texture->enableHint(hint);
    return Function424BA0(texture, width, height, positional_3);
}

// TEMPLATE: WIZ8 0x00429A40
// srClassSupport<srColorSurface,srColorSurfaceIFace,0,12560>::getClassID

// TEMPLATE: WIZ8 0x00429A50
// srClassSupport<srColorSurface,srColorSurfaceIFace,0,12560>::getClassName

// TEMPLATE: WIZ8 0x00429A60
// srClassSupport<srColorSurface,srColorSurfaceIFace,0,12560>::getClassNode

// TEMPLATE: WIZ8 0x00429AD0
// srClassSupport<srColorSurface,srColorSurfaceIFace,0,12560>::clone

extern "C" {
extern int g_pixel_format_603c48;
extern srModeler* g_modeler_65963c;
extern srScene* g_scene_user_659640;
extern srScene* g_scene_fullscreen_659644;
extern srScene* g_scene_permanent_659648;
extern srScene* g_scene_prerender0_65964c;
extern srScene* g_scene_prerender1_659650;
extern srScene* g_scene_overlay0_659654;
extern srScene* g_scene_overlay1_659658;
extern srScene* g_scene_square_65965c;
extern srColorSurface* g_primary_color_surface_659660;
extern srCamera* g_overlay_camera_659670;
extern srCamera* g_square_camera_659674;

}

extern "C" void PresentMenuOverlayFrame(void)
{
    srNode::ProcessInfo process;

    Function425B40();
    g_gerd_659634->beginFrame();
    process.renderer = g_gerd_659634;
    g_surface_node_659664->process(
        process, (srNode::e_processType)0);
    g_gerd_659634->flushRenderers();
    g_gerd_659634->endFrame();
}

// FUNCTION: WIZ8 0x00424040
unsigned char InitializeMouseSurface(void)
{
    srPixelConvert::e_surfaceType type;

    if (g_pixel_format_603c48 == 7) {
        type = srPixelConvert::SURFACE_RGB565;
    } else if (g_pixel_format_603c48 == 8) {
        type = srPixelConvert::SURFACE_RGB555;
    } else if (g_pixel_format_603c48 == 9) {
        type = srPixelConvert::SURFACE_ARGB1555;
    } else {
        return 0;
    }

    g_mouse_surface_659688 = new W8ColorSurface005EBD10(type, 128, 128);
    if (!g_mouse_surface_659688) {
        srAssertFail("psrMouseSurface", "C:\\Projects\\Wizardry 8\\Engine Code\\Video2.cpp",
                     0x635, 0);
    }
    g_mouse_surface_659688->setFilter(&srBoxFilter);
    g_mouse_surface_659688->fill(0);
    return 1;
}

static W8Scene005EBE48* make_scene(const char* name)
{
    W8Scene005EBE48* scene = new W8Scene005EBE48(0);
    scene->setName(name);
    scene->ClearOverlayState();
    return scene;
}

static W8Camera005EBE14* make_camera(srNode* parent, const char* name,
                                    double view_top)
{
    W8Camera005EBE14* camera = new W8Camera005EBE14(parent);
    srCamera::Rect view;
    camera->setName(name);
    camera->setClipRange(0.01, 2.0);
    camera->setLocation(0.0, 0.0, -1.0);
    camera->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = view_top;
    camera->setViewPlane(view, 1.0);
    camera->setEnvironmentRange(0.0f, 0.0f);
    return camera;
}

// FUNCTION: WIZ8 0x00423500
extern "C" unsigned char InitializeRendererSceneObjects(void)
{
    DDSURFACEDESC surface_description;
    W8Material005EBDE0* material;
    char renderer_name[128];

    InitializeMouseSurface();
    g_modeler_65963c = new srModeler;
    g_scene_permanent_659648 = make_scene("2D Permanent Overlay Scene");
    g_scene_user_659640 = make_scene("2D User Overlay Scene");
    g_scene_fullscreen_659644 = make_scene("Full Screen Overlay Scene");
    g_scene_overlay0_659654 = make_scene("2D Overlay Scene (0)");
    g_scene_overlay1_659658 = make_scene("2D Overlay Scene (1)");
    g_scene_square_65965c = make_scene("2D Square Overlay Scene");
    g_scene_prerender0_65964c = make_scene("2D Pre-render Overlay Scene (0)");
    g_scene_prerender1_659650 = make_scene("2D Pre-render Overlay Scene (1)");

    g_overlay_camera_659670 = make_camera(0, "2D Overlay Camera", 1.0);
    g_square_camera_659674 = make_camera(
        g_scene_square_65965c, "2D Square Overlay Camera", 0.75);

    material = new W8Material005EBDE0;
    g_blit_material_65967c = material;
    material->setName("Blit Rect Material");
    material->initializeBlitRect();

    memset(g_surface_nodes_654adc, 0, sizeof(g_surface_nodes_654adc));
    memset(g_block_652ddc, 0, sizeof(g_block_652ddc));
    g_viewport_left_6595e8 = 0;
    g_viewport_top_6595ec = 0;
    g_viewport_right_6595f0 = 0;
    g_surface_state_6595dc = 0x100a017;
    g_surface_state_654ad8 = 0x100c0b7;
    g_dword_6596d8 = 0;
    g_viewport_bottom_6595f4 = 0;

    memset(&surface_description, 0, sizeof(surface_description));
    surface_description.dwSize = sizeof(surface_description);
    DDLockSurface(g_primary_surface_6596a8, 0, &surface_description, 0, 0);
    DDUnlockSurface(g_primary_surface_6596a8, 0);
    g_primary_color_surface_659660 = new W8ColorSurface005EBD10(
        srPixelConvert::SURFACE_ARGB1555, surface_description.lpSurface,
        640, 480, surface_description.lPitch);
    if (!g_primary_color_surface_659660) return 0;

    g_surface_node_659664 = new stSurface2D(
        g_primary_color_surface_659660, 640, 480,
        g_scene_overlay0_659654, 128);
    if (!g_surface_node_659664) return 0;

    strncpy(renderer_name, g_gerd_659634->getName(), 127);
    renderer_name[127] = 0;
    _strupr(renderer_name);
    if (strstr(renderer_name, "GLIDE")) {
        g_surface_node_659664->enableRendererFlag(1);
    }
    g_renderer_mode_603d74 =
        strstr(renderer_name, "DIRECT3D") ||
        strstr(renderer_name, "GLIDE") ||
        strstr(renderer_name, "SOFTWARE2");
    return 1;
}

/* Zero a rectangle of the primary surface, one row at a time. The span is
   doubled because the surface holds sixteen-bit pixels, and the row clear is an
   ordinary memset that VC6 expands into a dword run with a byte remainder.

   Unlike the other lock site in this unit, the descriptor is not cleared before
   locking. That is the original's own sequence, reproduced. */
// FUNCTION: WIZ8 0x004263f0
extern "C" void ClearSurfaceRect(int left, unsigned int top, int right, unsigned int bottom)
{
    DDSURFACEDESC surface_description;
    unsigned char* row;
    int rows;

    DDLockSurface(g_primary_surface_6596a8, 0, &surface_description, 0, 0);
    if (surface_description.lpSurface != 0) {
        if (top < bottom) {
            row = reinterpret_cast<unsigned char*>(surface_description.lpSurface) +
                  left * 2 + surface_description.lPitch * top;
            rows = bottom - top;
            do {
                memset(row, 0, (right - left) * 2);
                row = row + surface_description.lPitch;
                --rows;
            } while (rows != 0);
        }
        DDUnlockSurface(g_primary_surface_6596a8, 0);
    }
}

// FUNCTION: WIZ8 0x00426500
extern "C" void Function426500(srScene* scene)
{
    srNode* node;

    if (!scene) return;
    node = scene->firstChild();
    while (node) {
        srNode* next = node->nextSibling();
        unsigned long class_id = node->getClassID();
        srModelInstance* instance = static_cast<srModelInstance*>(node);
        unsigned char display_state = 0;
        if (class_id == 0x10004) {
            display_state = static_cast<stModelInstance*>(node)->displayState();
        }
        else if (class_id == 0x10005) {
            display_state = static_cast<stModelInstance2D*>(node)->displayState();
        }
        if ((class_id == 0x10004 || class_id == 0x10005) &&
            display_state != 3) {
            int index;
            for (index = 0; index != 0x12c0; ++index) {
                if (g_surface_nodes_654adc[index] == node) {
                    g_surface_nodes_654adc[index] = 0;
                    g_block_652ddc[index] = 0;
                }
            }
            if (instance->model()) {
                srMeshModel* model =
                    static_cast<srMeshModel*>(instance->model());
                if (model) {
                    srTextureIFace* texture = model->getTexture(0, 0);
                    if (texture) texture->invalidate();
                }
            }
            node->release();
        }
        node = next;
    }
}
