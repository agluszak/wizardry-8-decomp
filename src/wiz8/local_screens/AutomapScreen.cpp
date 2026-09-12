#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/screen_state.h"
#include "wiz8/vector.h"
#include "surrender/srTypeRegistry.h"
#include "surrender/srClipPlane.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stLight.h"
#include "wiz8/engine_code/stScript.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/game_status.h"
#include "wiz8/float_constants.h"
#include "wiz8/location_variables.h"
#include "wiz8/sr_api.h"
#include "wiz8/item_spawning.h"
#include "wiz8/dialog_code/DialogButton.h"
#include "surrender/srColorSurface.h"
#include "wiz8/cursor.h"
#include "wiz8/render_state.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/xstatus.h"
#include "wiz8/wiz8_windows.h"
#include "input.h"
#include "Types.h"
#include "mousesystem.h"
#include "wiz8/local_screens/AutomapScreen.h"

#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

/* Lifecycle record 8; the automap screen. Its original screen-class name is
   unknown, so the existing compilation boundary is retained. */

// GLOBAL: WIZ8 0x0068f104
unsigned char g_flag_68f104;
// GLOBAL: WIZ8 0x0068f105
unsigned char g_flag_68f105;

/* Lifecycle record 8's own state, all of it released by the finalizer below and
   nothing here naming what any of it holds. The list is vector.cpp's, created by
   this record's initializer at 0x0057E5D0. */
/* vector.cpp defines this with C++ linkage; the spelling has to agree or the
   reference resolves to the image base under /FORCE. */

/* Two owned index arrays, released through BitArray's destructor. */
// GLOBAL: WIZ8 0x0068F288
BitArray* g_bits_68f288;
// GLOBAL: WIZ8 0x0068F28C
BitArray* g_bits_68f28c;
// GLOBAL: WIZ8 0x0068F280
void* g_block_68f280;

// GLOBAL: WIZ8 0x0068F284
W8HashTable<unsigned int, int>* g_record_68f284;
// GLOBAL: WIZ8 0x0068F29C
srNode* g_class_68f29c;
// GLOBAL: WIZ8 0x0068F2A0
srClass* g_class_68f2a0;
// GLOBAL: WIZ8 0x0068F2A4
srClass* g_class_68f2a4;
// GLOBAL: WIZ8 0x0068F2A8
srClass* g_class_68f2a8;

/* Leave releases the pointed-to objects and erases their vector entries. */
// GLOBAL: WIZ8 0x0068F1F4
W8GrowableVector<srClass*>* g_releasable_68f1f4;

extern float g_float_64b914;

// GLOBAL: WIZ8 0x0068f220
W8GrowableVector<srClipPlane::ClientType*> g_automap_created_layers;
// GLOBAL: WIZ8 0x0068f24c
W8DialogButton** g_automap_buttons;
struct W8AutomapState {
    unsigned char unknown_000[0xf4];
    unsigned int blink_time;
    unsigned char blink_enabled;
    unsigned char unknown_0f9[3];
};
static_assert(sizeof(W8AutomapState) == 0xfc, "W8AutomapState_size");
// GLOBAL: WIZ8 0x0068f268
W8AutomapState* g_automap_state;
// GLOBAL: WIZ8 0x0068f274
srColorSurface* g_automap_surface;

