#include "wiz8/engine_code/game_timer.h"
#include "wiz8/float_constants.h"
#include "wiz8/cursor.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/TextBuffer.h"
#include "Font.h"
#include "surrender/srFilter.h"
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
#include "vobject_blitters.h"

#include <math.h>
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

    FlushDirtyTiles00425B40();
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
        SR_NEW(W8ColorSurface)(
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
        SR_NEW(W8ColorSurface)(
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
void __fastcall ReleaseOwnedClass00429AF0(srClass** owner)
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
            PauseSharedGameTimers00439BC0();
            surface_io_manager->exportSurface(filename, *surface, options);
            ResumeSharedGameTimers00439CA0();
        }
        g_gerd_659634->unlockBuffer();
    }
    g_screenshot_page_659728 = (g_screenshot_page_659728 - 1) & 1;
}

/* Tooltip placement state. The left/top pair records the last position the
   tooltip builder used; the scale participates in the texture mapping. */
// GLOBAL: WIZ8 0x00654ab8
int g_help_box_x_654ab8;
// GLOBAL: WIZ8 0x00654abc
int g_help_box_y_654abc;
// GLOBAL: WIZ8 0x006596e4
unsigned char g_flag_6596e4;
// GLOBAL: WIZ8 0x00654ab0
int g_screen_transition_object_capacity_654ab0;
// GLOBAL: WIZ8 0x006596ec
int g_value_6596ec;
// GLOBAL: WIZ8 0x006596f0
int g_value_6596f0;

extern "C" {
// GLOBAL: WIZ8 0x005ebe88
double g_double_005ebe88 = 0.0020833333333333333;
// GLOBAL: WIZ8 0x005ebe90
double g_double_005ebe90 = 0.0015625;
// GLOBAL: WIZ8 0x005ebf40
double g_double_005ebf40 = 0.75;
}

/* Packs four normalized colour components into the surface byte order:
   red, green, blue, alpha from the high byte down. */
// FUNCTION: WIZ8 0x00429700
void __fastcall PackColour00429700(
    unsigned char* colour, double red, double green, double blue, double alpha)
{
    colour[3] = (int)(red * 255.0);
    colour[2] = (int)(green * 255.0);
    colour[1] = (int)(blue * 255.0);
    colour[0] = (int)(alpha * 255.0);
}

/* Places one tooltip node at a screen position in normalized coordinates.
   With positional set, the position is snapped to the renderer's pixel grid;
   the node keeps the screen x/y in its right/bottom extent fields. */
// FUNCTION: WIZ8 0x004255F0
void PositionToolTipNode(srNode* node, int x, int y, char positional)
{
    stModelInstance2D* instance = static_cast<stModelInstance2D*>(node);
    double position_x = (double)x * g_double_005ebe90;
    double position_y = (double)y * g_double_005ebe88;

    if (positional != 0 && g_gerd_659634 != 0) {
        double whole;
        long width = g_gerd_659634->getWidth();
        double fraction = modf((double)width * position_x, &whole);
        position_x -= fraction / (double)width;
        long height = g_gerd_659634->getHeight();
        fraction = modf((double)height * position_y, &whole);
        position_y -= fraction / (double)height;
    }

    int width = instance->GetWidth00480EF0() & 0xffff;
    double half_width = (double)width * g_double_005ebe90 * g_double_005ebe80;
    int height = instance->GetHeight00480F70() & 0xffff;
    double half_height = (double)height * g_double_005ebe88 * g_double_005ebe80;

    srVector3T<double> location;
    location.x = half_width + position_x;
    location.z = -0.0001;
    if ((instance->state_160 & 1U) == 0) {
        location.y = g_double_005ebc30 - (half_height + position_y);
        g_value_6596f0 = 2;
    }
    else {
        location.y = g_double_005ebf40 - (half_height + position_y) * g_double_005ebf40;
    }
    node->setLocation(location);
    g_value_6596ec = 2;
    instance->right_16c = (short)x;
    instance->bottom_16e = (short)y;
}

/* Positions every live tooltip object left to right starting at x, advancing
   the cursor by each node's scaled width. With no live objects, just record
   the requested position. */
