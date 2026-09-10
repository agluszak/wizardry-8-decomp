#include "wiz8/engine_code/game_timer.h"
#include "surrender/srImporter.h"
#include "surrender/srExtension.h"
#include <stdio.h>
#include "wiz8/dirty_tiles.h"
#include "wiz8/engine_code/Video2.h"
#include "surrender/srColorSurface.h"
#include "surrender/srCore.h"
#include "surrender/srGERD.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"
#include "surrender/srVertexProcessor.h"
#include "wiz8/sr_api.h"
#include "wiz8/render_state.h"
#include "wiz8/surface2d.h"
#include "wiz8/utility.h"
#include "wiz8/wiz8_windows.h"
#include "DirectDraw Calls.h"
#include "himage.h"

#include <string.h>

// GLOBAL: WIZ8 0x00603c70
char g_video_config_file[260] = "3DVideo.CFG";

// GLOBAL: WIZ8 0x006598a8
unsigned char g_flag_6598a8;

// FUNCTION: WIZ8 0x004229d0
void PrintScreen(void)
{
    g_flag_659711 = 1;
}

// FUNCTION: WIZ8 0x004277d0
void VideoInspectorEnable(void)
{
    g_flag_65970f = 1;
}

// FUNCTION: WIZ8 0x0042bc00
void NoOct(void)
{
    g_flag_6598a8 = 1;
}

// GLOBAL: WIZ8 0x006548a0
INT32 g_help_box_width;
// GLOBAL: WIZ8 0x00654acc
INT32 g_help_box_height;

// GLOBAL: WIZ8 0x00654aac
int g_screen_transition_object_count_654aac;
// GLOBAL: WIZ8 0x00654ab4
srClass** g_screen_transition_objects_654ab4;

// FUNCTION: WIZ8 0x00429770
void VideoRemoveToolTip(void)
{
    int index;
    srClass* object;

    while (g_screen_transition_object_count_654aac != 0) {
        object = g_screen_transition_objects_654ab4[0];
        if (g_screen_transition_object_count_654aac > 0) {
            for (index = 0; index < g_screen_transition_object_count_654aac - 1; ++index) {
                g_screen_transition_objects_654ab4[index] =
                    g_screen_transition_objects_654ab4[index + 1];
            }
            --g_screen_transition_object_count_654aac;
        }
        object->release();
    }
}

// VTABLE: WIZ8 0x005EBE98
// class srClassSupport<srMeshModel, class srMeshModel, 0, 8208>

// TEMPLATE: WIZ8 0x00429B30
// srClassSupport<srMeshModel,srMeshModel,0,8208>::getClassID

// TEMPLATE: WIZ8 0x00429B40
// srClassSupport<srMeshModel,srMeshModel,0,8208>::getClassName

// TEMPLATE: WIZ8 0x00429B50
// srClassSupport<srMeshModel,srMeshModel,0,8208>::getClassNode

// TEMPLATE: WIZ8 0x00429BC0
// srClassSupport<srMeshModel,srMeshModel,0,8208>::clone

/* CVDUMP includes the class tag on the repeated self-type argument in each
   vftable symbol below. These remain ordinary self-support instantiations. */
// VTABLE: WIZ8 0x005EBEEC
// class srClassSupport<srTextureMap, class srTextureMap, 0, 8465>

// SYNTHETIC: WIZ8 0x00424B70
// srClassSupport<srTextureMap,srTextureMap,0,8465>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00429BE0
// srClassSupport<srTextureMap,srTextureMap,0,8465>::getClassID

// TEMPLATE: WIZ8 0x00429BF0
// srClassSupport<srTextureMap,srTextureMap,0,8465>::getClassName

// TEMPLATE: WIZ8 0x00429C00
// srClassSupport<srTextureMap,srTextureMap,0,8465>::getClassNode

// TEMPLATE: WIZ8 0x00429CA0
// srClassSupport<srTextureMap,srTextureMap,0,8465>::clone

// VTABLE: WIZ8 0x005EBDE0
// class srClassSupport<srMaterial, class srMaterial, 0, 8720>

// TEMPLATE: WIZ8 0x00429CC0
// srClassSupport<srMaterial,srMaterial,0,8720>::getClassID

// TEMPLATE: WIZ8 0x00429CD0
// srClassSupport<srMaterial,srMaterial,0,8720>::getClassName

// TEMPLATE: WIZ8 0x00429CE0
// srClassSupport<srMaterial,srMaterial,0,8720>::getClassNode

// TEMPLATE: WIZ8 0x00429D50
// srClassSupport<srMaterial,srMaterial,0,8720>::clone