// GLOBAL: WIZ8 0x0064b8e4
int g_automap_cursor_offsets[5][2] = {{0, 0}, {8, 7}, {1, 24}, {1, 24}, {8, 7}};
// GLOBAL: WIZ8 0x0064b910
float g_automap_range_0064b910;
// GLOBAL: WIZ8 0x0064b918
int g_automap_layer = -1;
// GLOBAL: WIZ8 0x0064b91c
unsigned char g_automap_bounds_dirty_0064b91c;
// GLOBAL: WIZ8 0x0068f138
EnvironmentColour g_automap_saved_light_direction;
// GLOBAL: WIZ8 0x0068f144
EnvironmentColour g_automap_saved_ambient_light;
// GLOBAL: WIZ8 0x0068f150
unsigned char g_automap_saved_sky;
// GLOBAL: WIZ8 0x0068f154
W8WorldCameraState g_automap_saved_camera;
// GLOBAL: WIZ8 0x0068f190
float g_automap_saved_far_clip;
// GLOBAL: WIZ8 0x0068f194
float g_automap_saved_world_value;
// GLOBAL: WIZ8 0x0068f198
unsigned char g_automap_saved_render_flags[4];
// GLOBAL: WIZ8 0x0068f19c
int g_automap_saved_texture_policy;
// GLOBAL: WIZ8 0x0068f1a8
W8GrowableVector<srClipPlane::ClientType*> g_automap_layers;
// GLOBAL: WIZ8 0x0068f1b8
srVector3T<float> g_automap_bounds_max;
// GLOBAL: WIZ8 0x0068f1c8
srVector3T<float> g_automap_grid_max_0068f1c8;
// GLOBAL: WIZ8 0x0068f1d8
srVector3T<float> g_automap_grid_min_0068f1d8;
// GLOBAL: WIZ8 0x0068f1e8
srVector3T<float> g_automap_position;
// GLOBAL: WIZ8 0x0068f1f8
srVector3T<float> g_automap_grid_center_0068f1f8;
// GLOBAL: WIZ8 0x0068f204
float g_automap_top_y;
// GLOBAL: WIZ8 0x0068f210
srVector3T<float> g_automap_bounds_min;
// GLOBAL: WIZ8 0x0068f230
W8ScreenRect g_automap_viewport;
// GLOBAL: WIZ8 0x0068f240
srVector3T<float> g_automap_grid_origin_0068f240;
// GLOBAL: WIZ8 0x0068f250
int g_automap_tool;
// GLOBAL: WIZ8 0x0068f254
unsigned char g_automap_cursor_inside;
// GLOBAL: WIZ8 0x0068f25c
unsigned char g_automap_redraw;
// GLOBAL: WIZ8 0x0068f25d
unsigned char g_automap_overlay_redraw;
// GLOBAL: WIZ8 0x0068f26c
float g_automap_zoom;
// GLOBAL: WIZ8 0x0068f270
unsigned char g_automap_surface_mode;
// GLOBAL: WIZ8 0x0068f278
int g_automap_zoom_mode;
// GLOBAL: WIZ8 0x0068f290
unsigned char g_automap_position_initialized;
// GLOBAL: WIZ8 0x0068f294
W8AutomapNote* g_automap_editing_note;
// GLOBAL: WIZ8 0x0068f298
W8AutomapNote* g_automap_hovered_note;

unsigned char Function584690(const InputAtom* input);
void Function584250(const InputAtom* input);
void Function581460(W8AutomapNote* note);
void Function57FFC0(const srVector3T<float>* position);
void Function57FC70(const srVector3T<float>* position);
void SetAutomapButtonMode(int update);
void Function57FE40(void);
void Function427460(int x, int y);
void Function581030(void);
void Function46F760(W8World* world, int value);
void Function581200(void);
void Function425C90(int left, int top, int right, int bottom);
void Function580380(void);
void Function474FB0(int value);

// FUNCTION: WIZ8 0x00581000
unsigned char HasAutomapLayer(int layer)
{
    return layer >= 0 && layer < g_automap_layers.GetCount() && *g_automap_layers.GetAt(layer) != 0;
}

namespace {

/* Pack a grid position into the record table's cell key: eleven bits of z,
   then eleven of x, then ten of y, each scaled to grid cells. */
inline unsigned int PackAutomapCell(const srVector3T<float>& position)
{
    return ((static_cast<unsigned int>(position.z / g_float_64b914) & 0x7ff) |
            static_cast<unsigned int>(position.x / g_float_64b914) << 11)
               << 10 |
           (static_cast<unsigned int>(position.y / g_float_64b914) & 0x3ff);
}

} // namespace

/* Rebuild the automap view for the level that just loaded: release every
   note, size the query range from the level, seed the cell grid from the
   octree bounds (or a fixed cube when there is no octree), and mark the
   camera's own cell in the visited bitmap, retrying one cell higher when the
   packed cell misses the record table. */