// FUNCTION: WIZ8 0x00429210
void VideoPositionToolTip(INT32 x, INT32 y)
{
    if (g_screen_transition_object_count_654aac > 0) {
        INT32 offset = x;
        for (int index = 0; index < g_screen_transition_object_count_654aac; ++index) {
            srNode* node =
                static_cast<srNode*>(g_screen_transition_objects_654ab4[index]);
            PositionToolTipNode(node, offset, y, 1);
            offset += static_cast<stModelInstance2D*>(node)->GetWidth00480EF0() & 0xffff;
        }
        g_help_box_y_654abc = y;
        g_help_box_x_654ab8 = x;
        return;
    }
    g_help_box_x_654ab8 = x;
    g_help_box_y_654abc = y;
}

/* Copies the tooltip source rectangle into a size-rounded 16-bit surface,
   repeating its border one pixel outward, and reports the texture mapping
   scales for the resulting polygon brush. */
// FUNCTION: WIZ8 0x00428B90
unsigned char CopySurfaceWithBorder(
    srColorSurface* surface, int* rect, void* source, int source_pitch,
    float* scale_x, float* scale_y, float* mapping_x, float* mapping_y)
{
    if (surface == 0 || rect == 0 || source == 0 || source_pitch == 0 ||
        scale_x == 0 || scale_y == 0 || mapping_x == 0 || mapping_y == 0) {
        return 0;
    }
    int width = (rect[2] > 0x27f ? 0x280 : rect[2]) - rect[0];
    int height = (rect[3] > 0x1df ? 0x1e0 : rect[3]) - rect[1];
    UINT16* dest = (UINT16*)surface->getDataPtr();
    UINT32 dest_pitch = (UINT32)surface->getPitch();
    UINT16* src = (UINT16*)source;
    UINT32 src_pitch = (UINT32)source_pitch;
    int right = width + 1;
    int bottom = height + 1;

    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 1, 1,
                    rect[0], rect[1], width, height);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 1, 0,
                    rect[0], rect[1], width, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 1, bottom,
                    rect[0], rect[1] - 1 + height, width, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 0, 1,
                    rect[0], rect[1], 1, height);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, right, 1,
                    rect[0] - 1 + width, rect[1], 1, height);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 0, 0,
                    rect[0], rect[1], 1, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, right, 0,
                    rect[0] - 1 + width, rect[1], 1, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 0, bottom,
                    rect[0], rect[1] - 1 + height, 1, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, right, bottom,
                    rect[0] - 1 + width, rect[1] - 1 + height, 1, 1);

    float scale = 1.0f / (float)surface->getWidth();
    *scale_x = scale;
    *scale_x = scale * g_surface_scale_659680 + scale;
    scale = 1.0f / (float)surface->getHeight();
    *scale_y = scale;
    *scale_y = scale * g_surface_scale_659680 + scale;
    *mapping_x = (float)(rect[2] - rect[0]) / (float)surface->getWidth();
    *mapping_y = (float)(rect[3] - rect[1]) / (float)surface->getHeight();
    return 1;
}

/* Builds a polygon brush from a tooltip surface rectangle. The larger source
   extent is rounded up to the next power of two between 16 and 256, the copy
   repeats its border, and the node records the rectangle extents. */
// FUNCTION: WIZ8 0x00424280
srModelInstance* Video2DRectToPolygon(
    int* rect, void* source, int source_pitch, srNode* parent, unsigned char overlay)
{
    double left = (double)rect[0] * g_double_005ebe90;
    int extent = rect[2] - rect[0];
    double top = (double)rect[1] * g_double_005ebe88;
    int rect_height = rect[3] - rect[1];
    double width = (double)rect[2] * g_double_005ebe90 - left;
    double height = (double)rect[3] * g_double_005ebe88 - top;

    if (extent <= rect_height) {
        extent = rect_height;
    }
    if (extent < 0x10) {
        extent = 0x10;
    }
    else if (extent < 0x20) {
        if (extent != 0x10) {
            extent = 0x20;
        }
    }
    else if (extent < 0x40) {
        if (extent != 0x20) {
            extent = 0x40;
        }
    }
    else if (extent < 0x80) {
        if (extent != 0x40) {
            extent = 0x80;
        }
    }
    else {
        if (0x100 < extent) {
            return 0;
        }
        if (extent != 0x80) {
            extent = 0x100;
        }
    }

    srColorSurface* surface = SR_NEW(W8ColorSurface)(
        srPixelConvert::SURFACE_ARGB1555, (unsigned long)extent, (unsigned long)extent);
    if (surface == 0) {
        return 0;
    }
    surface->setFilter(&srBoxFilter);
    float scale_x;
    float scale_y;
    float mapping_x;
    float mapping_y;
    if (!CopySurfaceWithBorder(surface, rect, source, source_pitch,
                        &scale_x, &scale_y, &mapping_x, &mapping_y)) {
        surface->release();
        return 0;
    }
    surface->getDataPtr();
    srModelInstance* node = MakePolygonBrush(
        parent, surface, width, height, scale_x, scale_y, mapping_x, mapping_y, overlay);
    if (node != 0) {
        stModelInstance2D* instance = static_cast<stModelInstance2D*>(node);
        instance->state_160 = g_flag_6596e4;
        instance->left_168 = (short)(rect[2] - rect[0]);
        instance->top_16a = (short)(rect[3] - rect[1]);
        instance->right_16c = (short)rect[0];
        instance->bottom_16e = (short)rect[1];
        srVector3T<double> location;
        location.x = width * g_double_005ebe80 + left;
        location.y = g_double_005ebc30 - (height * g_double_005ebe80 + top);
        location.z = -0.0001;
        node->setLocation(location);
        instance->setName("Video2DRectToPolygon");
    }
    return node;
}

