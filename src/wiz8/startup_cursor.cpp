#include "surrender/srColorSurface.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"
#include "surrender/srTexture.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/render_state.h"
#include "himage.h"
#include "video.h"
#include "vobject.h"

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

}

extern "C" {

extern srModeler* g_modeler_65963c;
extern float g_surface_scale_659680;
extern unsigned char g_video_objects_ready_650e20;

srScene* g_cursor_scene_659684;
srMeshModel* g_cursor_model_65968c;
srTexture* g_cursor_texture_659690;
srModelInstance* g_cursor_node_659694;
unsigned int g_cursor_move_tick_659698;
int g_cursor_width_654ad0;
int g_cursor_height_654ad4;
int g_cursor_image_width_6596b4;
int g_cursor_image_height_6596b8;
int g_cursor_hotspot_x_6596bc;
int g_cursor_hotspot_y_6596c0;
unsigned char g_system_cursor_visible_6596c4;
extern unsigned char g_fullscreen_603c39;

}

// SYNTHETIC: WIZ8 0x00424A50
// srClassSupport<srMeshModel,srMeshModel,0,8208>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00424EB0
static srModelInstance* MakePolygonBrush(
    srNode* parent, srColorSurfaceIFace* surface,
    double width, double height,
    float mapping_x, float mapping_y,
    float mapping_width, float mapping_height,
    unsigned char overlay)
{
    srMeshModel* model;
    srTextureMap* texture;
    stModelInstance2D* instance;
    srModeler::MappingInfo mapping;
    srVector3T<float> scale;
    srShader shader;

    model =
        new srClassSupport<srMeshModel, srMeshModel, false, 0x2010>(0L, 0L);
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
        texture =
            new srClassSupport<srTextureMap, srTextureMap, false, 0x2111>(
                static_cast<srColorSurfaceIFace*>(0));
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

    instance = new stModelInstance2D(parent);
    instance->setName("Video2DMakePolygonBrush");
    instance->SetModel0047F3A0(model);
    instance->configure2D(
        static_cast<short>(width * 640.0),
        static_cast<short>(height * 480.0));
    return instance;
}

void PositionMouseCursor(int width, int height, unsigned char reset_tick);

static BOOLEAN BlitVideoObjectToColorSurface(
    UINT32 video_object, UINT16 region, srColorSurface* destination,
    UINT16 x, UINT16 y)
{
    HVOBJECT object;
    ETRLEObject properties;

    if (!GetVideoObject(&object, video_object)) {
        return FALSE;
    }
    if (!GetVideoObjectETRLEProperties(object, &properties, region)) {
        return FALSE;
    }
    return BltVideoObjectToBuffer(
        static_cast<UINT16*>(destination->getDataPtr()),
        destination->getPitch(), object, region, x, y,
        VO_BLT_SRCTRANSPARENCY, 0);
}

static void MoveSystemCursor(int x, int y)
{
    RECT client;
    POINT top_left;
    POINT bottom_right;

    if (!g_fullscreen_603c39) {
        GetClientRect(ghWindow, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(ghWindow, &top_left);
        ClientToScreen(ghWindow, &bottom_right);
        x += top_left.x;
        y += top_left.y;
    }
    SetCursorPos(x, y);
}

static BOOLEAN ResizeMouseCursorSurface(int width, int height)
{
    int extent;
    float mapping_scale;

    if (g_cursor_image_width_6596b4 == width &&
        g_cursor_image_height_6596b8 == height) {
        return TRUE;
    }
    extent = width > height ? width : height;
    if (extent <= 16) extent = 16;
    else if (extent <= 32) extent = 32;
    else if (extent <= 64) extent = 64;
    else if (extent <= 128) extent = 128;
    else if (extent <= 256) extent = 256;
    else extent = -1;
    if (!g_mouse_surface_659688->resize(extent, extent)) {
        return FALSE;
    }
    if (g_cursor_node_659694) {
        g_cursor_node_659694->release();
    }
    if (g_cursor_texture_659690) {
        g_cursor_texture_659690->release();
    }
    mapping_scale = g_surface_scale_659680 / extent;
    g_cursor_node_659694 = MakePolygonBrush(
        g_cursor_scene_659684, g_mouse_surface_659688,
        static_cast<double>(extent) / 640.0,
        static_cast<double>(extent) / 480.0,
        mapping_scale, mapping_scale, 1.0f, 1.0f, 1);
    g_cursor_node_659694->setName("MouseResize");
    PositionMouseCursor(g_cursor_width_654ad0, g_cursor_height_654ad4, 0);
    g_cursor_model_65968c = static_cast<srMeshModel*>(g_cursor_node_659694->model());
    g_cursor_model_65968c->enableStartupControls();
    static_cast<stModelInstance2D*>(g_cursor_node_659694)->setRenderDepth(
        0xc7c35000);
    g_cursor_model_65968c->enableStartupControls();
    g_cursor_model_65968c->setName("Mouse Cursor Mesh");
    g_cursor_texture_659690 = static_cast<srTexture*>(
        g_cursor_model_65968c->getTexture(0, 0));
    g_cursor_texture_659690->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
    g_cursor_texture_659690->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
    g_cursor_texture_659690->addReference();
    return TRUE;
}

// FUNCTION: WIZ8 0x00427ab0
extern "C" BOOLEAN SetMouseCursorFromVideoObject(
    UINT32 video_object, UINT16 region, INT16 offset_x, INT16 offset_y)
{
    ETRLEObject properties;
    int x;
    int y;

    if (!g_video_objects_ready_650e20) {
        return FALSE;
    }
    if (!GetVideoObjectETRLEPropertiesFromIndex(
            video_object, &properties, region)) {
        return FALSE;
    }
    if (!ResizeMouseCursorSurface(properties.usWidth + 1,
                                  properties.usHeight + 1)) {
        return FALSE;
    }
    if (g_cursor_hotspot_x_6596bc != offset_x ||
        g_cursor_hotspot_y_6596c0 != offset_y) {
        x = g_cursor_width_654ad0 - offset_x + g_cursor_hotspot_x_6596bc;
        y = g_cursor_height_654ad4 - offset_y + g_cursor_hotspot_y_6596c0;
        MoveSystemCursor(x, y);
        PositionMouseCursor(x, y, 1);
        g_cursor_hotspot_x_6596bc = offset_x;
        g_cursor_hotspot_y_6596c0 = offset_y;
    }
    g_cursor_image_width_6596b4 = properties.usWidth;
    g_cursor_image_height_6596b8 = properties.usHeight;
    g_mouse_surface_659688->fill(0);
    return BlitVideoObjectToColorSurface(
        video_object, region, g_mouse_surface_659688, 0, 0);
}

// FUNCTION: WIZ8 0x00427fc0
extern "C" void BlitToMouseCursor(
    UINT32 video_object, UINT16 region, UINT16 x, UINT16 y)
{
    BlitVideoObjectToColorSurface(
        video_object, region, g_mouse_surface_659688, x, y);
}

// FUNCTION: WIZ8 0x00427ff0
extern "C" void RefreshMouseCursorTexture(void)
{
    g_mouse_surface_659688->touch();
    g_cursor_texture_659690->invalidate();
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
                       + static_cast<double>(g_mouse_surface_659688->getWidth()) / 1280.0;
            location.y = 1.0
                       - static_cast<double>(g_cursor_height_654ad4) / 480.0
                       - static_cast<double>(g_mouse_surface_659688->getHeight()) / 960.0;
            location.z = 0.0;
            g_cursor_node_659694->setLocation(location);
            if (reset_tick) {
                g_cursor_move_tick_659698 = GetTickCount();
            }
        }
    }
}

