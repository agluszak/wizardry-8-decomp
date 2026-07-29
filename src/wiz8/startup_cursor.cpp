#include "surrender/srColorSurface.h"
#include "surrender/srCore.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"
#include "surrender/srTexture.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/render_state.h"

namespace {

unsigned long float_bits(float value)
{
    union {
        float floating;
        unsigned long bits;
    } representation;
    representation.floating = value;
    return representation.bits;
}

srRegistry::ClassNode* class_node(unsigned long id, const char* name,
                                  unsigned long parent_id,
                                  const char* parent_name)
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(id);
    if (!node) {
        srRegistry::ClassNode* parent = registry->getClassNode(parent_id);
        if (!parent) {
            parent = registry->registerClass(
                parent_name, srClass::sGetClassNode(), parent_id, 1);
        }
        node = registry->registerClass(name, parent, id, 0);
    }
    return node;
}

class CursorScene : public srScene {
public:
    CursorScene() : srScene(0) {
        int index;
        for (index = 0; index != 6; ++index) {
            overlay_state_[index] = 0;
        }
    }

    virtual const char* getClassName() const override { return "srScene"; }
    virtual unsigned long getClassID() const override { return 0x1010; }
    virtual srRegistry::ClassNode* getClassNode() const override {
        return class_node(0x1010, "srScene", 0x1000, "srNode");
    }
};

class CursorMeshModel : public srMeshModel {
public:
    CursorMeshModel() : srMeshModel(0, 0) {}

    virtual const char* getClassName() const override { return "srMeshModel"; }
    virtual unsigned long getClassID() const override { return 0x2010; }
    virtual srRegistry::ClassNode* getClassNode() const override {
        return class_node(0x2010, "srMeshModel", 0x2000, "srModel");
    }
};

class CursorTextureMap : public srTextureMap {
public:
    CursorTextureMap(srColorSurfaceIFace* surface) : srTextureMap(surface) {}

    virtual const char* getClassName() const override { return "srTextureMap"; }
    virtual unsigned long getClassID() const override { return 0x2111; }
    virtual srRegistry::ClassNode* getClassNode() const override {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x2111);
        if (!node) {
            srRegistry::ClassNode* texture = registry->getClassNode(0x2110);
            if (!texture) {
                srRegistry::ClassNode* iface = registry->getClassNode(0x2100);
                if (!iface) {
                    iface = registry->registerClass(
                        "srTextureIFace", srClass::sGetClassNode(), 0x2100, 1);
                }
                texture = registry->registerClass(
                    srTexture::sGetClassName(), iface, 0x2110, 0);
            }
            node = registry->registerClass(
                "srTextureMap", texture, 0x2111, 0);
        }
        return node;
    }
    virtual srTextureIFace* clone() override {
        CursorTextureMap* copy = new CursorTextureMap(0);
        static_cast<srTextureMap&>(*copy) = *this;
        return copy;
    }
    virtual void update() override { invalidate(); }
};

class CursorModelInstance : public srModelInstance {
public:
    CursorModelInstance(srNode* parent) : srModelInstance(parent) {}

    virtual const char* getClassName() const override { return "stModelInstance2D"; }
    virtual unsigned long getClassID() const override { return 0x10005; }
    virtual srRegistry::ClassNode* getClassNode() const override {
        return class_node(0x10005, "stModelInstance2D", 0x1100,
                          "srModelInstance");
    }
};

}

extern "C" {

extern srModeler* g_modeler_65963c;
extern float g_surface_scale_659680;

srScene* g_cursor_scene_659684;
srMeshModel* g_cursor_model_65968c;
srTexture* g_cursor_texture_659690;
srModelInstance* g_cursor_node_659694;
unsigned int g_cursor_move_tick_659698;
int g_cursor_width_654ad0;
int g_cursor_height_654ad4;

}