// TEMPLATE: WIZ8 0x00429E80
// srClassSupport<srMaterialIFace,srClass,1,8704>::getClassID

/* CVDUMP includes the class tag on the repeated self-type argument in the
   vftable symbol.  It is still the ordinary srCamera self-support template. */
// VTABLE: WIZ8 0x005EBE14
// class srClassSupport<srCamera, class srCamera, 0, 5120>

// TEMPLATE: WIZ8 0x0042A010
// srClassSupport<srCamera,srCamera,0,5120>::getClassID

// TEMPLATE: WIZ8 0x0042A020
// srClassSupport<srCamera,srCamera,0,5120>::getClassName

// TEMPLATE: WIZ8 0x0042A030
// srClassSupport<srCamera,srCamera,0,5120>::getClassNode

// TEMPLATE: WIZ8 0x0042A0A0
// srClassSupport<srCamera,srCamera,0,5120>::clone

/* CVDUMP includes the class tag on the repeated self-type argument in the
   vftable symbol.  It is still the ordinary srScene self-support template. */
// VTABLE: WIZ8 0x005EBE48
// class srClassSupport<srScene, class srScene, 0, 4112>

// TEMPLATE: WIZ8 0x0042A0C0
// srClassSupport<srScene,srScene,0,4112>::getClassID

// TEMPLATE: WIZ8 0x0042A0D0
// srClassSupport<srScene,srScene,0,4112>::getClassName

// TEMPLATE: WIZ8 0x0042A0E0
// srClassSupport<srScene,srScene,0,4112>::getClassNode

// TEMPLATE: WIZ8 0x0042A150
// srClassSupport<srScene,srScene,0,4112>::clone

/* srVertexProcessor::~srVertexProcessor is defined in its own header - see
   the marker there - because 0x0049C430 expands it rather than calling it.
   0x0042A360 is the out-of-line COMDAT the secondary vtables reference. */

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
    srTextureMap* texture =
        SR_NEW(srTextureMap)(
            static_cast<srColorSurfaceIFace*>(0));
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

// VTABLE: WIZ8 0x005EBD10
// class srClassSupport<srColorSurface, class srColorSurface, 0, 12560>

// TEMPLATE: WIZ8 0x00429A40
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassID

// TEMPLATE: WIZ8 0x00429A50
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassName

// TEMPLATE: WIZ8 0x00429A60
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassNode

// TEMPLATE: WIZ8 0x00429AD0
// srClassSupport<srColorSurface,srColorSurface,0,12560>::clone

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

// FUNCTION: WIZ8 0x00425570
void SetPrimarySurfaceTextureHint2Enabled(unsigned char enabled)
{
    if (g_surface_node_659664) {
        g_surface_node_659664->setTextureHint2Enabled(enabled);
    }
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

    g_mouse_surface_659688 =
        SR_NEW(srColorSurface)(
            type, 128UL, 128UL);
    if (!g_mouse_surface_659688) {
        srAssertFail("psrMouseSurface", "C:\\Projects\\Wizardry 8\\Engine Code\\Video2.cpp",
                     0x635, 0);
    }
    g_mouse_surface_659688->setFilter(&srBoxFilter);
    g_mouse_surface_659688->fill(0);
    return 1;
}