// FUNCTION: WIZ8 0x005817d0
void ResetAutomapView005817D0(void)
{
    if (g_automap_state == 0) {
        g_automap_state = (W8AutomapState*)malloc(sizeof(W8AutomapState));
        if (g_automap_state == 0) {
            srAssertFail("gpAMSV", "C:\\Projects\\Wizardry 8\\Local Screens\\AutomapScreen.cpp",
                         0x8b5, 0);
        }
        memset(g_automap_state, 0, sizeof(W8AutomapState));
    }
    while (g_automap_notes->count != 0) {
        W8AutomapNote* note = *g_automap_notes->GetAt(0);
        free(note->text);
        delete note;
        g_automap_notes->RemoveAt(0);
    }
    g_automap_redraw = 1;
    if (g_loaded_level_id == 0x18 || (g_loaded_level_id > 0x1a && g_loaded_level_id <= 0x22)) {
        g_automap_range_0064b910 = 30000.0f;
    } else {
        g_automap_range_0064b910 = 10000.0f;
    }
    if (g_octree_6598a4 == 0) {
        g_automap_grid_min_0068f1d8.x = -250000.0f;
        g_automap_grid_min_0068f1d8.y = -250000.0f;
        g_automap_grid_min_0068f1d8.z = -250000.0f;
        g_automap_grid_max_0068f1c8.x = 250000.0f;
        g_automap_grid_max_0068f1c8.y = 250000.0f;
        g_automap_grid_max_0068f1c8.z = 250000.0f;
    } else {
        g_octree_6598a4->spatial_000.GetClippedBounds0046CE30(&g_automap_grid_min_0068f1d8,
                                                              &g_automap_grid_max_0068f1c8);
    }
    g_automap_grid_origin_0068f240 = g_automap_grid_min_0068f1d8;
    g_automap_grid_center_0068f1f8.x =
        (g_automap_grid_min_0068f1d8.x + g_automap_grid_max_0068f1c8.x) * g_W8RangeHalfStep005EBC7C;
    g_automap_grid_center_0068f1f8.y = 0.0f;
    g_automap_grid_center_0068f1f8.z =
        (g_automap_grid_min_0068f1d8.z + g_automap_grid_max_0068f1c8.z) * g_W8RangeHalfStep005EBC7C;
    g_automap_bounds_dirty_0064b91c = 1;
    float span_x = g_automap_grid_max_0068f1c8.x - g_automap_grid_min_0068f1d8.x;
    float span_z = g_automap_grid_max_0068f1c8.z - g_automap_grid_min_0068f1d8.z;
    float largest = span_z < span_x ? span_x : span_z;
    if (largest <= g_float_005ec360) {
        g_automap_top_y = 25000.0f;
    } else {
        g_automap_top_y = largest;
    }
    g_automap_position_initialized = 0;
    g_automap_layer = 0;

    srVector3T<float> camera;
    GetCameraPosition(&camera);
    srVector3T<float> relative(camera.x - g_automap_grid_origin_0068f240.x,
                               camera.y - g_automap_grid_origin_0068f240.y,
                               camera.z - g_automap_grid_origin_0068f240.z);
    unsigned int key = PackAutomapCell(relative);
    int cell = g_record_68f284->Lookup(&key);
    if (cell > 1) {
        g_bits_68f288->Set(cell - 1);
        return;
    }
    relative.y = camera.y + g_float_64b914 - g_automap_grid_origin_0068f240.y;
    key = PackAutomapCell(relative);
    cell = g_record_68f284->Lookup(&key);
    if (cell > 1) {
        g_bits_68f288->Set(cell - 1);
    }
}

