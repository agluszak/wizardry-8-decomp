#include "surrender/srColorSurface.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srScene.h"
#include "wiz8/sr_api.h"
#include "wiz8/render_state.h"
#include "wiz8/surface2d.h"
#include "wiz8/wiz8_windows.h"

#include <string.h>

extern "C" void Function425B40(void);

static srRegistry::ClassNode* node_class_node()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1000);
    if (!node) {
        node = registry->registerClass(
            srNode::sGetClassName(), srClass::sGetClassNode(), 0x1000, 1);
    }
    return node;
}

static srRegistry::ClassNode* concrete_class_node(
    unsigned long class_id, const char* name)
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(class_id);
    if (!node) {
        node = registry->registerClass(name, node_class_node(), class_id, 0);
    }
    return node;
}

static srRegistry::ClassNode* color_surface_class_node()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x3110);
    if (!node) {
        srRegistry::ClassNode* parent = registry->getClassNode(0x3100);
        if (!parent) {
            parent = registry->registerClass(
                srColorSurfaceIFace::sGetClassName(),
                srClass::sGetClassNode(), 0x3100, 1);
        }
        node = registry->registerClass(
            srColorSurface::sGetClassName(), parent, 0x3110, 0);
    }
    return node;
}

static srRegistry::ClassNode* material_class_node()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x2210);
    if (!node) {
        srRegistry::ClassNode* parent = registry->getClassNode(0x2200);
        if (!parent) {
            parent = registry->registerClass(
                srMaterialIFace::sGetClassName(),
                srClass::sGetClassNode(), 0x2200, 1);
        }
        node = registry->registerClass(
            srMaterial::sGetClassName(), parent, 0x2210, 0);
    }
    return node;
}

class W8ColorSurface : public srColorSurface {
public:
    W8ColorSurface(srPixelConvert::e_surfaceType type,
                   unsigned long width, unsigned long height)
        : srColorSurface(type, width, height) {}

    W8ColorSurface(srPixelConvert::e_surfaceType type, void* data,
                   unsigned long width, unsigned long height,
                   unsigned long pitch)
        : srColorSurface(type, data, width, height, pitch) {}

    virtual const char* getClassName() const {
        return srColorSurface::sGetClassName();
    }
    virtual unsigned long getClassID() const { return 0x3110; }
    virtual srRegistry::ClassNode* getClassNode() const {
        return color_surface_class_node();
    }
    virtual srColorSurfaceIFace* clone() {
        srColorSurface* copy = static_cast<srColorSurface*>(vInstance());
        *copy = *this;
        return copy;
    }
};

class W8Scene : public srScene {
public:
    W8Scene(srNode* parent) : srScene(parent) {}

    virtual const char* getClassName() const { return "srScene"; }
    virtual unsigned long getClassID() const { return 0x1010; }
    virtual srRegistry::ClassNode* getClassNode() const {
        return concrete_class_node(0x1010, "srScene");
    }
    virtual srNode* clone() {
        srScene* copy = static_cast<srScene*>(vInstance());
        *copy = *this;
        return copy;
    }

    void clearOverlayState() {
        int index;
        for (index = 0; index != 6; ++index) overlay_state_[index] = 0;
    }
};

class W8Camera : public srCamera {
public:
    W8Camera(srNode* parent) : srCamera(parent) {}

    virtual const char* getClassName() const { return "srCamera"; }
    virtual unsigned long getClassID() const { return 0x1400; }
    virtual srRegistry::ClassNode* getClassNode() const {
        return concrete_class_node(0x1400, "srCamera");
    }
    virtual srNode* clone() {
        srCamera* copy = static_cast<srCamera*>(vInstance());
        *copy = *this;
        return copy;
    }
};

class W8Material : public srMaterial {
public:
    W8Material() {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* iface = registry->getClassNode(0x2200);
        if (!iface) {
            iface = registry->registerClass(
                srMaterialIFace::sGetClassName(),
                srClass::sGetClassNode(), 0x2200, 1);
        }
        registry->registerInstance(iface, this);
        registry->registerInstance(material_class_node(), this);
        field_68 = 0;
        field_6c = 0;
        reset();
    }

    virtual const char* getClassName() const {
        return srMaterial::sGetClassName();
    }
    virtual unsigned long getClassID() const { return 0x2210; }
    virtual srRegistry::ClassNode* getClassNode() const {
        return material_class_node();
    }
    virtual srMaterial* vslot7() {
        srMaterial* copy = static_cast<srMaterial*>(vInstance());
        *copy = *this;
        return copy;
    }

    void initializeBlitRect() {
        srVector4T<float> value;
        value.x = 1.0f;
        value.y = 1.0f;
        value.z = 1.0f;
        value.w = 1.0f;
        setVector(vector_54, value);
        value.x = value.y = value.z = value.w = 0.0f;
        setVector(vector_18, value);
        setVector(vector_38, value);
        vector_18.w = 1.0f;
        field_74 = 1;
    }
};

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

extern void DDUnlockSurface(void* surface, void* locked);
extern void DDLockSurface(void* surface, RECT* area,
                          DDSURFACEDESC* description, int flags, void* event);
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

    g_mouse_surface_659688 = new W8ColorSurface(type, 128, 128);
    if (!g_mouse_surface_659688) {
        srAssertFail("psrMouseSurface", "C:\\Projects\\Wizardry 8\\Engine Code\\Video2.cpp",
                     0x635, 0);
    }
    g_mouse_surface_659688->setFilter(&srBoxFilter);
    g_mouse_surface_659688->fill(0);
    return 1;
}

static W8Scene* make_scene(const char* name)
{
    W8Scene* scene = new W8Scene(0);
    scene->setName(name);
    scene->clearOverlayState();
    return scene;
}

static W8Camera* make_camera(srNode* parent, const char* name,
                             double view_top)
{
    W8Camera* camera = new W8Camera(parent);
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
extern "C" unsigned char Function423500(void)
{
    DDSURFACEDESC surface_description;
    W8Material* material;
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

    material = new W8Material;
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
    g_primary_color_surface_659660 = new W8ColorSurface(
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

class W8ModelProvider {
public:
    virtual void slot0() = 0;
    virtual void slot1() = 0;
    virtual void slot2() = 0;
    virtual srMeshModel* getModel() = 0;
};

class W8OverlaySceneNode : public srNode {
public:
    W8ModelProvider* model_provider;        /* 0x138 */
    unsigned char unknown_13c[0x34];
    int display_state;                      /* 0x170 */
};

// FUNCTION: WIZ8 0x00426500
extern "C" void Function426500(srScene* scene)
{
    srNode* node;

    if (!scene) return;
    node = scene->firstChild();
    while (node) {
        srNode* next = node->nextSibling();
        unsigned long class_id = node->getClassID();
        if ((class_id == 0x10004 || class_id == 0x10005) &&
            static_cast<W8OverlaySceneNode*>(node)->display_state != 3) {
            int index;
            W8OverlaySceneNode* overlay =
                static_cast<W8OverlaySceneNode*>(node);
            for (index = 0; index != 0x12c0; ++index) {
                if (g_surface_nodes_654adc[index] == node) {
                    g_surface_nodes_654adc[index] = 0;
                    g_block_652ddc[index] = 0;
                }
            }
            if (overlay->model_provider) {
                srMeshModel* model = overlay->model_provider->getModel();
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