/* Builds the help box: renders the text into an ARGB1555 surface, draws the
   border, converts the surface to a polygon brush, appends it to the live
   tooltip objects and positions them above the cursor. Only one tooltip is
   alive at a time. */
// FUNCTION: WIZ8 0x00429290
void VideoToolTip(UINT16* text)
{
    if (g_screen_transition_object_count_654aac != 0) {
        return;
    }
    srColorSurface* surface = SR_NEW(W8ColorSurface)(
        srPixelConvert::SURFACE_ARGB1555, 0xfeUL, 0xfeUL);
    if (surface == 0) {
        return;
    }
    W8ControlsRect bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = 0xfa;
    bounds.bottom = 0xfa;
    W8TextBuffer* buffer = new W8TextBuffer(
        &bounds, (const wchar_t*)text, g_font10arial_683668,
        g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548, 4);
    if (buffer == 0) {
        return;
    }
    g_help_box_width = (int)buffer->m_maxLineWidth + 4;
    g_help_box_height =
        GetFontHeight(g_font10arial_683668) * buffer->m_lineCount + 2;
    surface->fill(0);
    void* data = surface->getDataPtr();
    unsigned char colour[4];
    for (int y = 0; y < g_help_box_height; ++y) {
        PackColour00429700(colour, 1.0, 0.0, 0.0, 0.0);
        surface->setHLine(0, y, g_help_box_width, *(unsigned long*)colour);
    }
    buffer->RenderText((int)data, (int)surface->getPitch(), 2, 1, 1);
    surface->setHLine(0, 0, g_help_box_width, 0xffed9954);
    surface->setHLine(0, g_help_box_height - 1, g_help_box_width, 0xffed9954);
    surface->setVLine(0, 0, g_help_box_height, 0xffed9954);
    surface->setVLine(g_help_box_width - 1, 0, g_help_box_height, 0xffed9954);

    int rect[4];
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = 0xfe;
    rect[3] = 0xfe;
    srModelInstance* node = Video2DRectToPolygon(
        rect, data, (int)surface->getPitch(), g_cursor_scene_659684, 1);
    if (node != 0) {
        int count = g_screen_transition_object_count_654aac;
        bool append = true;
        if (g_screen_transition_object_capacity_654ab0 < count + 1) {
            srClass** objects = new srClass*[count + 1];
            if (objects == 0) {
                append = false;
            }
            else {
                for (int index = 0; index < count; ++index) {
                    objects[index] = g_screen_transition_objects_654ab4[index];
                }
                delete[] g_screen_transition_objects_654ab4;
                g_screen_transition_objects_654ab4 = objects;
                g_screen_transition_object_capacity_654ab0 = count + 1;
            }
        }
        if (append) {
            g_screen_transition_objects_654ab4[count] = static_cast<srClass*>(node);
            g_screen_transition_object_count_654aac = count + 1;
        }
    }

    int position_y = g_cursor_height_654ad4 - g_help_box_height;
    int position_x = g_cursor_width_654ad0;
    int offset = position_x;
    for (int index = 0; index < g_screen_transition_object_count_654aac; ++index) {
        srNode* object =
            static_cast<srNode*>(g_screen_transition_objects_654ab4[index]);
        PositionToolTipNode(object, offset, position_y, 1);
        offset += static_cast<stModelInstance2D*>(object)->GetWidth00480EF0() & 0xffff;
    }
    g_help_box_x_654ab8 = position_x;
    g_help_box_y_654abc = position_y;
    surface->release();
    delete buffer;
}