// FUNCTION: WIZ8 0x00423500
unsigned char InitializeRendererSceneObjects(void)
{
    DDSURFACEDESC surface_description;
    srCamera::Rect view;
    srMaterial* material;
    srVector4T<float> material_value;
    char renderer_name[128];

    InitializeMouseSurface();
    g_modeler_65963c = new srModeler;
    g_scene_permanent_659648 =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_permanent_659648->setName("2D Permanent Overlay Scene");
    g_scene_permanent_659648->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_permanent_659648->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_user_659640 =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_user_659640->setName("2D User Overlay Scene");
    g_scene_user_659640->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_user_659640->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_fullscreen_659644 =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_fullscreen_659644->setName("Full Screen Overlay Scene");
    g_scene_fullscreen_659644->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_fullscreen_659644->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_overlay0_659654 =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_overlay0_659654->setName("2D Overlay Scene (0)");
    g_scene_overlay0_659654->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_overlay0_659654->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_overlay1_659658 =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_overlay1_659658->setName("2D Overlay Scene (1)");
    g_scene_overlay1_659658->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_overlay1_659658->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_square_65965c =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_square_65965c->setName("2D Square Overlay Scene");
    g_scene_square_65965c->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_square_65965c->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_prerender0_65964c =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_prerender0_65964c->setName("2D Pre-render Overlay Scene (0)");
    g_scene_prerender0_65964c->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_prerender0_65964c->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_prerender1_659650 =
        SR_NEW(srScene)(
            static_cast<srNode*>(0));
    g_scene_prerender1_659650->setName("2D Pre-render Overlay Scene (1)");
    g_scene_prerender1_659650->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_prerender1_659650->setFogColor(0.0f, 0.0f, 0.0f);

    g_overlay_camera_659670 =
        SR_NEW(srCamera)(
            static_cast<srNode*>(0));
    g_overlay_camera_659670->setName("2D Overlay Camera");
    g_overlay_camera_659670->setClipRange(0.01, 2.0);
    g_overlay_camera_659670->setLocation(0.0, 0.0, -1.0);
    g_overlay_camera_659670->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = 1.0;
    g_overlay_camera_659670->setViewPlane(view, 1.0);
    g_overlay_camera_659670->setEnvironmentRange(0.0f, 0.0f);

    g_square_camera_659674 =
        SR_NEW(srCamera)(
            g_scene_square_65965c);
    g_square_camera_659674->setName("2D Square Overlay Camera");
    g_square_camera_659674->setClipRange(0.01, 2.0);
    g_square_camera_659674->setLocation(0.0, 0.0, -1.0);
    g_square_camera_659674->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = 0.75;
    g_square_camera_659674->setViewPlane(view, 1.0);
    g_square_camera_659674->setEnvironmentRange(0.0f, 0.0f);

    material =
        SR_NEW(srMaterial);
    g_blit_material_65967c = material;
    material->setName("Blit Rect Material");
    material_value = 1.0f;
    material->setEmissive(material_value);
    material_value = 0.0f;
    material->setDiffuse(material_value);
    material->setSpecular(material_value);
    material->setOpacity(1.0);

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
    g_primary_color_surface_659660 =
        SR_NEW(srColorSurface)(
            srPixelConvert::SURFACE_ARGB1555, surface_description.lpSurface,
            640UL, 480UL,
            static_cast<unsigned long>(surface_description.lPitch));
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
void PurgeInactiveSceneInstances(srScene* scene)
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

// FUNCTION: WIZ8 0x00425820
void ClearNodeFlag(srNode* node)
{
    if (node) {
        node->setFlag(srNode::FLAG_POSITIONAL_0);
    }
}

// FUNCTION: WIZ8 0x00427810
srModelInstance* GetValue65962C(void)
{
    return g_current_model_instance_65962c;
}

// FUNCTION: WIZ8 0x00427820
void SetValue65962C(srModelInstance* value)
{
    g_current_model_instance_65962c = value;
}

// FUNCTION: WIZ8 0x00428010
unsigned char ClearFlag603C60(void)
{
    g_flag_603c60 = 0;
    return 1;
}

// FUNCTION: WIZ8 0x00428020
unsigned char SetFlag603C60(void)
{
    g_flag_603c60 = 1;
    return 1;
}

/* Release a renderer-owned object, leaving the renderer in 2D mode, or in
   the paired mode when its state byte says otherwise. The handle travels as
   an int, matching the level-block slots that carry it. */
// FUNCTION: WIZ8 0x004257F0
void Function4257F0(int value)
{
    if ((*(unsigned char*)(value + 0x160) & 1) != 0) {
        g_dword_6596ec = 2;
    }
    else {
        g_dword_6596f0 = 2;
        g_dword_6596ec = 2;
    }
    reinterpret_cast<srClass*>(value)->release();
}

// FUNCTION: WIZ8 0x00428A90
void SetRendererMode6596EC(void)
{
    g_dword_6596ec = 2;
}

// FUNCTION: WIZ8 0x00428AA0
void SetRendererModePair(void)
{
    g_dword_6596f0 = 2;
    g_dword_6596ec = 2;
}

// FUNCTION: WIZ8 0x004291C0
unsigned char GetRendererModeByte(void)
{
    return (unsigned char)g_renderer_mode_603d74;
}

// FUNCTION: WIZ8 0x00429200
void SetValue659668(int value)
{
    g_value_659668 = value;
}

// FUNCTION: WIZ8 0x004297D0
bool HasScreenTransitionObjects(void)
{
    return g_screen_transition_object_count_654aac != 0;
}

// FUNCTION: WIZ8 0x004298E0
void SetFlag603C4C(unsigned char value)
{
    g_flag_603c4c = value;
}

// FUNCTION: WIZ8 0x004298F0
unsigned char HasEnoughFreeDiskSpace(void)
{
    FARPROC extended;
    LARGE_INTEGER available;
    LARGE_INTEGER capacity;
    LARGE_INTEGER free_bytes;
    DWORD sectors_per_cluster;
    DWORD bytes_per_sector;
    DWORD free_clusters;
    DWORD total_clusters;
    unsigned int megabytes;
    unsigned char enough;

    extended = GetProcAddress(GetModuleHandleA("kernel32.dll"),
                              "GetDiskFreeSpaceExA");
    if (extended != NULL) {
        GetDiskFreeSpaceExA(NULL, (PULARGE_INTEGER)&available,
                            (PULARGE_INTEGER)&capacity,
                            (PULARGE_INTEGER)&free_bytes);
        megabytes = (unsigned int)(free_bytes.QuadPart / 0x100000);
        enough = megabytes >= 0x100;
        return enough;
    }
    GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector,
                      &free_clusters, &total_clusters);
    megabytes = (unsigned int)((__int64)sectors_per_cluster * bytes_per_sector
                               * free_clusters / 0x400 / 0x400);
    enough = megabytes >= 0x100;
    return enough;
}