// FUNCTION: WIZ8 0x0057E490
unsigned char Function57E490(void)
{
    if (g_flag_68f105 != 0) {
        switch (g_automap_tool) {
        case 6:
        case 7:
        case 15:
        case 18:
        case 23:
        case 29:
            return 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x00584210
void RestoreAutomapCameraPosition(void)
{
    g_automap_position.x = g_automap_saved_camera.position.x;
    g_automap_position.z = g_automap_saved_camera.position.z;
    Function57FC70(&g_automap_position);
}

// FUNCTION: WIZ8 0x0057e660
unsigned char AutomapScreenEnter(void)
{
    stScript script;
    MSYS_Init();
    GetLightDirection(&g_automap_saved_light_direction);
    GetWorldLightValue(g_world, &g_automap_saved_ambient_light);
    GetWorldCameraState(GetWorld(), &g_automap_saved_camera);
    g_automap_saved_far_clip = static_cast<float>(WorldGetFarClip(g_world));
    g_automap_saved_world_value = WorldGetValue78(g_world);
    g_automap_saved_sky = g_sky_enabled_0065b9ae;
    g_automap_saved_render_flags[0] = GetRenderOptionState(11);
    g_automap_saved_render_flags[1] = GetRenderOptionState(10);
    g_automap_saved_render_flags[2] = g_monster_shadow_updates_enabled_0065970c;
    g_automap_saved_render_flags[3] = g_flag_65970d;
    g_automap_saved_texture_policy = g_resident_texture_policy_659714;
    EnvironmentColour direction;
    direction = 0.0;
    SetLightDirection(&direction);
    SetWorldEnvironmentColour00483A60(g_world, EnvironmentColour(0.0, 0.0, 0.0));
    DisableSky();
    DisableRenderOption(10);
    g_monster_shadow_updates_enabled_0065970c = 1;
    g_flag_65970d = 1;
    DisableSky();
    g_world->camera->setClipRange(1.0, 1500000.0);
    WorldSetValue74(g_world, 1500000.0f);
    g_world->camera->setRotation(3.141592653589793 * (1.0f / 180.0f) * 90.0f, 0.0, 0.0);
    int layer_number = 1;
    g_world->camera->setProjectionType(static_cast<srCamera::e_project>(1));
    RestoreAutomapCameraPosition();
    UpdateWorldMesh004BAF60(g_world);
    Function46F760(g_world, 1);
    g_light_update_flags_0060bfdc &= ~1u;
    Function427830(0);
    if (!g_automap_state) {
        g_automap_state = static_cast<W8AutomapState*>(malloc(sizeof(W8AutomapState)));
        if (!g_automap_state)
            return 0;
        memset(g_automap_state, 0, sizeof(W8AutomapState));
    }
    g_automap_state->blink_time = GetTickCount();
    g_automap_viewport.left = 12;
    g_automap_viewport.top = 32;
    g_automap_viewport.right = 467;
    g_automap_viewport.bottom = 467;
    Function56AA30();
    g_automap_layers.Clear();
    g_automap_layers.Add(0);
    char layer_name[16];
    sprintf(layer_name, "LAYER_%d", layer_number);
    srClipPlane::ClientType* layer = static_cast<srClipPlane::ClientType*>(
        srCore.getRegistry()->find(srClipPlane::ClientType::sGetClassNode(), layer_name, 0));
    while (layer) {
        g_automap_layers.Add(layer);
        ++layer_number;
        sprintf(layer_name, "LAYER_%d", layer_number);
        layer = static_cast<srClipPlane::ClientType*>(
            srCore.getRegistry()->find(srClipPlane::ClientType::sGetClassNode(), layer_name, 0));
    }
    unsigned int monster_count = PLLength(gXStatus.plsMonsterList);
    for (unsigned int index = 0; index < monster_count; ++index) {
        W8MonsterInfo* monster = static_cast<W8MonsterInfo*>(PLGet(gXStatus.plsMonsterList, index));
        if (monster->monster)
            monster->monster->DetachRepresentation004A7A70(g_world);
    }
    for (W8WorldItem* item = GetNextWorldItem(1); item; item = GetNextWorldItem(0)) {
        if (item->owner)
            item->owner->DetachMesh0049FA30(g_world);
    }
    gfTrackMousePos = 1;
    g_automap_cursor_inside = IsCursorInsideViewport();
    g_automap_tool = 0;
    Function5820F0(0);
    Function5822C0();
    Function583BC0();
    g_class_68f29c->setParent(0, 1);
    g_automap_surface_mode = Function427260();
    if (g_automap_surface_mode) {
        g_flag_65970d = 0;
        g_monster_shadow_updates_enabled_0065970c = 0;
        if (!g_automap_surface) {
            srColorSurface* surface =
                SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, 640, 480);
            g_automap_surface = surface;
            if (surface)
                surface->setFilter(&srBoxFilter);
        }
    }
    g_automap_redraw = 1;
    g_automap_overlay_redraw = 1;
    SetPrimarySurfaceTextureHint2Enabled(0);
    ClearSurfaceRect(0, 0, 640, 480);
    ClearFlag603C60();
    DrawCatalogImageAndInvalidate(-14, 0x14a, 0, 0, 0, 0, 2, 0);
    Function581200();
    for (int button = 0; button < 16; ++button) {
        if (g_automap_buttons[button]) {
            g_automap_buttons[button]->m_dirty = 1;
            g_automap_buttons[button]->Draw();
        }
    }
    ResetTransientRenderScenes();
    RenderFrame();
    RenderFrame();
    SetFlag603C60();
    Function425C90(12, 32, 467, 467);
    Function580380();
    if (script.Load004CF3B0("Data\\Automap\\MapFilters.txt")) {
        Function474FB0(5);
        W8GrowableVector<char*> excluded_textures;
        int line = 0;
        int section = -1;
        while (section < g_status_685170.current_level && line < script.lines.count) {
            if (strchr((*script.lines.GetAt(line))->text, '['))
                ++section;
            ++line;
        }
        if (line < script.lines.count && section == g_status_685170.current_level) {
            for (; line < script.lines.count; ++line) {
                char* text = (*script.lines.GetAt(line))->text;
                if (strchr(text, '['))
                    break;
                if (!strstr(text, "LAYER_")) {
                    excluded_textures.Add(text);
                } else {
                    float height = static_cast<float>(atof(text + 6));
                    srClipPlane::ClientType* clip = SR_NEW(srClipPlane)(static_cast<srNode*>(0));
                    if (clip) {
                        sprintf(layer_name, "LAYER_%d", layer_number);
                        clip->setName(layer_name);
                        srVector4T<float> plane;
                        plane.Set(0.0f, 1.0f, 0.0f, 0.0f);
                        clip->setClipPlane(plane);
                        srVector3T<double> position(0.0, static_cast<double>(height * 500.0f), 0.0);
                        clip->setLocation(position);
                        clip->setFlag(static_cast<srNode::e_flag>(2));
                        clip->setClipType(srClipPlane::CLIP_POSITIONAL_0);
                        clip->setFlag(static_cast<srNode::e_flag>(0));
                        g_automap_layers.Add(clip);
                        ++layer_number;
                        g_automap_created_layers.Add(clip);
                    }
                }
            }
            for (unsigned int mesh = 0; mesh < g_world->octree->m_meshCount_1b4; ++mesh) {
                for (stMeshModel* model =
                         static_cast<stMeshModel*>(g_world->psrMeshes[mesh]->model());
                     model; model = model->next) {
                    model->ApplyAutomapPolygonFilter(&excluded_textures);
                }
            }
        }
    }
    if (!g_automap_position_initialized) {
        g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
        g_automap_position_initialized = 1;
        srVector3T<float> position = (g_automap_bounds_min + g_automap_bounds_max) / 2.0;
        position.y = g_automap_top_y;
        Function57FC70(&position);
        Function5820F0(g_automap_tool);
        SetAutomapButtonMode(0);
    } else {
        g_automap_position.x = g_automap_saved_camera.position.x;
        g_automap_position.z = g_automap_saved_camera.position.z;
        Function57FC70(&g_automap_position);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005806b0
EnvironmentColour::EnvironmentColour(double red_value, double green_value, double blue_value)
{
    red = static_cast<float>(red_value);
    green = static_cast<float>(green_value);
    blue = static_cast<float>(blue_value);
    if (red > 0.0f) {
        if (red >= 1.0f)
            red = 1.0f;
    } else {
        red = 0.0f;
    }
    if (green > 0.0f) {
        if (green >= 1.0f)
            green = 1.0f;
    } else {
        green = 0.0f;
    }
    if (blue > 0.0f) {
        if (blue >= 1.0f)
            blue = 1.0f;
    } else {
        blue = 0.0f;
    }
}

// FUNCTION: WIZ8 0x00580940
void EnvironmentColour::Set(double red_value, double green_value, double blue_value)
{
    red = static_cast<float>(red_value);
    green = static_cast<float>(green_value);
    blue = static_cast<float>(blue_value);
    if (red > 0.0f) {
        if (red >= 1.0f)
            red = 1.0f;
    } else {
        red = 0.0f;
    }
    if (green > 0.0f) {
        if (green >= 1.0f)
            green = 1.0f;
    } else {
        green = 0.0f;
    }
    if (blue > 0.0f) {
        if (blue >= 1.0f)
            blue = 1.0f;
    } else {
        blue = 0.0f;
    }
}

// FUNCTION: WIZ8 0x00581360
W8AutomapNote* CreateAutomapNote(const srVector2T<float>* position, int layer, const wchar_t* text)
{
    if (text && wcslen(text) < 40 && g_automap_notes->count < 200) {
        W8AutomapNote* note = new W8AutomapNote;
        if (note) {
            note->position = *position;
            note->layer = layer;
            note->text = static_cast<wchar_t*>(malloc(0x50));
            wcscpy(note->text, text);
            g_automap_notes->Add(note);
            g_automap_redraw = 1;
            return note;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x0057f1f0
void AutomapScreenFrame(void)
{
    InputAtom input;
    while (DequeueEvent(&input) == 1) {
        if (g_automap_editing_note) {
            Function584250(&input);
            continue;
        }
        if (Function584690(&input))
            continue;
        if (!IsCursorInsideViewport()) {
            if (g_automap_cursor_inside) {
                g_automap_cursor_inside = 0;
                SetMouseCursorFromVideoObject(
                    GetCatalogVideoObjectHandle(g_automap_tool + 0x14b, 0),
                    GetCatalogVideoObjectYOffset(g_automap_tool + 0x14b),
                    g_automap_cursor_offsets[g_automap_tool][0],
                    g_automap_cursor_offsets[g_automap_tool][1]);
                gXStatus.iCurrentCursor = 7;
                RefreshMouseCursorTexture();
            }
            POINT point;
            SGPMouseGetPos(&point);
            MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, point.x, point.y, gfLeftButtonState,
                                        gfRightButtonState);
            unsigned short reason;
            switch (input.usEvent) {
            case LEFT_BUTTON_DOWN:
            case LEFT_BUTTON_REPEAT:
                reason = LEFT_BUTTON_DOWN;
                break;
            case LEFT_BUTTON_UP:
                reason = LEFT_BUTTON_UP;
                break;
            case RIGHT_BUTTON_DOWN:
                reason = RIGHT_BUTTON_DOWN;
                break;
            case RIGHT_BUTTON_UP:
                reason = RIGHT_BUTTON_UP;
                break;
            default:
                continue;
            }
            MSYS_SGP_Mouse_Handler_Hook(reason, point.x, point.y, gfLeftButtonState,
                                        gfRightButtonState);
            continue;
        }
        if (!g_automap_cursor_inside) {
            g_automap_cursor_inside = 1;
            int cursor = g_automap_tool;
            if (cursor == 0)
                cursor = g_automap_zoom > 25000.0f ? 1 : 4;
            SetMouseCursorFromVideoObject(GetCatalogVideoObjectHandle(cursor + 0x14b, 0),
                                          GetCatalogVideoObjectYOffset(cursor + 0x14b),
                                          g_automap_cursor_offsets[cursor][0],
                                          g_automap_cursor_offsets[cursor][1]);
            gXStatus.iCurrentCursor = 7;
            RefreshMouseCursorTexture();
        }
        if (input.usEvent == LEFT_BUTTON_UP) {
            srVector3T<float> point;
            if (g_automap_tool == 2) {
                if (GetCursorPositionInViewport(&point)) {
                    int layer = g_automap_layer + 1;
                    if (HasAutomapLayer(layer)) {
                        (*g_automap_layers.GetAt(layer))->getLocationY();
                    }
                    srVector2T<float> location;
                    location.Set((point.x - 0.5f) * g_automap_zoom + g_automap_position.x,
                                 g_automap_position.z - (point.y - 0.5f) * g_automap_zoom);
                    g_automap_editing_note = CreateAutomapNote(&location, g_automap_layer, L"_");
                }
            } else if (g_automap_tool == 3) {
                W8AutomapNote* note = Function582180();
                if (note) {
                    int index = g_automap_notes->IndexOf(note);
                    if (index >= 0)
                        g_automap_notes->RemoveAt(index);
                    free(note->text);
                    delete note;
                    if (g_automap_hovered_note == note)
                        g_automap_hovered_note = 0;
                    if (g_automap_editing_note == note)
                        g_automap_editing_note = 0;
                    g_automap_redraw = 1;
                }
            } else if (GetCursorPositionInViewport(&point)) {
                Function57FFC0(&point);
            }
        } else if (input.usEvent == RIGHT_BUTTON_UP) {
            if (g_automap_tool == 0) {
                if (g_automap_position.y < g_automap_top_y) {
                    if (g_automap_zoom_mode == 1) {
                        g_automap_zoom = g_automap_top_y - g_automap_bounds_min.y;
                        srVector3T<float> position =
                            (g_automap_bounds_max + g_automap_bounds_min) / 2.0;
                        position.y = g_automap_top_y;
                        Function57FC70(&position);
                        Function5820F0(g_automap_tool);
                        SetAutomapButtonMode(0);
                    } else {
                        float ground_y = g_automap_position.y - g_automap_zoom;
                        float height = g_automap_top_y - (g_automap_top_y - ground_y) * 0.5f;
                        if (g_automap_position.y <= height) {
                            g_automap_position.y = height;
                            SetAutomapButtonMode(1);
                            g_automap_zoom = g_automap_position.y - ground_y;
                            Function57FC70(&g_automap_position);
                            Function5820F0(g_automap_tool);
                        } else {
                            Function57FE40();
                        }
                    }
                }
            } else {
                g_automap_tool = 0;
                Function5820F0(0);
            }
        } else if (input.usEvent == MOUSE_POS) {
            W8AutomapNote* previous = g_automap_hovered_note;
            g_automap_hovered_note = Function582180();
            if (previous != g_automap_hovered_note) {
                if (previous)
                    Function581460(previous);
                Function582930();
            }
            srVector3T<float> point;
            if (Function582050(&point)) {
                point.y = 0.0f;
                srVector3T<float> distance(point.x - g_automap_saved_camera.position.x, 0.0f,
                                           point.z - g_automap_saved_camera.position.z);
                if (distance.Length() < g_automap_zoom * 0.05f && g_automap_tool == 0 &&
                    g_automap_zoom_mode != 2) {
                    ClearFlag603C60();
                    srVector3T<double> scale(0.44f, 0.44f, 0.44f);
                    g_class_68f29c->setScale(scale);
                    continue;
                }
            }
            float factor = (1.0f / (g_automap_zoom * 0.00004f)) * 0.44f;
            srVector3T<double> scale(factor, factor, factor);
            g_class_68f29c->setScale(scale);
            SetFlag603C60();
        }
    }
    bool moved = false;
    if (gfKeyState[0x25]) {
        g_automap_position.x -= g_automap_zoom * 0.35f;
        moved = true;
    }
    if (gfKeyState[0x27]) {
        g_automap_position.x += g_automap_zoom * 0.35f;
        moved = true;
    }
    if (gfKeyState[0x26]) {
        g_automap_position.z += g_automap_zoom * 0.35f;
        moved = true;
    }
    if (gfKeyState[0x28]) {
        g_automap_position.z -= g_automap_zoom * 0.35f;
        moved = true;
    }
    if (moved)
        Function57FC70(&g_automap_position);
    if (GetTickCount() - g_automap_state->blink_time > 500) {
        if (g_automap_state->blink_enabled)
            Function427460(0xdc, 0x32);
        g_automap_state->blink_time = GetTickCount();
    }
    Function581030();
}

// FUNCTION: WIZ8 0x0057fb40
void RestoreAutomapWorldSettings(void)
{
    SetWorldEnvironmentColour00483A60(g_world, g_automap_saved_ambient_light);
    SetLightDirection(&g_automap_saved_light_direction);
    RestoreWorldCameraState(GetWorld(), 0, &g_automap_saved_camera);
    WorldSetFarClip(g_world, g_automap_saved_far_clip);
    WorldSetValue74(g_world, g_automap_saved_world_value);
    if (g_automap_saved_sky)
        EnableSky();
    SetRenderOption(11, g_automap_saved_render_flags[0]);
    SetRenderOption(10, g_automap_saved_render_flags[1]);
    g_monster_shadow_updates_enabled_0065970c = g_automap_saved_render_flags[2];
    g_flag_65970d = g_automap_saved_render_flags[3];
    g_world->camera->setRotation(0.0, 0.0, 0.0);
    g_world->camera->flags_138.value &= ~1ul;
    Function46F760(g_world, 0);
    g_light_update_flags_0060bfdc |= 1u;
    SetResidentTexturePolicy(g_resident_texture_policy_659714);
    Function427830(1);
}

// FUNCTION: WIZ8 0x0057efe0
unsigned char AutomapScreenLeave(int)
{
    RestoreAutomapWorldSettings();
    free(g_automap_state);
    g_automap_state = 0;
    MarkRendererReady();
    SetValue659668(0);
    Function56AAB0();
    srClass* clipping_plane = static_cast<srClass*>(srCore.getRegistry()->find(
        srClipPlane::ClientType::sGetClassNode(), "Clipping Plane 1", 0));
    if (clipping_plane) {
        g_world->level->setParent(g_world->static_scene, 1);
        g_world->dynamic_scene->setParent(g_world->static_scene, 1);
        clipping_plane->release();
    }
    gfTrackMousePos = 0;
    if (g_automap_surface) {
        g_automap_surface->release();
        g_automap_surface = 0;
    }
    while (g_releasable_68f1f4->count) {
        srClass* object = g_releasable_68f1f4->data[0];
        object->release();
        int index = g_releasable_68f1f4->IndexOf(object);
        if (index >= 0)
            g_releasable_68f1f4->RemoveAt(index);
    }
    g_class_68f29c->setParent(0, 1);
    for (int index = 0; index < 16; ++index) {
        delete g_automap_buttons[index];
    }
    delete[] g_automap_buttons;
    g_automap_buttons = 0;
    MSYS_Shutdown();
    UpdateHeldItemCursor();
    SetFlag603C60();
    for (unsigned int mesh = 0; mesh < g_world->octree->m_meshCount_1b4; ++mesh) {
        for (stMeshModel* model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh]->model());
             model; model = model->next) {
            model->ClearAutomapPolygonFilter();
        }
    }
    while (g_automap_created_layers.count) {
        if (g_automap_created_layers.data[0])
            g_automap_created_layers.data[0]->release();
        g_automap_created_layers.RemoveAt(0);
    }
    return 1;
}

/* Lifecycle record 8's finalizer, the fifth slot of its record and the third
   allocate/release pair that establishes what that slot is for. Every guard is
   the original's own, and each pointer is cleared after its release. */
// FUNCTION: WIZ8 0x0057fa20
unsigned char AutomapScreenFinalize(void)
{
    if (g_automap_notes) {
        delete g_automap_notes;
        g_automap_notes = 0;
    }
    BitArray* bits = g_bits_68f288;
    if (bits) {
        delete bits;
        g_bits_68f288 = 0;
    }
    bits = g_bits_68f28c;
    if (bits) {
        delete bits;
        g_bits_68f28c = 0;
    }
    if (g_block_68f280) {
        free(g_block_68f280);
        g_block_68f280 = 0;
    }
    W8HashTable<unsigned int, int>* record = g_record_68f284;
    if (record) {
        delete record;
        g_record_68f284 = 0;
    }
    if (g_class_68f29c) {
        g_class_68f29c->release();
        g_class_68f29c = 0;
    }
    if (g_class_68f2a0) {
        g_class_68f2a0->release();
        g_class_68f2a0 = 0;
    }
    if (g_class_68f2a4) {
        g_class_68f2a4->release();
        g_class_68f2a4 = 0;
    }
    if (g_class_68f2a8) {
        g_class_68f2a8->release();
        g_class_68f2a8 = 0;
    }
    if (g_releasable_68f1f4) {
        delete g_releasable_68f1f4;
        g_releasable_68f1f4 = 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x0057DBB0
unsigned char GetFlag68F105(void)
{
    return g_flag_68f105;
}
// FUNCTION: WIZ8 0x0057DBC0
unsigned char GetFlag68F104(void)
{
    return g_flag_68f104;
}

/* Select which automap buttons are enabled for the update mode, then dirty
   and redraw both and remember the mode. */
// FUNCTION: WIZ8 0x0057FD90
void SetAutomapButtonMode(int update)
{
    if (g_automap_buttons == 0) {
        g_automap_zoom_mode = update;
        return;
    }
    if (update == 0) {
        g_automap_buttons[0]->SetEnabled(1);
        g_automap_buttons[1]->SetEnabled(0);
    } else if (update == 1) {
        g_automap_buttons[0]->SetEnabled(1);
        g_automap_buttons[1]->SetEnabled(1);
    } else if (update == 2) {
        g_automap_buttons[0]->SetEnabled(0);
        g_automap_buttons[1]->SetEnabled(1);
    }
    g_automap_buttons[0]->m_dirty = 1;
    g_automap_buttons[0]->Draw();
    g_automap_buttons[1]->m_dirty = 1;
    g_automap_buttons[1]->Draw();
    g_automap_zoom_mode = update;
}

// GLOBAL: WIZ8 0x0064b914
float g_float_64b914 = 2000.0f;
// GLOBAL: WIZ8 0x0068f2b0
int g_value_68f2b0;
// GLOBAL: WIZ8 0x0068f2c4
int g_value_68f2c4;

// FUNCTION: WIZ8 0x00585300
void SetFloat64B914(float value)
{
    g_float_64b914 = value;
}

// FUNCTION: WIZ8 0x00585310
float GetFloat64B914(void)
{
    return g_float_64b914;
}

// FUNCTION: WIZ8 0x00587C10
void SetValue68F2B0(int value)
{
    g_value_68f2b0 = value;
}

// FUNCTION: WIZ8 0x0058A870
void SetValue68F2C4(int value)
{
    g_value_68f2c4 = value;
}