/* Keep the rendered cursor synchronized with the OS cursor. In windowed mode
   the OS cursor is visible outside the client area and hidden while the game
   owns it; fullscreen coordinates are clamped to the 640x480 game surface. */
// FUNCTION: WIZ8 0x00428340
extern "C" void Function00428340(void)
{
    POINT cursor;
    RECT client;
    POINT top_left;
    POINT bottom_right;

    GetCursorPos(&cursor);
    if (!g_fullscreen_603c39) {
        GetClientRect(ghWindow, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(ghWindow, &top_left);
        ClientToScreen(ghWindow, &bottom_right);
        if (cursor.x < top_left.x || cursor.x >= bottom_right.x ||
            cursor.y < top_left.y || cursor.y >= bottom_right.y) {
            if (g_system_cursor_visible_6596c4 != 1) {
                g_system_cursor_visible_6596c4 = 1;
                ShowCursor(TRUE);
            }
            return;
        }
        cursor.x -= top_left.x;
        cursor.y -= top_left.y;
        if (cursor.x != g_cursor_width_654ad0 ||
            cursor.y != g_cursor_height_654ad4) {
            PositionMouseCursor(cursor.x, cursor.y, 1);
        }
        if (g_system_cursor_visible_6596c4 != 0) {
            g_system_cursor_visible_6596c4 = 0;
            ShowCursor(FALSE);
        }
        return;
    }

    if (cursor.x < 1) cursor.x = 0;
    else if (cursor.x >= 640) cursor.x = 640;
    if (cursor.y < 1) cursor.y = 0;
    else if (cursor.y >= 480) cursor.y = 480;
    if (cursor.x != g_cursor_width_654ad0 ||
        cursor.y != g_cursor_height_654ad4) {
        PositionMouseCursor(cursor.x, cursor.y, 1);
        if (!g_fullscreen_603c39) {
            GetClientRect(ghWindow, &client);
            top_left.x = client.left;
            top_left.y = client.top;
            bottom_right.x = client.right;
            bottom_right.y = client.bottom;
            ClientToScreen(ghWindow, &top_left);
            ClientToScreen(ghWindow, &bottom_right);
            SetCursorPos(top_left.x + cursor.x, top_left.y + cursor.y);
        } else {
            SetCursorPos(cursor.x, cursor.y);
        }
    }
}

/* Creates the shipped 128x128 mouse polygon inside its dedicated scene. */
// FUNCTION: WIZ8 0x004285c0
extern "C" unsigned char Function4285C0(void)
{
    srScene* cursor_scene =
        new srClassSupport<srScene, srScene, false, 0x1010>(
            static_cast<srNode*>(0));
    cursor_scene->setAmbientLight(0.0f, 0.0f, 0.0f);
    cursor_scene->setFogColor(0.0f, 0.0f, 0.0f);
    g_cursor_scene_659684 = cursor_scene;
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
        static_cast<stModelInstance2D*>(g_cursor_node_659694)->setRenderDepth(
            0xc7c35000);
        g_cursor_texture_659690 = static_cast<srTexture*>(
            g_cursor_model_65968c->getTexture(0, 0));
        g_cursor_texture_659690->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
        g_cursor_texture_659690->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
        g_cursor_texture_659690->addReference();
        PositionMouseCursor(640, 480, 1);
    }
    return 1;
}