static srModelInstance* MakePolygonBrush(
    srNode* parent, srColorSurfaceIFace* surface,
    double width, double height,
    float mapping_x, float mapping_y,
    float mapping_width, float mapping_height,
    unsigned char overlay)
{
    CursorMeshModel* model;
    CursorTextureMap* texture;
    srModelInstance* instance;
    srModeler::MappingInfo mapping;
    srVector3T<float> scale;
    srShader shader;

    model = new CursorMeshModel;
    if (!model) {
        return 0;
    }
    model->autoRelease();
    model->setName("Video2DMakePolygonBrush");

    g_modeler_65963c->createGrid(1, 1);
    mapping.unknown_00 = 0;
    mapping.unknown_04 = 1;
    mapping.unknown_08 = float_bits(mapping_width);
    mapping.unknown_0c = float_bits(mapping_height);
    mapping.unknown_10 = float_bits(mapping_x);
    mapping.unknown_14 = float_bits(mapping_y);
    g_modeler_65963c->planarMap(0, 0, mapping);
    scale.x = static_cast<float>(width);
    scale.y = static_cast<float>(height);
    scale.z = 1.0f;
    g_modeler_65963c->scale(scale);
    g_modeler_65963c->convert(*model, 1);
    g_modeler_65963c->discard();

    shader.value = overlay ? g_surface_state_654ad8 : g_surface_state_6595dc;
    if (!surface) {
        shader.value &= 0xffff7fff;
    } else {
        texture = new CursorTextureMap(0);
        texture->autoRelease();
        texture->setName("Video2DMakePolygonBrush");
        texture->setSurfacePtr(surface);
        texture->setCorrection(static_cast<srTextureIFace::e_correction>(0));
        texture->setMagFilter(static_cast<srTextureIFace::e_filter>(3));
        texture->setMinFilter(static_cast<srTextureIFace::e_filter>(3));
        texture->setMipmap(static_cast<srTextureIFace::e_mipmap>(0));
        texture->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
        texture->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
        model->setMaterial(g_blit_material_65967c, 0,
                           static_cast<srMeshModel::e_side>(0));
        model->setTexture(texture, 0, 0);
        texture->enableHint(static_cast<srTextureIFace::e_hint>(overlay ? 2 : 1));
        texture->enableHint(static_cast<srTextureIFace::e_hint>(3));
    }
    model->setShader(shader, 0);

    instance = new CursorModelInstance(parent);
    instance->setName("Video2DMakePolygonBrush");
    instance->assignModel(model);
    instance->configure2D(
        static_cast<short>(width * 640.0),
        static_cast<short>(height * 480.0));
    return instance;
}

// FUNCTION: WIZ8 0x00428140
void PositionMouseCursor(int width, int height, unsigned char reset_tick)
{
    srVector3T<double> location;

    if (g_mouse_surface_659688) {
        g_cursor_width_654ad0 = width < 641 ? width : 640;
        g_cursor_height_654ad4 = height < 481 ? height : 480;
        if (g_cursor_node_659694) {
            location.x = static_cast<double>(g_cursor_width_654ad0) / 640.0
                       + static_cast<double>(g_mouse_surface_659688->width()) / 1280.0;
            location.y = 1.0
                       - static_cast<double>(g_cursor_height_654ad4) / 480.0
                       - static_cast<double>(g_mouse_surface_659688->height()) / 960.0;
            location.z = 0.0;
            g_cursor_node_659694->setLocation(location);
            if (reset_tick) {
                g_cursor_move_tick_659698 = GetTickCount();
            }
        }
    }
}

/* Creates the shipped 128x128 mouse polygon inside its dedicated scene. */
// FUNCTION: WIZ8 0x004285C0
extern "C" unsigned char Function4285C0(void)
{
    g_cursor_scene_659684 = new CursorScene;
    g_cursor_scene_659684->setName("Mouse Cursor Scene");
    if (!g_mouse_surface_659688) {
        return 0;
    }
    g_mouse_surface_659688->fill(0);
    if (g_cursor_texture_659690) {
        g_cursor_texture_659690->release();
    }
    g_cursor_node_659694 = MakePolygonBrush(
        g_cursor_scene_659684, g_mouse_surface_659688,
        0.2, 0.26666666666666666,
        g_surface_scale_659680, g_surface_scale_659680,
        1.0f, 1.0f, 1);
    if (g_cursor_node_659694) {
        g_cursor_node_659694->setName("MouseInit");
        g_cursor_model_65968c = static_cast<srMeshModel*>(g_cursor_node_659694->model());
        g_cursor_model_65968c->enableStartupControls();
        g_cursor_node_659694->setRenderDepth(0xc7c35000);
        g_cursor_texture_659690 = static_cast<srTexture*>(
            g_cursor_model_65968c->getTexture(0, 0));
        g_cursor_texture_659690->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
        g_cursor_texture_659690->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
        g_cursor_texture_659690->addReference();
        PositionMouseCursor(640, 480, 1);
    }
    return 1;
}