// FUNCTION: WIZ8 0x00429AF0
void __fastcall ReleaseOwnedClass(srClass** owner)
{
    if (*owner) {
        (*owner)->release();
    }
}

// FUNCTION: WIZ8 0x00427a30
void VideoSetConfigFile(const char* path)
{
    strcpy(g_video_config_file, path);
}

// FUNCTION: WIZ8 0x00427a60
char* VideoGetConfigFile(void)
{
    return g_video_config_file;
}

// FUNCTION: WIZ8 0x004229b0
unsigned char VideoIsFullScreen(void)
{
    return g_fullscreen_603c39;
}

// FUNCTION: WIZ8 0x00428b80
int VideoDumpMemoryLeaks(void)
{
    return 0;
}

// FUNCTION: WIZ8 0x0042b830
BOOLEAN CheckCdPresent(void)
{
    return TRUE;
}

// FUNCTION: WIZ8 0x00427a70
void VideoGetClientRect(RECT* rect)
{
    GetClientRect(ghWindow, rect);
    ClientToScreen(ghWindow, (POINT*)rect);
    ClientToScreen(ghWindow, (POINT*)&rect->right);
}

// FUNCTION: WIZ8 0x00421f20
IDirectDrawSurface2* GetFrameBufferObject(void)
{
    return g_primary_surface_6596a8;
}

// FUNCTION: WIZ8 0x00421f40
unsigned char GetPrimaryRGBDistributionMasks(
    unsigned int* red, unsigned int* green, unsigned int* blue)
{
    *red = gusRedMask;
    *green = gusGreenMask;
    *blue = gusBlueMask;
    return 1;
}

// FUNCTION: WIZ8 0x00421fd0
void* LockMouseBuffer(unsigned int* pitch)
{
    *pitch = g_mouse_surface_659688->getPitch();
    return g_mouse_surface_659688->getDataPtr();
}

/* The retail linker folds this empty body with other empty C functions. */
void UnlockMouseBuffer(void)
{
}

/* The retail empty video-capture entry folds with the shared 0x4023a0 ret. */
void VideoCaptureToggle(void)
{
}

// FUNCTION: WIZ8 0x00421f30
IDirectDraw2* GetDirectDraw2Object(void)
{
    return g_direct_draw2_6596a0;
}

// GLOBAL: WIZ8 0x006596f4
unsigned char g_flag_6596f4;
// GLOBAL: WIZ8 0x00659724
int g_screenshot_index_659724;
// GLOBAL: WIZ8 0x00659728
int g_screenshot_page_659728;

// FUNCTION: WIZ8 0x004229e0
void Function4229E0(void)
{
    srSurfaceIOManager* surface_io_manager =
        srCore.getSurfaceIOManager();
    srExtension::load("JPEGImporter", 0);

    srColorSurfaceIFace* surface = g_gerd_659634->lockBuffer();
    int screenshot_index = g_screenshot_index_659724;
    if (surface != 0) {
        char filename[32];
        srSurfaceIOManager::ExportInfo options;
        options.unknown_00 = 0;
        options.unknown_04 = 1;
        options.option_string = 0;

        ++g_screenshot_index_659724;
        sprintf(filename, "Wiz8%5.5d.JPG", screenshot_index);
        if (g_flag_6596f4 == 0) {
            surface_io_manager->exportSurface(filename, *surface, options);
        }
        else {
            options.option_string = "QUALITY=0.35";
            Function439BC0();
            surface_io_manager->exportSurface(filename, *surface, options);
            Function439CA0();
        }
        g_gerd_659634->unlockBuffer();
    }
    g_screenshot_page_659728 = (g_screenshot_page_659728 - 1) & 1;
}
